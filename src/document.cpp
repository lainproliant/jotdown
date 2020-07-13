/*
 * document.cpp
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
#include "jotdown/api.h"

#include <functional>

namespace py = pybind11;

namespace jotdown {
namespace python {

//-------------------------------------------------------------------
shared_class<object::Document> declare_document(
    py::module& m,
    shared_class<object::Container>& container) {

    auto document = shared_class<object::Document>(m, "Document", container)
        .def(py::init<>())
        .def("add", [](object::Document& document,
                       std::shared_ptr<object::Section> section) {
            document.add(section);
        })
        .def("save", [](std::shared_ptr<const Document> document,
                        const std::string& filename) {
            jotdown::save(document, filename);
        });
    return document;
}

}
}
