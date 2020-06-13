/*
 * declarators.h
 *
 * Author: Lain Musgrove (lain.proliant@gmail.com)
 * Date: Friday June 12, 2020
 *
 * Distributed under terms of the MIT license.
 */

#ifndef __JOTDOWN_PYTHON_DECLARATORS_H
#define __JOTDOWN_PYTHON_DECLARATORS_H

#include "pybind11/pybind11.h"

namespace jotdown {
namespace python {

namespace py = pybind11;

void declare_objects(py::module& m);
void declare_api(py::module& m);

}
}


#endif /* !__JOTDOWN_PYTHON_DECLARATORS_H */
