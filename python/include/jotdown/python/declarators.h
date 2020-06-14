/*
 * declarators.h
 *
 * Author: Lain Musgrove (lain.proliant@gmail.com)
 * Date: Friday June 12, 2020
 *
 * Distributed under terms of the MIT license.
 */

#ifndef __JOTDOWN_PYTHON_DECLARATORS_H
#define __JOTDOWN_PYTHON_DECLARATORS_H

#include "pybind11/pybind11.h"

namespace py = pybind11;

namespace jotdown {
namespace python {

// API declarators: api.cpp
void declare_api(py::module& m);

// Base Object declarators: object.cpp
void declare_object_config(py::module& m);
void declare_object_error(py::module& m);
void declare_location(py::module& m);
void declare_object(py::module& m);

// Container declarator: container.cpp
void declare_container(py::module& m);

// Simple object declarators: simple.cpp
void declare_simple_objects(py::module& m);

// Textblock declarator: textblock.cpp
void declare_textblock(py::module& m);

// Base List declarators: list.cpp
void declare_list(py::module& m);

// Ordered List declarators: ordered_list.cpp
void declare_ordered_list(py::module& m);

// Unordered List declarators: unordered_list.cpp
void declare_unordered_list(py::module& m);

// Code Block declarator: codeblock.cpp
void declare_code_block(py::module& m);

// Section declarator: section.cpp
void declare_section(py::module& m);

// Document declarator: document.cpp
void declare_document(py::module& m);

}
}


#endif /* !__JOTDOWN_PYTHON_DECLARATORS_H */
