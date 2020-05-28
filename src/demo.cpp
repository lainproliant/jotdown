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

int main() {
    jotdown::Parser parser(std::cin);

    for (auto iter = parser.begin();
         iter != parser.end();
         iter++) {
        std::cout << *iter << std::endl;
    }
}
