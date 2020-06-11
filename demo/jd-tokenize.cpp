/*
 * parse.cpp
 *
 * Author: Lain Musgrove (lain.proliant@gmail.com)
 * Date: Wednesday May 27, 2020
 *
 * Distributed under terms of the MIT license.
 */

#include <iostream>
#include <algorithm>
#include "jotdown/parser.h"
#include "jotdown/compiler.h"
#include "jotdown/object.h"
#include "moonlight/core.h"
#include "moonlight/cli.h"

std::vector<jotdown::parser::token_t> load(std::istream& infile, const std::string& name = "<input>") {
    jotdown::parser::Parser parser(infile, name);
    std::vector<jotdown::parser::token_t> result;
    for (auto iter = parser.begin();
         iter != parser.end();
         iter++) {
        result.push_back(*iter);
    }
    return result;
}

int main(int argc, char** argv) {
    auto cmd = moonlight::cli::parse(argc, argv);
    std::vector<jotdown::parser::token_t> tokens;

    if (cmd.args().size() != 1) {
        tokens = load(std::cin);
    } else {
        auto infile = moonlight::file::open_r(cmd.args()[0]);
        tokens = load(infile, cmd.args()[0]);
    }

    for (auto tk : tokens) {
        std::cout << tk->repr() << std::endl;
    }

    return 0;
}
