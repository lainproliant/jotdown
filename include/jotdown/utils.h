/*
 * utils.h
 *
 * Author: Lain Musgrove (lain.proliant@gmail.com)
 * Date: Wednesday May 27, 2020
 *
 * Distributed under terms of the MIT license.
 */

#ifndef __JOTDOWN_UTILS_H
#define __JOTDOWN_UTILS_H

#include <string>
#include <vector>
#include <map>
#include <sstream>

#include "tinyformat/tinyformat.h"

namespace jotdown {

inline std::string strliteral(const std::string& str) {
    static const std::map<char, std::string> ESCAPE_SEQUENCES = {
        {'\a', "\\a"},
        {'\b', "\\b"},
        {'\e', "\\e"},
        {'\f', "\\f"},
        {'\n', "\\n"},
        {'\r', "\\r"},
        {'\t', "\\t"},
        {'\v', "\\v"},
        {'\\', "\\\\"},
        {'"', "\\\""}
    };

    std::stringstream sb;

    for (size_t x = 0; str[x] != '\0'; x++) {
        char c = str[x];
        std::string repr = "";

        auto iter = ESCAPE_SEQUENCES.find(c);
        if (iter != ESCAPE_SEQUENCES.end()) {
            repr = iter->second;
        }

        if (repr == "" && !isprint(c)) {
            repr = tfm::format("\\x%x", c);
        }

        if (repr != "") {
            sb << repr;
        } else {
            sb << c;
        }
    }

    return sb.str();
}

}

#endif /* !__JOTDOWN_UTILS_H */
