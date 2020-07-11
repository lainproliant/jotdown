/*
 * query.cpp
 *
 * Author: Lain Musgrove (lain.proliant@gmail.com)
 * Date: Saturday July 11, 2020
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
std::vector<std::pair<std::string, std::string>> type_sequence_zip(
    const std::vector<obj_t>& objects,
    const std::vector<Object::Type>& types) {
    std::vector<std::pair<std::string, std::string>> reprs;

    for (size_t x = 0; x < objects.size() || x < types.size(); x++) {
        std::string obj_str = "???";
        std::string type_str = "???";

        if (x < objects.size()) {
            obj_str = Object::type_name(objects[x]->type());
        }

        if (x < types.size()) {
            type_str = Object::type_name(types[x]);
        }

        reprs.push_back({obj_str, type_str});
    }

    return reprs;
}

// ------------------------------------------------------------------
void assert_type_sequence(const std::vector<obj_t>& objects,
                          const std::vector<Object::Type>& types) {

    auto reprs = type_sequence_zip(objects, types);
    for (const auto& repr : reprs) {
        tfm::printf("%20s %-20s\n", repr.first, repr.second);
    }
    assert_equal(objects.size(), types.size(), tfm::format("type sequence size %d == objects size %d",
                                                  types.size(), objects.size()));
    for (size_t x = 0; x < types.size(); x++) {
        assert_equal(types[x], objects[x]->type(),
                     tfm::format("%s == %s", Object::type_name(types[x]),
                                 Object::type_name(objects[x]->type())));
    }
}

// ------------------------------------------------------------------
int main() {
    return TestSuite("jotdown queries")
    .test("children", []() {
        auto doc = jotdown::load("data/query/children.md");
        auto results = jotdown::q(doc, ">/section/>");
        assert_type_sequence(results, {
            Object::Type::UNORDERED_LIST,
            Object::Type::LINE_BREAK,
            Object::Type::TEXT_CONTENT
        });
        return true;
    })
    .test("descendents", []() {
        auto doc = jotdown::load("data/query/children.md");
        auto results = jotdown::q(doc, ">>");
        assert_type_sequence(results, {
            Object::Type::SECTION,
            Object::Type::UNORDERED_LIST,
            Object::Type::LINE_BREAK,
            Object::Type::TEXT_CONTENT,
            Object::Type::UNORDERED_LIST_ITEM,
            Object::Type::UNORDERED_LIST_ITEM,
            Object::Type::UNORDERED_LIST,
            Object::Type::UNORDERED_LIST_ITEM,
            Object::Type::TEXT
        });
    })
    .test("antecedents", []() {
        auto doc = jotdown::load("data/query/children.md");
        auto results = jotdown::q(doc, ">>/uli/level/2/<<");
        assert_type_sequence(results, {
            Object::Type::UNORDERED_LIST,
            Object::Type::UNORDERED_LIST_ITEM,
            Object::Type::UNORDERED_LIST,
            Object::Type::SECTION,
            Object::Type::DOCUMENT
        });
    })
    .run();
}
