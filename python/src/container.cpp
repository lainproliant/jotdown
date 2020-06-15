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
shared_class<object::Container> declare_container(py::module& m, obj_class& obj) {
    auto container = shared_class<object::Container>(m, "Container", obj)
        .def_property("contents", [](const object::Container& self) {
            return self.contents();
        },
        [](object::Container& self, const std::vector<object::obj_t>& contents) {
            self.contents(contents);
        });

    return container;
}

}
}
