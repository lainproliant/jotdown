/*
 * object.cpp
 *
 * Author: Lain Musgrove (lain.proliant@gmail.com)
 * Date: Saturday June 13, 2020
 *
 * Distributed under terms of the MIT license.
 */

#include "pybind11/pybind11.h"
#include "pybind11/operators.h"
#include "pybind11/stl.h"
#include "jotdown/python/declarators.h"
#include "jotdown/object.h"
#include "jotdown/query.h"
#include "moonlight/hash.h"

#include <functional>

namespace py = pybind11;

namespace jotdown {
namespace python {

//-------------------------------------------------------------------
py::class_<object::Config> declare_object_config(py::module& m) {
    py::class_<object::Config> object_config(m, "ObjectConfig");
    object_config
        .def(py::init<>())
        .def_readwrite("list_indent", &object::Config::list_indent);
    return object_config;
}

//-------------------------------------------------------------------
void declare_object_error(py::module& m) {
    py::register_exception<object::ObjectError>(m, "ObjectError");
}

//-------------------------------------------------------------------
py::class_<Location> declare_location(py::module& m) {
    auto location = py::class_<Location>(m, "Location")
        .def(py::init([]() {
            return NOWHERE;
        }))
        .def(py::init([](const std::string& filename,
                         int line, int col) {
            return (Location){filename, line, col};
        }), py::arg("filename"), py::arg("line"), py::arg("col"))
        .def_readwrite("filename", &Location::filename)
        .def_readwrite("line", &Location::line)
        .def_readwrite("col", &Location::col)
        .def("__repr__", [](const Location& self) {
            return tfm::format("Location<file=\"%s\" loc=\"%d:%d\">",
                               self.filename,
                               self.line,
                               self.col);
        })
        .def("__hash__", [](const Location& self) {
            size_t hash;
            moonlight::hash::combine(hash, self.filename);
            moonlight::hash::combine(hash, self.line);
            moonlight::hash::combine(hash, self.col);
            return hash;
        })
        .def("to_json", [](const Location& self) {
            return json_to_dict(self.to_json());
        })
        .def(py::self == py::self);
    m.attr("NOWHERE") = NOWHERE;
    return location;
}

//-------------------------------------------------------------------
py::class_<Range> declare_range(py::module& m) {
    auto range = py::class_<Range>(m, "Range")
        .def(py::init([]() {
            return (Range){NOWHERE, NOWHERE};
        }))
        .def(py::init([](const Location& begin,
                         const Location& end) {
            return (Range){begin, end};
        }), py::arg("begin"), py::arg("end"))
        .def_readwrite("begin", &Range::begin)
        .def_readwrite("end", &Range::end)
        .def("__repr__", [](const Range& self) {
            return tfm::format("Range<start=\"%d:%d\", end=\"%d:%d\">",
                               self.begin.line,
                               self.begin.col,
                               self.end.line,
                               self.end.col);
        })
        .def("to_json", [](const Range& self) {
            return json_to_dict(self.to_json());
        })
        .def("__eq__", [](const Range& self, const Range& other) {
            return self == other;
        });
    return range;
}

//-------------------------------------------------------------------
obj_class declare_object(py::module& m) {
    auto object = obj_class(m, "Object");
    py::enum_<object::Object::Type>(object, "Type")
        .value("NONE", object::Object::Type::NONE)
        .value("ANCHOR", object::Object::Type::ANCHOR)
        .value("CODE", object::Object::Type::CODE)
        .value("CODE_BLOCK", object::Object::Type::CODE_BLOCK)
        .value("DOCUMENT", object::Object::Type::DOCUMENT)
        .value("HASHTAG", object::Object::Type::HASHTAG)
        .value("LINE_BREAK", object::Object::Type::LINE_BREAK)
        .value("ORDERED_LIST", object::Object::Type::ORDERED_LIST)
        .value("ORDERED_LIST_ITEM", object::Object::Type::ORDERED_LIST_ITEM)
        .value("REF", object::Object::Type::REF)
        .value("SECTION", object::Object::Type::SECTION)
        .value("TEXT", object::Object::Type::TEXT)
        .value("TEXT_BLOCK", object::Object::Type::TEXT_CONTENT)
        .value("UNORDERED_LIST", object::Object::Type::UNORDERED_LIST)
        .value("UNORDERED_LIST_ITEM", object::Object::Type::UNORDERED_LIST_ITEM);

    object.def("type", &object::Object::type);
    object.def("range", [](const object::Object& self) {
        return self.range();
    });
    object.def_property(
        "range",
        [](const object::Object& self) {
            return self.range();
        },
        [](object::Object& self, const Range& range) {
            self.range(range);
        });
    object.def_property_readonly(
        "parent",
        [](const object::Object& self) {
            return self.cparent();
        });
    object.def_static("type_name", [](object::Object::Type type) {
        return object::Object::type_name(type);
    });
    object.def_property_static(
        "config",
        []() {
            return object::Object::config();
        },
        [](const object::Config& config) {
            object::Object::config() = config;
        });
    object.def("clone", [](const object::Object& obj) {
        return obj.clone();
    });
    object.def("q", [](std::shared_ptr<object::Object> object, const std::string& query_str) {
        return query::parse(query_str).select({object});
    });
    object.def("to_json", [](const object::Object& obj) {
        return json_to_dict(obj.to_json());
    });
    object.def("to_jotdown", [](const object::Object& obj) {
        return obj.to_jotdown();
    });
    object.def("query", [](object::obj_t obj, const std::string& query_str) {
        return query::parse(query_str).select({obj});
    });
    object.def("__repr__", [](const object::Object& obj) {
        return obj.repr();
    });

    return object;
}

}
}
