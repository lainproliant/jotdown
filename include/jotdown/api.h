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
#include "jotdown/query.h"

namespace jotdown {

inline std::shared_ptr<Document> load(std::istream& infile, const std::string& filename = "<in>") {
    parser::Parser parser(infile, filename);
    Compiler compiler;
    return compiler.compile(parser.begin(), parser.end());
}

inline std::shared_ptr<Document> load(const std::string& filename) {
    auto infile = moonlight::file::open_r(filename);
    return load(infile, filename);
}

inline void save(std::shared_ptr<const Document> doc, const std::string& filename) {
    auto outfile = moonlight::file::open_w(filename);
    outfile << doc->to_jotdown();
}

inline q::Query query(const std::string& query_str) {
    return q::parse(query_str);
}

inline std::vector<obj_t> query(const std::vector<obj_t>& objects,
                                const std::string& query_str) {
    return q::parse(query_str).select(objects);
}

inline std::vector<obj_t> query(obj_t obj,
                                const std::string& query_str) {
    return query(std::vector<obj_t>{obj}, query_str);
}

}

#endif /* !__JOTDOWN_API_H */
