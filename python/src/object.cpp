/*
 * object.cpp
 *
 * Author: Lain Musgrove (lain.proliant@gmail.com)
 * Date: Saturday June 13, 2020
 *
 * Distributed under terms of the MIT license.
 */

#include "pybind11/pybind11.h"
#include "jotdown/python/declarators.h"
#include "jotdown/object.h"

namespace py = pybind11;

namespace jotdown {
namespace python {

void declare_object_config(py::module& m) {
    py::class_<object::Config>(m, "ObjectConfig")
        .def(py::init<>())
        .def_readwrite("list_indent", &object::Config::list_indent);
}

void declare_object_error(py::module& m) {
    py::register_exception<object::ObjectError>(m, "ObjectError");
}

void declare_object(py::module& m) {
    py::class_<object::Object> object(m, "Object");

    py::enum_<object::Object::Type>(object, "Type")
        .value("NONE", object::Object::Type::NONE)
        .value("ANCHOR", object::Object::Type::ANCHOR)
        .value("CODE", object::Object::Type::CODE)
        .value("CODE_BLOCK", object::Object::Type::CODE_BLOCK)
        .value("DOCUMENT", object::Object::Type::DOCUMENT)
        .value("HASHTAG", object::Object::Type::HASHTAG)
        .value("LINE_BREAK", object::Object::Type::LINE_BREAK)
        .value("ORDERED_LIST", object::Object::Type::ORDERED_LIST)
        .value("ORDERED_LIST_ITEM", object::Object::Type::ORDERED_LIST_ITEM)
        .value("REF", object::Object::Type::REF)
        .value("SECTION", object::Object::Type::SECTION)
        .value("STATUS", object::Object::Type::STATUS)
        .value("TEXT", object::Object::Type::TEXT)
        .value("TEXT_BLOCK", object::Object::Type::TEXT_CONTENT)
        .value("UNORDERED_LIST", object::Object::Type::UNORDERED_LIST)
        .value("UNORDERED_LIST_ITEM", object::Object::Type::UNORDERED_LIST_ITEM);
}

}
}
