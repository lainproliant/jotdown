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

## Examples (revised)
```
/                       -> all top level objects
                        -> all top level objects
//                      -> all objects at all levels
/s                      -> all top-level sections
s                       -> all top-level sections
//s                     -> all sections at all levels.
//s{test}               -> all sections at all levels with 'test' in the name.
//s2                    -> all level 2 sections at all levels. 
//l<c>                  -> all lists containing checklist items.
//l<c/#shopping>        -> all lists contianing checklist items with the 'shopping' hashtag.
//#                     -> all hashtags at all levels.
//ul<c>                 -> all unordered lists with checklist items.
//ul<*c><!*c{X}>        -> all unordered lists who's direct children are all checklist items
                           and who's direct children are not all completed checklist items.
//c/^                   -> select all direct parents of all checklist items.
//c/^/^                 -> select all grandparents of all checklist items.
//uli1                  -> select all level-1 unordered list items.
```

