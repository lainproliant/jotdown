# Jotdown - A hypertext markup language for notes.

# Language Definition
## Text `text`
Text content is arbitrary string data contained within or without headers or
inside elements below that contain text content matching the given patterns.

## Reference `ref`
### Grammar
```
ref = '@' ('&' anchor_name:[alphanum+]) | target:[alphanum+|url] | address
```
### Example
```
This sentence contains a reference to another document, see @other_document.
This sentence contains an HTTP hyperlink, see @google.com
This sentence contains an email address, see @lain.proliant@gmail.com
This sentence contains a reference to another section, see @&other_section.
Reference to a section in another document @other_document&section
Reference to a list element in another document @doc&list.1.a.iii
```
TODO document refs further.

## Hashtag
### Grammar
```
hashtag = '#' tag:[alphanum+] 
```
TODO document hashtags further.

## Header `h`
### Grammar
```
header(x) = '#'{x} \s+ title:text \s+ \n
0 < x <= 5
```
### Example
```
# Level 1 Header
## Level 2 Header
### Level 3 Header
#### Level 4 Header
##### Level 5 Header
```
A header is one or more hash characters followed by a space, and then the text
content of the header title.  These correlate to their Markdown equivalents, but
unlike Markdown, this is the only supported header format.  Headers are
inherently nested when `x2 > x1`, and are separate when `x2 <= x1`.

## Unordered List `ul`
### Grammar
```
num = 0-9
alpha = a-z | A-Z
ul(x) = ul_element(x)+
indentation = \s+
ul_element(x) = ul_content(x) | ul(x+1) | ol(x+1, i:[alphanum])
ul_content(x) = indentation{x*n} '-' \s+ content:text \n
                (indentation{x*n} \s* content:text \n)*
0 < x < 20
0 < n
```
### Example
```
- This is a level 1 list item.
    - This item is level 2.
      It has multiple lines, all of which are part of the same element.
    - This is another list element at level 2.
        - This is a list element at level 3.
    - This is the final element at level 2.
        1. Unordered lists may contain ordered sub-lists.
        2. You can't mix ordered and unordered items in the same list.
            a. See "Ordered List" below.
- This is another level 1 list item.
```
An unordered list contains list elements with text content or nested unordered
or ordered lists.  Note that an unordered list may not contain ordered list
elements.  Elements within the same list must be indented to the same level, and
must return to a previous level.

## Ordered List `ol`
### Grammar
```
ol(x, i:[alphanum]) = ol_element(x, i+1) | ul(x+1) | ol(x+1, j:[alphanum])
ol(x) = ol_element(x)+
indentation = \s+
ol_element(x, i:[alphanum]) = ol_content(x, i) | ul(x+1) | ol(x+1, j:[alphanum])
ol_content = indentation{x*n} ${i} '.' \s content:text \n
             (indentation{x*n} \s* content_text \n)*
ul(i:[alphanum]) = ${i} '.' \s 
```
### Example
1. First item.
    a. First item.
    b. Second item.
    c. Third item.
2. Second item.
    - This is an unordered list inside the second item.
        999. This is a new ordered list.
        1000. The numbers can start anywere.
        1. They don't have to be in order.
    - This is a weird example.
        alpha. They can even be words.
        beta. Though the grammer suggests they must be in order, they don't have to be.
        omega. These just happen to be.

An ordered list is just like an unordered list, but each list element has a label.  These labels
don't have to be in order, but they must be unique.
