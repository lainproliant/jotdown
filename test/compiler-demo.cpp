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
    jotdown::parser::Parser parser(infile, "test.md");
    jotdown::compiler::Compiler compiler;

    auto doc = compiler.compile(parser.begin(), parser.end());
    std::cout << doc.to_json().to_string(true) << std::endl;

    return 0;
}
