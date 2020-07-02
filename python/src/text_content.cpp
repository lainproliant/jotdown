/*
 * text_content.cpp
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
shared_class<object::TextContent> declare_text_content(
    py::module& m,
    shared_class<object::Container>& container) {

    auto text_content = shared_class<object::TextContent>(
        m, "TextContent", container)
        .def(py::init<>())
        .def("add", [](object::TextContent& self,
                       std::shared_ptr<object::Anchor> anchor) {
            return self.add(anchor);
        })
        .def("add", [](object::TextContent& self,
                       std::shared_ptr<object::Hashtag> hashtag) {
            return self.add(hashtag);
        })
        .def("add", [](object::TextContent& self,
                       std::shared_ptr<object::Code> code) {
            return self.add(code);
        })
        .def("add", [](object::TextContent& self,
                       std::shared_ptr<object::Ref> ref) {
            return self.add(ref);
        })
        .def("add", [](object::TextContent& self,
                       std::shared_ptr<object::Text> text) {
            return self.add(text);
        });

    return text_content;
}

}
}
