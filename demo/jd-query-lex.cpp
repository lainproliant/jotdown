/*
 * jd-query-lex.cpp
 *
 * Author: Lain Musgrove (lain.proliant@gmail.com)
 * Date: Sunday September 11, 2022
 *
 * Distributed under terms of the MIT license.
 */

#include "jotdown/query.h"

int main(int argc, char** argv) {
    moonlight::lex::Lexer lex;
    auto grammar = jotdown::make_query_grammar();
    lex.debug_print_tokens(grammar);
    return 0;
}
