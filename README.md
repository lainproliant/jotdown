# Jotdown
Jotdown is a structured document language.  It is an opinionated, augmented
subset of Markdown with features inspired by plain-text organizational tools
like org-mode.  The main purpose of Jotdown is to centralize personal and
project organization into a set of plain-text documents that can be easily
edited, parsed, and manipulated both by scripts and humans alike.

Jotdown is offered simultaneously as a header-only C++ library and a compiled
Python module sharing the same code.  The Python bindings are enabled via
pybind11.  It is released under an MIT license, see LICENSE for more info.

See [DOCUMENTATION.md] for a detailed explanation of the language and its
features.

See [API_CXX.md] for documentation regarding the header-only C++ library.

See [API_PY.md] for documentation regarding the Python module, including how to
build and install it.

## Goals
These are the primary goals of the Jotdown language and project:

- Support the creation, analysis, and modification of plain-text structured
    documents.
- Support the hypertext linkage of documents within a document-set, and to
    outside resources on the web or elsewhere.
- First-class support for loading, querying, modifying, saving, and creating
    documents in C++ and Python.

## Q/A
### Why not just use Markdown for this?
While Markdown is great as a text formatting language, it doesn't work as an
easily parsable structured document language.  It's main focus has always been 
generating HTML, and as such many of the existing parsers don't even bother
creating an intermediate AST.  I also feel that Markdown, though it is formally
defined in the CommonMark and Github Flavored Markdown specs, has hugely complex
rules for parsing to support features that aren't often used or don't need to
exist.

### Why is Jotdown offered in C++ and Python?
I wanted to be able to support the runtime power and expressiveness of C++ while
also extending support into Python where scripts and plugins can be easily built
around Jotdown document structures.

### Why is my language not supported?
I'm sorry!  I'm really happy you're interested in Jotdown though.  Feel free to
submit an issue to add support for your language.  I can't guarantee I'll ever
get around to writing bindings myself, but someone in the community might step
up and help out!

### What are you planning to do with Jotdown in the future?
Some things I'm planning on building around Jotdown include:

- A Vim plugin which will support advanced editing features and highlighting.
- Command line productivity tools built around the concept of a Jotdown document set.
- A web wiki built around the concept of a Jotdown document set.

# Change Log
### 01/24/2021: v2.0.0
- Removed support for custom '@' style links, replaced with Markdown style
  links and link indexes.

### 07/24/2020: v1.3.0
- Added support for document front-matter.
- New features for the Python interface:
  - Collections are now callable.  With no arguments, calling them will return
    their contents as a list.  With arguments, they will return themselves with
    all of the arguments being added in sequence.
  - Section can now be constructed with a string or a TextContent for the header.
  - TextContent can now be initialized with a string.

### 07/17/2020: v1.2.0
- Refactored `objects.h` to do runtime validation of insertion types rather than
  limiting via the C++ type system to reduce code duplication.
- Added `Container::insert_before` and `Container::insert_after`.

