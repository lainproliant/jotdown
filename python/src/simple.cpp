/*
 * simple.cpp
 *
 * Author: Lain Musgrove (lain.proliant@gmail.com)
 * Date: Saturday June 14, 2020
 *
 * Distributed under terms of the MIT license.
 */

#include "pybind11/pybind11.h"
#include "pybind11/stl.h"
#include "pybind11/operators.h"
#include "jotdown/python/declarators.h"
#include "jotdown/object.h"

#include <functional>

namespace py = pybind11;

namespace jotdown {
namespace python {

//-------------------------------------------------------------------
shared_class<object::Anchor> declare_anchor(py::module& m, obj_class& obj) {
    auto anchor = shared_class<object::Anchor>(m, "Anchor", obj)
        .def(py::init<const std::string&>(), py::arg("name"))
        .def_property_readonly("name", &object::Anchor::name)
        .def("__repr__", [](const object::Anchor& self) {
            return tfm::format("%s<%s>",
                               self.type_name(self.type()),
                               self.name());
        });
    return anchor;
}

//-------------------------------------------------------------------
shared_class<object::Text> declare_text(py::module& m, obj_class& obj) {
    auto text = shared_class<object::Text>(m, "Text", obj)
        .def(py::init<const std::string&>(), py::arg("text"))
        .def_property_readonly("text", &object::Text::text)
        .def("__repr__", [](const object::Text& self) {
            return tfm::format("%s<%s>",
                               self.type_name(self.type()),
                               strliteral(self.text()));
        });
    return text;
}

//-------------------------------------------------------------------
shared_class<object::Hashtag> declare_hashtag(py::module& m, obj_class& obj) {
    auto hashtag = shared_class<object::Hashtag>(m, "Hashtag", obj)
        .def(py::init<const std::string&>(), py::arg("tag"))
        .def_property_readonly("tag", &object::Hashtag::tag)
        .def("__repr__", [](const object::Hashtag& self) {
            return tfm::format("%s<%s>",
                               self.type_name(self.type()),
                               self.tag());
        });
    return hashtag;
}

//-------------------------------------------------------------------
shared_class<object::LineBreak> declare_line_break(py::module& m, obj_class& obj) {
    auto line_break = shared_class<object::LineBreak>(m, "LineBreak", obj)
        .def(py::init<>());
    return line_break;
}

//-------------------------------------------------------------------
shared_class<object::Code> declare_code(py::module& m, obj_class& obj) {
    auto code = shared_class<object::Code>(m, "Code", obj)
        .def(py::init<const std::string&>(), py::arg("code"))
        .def_property_readonly("code", &object::Code::code);
    return code;
}

//-------------------------------------------------------------------
shared_class<object::Ref> declare_ref(py::module& m, obj_class& obj) {
    auto ref = shared_class<object::Ref>(m, "Ref", obj)
        .def(py::init<const std::string&, const std::string&>(),
             py::arg("link"), py::arg("text") = "")
        .def_property_readonly("link", &object::Ref::link)
        .def_property_readonly("text", &object::Ref::text)
        .def("__repr__", [](const object::Ref& self) {
            if (self.link() == self.text()) {
                return tfm::format("%s<%s>",
                                   self.type_name(self.type()),
                                   self.link());
            } else {
                return tfm::format("%s<%s (%s)>",
                                   self.type_name(self.type()),
                                   self.link(),
                                   self.text());
            }
        });
    return ref;
}

//-------------------------------------------------------------------
shared_class<object::CodeBlock> declare_code_block(py::module& m, obj_class& obj) {
    auto code_block = shared_class<object::CodeBlock>(m, "CodeBlock", obj)
        .def(py::init<const std::string&, const std::string&>(),
             py::arg("code"), py::arg("language") = "")
        .def_property_readonly("code", &object::CodeBlock::code)
        .def_property_readonly("language", &object::CodeBlock::language)
        .def("__repr__", [](const object::CodeBlock& self) {
            if (self.language().size() > 0) {
                return tfm::format("%s<(%s) %s>",
                                   self.type_name(self.type()),
                                   self.language(),
                                   strliteral(self.code()));
            } else {
                return tfm::format("%s<%s>",
                                   self.type_name(self.type()),
                                   strliteral(self.code()));
            }
        });
    return code_block;
}

}
}
