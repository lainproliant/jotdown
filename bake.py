# --------------------------------------------------------------------
# bake.py
#
# Author: Lain Musgrove (lain.proliant@gmail.com)
# Date: Saturday February 22, 2020
#
# Distributed under terms of the MIT license.
# --------------------------------------------------------------------
from pathlib import Path
from panifex import build, sh, default, seq

# -------------------------------------------------------------------
sh.env(CC="g++",
       CFLAGS=("-g",
               "-I./include",
               "-I./moonlight/include",
               "-I./pybind11/include",
               "--std=c++2a",
               "-DMOONLIGHT_DEBUG",
               "-DMOONLIGHT_ENABLE_STACKTRACE",
               "-DMOONLIGHT_STACKTRACE_IN_DESCRIPTION"),
       LDFLAGS=("-rdynamic", "-g", "-ldl"))


# -------------------------------------------------------------------
def compile_app(src, headers):
    return sh(
        "{CC} {CFLAGS} {input} {LDFLAGS} -o {output}",
        input=src,
        output=Path(src).with_suffix(""),
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

    @default
    def common(self, demos, tests):
        pass

    def all(self, common, pybind11_tests):
        pass
