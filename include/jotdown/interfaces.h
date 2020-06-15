/*
 * interfaces.h
 *
 * Author: Lain Musgrove (lain.proliant@gmail.com)
 * Date: Tuesday May 26, 2020
 *
 * Distributed under terms of the MIT license.
 */

#ifndef __JOTDOWN_INTERFACES_H
#define __JOTDOWN_INTERFACES_H

#include <string>
#include "moonlight/json.h"

namespace jotdown {

using JSON = moonlight::json::Wrapper;

// ------------------------------------------------------------------
struct Location {
    std::string filename;
    int line, col;

    static const Location& nowhere() {
        static Location nowhere = {
            "<none>", -1, -1
        };
        return nowhere;
    }

    bool operator==(const Location& other) const {
        return (
            filename == other.filename &&
            line == other.line &&
            col == other.col
        );
    }

    bool operator!=(const Location& other) const {
        return ! operator==(other);
    }

    JSON to_json() const {
        JSON json;
        json.set<std::string>("filename", filename);
        json.set<float>("line", line);
        json.set<float>("col", col);
        return json;
    }
};

const struct Location NOWHERE = {"<none>", -1, -1};

//-------------------------------------------------------------------
struct Range {
    Location begin;
    Location end;

    JSON to_json() const {
        JSON json, begin_json, end_json;
        begin_json.set<float>("line", begin.line);
        begin_json.set<float>("col", begin.col);
        end_json.set<float>("line", end.line);
        end_json.set<float>("col", end.col);
        json.set_object("begin", begin_json);
        json.set_object("end", end_json);
        return json;
    }

    bool operator==(const Range& other) const {
        return begin == other.begin && end == other.end;
    }
};

}

#endif /* !__JOTDOWN_INTERFACES_H */
