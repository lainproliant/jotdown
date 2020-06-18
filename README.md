# Jotdown
Jotdown is a structured document language.  It is an opinionated, augmented
subset of Markdown with features inspired by plain-text organizational tools
like org-mode.  The main purpose of Jotdown is to centralize personal and
project organization into a set of plain-text documents that can be easily
edited, parsed, and manipulated both by scripts and humans alike.

Jotdown can be parsed into a list of plain tokens or into a compiled DOM
(Document Object Model).  From the DOM it can then be translated into Markdown
for formatting and display purposes, or back into Jotdown after being modified
in its structured form.

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

## Anti-Goals
These are specific anti-goals: intended limitations to the scope of the project:

- Full compatibility with Markdown.
- First class support for layout, display, and formatting.
    - Jotdown is less concerned with the formatting of text than it is with the
        content of text as a structured document.  Formatting is left up to the
        user to determine, though it is advised to use Markdown formatting where
        possible if you intend to render your document by converting it to
        Markdown first.
- Jotdown -> HTML conversion.
    - This can be achieved indirectly via Jotdown -> Markdown conversion.

## Q/A
### Why not just use Markdown for this?
While Markdown is great as a text formatting language, it doesn't work as well
as a structured document language.  It's main focus has always been generating
HTML, and as such many of the existing parsers don't even bother creating an
intermediate AST.  Additionally, I wanted a bit more uniformity across documents
and as such I wanted to make sure there was (as much as feasible) only one way
to represent a specific document structure.  For example, Jotdown forgoes the
"underlined headers" format in favor of only supporting headers with hashes.

### Why is Jotdown offered in C++ and Python?
I wanted to be able to support the runtime power and expressiveness of C++ while
also extending support into Python where scripts and plugins can be easily built
around Jotdown document structures.

### Why is my language not supported?
I'm sorry!  I'm really happy you're interested in Jotdown though.  Feel free to
submit an issue to add support for your language.  I can't guarantee I'll ever
get around to writing bindings myself, but someone in the community might step
up and help out!

### Why are there functions for shifting the order of objects in containers?
I intend to use these functions to facilitate high-level document editing
features in an upcoming Vim plugin and toolchain for Jotdown document sets.
