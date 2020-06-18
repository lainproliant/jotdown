# Node names
```
a           anchor
c           code
C           codeblock
#           hash
L           list (any)
ol          orderedlist
ul          unorderedlist
li          listitem (any)
uli         unorderedlistitem (any)
oli         orderedlistitem (any)
r           ref (aka "link")
s           section
t           text-content
```

## Examples
```
s                       -> all top-level sections
*s                      -> all sections (all levels)
s#1                     -> all top-level h1 sections
*s#3                    -> all h3 sections (all levels)
s1                      -> first section
*s/TODO/                -> any section with "TODO" in the name
*s/TODO/i               -> any section with "TODO" in the name (case insensitive)
*s\\TODO\\i             -> any section with "TODO" in the name (case insensitive, alternative syntax)
*s/TODO/i[0]            -> the first section at any level with "TODO" in the name (case insensitive)
*s/TODO/i[0:3]          -> the first three sections at any level with "TODO" in the name (case insensitive)
*s/TODO/i[-1]           -> the last section at any level with "TODO" in the name (case insensitive)
s[-1]                   -> the last top level section
*s/TODO/[0]l.i.l.i      -> all second-level list items in the first "TODO" section at any level.
*s/TODO/[0]l.i.l.i!`X`  -> all incomplete second-level list items in the first "TODO" section at any level.
*s/TODO/[0]l<i.l.i!`X`  -> all lists containing second-level list items in the first "TODO" section at any level.
```


- `**.a`: select all anchors in the document.
- `s`/`s[0]`: select the first top-level section in the document.
- `s*`: select all top-level sections in the document.
- `s[1].*.li*`: select all list items of top-level lists in the second top level section.
- `s[2].**.li*`: select all list items at any level in the third top-level section.
- `s{Todo Items}.*.li`: select all list items under the top level section matching the regular expression "Todo Items".
- `s=3.t`: Select all text-content under level 3 sections.
