# --------------------------------------------------------------------
# bake.py
#
# Author: Lain Musgrove (lain.proliant@gmail.com)
# Date: Saturday February 22, 2020
#
# Distributed under terms of the MIT license.
# --------------------------------------------------------------------
from pathlib import Path
from panifex import build, sh, default
from logging import Logger

# -------------------------------------------------------------------
sh.env(CC="g++",
       CFLAGS=("-g",
               "-I./include",
               "-I./moonlight/include",
               "--std=c++2a",
               "-DMOONLIGHT_DEBUG",
               "-DMOONLIGHT_ENABLE_STACKTRACE",
               "-DMOONLIGHT_STACKTRACE_IN_DESCRIPTION"),
       LDFLAGS=("-rdynamic", "-g", "-ldl"))


# -------------------------------------------------------------------
def compile_test(src):
    return sh(
        "{CC} {CFLAGS} {input} {LDFLAGS} -o {output}",
        input=src,
        output=Path(src).with_suffix("")
    )

# -------------------------------------------------------------------
@build
class Jotdown:
    def submodules(self):
        return sh("git submodule update --init --recursive")

    def headers(self):
        return Path.cwd().glob("**/*.h")

    def test_sources(self, submodules):
        return Path.cwd().glob("test/*.cpp")

    def tests(self, test_sources):
        return [compile_test(src) for src in test_sources]

    @default
    def run_tests(self, tests):
        return (sh("{input}",
                   input=test,
                   cwd="test").interactive()
                for test in tests)
