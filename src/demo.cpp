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
#include "moonlight/core.h"

int main() {
    auto infile = moonlight::file::open_r("README.md");
    jotdown::Parser parser(infile);

    for (auto iter = parser.begin();
         iter != parser.end();
         iter++) {
        auto tk = *iter;
        std::cout << tk->repr() << std::endl;
    }

    return 0;
}
