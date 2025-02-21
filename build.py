#!/usr/bin/env python3
# --------------------------------------------------------------------
# bake.py
#
# Author: Lain Musgrove (lain.proliant@gmail.com)
# Date: Saturday February 22, 2020
#
# Distributed under terms of the MIT license.
# --------------------------------------------------------------------
import shlex
from pathlib import Path

from xeno.build import Recipe, build, factory, provide, recipe, task
from xeno.recipes import checkout, sh
from xeno.recipes.cxx import ENV, compile
from xeno.shell import check

# -------------------------------------------------------------------
PYPI_USERNAME = "lainproliant"
PYPI_KEY_NAME = "pypi"

PERFORMANCE = "-O3"

DEPS = [
    "https://github.com/lainproliant/moonlight",
    "https://github.com/pybind/pybind11",
]

INCLUDES = [
    "-I./include",
    "-I./deps/moonlight/include",
    "-I./deps/moonlight/deps",
    "-I./deps/pybind11/include",
]

LDFLAGS = ("-rdynamic", PERFORMANCE, "-ldl")

RELEASE_CFLAGS = (*INCLUDES, "--std=c++2a")

TEST_CFLAGS = (
    *RELEASE_CFLAGS,
    "-DMOONLIGHT_AUTOMATA_DEBUG",
    "-DMOONLIGHT_DEBUG",
    "-DMOONLIGHT_ENABLE_STACKTRACE",
    "-DMOONLIGHT_AUTOMATA_DEBUG",
    "-DMOONLIGHT_STACKTRACE_IN_DESCRIPTION",
)

RELEASE_ENV = ENV.copy().update(CFLAGS=RELEASE_CFLAGS, LDFLAGS=LDFLAGS)

TEST_ENV = ENV.copy().update(CFLAGS=TEST_CFLAGS, LDFLAGS=LDFLAGS)


# -------------------------------------------------------------------
@factory
def compile_app(src, headers, env):
    return compile(src, headers=headers, target=Path(src).with_suffix(""), env=env)


# -------------------------------------------------------------------
@factory
def compile_test(src, headers, env):
    return compile_app(src, headers, TEST_ENV)


# -------------------------------------------------------------------
@factory
def compile_demo(src, headers, env):
    return compile_app(src, headers, RELEASE_ENV)


# -------------------------------------------------------------------
@factory
def run_test(app):
    return sh(app, cwd="test", interactive=True)


# -------------------------------------------------------------------
@factory
def link_pybind11_module(pybind11_module_objects):
    return sh(
        "{CXX} -O3 -shared -Wall -std=c++2a -fPIC {input} -o {target}",
        env=RELEASE_ENV,
        input=pybind11_module_objects,
        target=Path(
            "jotdown%s" % check("pipenv run python3-config --extension-suffix")
        ),
    )


# -------------------------------------------------------------------
@recipe
def find_latest_tarball(path):
    tarballs = [*Path(path).glob("*.tar.gz")]
    return max(tarballs, key=lambda x: x.stat().st_mtime)


# -------------------------------------------------------------------
@recipe
def get_password(name: str):
    return check(f"pass {name}")


# -------------------------------------------------------------------
@factory
def compile_pybind11_module_object(src, headers, tests):
    return sh(
        "{CXX} -O3 -shared -Wall -std=c++2a -fPIC {flags} {src} -o {target}",
        env=RELEASE_ENV,
        src=src,
        tests=tests,
        target=Path(src).with_suffix(".o"),
        flags=INCLUDES + shlex.split(check("pipenv run python-config --includes")),
        requires=headers,
    )


# -------------------------------------------------------------------
@provide
def submodules():
    return sh("git submodule update --init --recursive")


# -------------------------------------------------------------------
@provide
def headers():
    return Path.cwd().glob("include/jotdown/*.h")


# -------------------------------------------------------------------
@provide
async def demo_sources():
    return Path.cwd().glob("demo/*.cpp")


# -------------------------------------------------------------------
@provide
def test_sources():
    return Path.cwd().glob("test/*.cpp")


# -------------------------------------------------------------------
@task(keep=True)
def deps():
    return [checkout(repo) for repo in DEPS]


# -------------------------------------------------------------------
@task(dep="deps")
def demos(deps, demo_sources, headers):
    return Recipe(
        [compile_demo(src, headers, RELEASE_ENV) for src in demo_sources],
        setup=submodules,
    )


# -------------------------------------------------------------------
@task(dep="deps")
def tests(deps, test_sources, headers, submodules):
    return Recipe(
        [compile_test(src, headers, TEST_ENV) for src in test_sources], setup=submodules
    )


# -------------------------------------------------------------------
@task
def run_tests(tests):
    return [run_test(app) for app in tests]


# -------------------------------------------------------------------
@task(sync=True, dep="deps")
def pybind11_tests(deps):
    return [
        sh("mkdir -p {target}", target="pybind11-test-build"),
        sh("cmake ../pybind11", cwd=Path("pybind11-test-build")),
        sh("make check -j 4", cwd=Path("pybind11-test-build"), interactive=True),
    ]


# -------------------------------------------------------------------
@provide
def pymodule_sources(submodules):
    return Path.cwd().glob("src/*.cpp")


# -------------------------------------------------------------------
@task(dep="deps")
def pymodule_objects(deps, pymodule_sources, headers, run_tests):
    return [
        compile_pybind11_module_object(src, headers, run_tests)
        for src in pymodule_sources
    ]


# -------------------------------------------------------------------
@task
def pymodule_dev(pymodule_objects):
    return link_pybind11_module(pymodule_objects)


# -------------------------------------------------------------------
@task(dep="deps")
def pymodule_sdist(deps):
    return sh("python3 setup.py sdist", target="dist")


# -------------------------------------------------------------------
@provide
def latest_tarball(pymodule_sdist):
    return find_latest_tarball(pymodule_sdist)


# -------------------------------------------------------------------
@provide
def pypi_password():
    return get_password(PYPI_KEY_NAME)


# -------------------------------------------------------------------
@task
def upload_to_pypi(pymodule_sdist, latest_tarball, pypi_password):
    return sh(
        "twine upload -u {PYPI_USERNAME} -p {password} {input}",
        PYPI_USERNAME=PYPI_USERNAME,
        password=pypi_password,
        input=latest_tarball,
        redacted=["password"],
    )


# -------------------------------------------------------------------
@task(default=True)
def all(demos, pymodule_dev):
    return [demos, pymodule_dev]


# -------------------------------------------------------------------
@task
def cc_json():
    return sh("intercept-build ./build.py compile\\* -R; ./build.py -c compile\\*")


# -------------------------------------------------------------------
build()
