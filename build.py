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

from xeno.build import Recipe, ValueRecipe, build, default, provide, sh, target, factory
from xeno.shell import check

# -------------------------------------------------------------------
PYPI_USERNAME = "lainproliant"
PYPI_KEY_NAME = "pypi"

# -------------------------------------------------------------------
INCLUDES = [
    "-I./include",
    "-I./moonlight/include",
    "-I./pybind11/include",
]

LDFLAGS=('-rdynamic', '-g', '-ldl')

RELEASE_CFLAGS=(
    *INCLUDES,
    "--std=c++2a"
)

TEST_CFLAGS=(
    *RELEASE_CFLAGS,
    "-DMOONLIGHT_AUTOMATA_DEBUG"
    "-DMOONLIGHT_DEBUG",
    "-DMOONLIGHT_ENABLE_STACKTRACE",
    "-DMOONLIGHT_AUTOMATA_DEBUG",
    "-DMOONLIGHT_STACKTRACE_IN_DESCRIPTION",
)

RELEASE_ENV = dict(
    CC="clang++",
    CFLAGS=RELEASE_CFLAGS,
    LDFLAGS=LDFLAGS
)

TEST_ENV = dict(
    CC="clang++",
    CFLAGS=TEST_CFLAGS,
    LDFLAGS=LDFLAGS
)

# -------------------------------------------------------------------
def compile_app(src, headers, env):
    return sh(
        "{CC} {CFLAGS} {src} {LDFLAGS} -o {output}",
        env=env,
        src=src,
        output=Path(src).with_suffix(""),
        includes=headers,
    )


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
    return sh("{test}", test=app, cwd="test", interactive=True)

# -------------------------------------------------------------------
@factory
def link_pybind11_module(pybind11_module_objects):
    return sh(
        "{CC} -O3 -shared -Wall -std=c++2a -fPIC {input} -o {output}",
        env=RELEASE_ENV,
        input=pybind11_module_objects,
        output=Path("jotdown%s" % check("python3-config --extension-suffix")),
    )


# -------------------------------------------------------------------
class FindLatestTarball(ValueRecipe):
    def __init__(self, path):
        super().__init__(input=[path])
        self.path = path.result

    async def compute(self):
        tarballs = [*self.path.glob("*.tar.gz")]
        return max(tarballs, key=lambda x: x.stat().st_mtime)


# -------------------------------------------------------------------
class GetPassword(ValueRecipe):
    def __init__(self, name: str):
        super().__init__()
        self._name = name

    async def compute(self):
        return check(f"pass {self._name}")


# -------------------------------------------------------------------
@factory
def compile_pybind11_module_object(src, headers, tests):
    return sh(
        "{CC} -O3 -shared -Wall -std=c++2a -fPIC {flags} {src} -o {output}",
        env=RELEASE_ENV,
        src=src,
        tests=tests,
        output=Path(src).with_suffix(".o"),
        flags=INCLUDES + shlex.split(check("python-config --includes")),
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
@target
def demos(demo_sources, headers):
    return [compile_demo(src, headers, RELEASE_ENV) for src in demo_sources]


# -------------------------------------------------------------------
@target
def tests(test_sources, headers, submodules):
    return Recipe([compile_test(src, headers, TEST_ENV) for src in test_sources], setup=submodules)


# -------------------------------------------------------------------
@target
def run_tests(tests):
    return [run_test(app) for app in tests]

# -------------------------------------------------------------------
@target
def pybind11_tests(submodules):
    return Recipe([
        sh("mkdir -p {output}", output="pybind11-test-build"),
        sh("cmake ../pybind11", cwd=Path("pybind11-test-build")),
        sh("make check -j 4", cwd=Path("pybind11-test-build"), interactive=True),
    ], synchronous=True, setup=submodules)


# -------------------------------------------------------------------
@provide
def pymodule_sources(submodules):
    return Path.cwd().glob("src/*.cpp")


# -------------------------------------------------------------------
@target
def pymodule_objects(pymodule_sources, headers, run_tests):
    return [compile_pybind11_module_object(src, headers, run_tests) for src in pymodule_sources]


# -------------------------------------------------------------------
@target
def pymodule_dev(pymodule_objects):
    return link_pybind11_module(pymodule_objects)


# -------------------------------------------------------------------
@target
def pymodule_sdist(submodules):
    return sh("python3 setup.py sdist", output="dist")


# -------------------------------------------------------------------
@provide
def latest_tarball(pymodule_sdist):
    return FindLatestTarball(pymodule_sdist)


# -------------------------------------------------------------------
@provide
def pypi_password():
    return GetPassword(PYPI_KEY_NAME)


# -------------------------------------------------------------------
@target
def upload_to_pypi(latest_tarball, pypi_password):
    return sh(
        "twine upload -u {PYPI_USERNAME} -p {password} {input}",
        PYPI_USERNAME=PYPI_USERNAME,
        password=pypi_password,
        input=latest_tarball,
        redacted=['password']
    )


# -------------------------------------------------------------------
@default
def all(demos, pymodule_dev):
    return [demos, pymodule_dev]


# -------------------------------------------------------------------
build()
