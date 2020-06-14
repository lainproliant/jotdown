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
    declare_object(m);

    // Container declarator: container.cpp
    declare_container(m);

    // Simple object declarators: simple.cpp
    declare_simple_objects(m);

    // Textblock declarator: textblock.cpp
    declare_textblock(m);

    // Base List declarators: list.cpp
    declare_list(m);

    // Ordered List declarators: ordered_list.cpp
    declare_ordered_list(m);

    // Unordered List declarators: unordered_list.cpp
    declare_unordered_list(m);

    // Code Block declarator: codeblock.cpp
    declare_code_block(m);

    // Section declarator: section.cpp
    declare_section(m);

    // Document declarator: document.cpp
    declare_document(m);
}

}
}
