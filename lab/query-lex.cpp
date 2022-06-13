/*
 * query-lex.cpp
 *
 * Author: Lain Musgrove (lain.proliant@gmail.com)
 * Date: Sunday May 29, 2022
 *
 * Distributed under terms of the MIT license.
 */

#include <iostream>
#include "moonlight/file.h"
#include "jotdown/query.h"

namespace lex = moonlight::lex;
namespace file = moonlight::file;

int main() {
    lex::Lexer lex;
    auto grammar = jotdown::query::make_lex_grammar();
    auto tokens = lex.lex(grammar, file::to_string(std::cin));
    for (auto tk : tokens) {
        std::cout << tk << std::endl;
    }
}
