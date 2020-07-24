/*
 * front-matter.cpp
 *
 * Author: Lain Musgrove (lain.proliant@gmail.com)
 * Date: Friday July 24, 2020
 *
 * Distributed under terms of the MIT license.
 */

#include "jotdown/api.h"
#include "jotdown/object.h"
#include "jotdown/query.h"
#include "moonlight/test.h"
#include "moonlight/json.h"

using namespace moonlight::test;
using Object = jotdown::object::Object;
using obj_t = std::shared_ptr<Object>;

// ------------------------------------------------------------------
int main() {
    return TestSuite("jotdown front-matter")
    .test("front-matter no langspec", []() {
        auto doc = jotdown::load("data/front-matter/front-matter-no-langspec.md");
        auto front_matter = doc->front_matter();
        assert_true(front_matter != nullptr);
        return true;
    })
    .test("front-matter json", []() {
        auto doc = jotdown::load("data/front-matter/front-matter-json.md");
        auto front_matter = doc->front_matter();
        assert_true(front_matter != nullptr);
        assert_equal(front_matter->language(), std::string("json"));
        auto json = moonlight::json::load_from_string(front_matter->code());
        assert_equal(json.get<std::string>("name"), std::string("Front Matter JSON"));
        assert_equal(json.get<std::string>("author"), std::string("Lain Musgrove"));
    })
    .run();
}
