# Jotdown Documentation
This section details the structures present in a Jotdown structured document. 

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
- Text

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
Lists can be either "ordered" or "unordered", and contain a homogenous sequence
of list items of the given type.  List items may themselves contain lists of
any type, allowing arbitrary nesting of list structures.  A list item at a
particular indent level signifies the beginning of a list of that type.  If
proceeded by a `TextContent` paragraph form, it must be also preceeded by a
`LineBreak`, otherwise it will instead be parsed as part of the `TextContent`
paragraph.

`OrderedListItem` objects are signified by what is called an `ordinal`.  The
`ordinal` is a sequence of alphanumeric symbols terminated by a period and
followed by whitespace.

`UnorderedListItem` objects are signified by a single dash `-` followed by
whitespace.

A list item "B" following another list item "A", where "B" is at a deeper
indent level, signifies the beginning of nested list of the "B" type embedded
within the list item "A".

List items may also contain a `status` property, signified by square brackets
`[ ]` enclosing the status of the list item at the beginning of its text.  This
special status text will not be included in the list item's label, but is
available in the API via the `status` property.  If there is no status, this
property will return an empty string.

#### Example

- This is a top level unordered list item.
- This is another unordered list item.
  1. This is an ordered list item nested in an unordered one.
  2. This is another.
  9. Ordinals are arbitrary and order is not enforced.
  9. There may even be duplicates.
  mcmlxxviii. And they may be of any length.

# Jotdown Filter Queries
This section describes Jotdown Filter Queries (JFQ), a method by which
structured information and search results can be gathered from one or more
Jotdown documents.

## Overview
### Basic Syntax and Behavior
A filter query is a single line of slash `/` delimited tokens, which represent
what items to filter from a progressive set of items.  Most of the tokens are
filters, but some represent transformations, such as fetching the children or
parents of items in the set.

### Guarantees
- The result set will always be unique, i.e. the same exact object instance
  will never appear more than once in the result set.  If two equivalent
  objects are in the result set, their ranges will differ.
- Objects will be returned in the order in which they are presented in the 
  document.

### Containers and Labels
Sections and List Items are containers in more than one way.  In particular,
these types are special in that they have direct contents, but they also contain
a `TextContent` element representing their label.  The Section label is the
text content of its header, and the List Item label is the text content of
the list item.

## Token Types
### Children w/Labels: `*`
Selects all of the direct children of items in the current set, and their
labels if they have labels.

### Descendants w/Labels: `**`
Recursively selects all of the children of items in the current set, and
all labels if any objects in the hierarchy have them.

### Direct Contents: `>`
Selects all of the direct children of the items in the current set.  This will
not include any label objects.

### Descendants: `>>`
Recursively selects all of the children of items in the current set.  This will
not include any label objects.

### Parents: `<`
Selects all of the direct parents of items in the current set.  Objects with
labels are the parents of their label objects.

### Antecedents (Ancestors): `<<`
Recursively selects all of the parents of items in the current set.  Objects
with labels are the parents of their label objects.

### Label: `label`
Selects all of the label objects of items in the current set, if they have
labels.

### Contains: `contains/{rest}...`
Selects all objects in the current set, where the result set of each item's
contents filtered by the remainder of the filter query is not empty.

In other words, this token selects items that contain objects that match the
remainder of the query.  of the filter query following `contains` is not empty.

### Not: `not/{next}`, `!/{next}`
Selects all objects in the current set which do _NOT_ match the next token.

### Search: `~{rx}`, `search/{rx}`
Selects all objects in the current set matching the given regular expression.

### Level: `level`
Select all objects in the current set at the given nesting level.  This only
applies to sections and list items currently.

### Line Break: `line_break`, `br`
Select all line break objects in the current set.

### Text: `text`, `t`
Select all text objects in the current set.  These are the plain text objects
within text contents such as paragraphs and labels.

### Text Content: `content`
Select all text content objects in the current set.  This includes labels
and paragraphs if they are present in the current set.

### List: `list`
Select all lists, ordered or unordered, in the current set.

### Ordered List: `ordered_list`, `ol`
Select all ordered lists in the current set.

### Unordered List: `unordered_list`, `ul`
Select all unordered lists in the current set.

### Check List: `check_list`, `task_list`
Select all "checklists" in the current set.  Checklists are lists that contain
any direct list items with a non-empty status property.

### Status: `status/{status}`
Select all checklist items in the current set with the given case-insensitive
status.

### List Item: `item`, `list_item`, `li`
Select all list items in the current set.

### Ordinal Select: `ordinal/{ord}`, `ord/{ord}`
Select all ordered list items with the given case-insensitive ordinal.

### Ordered List Item: `ordered_list_item`, `oli`:
Select all ordered list items in the current set.

### Unordered List Item: `unordered_list_item`, `uli`:
Select all unordered list items in the current set.

### Section: `section`, `s`
Select all sections in the current set.

### Check List Item / Task: `task`, `check_item`, `task_item`:
Select all checklist/task list items in the current set.  These are ordered or
unordered list items with a non-empty status property.

### Hashtag Select: `#{tag}`
Select all hashtags with the given case-insensitive tag.  If no tag is provided,
all hashtags match this token.

### Hashtag Search: `#~{rx}` [TODO]
Select all hashtags in the reuslt set where the tag matches the given
case-insensntive regular expression.

### Reference Select: `@{text}`
Select all references in the result set where the link property contains the
given text.

### Reference Search: `@~{rx}` [TODO]
Select all references in the result set where the link property matches the
given regular expression.

### Reference Search (case-insensitive): `@i~{rx}` [TODO]
Select all references in the result set where the link property matches the
given case insensitive regular expression.

### Reference Text Search: `@t~{rx}` [TODO]
Select all references in the result set where the text property matches the
given regular expression.

### Reference Text Search (case-insensitive): `@ti~{rx}`, `@it~{rx}` [TODO]
Select all references in the result set where the text property matches the
given case-insensitive regular expression.

