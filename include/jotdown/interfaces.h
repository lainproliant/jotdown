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

namespace jotdown {

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
};

const struct Location NOWHERE = {"<none>", -1, -1};

}

#endif /* !__JOTDOWN_INTERFACES_H */
