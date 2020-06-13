/*
 * module.cpp
 *
 * Author: Lain Musgrove (lain.proliant@gmail.com)
 * Date: Thursday June 11, 2020
 *
 * Distributed under terms of the MIT license.
 */

#include "pybind11/pybind11.h"
#include "jotdown/python/declarators.h"

namespace py = pybind11;

namespace jotdown {
namespace python {

PYBIND11_MODULE(jotdown, m) {
    declare_objects(m);
    declare_api(m);
}

}
}
