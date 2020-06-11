import os


# --------------------------------------------------------------------
def relative(filename):
    return os.path.join(os.path.dirname(__file__), filename)


# --------------------------------------------------------------------
INCLUDES = [".",
            "./include",
            "./moonlight/include",
            "./pybind11/include"]

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
    flags = [*FLAGS]
    for include in INCLUDES:
        flags.append("-I")
        flags.append(relative(include))

    return {"flags": flags, "do_cache": True}
