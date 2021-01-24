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
#include "moonlight/cli.h"

std::shared_ptr<jotdown::object::Document> load(std::istream& input, const std::string& name = "<input>") {
    jotdown::parser::Parser parser(input, name);
    jotdown::compiler::Compiler compiler;
    return compiler.compile(parser.begin(), parser.end());
}

int main(int argc, char** argv) {
    auto cmd = moonlight::cli::parse(argc, argv);
    std::shared_ptr<jotdown::object::Document> doc;

    if (cmd.args().size() != 1) {
        doc = load(std::cin);
    } else {
        auto infile = moonlight::file::open_r(cmd.args()[0]);
        doc = load(infile);
    }
    std::cout << doc->to_jotdown();

    return 0;
}
