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
#include "jotdown/query.h"
#include "jotdown/utils.h"
#include "moonlight/core.h"
#include "moonlight/cli.h"

void query_repl(std::shared_ptr<jotdown::object::Document> doc) {
    for (;;) {
        std::string str;
        std::cout << "jdq> ";
        std::cout.flush();
        std::cin >> str;

        if (str == "exit" || str == "quit") {
            break;
        }

        auto tokens = jotdown::query::tokenize(str);
        std::cout << moonlight::str::join(moonlight::collect::map<std::string>(tokens, [](auto token) -> std::string { return tfm::format("\"%s\"", jotdown::strliteral(token)); }), ",") << std::endl;
        jotdown::query::Query query;
        try {
            query = jotdown::query::parse(str);
        } catch (const std::exception& e) {
            std::cout << "ERROR: " << e.what() << std::endl;
            continue;
        }
        std::cout << query.repr() << std::endl;
        auto results = query.select(doc->contents());
        for (auto result : results) {
            std::cout << result->repr() << std::endl;
        }
    }
}

std::shared_ptr<jotdown::object::Document> load(std::istream& input, const std::string& name = "<input>") {
    jotdown::parser::Parser parser(input, name);
    jotdown::compiler::Compiler compiler;
    return compiler.compile(parser.begin(), parser.end());
}

int main(int argc, char** argv) {
    try {
        auto cmd = moonlight::cli::parse(argc, argv);
        std::shared_ptr<jotdown::object::Document> doc;

        if (cmd.args().size() != 1) {
            doc = load(std::cin);
        } else {
            auto infile = moonlight::file::open_r(cmd.args()[0]);
            doc = load(infile);
        }

        query_repl(doc);

    } catch(...) {
        std::cout << "oh no mr bill!";
    }

    return 0;
}
