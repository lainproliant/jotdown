/*
 * api.h
 *
 * Author: Lain Musgrove (lain.proliant@gmail.com)
 * Date: Thursday June 11, 2020
 *
 * Distributed under terms of the MIT license.
 */

#ifndef __JOTDOWN_API_H
#define __JOTDOWN_API_H

#include "jotdown/object.h"
#include "jotdown/parser.h"
#include "jotdown/compiler.h"
#include "moonlight/core.h"

namespace jotdown {

typedef object::Document Document;

std::shared_ptr<Document> load(const std::string& filename) {
    auto infile = moonlight::file::open_r(filename);
    parser::Parser parser(infile, filename);
    compiler::Compiler compiler;
    return compiler.compile(parser.begin(), parser.end());
}

void save(std::shared_ptr<Document> doc, const std::string& filename) {
    auto outfile = moonlight::file::open_w(filename);
    outfile << doc->to_jotdown();
}

}

#endif /* !__JOTDOWN_API_H */
