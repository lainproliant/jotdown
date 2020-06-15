/*
 * unordered_list.cpp
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
shared_class<object::UnorderedList> declare_unordered_list(
    py::module& m,
    shared_class<object::List>& list) {

    auto ul = shared_class<object::UnorderedList>(m, "UnorderedList", list)
        .def(py::init<>())
        .def("add", [](object::UnorderedList& self,
                       std::shared_ptr<object::UnorderedListItem> uli) {
            return self.add(uli);
        });
    return ul;
}

//-------------------------------------------------------------------
shared_class<object::UnorderedListItem> declare_unordered_list_item(
    py::module& m,
    shared_class<object::ListItem>& li) {

    auto uli = shared_class<object::UnorderedListItem>(
        m, "UnorderedListItem", li)
        .def(py::init([]() {
            return object::UnorderedListItem::create();
        }));
    return uli;
}

}
}
