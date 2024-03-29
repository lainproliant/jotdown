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
#include "moonlight/cli.h"

std::shared_ptr<jotdown::Document> load(std::istream& input, const std::string& name = "<input>") {
    jotdown::parser::Parser parser(input, name);
    jotdown::Compiler compiler;
    return compiler.compile(parser.begin(), parser.end());
}

int main(int argc, char** argv) {
    auto cmd = moonlight::cli::parse(argc, argv);
    std::shared_ptr<jotdown::Document> doc;

    if (cmd.args().size() != 1) {
        doc = load(std::cin);
    } else {
        auto infile = moonlight::file::open_r(cmd.args()[0]);
        doc = load(infile, cmd.args()[0]);
    }

    std::cout << moonlight::json::to_string(doc->to_json()) << std::endl;
    return 0;
}
