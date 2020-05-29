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
def compile(src, includes=None):
    return sh(
        "{CC} -c {CFLAGS} {input} -o {output}",
        input=src,
        includes=includes,
        output=Path(src).with_suffix(".o"),
    )


# -------------------------------------------------------------------
def link(executable, objects):
    return sh("{CC} {LDFLAGS} {input} -o {output}", input=objects, output=executable)


# -------------------------------------------------------------------
@build
class DungeonDive:
    def submodules(self):
        return sh("git submodule update --init --recursive")

    def headers(self):
        return Path.cwd().glob("**/*.h")

    def demo_src(self):
        return Path.cwd() / "src" / "demo.cpp"

    def demo_obj(self, demo_src, headers, submodules):
        return compile(demo_src, headers)

    @default
    def demo_exe(self, demo_obj):
        return link("demo", demo_obj)
