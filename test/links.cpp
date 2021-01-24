/*
 * links.cpp
 *
 * Author: Lain Musgrove (lain.proliant@gmail.com)
 * Date: Friday January 22, 2021
 *
 * Distributed under terms of the MIT license.
 */

#include "jotdown/api.h"
#include "jotdown/object.h"
#include "jotdown/query.h"
#include "moonlight/test.h"

using namespace moonlight::test;
using Object = jotdown::object::Object;
using obj_t = std::shared_ptr<Object>;

// ------------------------------------------------------------------
int main() {
    return TestSuite("jotdown links")
    .test("Parsing various link types", []() {
        auto doc = jotdown::load("data/links/links.md");
    })
    .run();
}
