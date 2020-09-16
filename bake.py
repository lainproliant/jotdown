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

import panifex as pfx

# -------------------------------------------------------------------
PYPI_USERNAME = "lainproliant"
PYPI_KEY_NAME = "pypi"

# -------------------------------------------------------------------
INCLUDES = [
    "-I./include",
    "-I./moonlight/include",
    "-I./pybind11/include",
]

sh = pfx.sh.env(
    CC="clang++",
    CFLAGS=(
        "-g",
        *INCLUDES,
        "--std=c++2a",
        "-DMOONLIGHT_DEBUG",
        "-DMOONLIGHT_ENABLE_STACKTRACE",
        "-DMOONLIGHT_STACKTRACE_IN_DESCRIPTION",
    ),
    LDFLAGS=("-rdynamic", "-g", "-ldl"),
)


# -------------------------------------------------------------------
@pfx.factory
def compile_app(src, headers):
    return sh(
        "{CC} {CFLAGS} {input} {LDFLAGS} -o {output}",
        input=src,
        output=Path(src).with_suffix(""),
        includes=headers,
    )


# -------------------------------------------------------------------
@pfx.factory
def link_pybind11_module(pybind11_module_objects):
    return sh(
        "{CC} -O3 -shared -Wall -std=c++2a -fPIC {input} -o {output}",
        input=pybind11_module_objects,
        output="jotdown%s" % pfx.check("python3-config --extension-suffix"),
    )


# -------------------------------------------------------------------
@pfx.recipe
def find_latest_tarball(path: Path) -> Path:
    tarballs = [*path.glob("*.tar.gz")]
    return max(tarballs, key=lambda x: x.stat().st_mtime)


# -------------------------------------------------------------------
@pfx.factory
def compile_pybind11_module_object(src, headers):
    return sh(
        "{CC} -O3 -shared -Wall -std=c++2a -fPIC {flags} {input} -o {output}",
        input=src,
        output=Path(src).with_suffix(".o"),
        flags=INCLUDES + shlex.split(pfx.check("python-config --includes")),
        includes=headers,
    )


# -------------------------------------------------------------------
@pfx.recipe
def get_password(name):
    return pfx.check(f"pass {name}")


# -------------------------------------------------------------------
@pfx.provide
def submodules():
    return sh("git submodule update --init --recursive")


# -------------------------------------------------------------------
@pfx.provide
def headers():
    return Path.cwd().glob("include/jotdown/*.h")


# -------------------------------------------------------------------
@pfx.provide
def demo_sources(submodules):
    return Path.cwd().glob("demo/*.cpp")


# -------------------------------------------------------------------
@pfx.provide
def test_sources(submodules):
    return Path.cwd().glob("test/*.cpp")


# -------------------------------------------------------------------
@pfx.target
def demos(demo_sources, headers):
    return pfx.poly(compile_app(src, headers) for src in demo_sources)


# -------------------------------------------------------------------
@pfx.target
def tests(test_sources, headers):
    return pfx.poly(compile_app(src, headers) for src in test_sources)


# -------------------------------------------------------------------
@pfx.target
def run_tests(tests):
    return pfx.poly(
        sh("{input}", input=test, cwd="test", interactive=True)
        for test in tests
    )


# -------------------------------------------------------------------
@pfx.target
def pybind11_tests(submodules):
    return pfx.seq(
        [
            sh("mkdir -p {output}", output="pybind11-test-build"),
            sh("cmake ../pybind11", cwd="pybind11-test-build"),
            sh("make check -j 4", cwd="pybind11-test-build", interactive=True),
        ]
    )


# -------------------------------------------------------------------
@pfx.provide
def pymodule_sources(submodules):
    return Path.cwd().glob("src/*.cpp")


# -------------------------------------------------------------------
@pfx.provide
def pymodule_objects(pymodule_sources, headers, run_tests):
    return pfx.poly(
        compile_pybind11_module_object(src, headers) for src in pymodule_sources
    )


# -------------------------------------------------------------------
@pfx.target
def pymodule_dev(pymodule_objects):
    return link_pybind11_module(pymodule_objects)


# -------------------------------------------------------------------
@pfx.target
def pymodule_sdist(submodules):
    return sh("python3 setup.py sdist", output="dist")


# -------------------------------------------------------------------
@pfx.provide
def latest_tarball(pymodule_sdist):
    return find_latest_tarball(pymodule_sdist)


# -------------------------------------------------------------------
@pfx.provide
def pypi_password():
    return get_password(PYPI_KEY_NAME)


# -------------------------------------------------------------------
@pfx.target
def upload_to_pypi(pymodule_sdist, latest_tarball, pypi_password):
    return sh(
        "twine upload -u {PYPI_USERNAME} -p {pypi_password} {input}",
        PYPI_USERNAME=PYPI_USERNAME,
        pypi_password=pypi_password,
        input=latest_tarball,
        echo=False,
        stdout=False,
    )


# -------------------------------------------------------------------
@pfx.target
def all(demos, pymodule_dev):
    return pfx.poly([demos, pymodule_dev])


# -------------------------------------------------------------------
pfx.build(default="all")
