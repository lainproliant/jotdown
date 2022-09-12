/*
 * query.h
 *
 * Author: Lain Musgrove (lain.proliant@gmail.com)
 * Date: Thursday August 11, 2022
 *
 * Distributed under terms of the MIT license.
 */

#ifndef __QUERY_H
#define __QUERY_H

#include "moonlight/file.h"
#include "moonlight/automata.h"
#include "moonlight/lex.h"
#include "jotdown/object.h"
#include <string>
#include <vector>
#include <sstream>

namespace jotdown {

namespace lex = moonlight::lex;

static lex::Grammar::Pointer make_query_grammar() {
    auto root = lex::Grammar::create()->named("root");
    auto ignore_whitespace = lex::ignore("\\s");

    auto symbols = root->sub()->named("symbols");
    auto literal = root->sub()->named("literal");
    auto expr = root->sub()->named("expr");
    auto tuple = root->sub()->named("tuple");

    root
        ->def(lex::match("/"), "slash")
        ->else_push(expr);

    symbols
        ->def(ignore_whitespace)
        ->def(lex::match("[0-9]+\\\\.[0-9]+"), "number")
        ->def(lex::match("\\\\.[0-9]+"), "number")
        ->def(lex::match("[0-9]+"), "number")
        ->def(lex::match("[A-Za-z_][A-za-z_0-9]*"), "word")
        ->def(lex::push("\\\"", literal), "open-literal");

    tuple
        ->inherit(expr)
        ->def(lex::pop("\\)"), "close-tuple")
        ->def(lex::match(","), "sep");

    expr
        ->inherit(symbols)
        ->def(lex::push("\\(", tuple), "open-tuple")
        ->else_pop();

    literal
        ->def(lex::match("\\\\\""), "literal-quote")
        ->def(lex::pop("\\\""), "close-literal")
        ->def(lex::match("[^\\\"]+"), "literal-chars");

    return root;
}

}

#endif /* !__QUERY_H */
