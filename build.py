#!/usr/bin/env python
# --------------------------------------------------------------------
# build.py
#
# Author: Lain Musgrove (lain.proliant@gmail.com)
# Date: Wednesday May 24, 2023
#
# Distributed under terms of the MIT license.
# --------------------------------------------------------------------
import shlex
from pathlib import Path

from xeno.build import build, factory, provide, recipe, task
from xeno.cookbook import sh
from xeno.shell import check

# --------------------------------------------------------------------
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

LDFLAGS = ("-rdynamic", "-g", "-ldl")

RELEASE_CFLAGS = (*INCLUDES, "--std=c++2a")

TEST_CFLAGS = (
    *RELEASE_CFLAGS,
    "-DMOONLIGHT_AUTOMATA_DEBUG",
    "-DMOONLIGHT_DEBUG",
    "-DMOONLIGHT_ENABLE_STACKTRACE",
    "-DMOONLIGHT_AUTOMATA_DEBUG",
    "-DMOONLIGHT_STACKTRACE_IN_DESCRIPTION",
)

RELEASE_ENV = dict(CC="clang++", CFLAGS=RELEASE_CFLAGS, LDFLAGS=LDFLAGS)
TEST_ENV = dict(CC="clang++", CFLAGS=TEST_CFLAGS, LDFLAGS=LDFLAGS)


# --------------------------------------------------------------------
# Recipes and Recipe Factories
# --------------------------------------------------------------------
@recipe
def latest_tarball(path):
    tarballs = [*path.glob("*.tar.gz")]
    return max(tarballs, key=lambda x: x.stat().st_mtime)


@factory(sigil=lambda r: f"{r.name}:{r.component_sigils()}")
def checkout_dep(repo):
    name = repo.split("/")[-1]
    return check_for_xeno_build(
        sh("git clone {repo} {target}", repo=repo, target=Path("deps") / name)
    )


@factory
def check_for_xeno_build(repo):
    return sh("[[ ! -f build.py ]] || ./build.py deps", repo=repo, cwd=repo.target)


# -------------------------------------------------------------------
@recipe(factory=True, sigil=lambda r: f"{r.name}:{r.target.name}")
def compile(src, headers, env):
    cmd = "{CC} {CFLAGS} {src} {LDFLAGS} -o {target}"

    return sh(cmd, env=env, src=src, target=src.with_suffix(""))


@factory
def compile_test(src, headers, env):
    return compile(src, headers, TEST_ENV)


@factory
def compile_demo(src, headers, env):
    return compile(src, headers, RELEASE_ENV)


@factory
def run_test(app):
    return sh(app.target, app=app, cwd="test", interactive=True)


@factory
def link_pybind11_module(pybind11_module_objects):
    return sh(
        "{CC} -O3 -shared -Wall -std=c++2a -fPIC {input} -o {target}",
        env=RELEASE_ENV,
        input=pybind11_module_objects,
        target=Path("jotdown%s" % check("python3-config --extension-suffix")),
    )


@factory
def compile_pybind11_module_object(src, headers):
    return sh(
        "{CC} -O3 -shared -Wall -std=c++2a -fPIC {flags} {src} -o {target}",
        env=RELEASE_ENV,
        src=src,
        target=Path(src).with_suffix(".o"),
        flags=INCLUDES + shlex.split(check("python-config --includes")),
    )


# --------------------------------------------------------------------
# Resources
# --------------------------------------------------------------------
@provide
def headers():
    return Path.cwd().glob("include/jotdown/*.h")


@provide
def demo_sources():
    return Path.cwd().glob("demo/*.cpp")


@provide
def test_sources():
    return Path.cwd().glob("test/*.cpp")


@provide
def pymodule_sources():
    return Path.cwd().glob("src/*.cpp")


# --------------------------------------------------------------------
# Tasks
# --------------------------------------------------------------------
@task
def deps():
    return [checkout_dep(repo) for repo in DEPS]


@task(dep="deps")
def demos(demo_sources, headers, deps):
    return [compile_demo(src, headers, RELEASE_ENV) for src in demo_sources]


@task(dep="deps")
def tests(test_sources, headers, deps):
    return [compile_test(src, headers, TEST_ENV) for src in test_sources]


@task
def run_tests(tests):
    return [run_test(app) for app in tests]


@task(dep="deps", sync=True)
def pybind11_tests(deps):
    return [
        sh("mkdir -p {target}", target="pybind11-test-build"),
        sh("cmake ../pybind11", cwd=Path("pybind11-test-build")),
        sh("make check -j 4", cwd=Path("pybind11-test-build"), interactive=True),
    ]


@task(dep="tests")
def pymodule_objects(pymodule_sources, headers, tests):
    return [compile_pybind11_module_object(src, headers) for src in pymodule_sources]


@task(default=True)
def pymodule_dev(pymodule_objects):
    return link_pybind11_module(pymodule_objects)


@task
def pymodule_sdist():
    return sh("python3 setup.py {target}", target="sdist")


@task
def upload_to_pypi(pymodule_sdist):
    return sh(
        "twine upload {input}", input=latest_tarball(pymodule_sdist), interact=True
    )


# -------------------------------------------------------------------
build()
