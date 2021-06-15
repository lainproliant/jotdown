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

namespace json = moonlight::json;

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

    json::Object to_json() const {
        json::Object json;
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

    json::Object to_json() const {
        json::Object json, begin_json, end_json;
        begin_json.set<float>("line", begin.line);
        begin_json.set<float>("col", begin.col);
        end_json.set<float>("line", end.line);
        end_json.set<float>("col", end.col);
        json.set("begin", begin_json);
        json.set("end", end_json);
        return json;
    }

    bool operator==(const Range& other) const {
        return begin == other.begin && end == other.end;
    }
};

}

#endif /* !__JOTDOWN_INTERFACES_H */
