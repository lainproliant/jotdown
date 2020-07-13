/*
 * container.cpp
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
shared_class<object::Container> declare_container(
    py::module& m, obj_class& obj) {
    auto container = shared_class<object::Container>(m, "Container", obj)
        .def_property_readonly("contents", [](const object::Container& self) {
            return self.contents();
        })
    .def("clear", [](object::Container& self) {
        self.clear();
    })
    .def("remove", [](object::Container& self, object::obj_t obj) {
        self.remove(obj);
    }, py::arg("obj"))
    .def("shift_up", [](object::Container& self, object::cobj_t obj) {
        self.shift_up(obj);
    }, py::arg("obj"))
    .def("shift_down", [](object::Container& self, object::cobj_t obj) {
        self.shift_down(obj);
    }, py::arg("obj"));

    return container;
}

}
}
