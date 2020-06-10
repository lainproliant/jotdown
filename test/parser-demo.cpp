/*
 * parse.cpp
 *
 * Author: Lain Musgrove (lain.proliant@gmail.com)
 * Date: Wednesday May 27, 2020
 *
 * Distributed under terms of the MIT license.
 */

#include <iostream>
#include "jotdown/parser.h"
#include "jotdown/compiler.h"
#include "jotdown/object.h"
#include "moonlight/core.h"

int main() {
    auto infile = moonlight::file::open_r("test.md");
    jotdown::parser::Parser parser(infile);
    std::vector<jotdown::parser::token_t> tokens;

    for (auto iter = parser.begin();
         iter != parser.end();
         iter++) {
        tokens.push_back(*iter);
    }

    for (auto tk : tokens) {
        std::cout << tk->repr() << std::endl;
    }

    return 0;
}
