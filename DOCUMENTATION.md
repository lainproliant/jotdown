# Jotdown Documentation
## Overview
This document details the language features and structures defined in the Jotdown parser and compiler.
It also discusses Jotdown Filter Queries and examples of how they can be used.

## Object Types
### Documents
A Document is a top-level structure containing Sections.  All Jotdown files are
represented by a Document when loaded.

#### Example
This document is a Jotdown document.

### Sections
Sections are the primary structural unit in Jotdown.  Sections are hierarchical subdivisions of a
document which give it structure and help to separate its distinct parts visually and contextually.

Syntactically, a `Section` is represented by its header line.  All of the content
following the header line is part of the section, until another header line is encountered at the same
or higher level.

A header line is one or more hash `#` symbols, followed by a whitespace, followed by a single line
of `TextContent`.

Sections can be nested arbitrarily, the level of the header being indicated by the number of hash
`#` symbols at the beginning of its header line.

#### Example
Look around you.  This document is full of examples of sections!  To
demonstrate the hierarchy, this particular section is located under `Jotdown
Documentation`, `Object Types`, `Sections`, `Example`.

#### Caveats
Jotdown documents _should_ begin with a header line.  However, a document may
contain content before the first header line.  In this case, there will be a single
level 0 section, containing the pre-header content and any top-level sections in the
document.

### TextContent
`TextContent` is a container for the textual elements of the document.  It
exists in a few different contexts:

- Paragraph: one or more contiguous lines of text content.
- Section headers: one line of text content following the section header hashes `#`.
- List items: one or more contiguous lines of text content indented to the
  level of the list item.

The following object types may appear within text content:
- Anchors
- Code
- Hashtags
- References

### Anchors
An `Anchor` is a directly referencable point in a document.  It is signified by
a word beginning with an ampersand `&` followed by one or more non-whitespace
characters providing its name.

#### Example
This &sentence is referencable by an anchor named `sentence`.

### Code
`Code` is a way to embed inline code into text content.  It can be used to highlight particular
words in a document as idiomatic, or to wrap text that would unintentionally be interpreted as
other Jotdown objects.

Backticks "` `" are used to enclose inline code, and can be escaped within inline code with
backslashes `\\`.  Double-backslashes are necessary to encode a literal backslash within
the inline code block.

#### Example
To install Jotdown, run `pip install jotdown`.

### Hashtags
A `hashtag` is a searchable case-insensitive keyword or sequence of characters,
typically used to provide subject context to the text before or around it.

It is signified by a word beginning with a single hash `#` followed by one or
more non-whitespace characters providing its name.  The idea of the hashtag
originated from the use of the hash `#` within Internet Relay Chat (IRC)
networks to label groups and topics.  It was suggested for use in the Twitter
social network by @tw:factoryjoe (Chris Messina), and its usage caught on and
entered the common vernacular.

In Jotdown, a hashtag may be used to provide subject context and tie multiple
objects or documents together by way of this subject context.

#### Example
Sometimes it is good to #refactor your #code.

### References
Jotdown is a hypertext language, intended to tie together documents within a
document set.  It also allows linkage to resources outside of Jotdown, such as
local files, email addresses, or myriad other resources.

A `Reference` is signified by a word beginning with an at-sign `@` followed by
one or more non-whitespace characters providing its link address.  Unless
otherwise specified by a hypertext protocol followed by `:`, the link points to
a document in the document set.  Document links may also contain an ampersand
`&` followed by a name indicating the anchor inside the target document being
referenced.

Following the reference target, a square-bracketed `[display text]` may also be
provided.  This is intended for systems that render Jotdown (or convert it to
Markdown) to determine what text to show for a hyperlink rather than the
reference target itself.  To include an open square bracket `[` in the
[ ] reference target (or a backslash) escape it with a backslash `\`.

The language parser/compiler is not concerned with parsing the hypertext protocol,
if it exists.  This work is done by tools built atop Jotdown and is documented
here for posterity.

### Text
`Text` objects are the body of text within `TextContent` forms.  Generally, any text
that is not parsed as another object type will reside within a `Text` object.  Text objects
are additionally split across newline boundaries.

#### Example
This paragraph contains four objects: one text object, `one code object`, and
another two text objects split by the newline in the middle.

#### Example
My cute little image website is at @https://images.lainproliant.com.
You can @mailto:lain.proliant@gmail.com[email] me too.

### LineBreaks
A `LineBreak` is signified by an empty line.  They are used to separate forms, such as
`TextContent` paragraphs and `List` objects that would otherwise be merged together.

#### Example
This is one paragraph.

This is another.

- This is a list.
- The end!

### Lists
Lists can be either "ordered" or "unordered", and contain a homogenous sequence of
list items of the given type.  List items may themselves contain lists of any type,
allowing arbitrary nesting of list structures.  A list item at a particular indent
level signifies the beginning of a list of that type.  If proceeded by a `TextContent`
paragraph form, it must be also preceeded by a `LineBreak`, otherwise it will instead
be parsed as part of the `TextContent` paragraph.

`OrderedListItem` objects are signified by what is called an `ordinal`.  The `ordinal`
is a sequence of alphanumeric symbols terminated by a period and followed by whitespace.

`UnorderedListItem` objects are signified by a single dash `-` followed by whitespace.

A list item "B" following another list item "A", where "B" is at a deeper indent level,
signifies the beginning of nested list of the "B" type embedded within the list item "A".

#### Example

- This is a top level unordered list item.
- This is another unordered list item.
  1. This is an ordered list item nested in an unordered one.
  2. This is another.
  9. Ordinals are arbitrary and order is not enforced.
  9. There may even be duplicates.
  mcmlxxviii. And they may be of any length.

