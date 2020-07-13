/*
 * section.cpp
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
shared_class<object::Section> declare_section(
    py::module& m,
    shared_class<object::Container>& container) {

    auto section = shared_class<object::Section>(m, "Section", container)
    .def(py::init([](int level) {
        return object::Section::create(level);
    }), py::arg("level") = 0)
    .def_property(
        "header",
        [](const object::Section& self) {
            return self.cheader();
        },
        [](object::Section& self, std::shared_ptr<object::TextContent> header) {
            self.header(header);
        })
    .def("add", [](object::Section& self, std::shared_ptr<object::CodeBlock> code_block) {
        return self.add(code_block);
    })
    .def("add", [](object::Section& self, std::shared_ptr<object::TextContent> text_content) {
        return self.add(text_content);
    })
    .def("add", [](object::Section& self, std::shared_ptr<object::OrderedList> ol) {
        return self.add(ol);
    })
    .def("add", [](object::Section& self, std::shared_ptr<object::UnorderedList> ul) {
        return self.add(ul);
    })
    .def("add", [](object::Section& self, std::shared_ptr<object::LineBreak> line_break) {
        return self.add(line_break);
    })
    .def("add", [](object::Section& self, std::shared_ptr<object::Section> section) {
        return self.add(section);
    });

    return section;
}

}
}
