## The document itself
`0`

## All top level objects
`*`

## The second top level section
`is section | level 2`

## All objects at all levels
`**`

## All top-level sections
`is section`

## All Sections at all levels
`** | is section`

## All level 2 sections at all levels
`** | is section | level 2`

## All sections at all levels with 'test' in the name
`** | is section | named test`

## All lists containing checklist items at any level
`** | is list | if { ** | is checklist_item }`

## All lists containing checklist items with the 'shopping' hashtag
`** | is checklist | #shopping`

## All hashtags
`** | is hashtag`

## All hyperlinks
`** | is link`

## All first-level unordered lists with checklist items at any level
`** | is ul | level 1 | if { ** | is checklist_item }`

## The last three hashtags in the document
`** | is hashtag | -4:-1`

## Fetch the front matter
`front`

## When querying multiple documents in sequence, get those with a particular author from front matter
`if { front | eq "Lain Musgrove" }`
