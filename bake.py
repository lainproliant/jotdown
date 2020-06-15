# --------------------------------------------------------------------
# bake.py
#
# Author: Lain Musgrove (lain.proliant@gmail.com)
# Date: Saturday February 22, 2020
#
# Distributed under terms of the MIT license.
# --------------------------------------------------------------------
import shlex
import subprocess
from pathlib import Path
from panifex import build, default, seq, sh


# -------------------------------------------------------------------
INCLUDES = [
    "-I./include",
    "-I./python/include",
    "-I./moonlight/include",
    "-I./pybind11/include",
]

sh.env(CC="clang++",
       CFLAGS=("-g",
               *INCLUDES,
               "--std=c++2a",
               "-DMOONLIGHT_DEBUG",
               "-DMOONLIGHT_ENABLE_STACKTRACE",
               "-DMOONLIGHT_STACKTRACE_IN_DESCRIPTION"),
       LDFLAGS=("-rdynamic", "-g", "-ldl"))


# --------------------------------------------------------------------
def check(cmd):
    return subprocess.check_output(shlex.split(cmd)).decode('utf-8').strip()


# -------------------------------------------------------------------
def compile_app(src, headers):
    return sh(
        "{CC} {CFLAGS} {input} {LDFLAGS} -o {output}",
        input=src,
        output=Path(src).with_suffix(""),
        includes=headers
    )


# -------------------------------------------------------------------
def compile_pybind11_module(src, headers):
    return sh(
        "{CC} -O3 -shared -Wall -std=c++2a -fPIC {flags} {input} -o {output}",
        input=src,
        output="jotdown%s" % check("python3-config --extension-suffix"),
        flags=INCLUDES + shlex.split(check("python-config --includes")),
        includes=headers
    )


# -------------------------------------------------------------------
@build
class Jotdown:
    def submodules(self):
        return sh("git submodule update --init --recursive")

    def headers(self):
        return Path.cwd().glob("include/jotdown/*.h")

    def demo_sources(self, submodules):
        return Path.cwd().glob("demo/*.cpp")

    def demos(self, demo_sources, headers):
        return [compile_app(src, headers) for src in demo_sources]

    def pybind11_tests(self, submodules):
        return seq(
            sh("mkdir -p {output}", output='pybind11-test-build'),
            sh("cmake ../pybind11", cwd='pybind11-test-build'),
            sh("make check -j 4", cwd='pybind11-test-build').interactive())

    def test_sources(self, submodules):
        return Path.cwd().glob("test/*.cpp")

    def tests(self, test_sources, headers):
        return [compile_app(src, headers) for src in test_sources]

    def run_tests(self, tests):
        return (sh("{input}",
                   input=test,
                   cwd="test")
                for test in tests)

    def pymodule_src(self):
        return Path.cwd().glob("python/src/*.cpp")

    def pymodule(self, pymodule_src, headers):
        return compile_pybind11_module(pymodule_src, headers)

    @default
    def common(self, demos, tests):
        pass

    def all(self, common, pybind11_tests):
        pass
