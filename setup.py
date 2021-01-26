from pathlib import Path
from setuptools import setup
from setuptools.extension import Extension

pymodule = Extension(
    name="jotdown",
    include_dirs=["include", "moonlight/include", "pybind11/include"],
    sources=[str(src) for src in Path.cwd().glob("src/*.cpp")],
    extra_compile_args=["-O3", "-shared", "-Wall", "-std=c++2a", "-fPIC"],
)

# Get the long description from the README file
with open(Path("README.md"), encoding="utf-8") as f:
    long_description = f.read()

setup(
    name="jotdown",
    version="2.0.1",
    description="Jotdown structrured document language, C++ to python wrapper module.",
    long_description=long_description,
    long_description_content_type="text/markdown",
    url="https://github.com/lainproliant/jotdown",
    author="Lain Musgrove (lainproliant)",
    author_email="lainproliant@gmail.com",
    license="BSD",
    classifiers=[
        "Development Status :: 5 - Production/Stable",
        "Intended Audience :: Developers",
        "Topic :: Software Development :: Libraries :: Application Frameworks",
        "License :: OSI Approved :: BSD License",
        "Programming Language :: Python :: 3.8",
    ],
    keywords="document structure parser query language",
    ext_modules=[pymodule],
    zip_safe=False,
    include_package_data=True,
)
