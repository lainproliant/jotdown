/*
 * list.cpp
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
shared_class<object::List> declare_list(
    py::module& m,
    shared_class<object::Container>& container) {

    return shared_class<object::List>(m, "List", container);
}

//-------------------------------------------------------------------
shared_class<object::ListItem> declare_list_item(
    py::module& m,
    shared_class<object::Container>& container) {

    auto li = shared_class<object::ListItem>(m, "ListItem", container)
        .def("add", [](object::ListItem& li,
                       std::shared_ptr<object::OrderedList> ol) {
            li.add(ol);
        })
        .def("add", [](object::ListItem& li,
                       std::shared_ptr<object::UnorderedList> ul) {
            li.add(ul);
        })
        .def_property(
            "text",
            [](const object::ListItem& li) {
                return li.ctext();
            },
            [](object::ListItem& li,
               std::shared_ptr<object::TextContent> text) {
                li.text(text);
            }
        )
        .def_property(
            "status",
            [](const object::ListItem& li) {
                return li.status();
            },
            [](object::ListItem& li, const std::string& status) {
                li.status(status);
            })
        .def("__repr__", [](const object::ListItem& self) {
            return tfm::format("%s<%s>",
                               self.type_name(self.type()),
                               self.ctext()->to_json().to_string());
        });
    return li;
}

}
}
