/*
 * query.h
 *
 * Author: Lain Musgrove (lain.proliant@gmail.com)
 * Date: Wednesday June 17, 2020
 *
 * Distributed under terms of the MIT license.
 */

#ifndef __JOTDOWN_QUERY_H
#define __JOTDOWN_QUERY_H

#include "moonlight/slice.h"
#include "moonlight/classify.h"
#include "moonlight/lex.h"
#include "jotdown/error.h"
#include "jotdown/interfaces.h"
#include "jotdown/object.h"
#include "jotdown/utils.h"
#include "tinyformat/tinyformat.h"

#include <regex>
#include <vector>
#include <set>

namespace jotdown {
namespace query {

namespace lex = moonlight::lex;

EXCEPTION_TYPE(QueryError);

inline lex::Grammar::Pointer make_lex_grammar() {
    auto root = lex::Grammar::create();

    auto ignore_whitespace = lex::ignore("\\s");
    auto string = root->sub();
    auto string_escape_seq = string->sub();
    auto block = root->sub();
    auto parenthesis = root->sub();
    auto bracket = root->sub();

    root
        ->def(ignore_whitespace)
        ->def(lex::push("\"", string), "string-begin")
        ->def(lex::push("\\{", block), "block-start")
        ->def(lex::push("\\(", parenthesis), "parenthesis-start")
        ->def(lex::push("\\[", bracket), "bracket-start")
        ->def(lex::match("[0-9]+\\.?[0-9]*"), "number")
        ->def(lex::match("[_\\-a-zA-Z0-9]+"), "bare")
        ->def(lex::match(","), "comma")
        ;

    string
        ->def(lex::push("\\\\", string_escape_seq), "escape-seq")
        ->def(lex::match("[^\"\\\\]+"), "string-content")
        ->def(lex::pop("\""), "string-end")
        ;

    string_escape_seq
        ->def(lex::pop("n"), "newline")
        ->def(lex::pop("t"), "tab")
        ->def(lex::pop("\""), "double-quote")
        ->def(lex::pop("\\\\"), "backslash")
        ;

    block
        ->inherit(root)
        ->def(lex::pop("\\}"), "block-end")
        ;

    parenthesis
        ->inherit(root)
        ->def(lex::pop("\\)"), "parenthesis-end")
        ;

    bracket
        ->inherit(root)
        ->def(lex::pop("\\]"), "bracket-end")
        ;

    return root;
}

}
}

#endif /* !__JOTDOWN_QUERY_H */
