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
shared_class<object::List> declare_list(py::module& m, obj_class& obj) {
    return shared_class<object::List>(m, "List", obj);
}

//-------------------------------------------------------------------
shared_class<object::ListItem> declare_list_item(py::module& m, obj_class& obj) {
    auto li = shared_class<object::ListItem>(m, "ListItem", obj)
    .def("add", [](object::ListItem& li, std::shared_ptr<object::OrderedList> ol) {
        li.add(ol);
    })
    .def("add", [](object::ListItem& li, std::shared_ptr<object::UnorderedList> ul) {
        li.add(ul);
    })
    .def_property(
        "status",
        [](const object::ListItem& li) {
            return li.status();
        },
        [](object::ListItem& li, const std::string& status) {
            li.status(status);
        });
    return li;
}

}
}
