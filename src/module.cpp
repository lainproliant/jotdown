/*
 * module.cpp
 *
 * Author: Lain Musgrove (lain.proliant@gmail.com)
 * Date: Thursday June 11, 2020
 *
 * Distributed under terms of the MIT license.
 */

#include "pybind11/pybind11.h"
#include "jotdown/python/declarators.h"

namespace py = pybind11;

namespace jotdown {
namespace python {

PYBIND11_MODULE(jotdown, m) {
    // API declarators: api.cpp
    declare_api(m);

    // Base Object declarators: object.cpp
    declare_object_config(m);
    declare_object_error(m);
    declare_location(m);
    auto obj = declare_object(m);

    // Container declarator: container.cpp
    auto container = declare_container(m, obj);

    // Simple object declarators: simple.cpp
    auto anchor = declare_anchor(m, obj);
    auto text = declare_text(m, obj);
    auto hashtag = declare_hashtag(m, obj);
    auto line_break = declare_line_break(m, obj);
    auto code = declare_code(m, obj);
    auto ref = declare_ref(m, obj);
    auto code_block = declare_code_block(m, obj);

    // Textblock declarator: textblock.cpp
    auto text_content = declare_text_content(m, container);

    // Base List declarators: list.cpp
    auto list = declare_list(m, container);
    auto li = declare_list_item(m, container);

    // Ordered List declarators: ordered_list.cpp
    auto ol = declare_ordered_list(m, list);
    auto oli = declare_ordered_list_item(m, li);

    // Unordered List declarators: unordered_list.cpp
    auto ul = declare_unordered_list(m, list);
    auto uli = declare_unordered_list_item(m, li);

    // Section declarator: section.cpp
    auto section = declare_section(m, container);

    // Document declarator: document.cpp
    auto document = declare_document(m, container);
}

}
}
