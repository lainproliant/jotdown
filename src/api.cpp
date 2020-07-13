/*
 * document.cpp
 *
 * Author: Lain Musgrove (lain.proliant@gmail.com)
 * Date: Saturday June 14, 2020
 *
 * Distributed under terms of the MIT license.
 */

#include "pybind11/pybind11.h"
#include "pybind11/operators.h"
#include "pybind11/stl.h"
#include "jotdown/python/declarators.h"
#include "jotdown/object.h"
#include "jotdown/api.h"
#include "jotdown/query.h"

#include <functional>

namespace py = pybind11;

namespace jotdown {
namespace python {

//-------------------------------------------------------------------
void declare_api(py::module& m) {
    m.def("load", [](const std::string& filename) {
        return load(filename);
    });
    m.def("save", [](std::shared_ptr<const Document> document,
                     const std::string& filename) {
        save(document, filename);
    });
    m.def("q", [](const std::vector<object::obj_t>& objects, const std::string& query_str) {
        return query::parse(query_str).select(objects);
    });
}

}
}
