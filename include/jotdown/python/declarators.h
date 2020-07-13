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
#include "jotdown/object.h"
#include "moonlight/json.h"

namespace jotdown {
namespace python {

namespace py = pybind11;

//-------------------------------------------------------------------
using JSON = moonlight::json::Wrapper;

//-------------------------------------------------------------------
template<class T, typename... options>
using shared_class = py::class_<T, std::shared_ptr<T>, options...>;

//-------------------------------------------------------------------
inline py::dict json_to_dict(const JSON& json) {
    auto pyjson = py::module::import("json");
    return pyjson.attr("loads")(json.to_string());
}

//-------------------------------------------------------------------
class ObjectTrampoline : public object::Object {
public:
    using object::Object::Object;

    object::obj_t clone() const override {
        PYBIND11_OVERLOAD_PURE(
            object::obj_t,
            object::Object,
            clone
        );
    }
};

//-------------------------------------------------------------------
using obj_class = shared_class<object::Object, ObjectTrampoline>;

//-------------------------------------------------------------------
// API declarators: api.cpp
void declare_api(py::module& m);

//-------------------------------------------------------------------
// Base Object declarators: object.cpp
py::class_<object::Config> declare_object_config(py::module& m);
void declare_object_error(py::module& m);
py::class_<Location> declare_location(py::module& m);
py::class_<Range> declare_range(py::module& m);
obj_class declare_object(py::module& m);

//-------------------------------------------------------------------
// Container declarator: container.cpp
shared_class<object::Container> declare_container(py::module& m, obj_class& obj);

//-------------------------------------------------------------------
// Simple object declarators: simple.cpp
//
shared_class<object::Anchor> declare_anchor(py::module& m, obj_class& obj);
shared_class<object::Text> declare_text(py::module& m, obj_class& obj);
shared_class<object::Hashtag> declare_hashtag(py::module& m, obj_class& obj);
shared_class<object::LineBreak> declare_line_break(py::module& m, obj_class& obj);
shared_class<object::Code> declare_code(py::module& m, obj_class& obj);
shared_class<object::Ref> declare_ref(py::module& m, obj_class& obj);
shared_class<object::CodeBlock> declare_code_block(py::module& m, obj_class& obj);


//-------------------------------------------------------------------
// Text Content declarator: text_content.cpp
shared_class<object::TextContent> declare_text_content(
    py::module& m,
    shared_class<object::Container>& container);

//-------------------------------------------------------------------
// Base List declarators: list.cpp
shared_class<object::List> declare_list(
    py::module& m,
    shared_class<object::Container>& container);
shared_class<object::ListItem> declare_list_item(
    py::module& m,
    shared_class<object::Container>& container);

//-------------------------------------------------------------------
// Ordered List declarators: ordered_list.cpp
shared_class<object::OrderedList> declare_ordered_list(
    py::module& m,
    shared_class<object::List>& list);
shared_class<object::OrderedListItem> declare_ordered_list_item(
    py::module& m,
    shared_class<object::ListItem>& li);

//-------------------------------------------------------------------
// Unordered List declarators: unordered_list.cpp
shared_class<object::UnorderedList> declare_unordered_list(
    py::module& m,
    shared_class<object::List>& list);
shared_class<object::UnorderedListItem> declare_unordered_list_item(
    py::module& m,
    shared_class<object::ListItem>& li);

//-------------------------------------------------------------------
// Section declarator: section.cpp
shared_class<object::Section> declare_section(
    py::module& m,
    shared_class<object::Container>& container);

//-------------------------------------------------------------------
// Document declarator: document.cpp
shared_class<object::Document> declare_document(
    py::module& m,
    shared_class<object::Container>& container);

}
}


#endif /* !__JOTDOWN_PYTHON_DECLARATORS_H */
