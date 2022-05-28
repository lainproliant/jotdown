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
    auto string_literal = root->sub();
    auto string_escape_seq = string_literal->sub();

    root
        ->def(ignore_whitespace)
        ->def(lex::push("\"", string_literal), "str-literal-begin")
        ->def(lex::match("[^\"]+"), "bare")
        ;

    string_literal
        ->def(lex::push("\\", string_escape_seq), "escape-seq")
        ->def(lex::match("[^\"\\]+"), "string-content")
        ->def(lex::pop("\""), "str-literal-end")
        ;

    string_escape_seq
        ->def(lex::pop("n"), "newline")
        ->def(lex::pop("t"), "tab")
        ->def(lex::pop("\""), "double-quote")
        ->def(lex::pop("\\"), "backslash")
        ;

    return root;
}

}
}

#endif /* !__JOTDOWN_QUERY_H */
