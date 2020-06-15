/*
 * ordered_list.cpp
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
shared_class<object::OrderedList> declare_ordered_list(
    py::module& m,
    shared_class<object::List>& list) {

    auto ol = shared_class<object::OrderedList>(m, "OrderedList", list)
    .def("add", [](object::OrderedList& self, std::shared_ptr<object::OrderedListItem> oli) {
        self.add(oli);
    });
    return ol;
}

//-------------------------------------------------------------------
shared_class<object::OrderedListItem> declare_ordered_list_item(
    py::module& m,
    shared_class<object::ListItem>& li) {

    return shared_class<object::OrderedListItem>(m, "OrderedListItem", li);
}

}
}
