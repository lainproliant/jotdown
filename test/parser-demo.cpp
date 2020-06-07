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
    auto infile = moonlight::file::open_r("README.md");
    jotdown::parser::Parser parser(infile);
    jotdown::compiler::Compiler compiler;
    std::vector<jotdown::parser::token_t> tokens;

    for (auto iter = parser.begin();
         iter != parser.end();
         iter++) {
        auto tk = *iter;
        std::cout << tk->repr() << std::endl;
        tokens.push_back(tk);
    }

    auto doc = compiler.compile(tokens.begin(), tokens.end());
    std::cout << doc.to_json().to_string(true) << std::endl;

    return 0;
}
