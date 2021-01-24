import os
import subprocess
import shlex


# --------------------------------------------------------------------
def python_config(*args):
    result = subprocess.check_output(['python-config', *args]).decode('utf-8')
    return shlex.split(result)


# --------------------------------------------------------------------
def relative(filename):
    return os.path.join(os.path.dirname(__file__), filename)


# --------------------------------------------------------------------
INCLUDES = [".",
            "./include",
            "./python/include",
            "./pybind11/include",
            "./moonlight/include"]

# --------------------------------------------------------------------
FLAGS = [
    "-Wall",
    "-Wextra",
    "-Werror",
    "-fexceptions",
    "-ferror-limit=10000",
    "-DNDEBUG",
    "-DUSE_CLANG_COMPLETER",
    "-DMOONLIGHT_AUTOMATA_DEBUG",
    "-std=c++2a",
    "-xc++",
]

# --------------------------------------------------------------------
SOURCE_EXTENSIONS = [".cpp", ".c", ".h"]


# --------------------------------------------------------------------
def Settings(filename, **kwargs):
    flags = [*FLAGS, *python_config("--includes")]
    for include in INCLUDES:
        flags.append("-I")
        flags.append(relative(include))

    return {"flags": flags, "do_cache": True}
