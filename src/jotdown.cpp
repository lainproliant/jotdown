/*
 * jotdown.cpp
 *
 * Author: Lain Musgrove (lain.proliant@gmail.com)
 * Date: Monday July 13, 2020
 *
 * Distributed under terms of the MIT license.
 */

#include "jotdown/api.h"
#include "jotdown/object.h"
#include "jotdown/query.h"
#include "moonlight/hash.h"
#include "pybind11/operators.h"
#include "pybind11/pybind11.h"
#include "pybind11/stl.h"

#include <functional>

namespace jotdown {
namespace python {

namespace py = pybind11;

//-------------------------------------------------------------------
// Utilities and Typedefs
//
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
// API Declaration
//
void declare_api(py::module& m) {
    m.def("load", [](const std::string& filename) {
        return load(filename);
    });
    m.def("load", [](std::istream& infile) {
        return load(infile);
    });
    m.def("save", [](std::shared_ptr<const Document> document,
                     const std::string& filename) {
        save(document, filename);
    });
    m.def("q", [](const std::vector<object::obj_t>& objects, const std::string& query_str) {
        return query::parse(query_str).select(objects);
    });
}


//-------------------------------------------------------------------
// Object Declaration
//
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
        .value("REF_INDEX", object::Object::Type::REF_INDEX)
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


//-------------------------------------------------------------------
// Container Declaration
//
shared_class<object::Container> declare_container(
    py::module& m, obj_class& obj) {
    auto container = shared_class<object::Container>(m, "Container", obj)
        .def_property_readonly("contents", [](const object::Container& self) {
            return self.contents();
        })
    .def("clear", [](object::Container& self) {
        self.clear();
    })
    .def("add", [](object::Container& self, object::obj_t obj) {
        return self.add(obj);
    })
    .def("insert_before", [](object::Container& self, object::cobj_t pivot, object::obj_t obj) {
        return self.insert_before(pivot, obj);
    })
    .def("insert_after", [](object::Container& self, object::cobj_t pivot, object::obj_t obj) {
        return self.insert_after(pivot, obj);
    })
    .def("remove", [](object::Container& self, object::obj_t obj) {
        self.remove(obj);
    }, py::arg("obj"))
    .def("shift_up", [](object::Container& self, object::cobj_t obj) {
        self.shift_up(obj);
    }, py::arg("obj"))
    .def("shift_down", [](object::Container& self, object::cobj_t obj) {
        self.shift_down(obj);
    }, py::arg("obj"))
    .def("__call__", [](std::shared_ptr<object::Container> self) {
        return self->contents();
    })
    .def("__call__", [](std::shared_ptr<object::Container> self, py::args args) {
        for (auto arg : args) {
            self->add(arg.cast<object::obj_t>());
        }
        return self;
    });

    return container;
}


//-------------------------------------------------------------------
// Document Declaration
//
shared_class<object::Document> declare_document(
    py::module& m,
    shared_class<object::Container>& container) {

    auto document = shared_class<object::Document>(m, "Document", container)
        .def(py::init<>())
        .def_property(
            "front_matter",
            [](const object::Document& self) {
                return self.front_matter();
            },
            [](object::Document& self, std::shared_ptr<object::FrontMatter> front_matter) {
                self.front_matter(front_matter);
            })
        .def("save", [](std::shared_ptr<const Document> document,
                        const std::string& filename) {
            jotdown::save(document, filename);
        });
    return document;
}


//-------------------------------------------------------------------
// Declare Section
//
shared_class<object::Section> declare_section(
    py::module& m,
    shared_class<object::Container>& container) {

    auto section = shared_class<object::Section>(m, "Section", container)
    .def(py::init([](int level) {
        return object::Section::create(level);
    }), py::arg("level") = 1)
    .def(py::init([](const std::string& header_text, int level) {
        auto header = std::make_shared<object::TextContent>();
        header->add(std::make_shared<object::Text>(header_text));
        auto section = object::Section::create(level);
        section->header(header);
        return section;
    }), py::arg("header_text"), py::arg("level") = 1)
    .def(py::init([](std::shared_ptr<object::TextContent> header, int level) {
        auto section = object::Section::create(level);
        section->header(header);
        return section;
    }), py::arg("header"), py::arg("level") = 1)
    .def_property(
        "header",
        [](const object::Section& self) {
            return self.cheader();
        },
        [&](object::Section& self, std::shared_ptr<object::TextContent> header) {
            self.header(header);
        });

    return section;
}


//-------------------------------------------------------------------
// List Declaration
//
shared_class<object::List> declare_list(
    py::module& m,
    shared_class<object::Container>& container) {

    auto list = shared_class<object::List>(m, "List", container)
        .def_property_readonly("level", [](const object::List& list) {
            return list.level();
        });
    return list;
}

//-------------------------------------------------------------------
shared_class<object::ListItem> declare_list_item(
    py::module& m,
    shared_class<object::Container>& container) {

    auto li = shared_class<object::ListItem>(m, "ListItem", container)
        .def_property(
            "text",
            [](const object::ListItem& li) {
                return li.ctext();
            },
            [](object::ListItem& li,
               std::shared_ptr<object::TextContent> text) {
                li.text(text);
            }
        )
        .def_property_readonly(
            "level",
            [](const object::ListItem& li) {
                return li.level();
            }
        )
        .def_property(
            "status",
            [](const object::ListItem& li) {
                return li.status();
            },
            [](object::ListItem& li, const std::string& status) {
                li.status(status);
            });
    return li;
}


//-------------------------------------------------------------------
// Declare OrderedList
//
shared_class<object::OrderedList> declare_ordered_list(
    py::module& m,
    shared_class<object::List>& list) {

    auto ol = shared_class<object::OrderedList>(m, "OrderedList", list)
        .def(py::init<>());
    return ol;
}

//-------------------------------------------------------------------
shared_class<object::OrderedListItem> declare_ordered_list_item(
    py::module& m,
    shared_class<object::ListItem>& li) {

    auto oli = shared_class<object::OrderedListItem>(m, "OrderedListItem", li)
    .def(py::init([](const std::string& ordinal) {
        return object::OrderedListItem::create(ordinal);
    }), py::arg("ordinal"));
    return oli;
}


//-------------------------------------------------------------------
// Declare UnorderedList
//
shared_class<object::UnorderedList> declare_unordered_list(
    py::module& m,
    shared_class<object::List>& list) {

    auto ul = shared_class<object::UnorderedList>(m, "UnorderedList", list)
        .def(py::init<>());
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


//-------------------------------------------------------------------
// Declare TextContent
//
shared_class<object::TextContent> declare_text_content(
    py::module& m,
    shared_class<object::Container>& container) {

    auto text_content = shared_class<object::TextContent>(
        m, "TextContent", container)
        .def(py::init<>())
        .def(py::init([](const std::string& text) {
            auto content = std::make_shared<object::TextContent>();
            content->add(std::make_shared<object::Text>(text));
            return content;
        }));

    return text_content;
}


//-------------------------------------------------------------------
// Declare Simple Types
//
shared_class<object::Anchor> declare_anchor(py::module& m, obj_class& obj) {
    auto anchor = shared_class<object::Anchor>(m, "Anchor", obj)
        .def(py::init<const std::string&>(), py::arg("name"))
        .def_property_readonly("name", &object::Anchor::name);
    return anchor;
}

//-------------------------------------------------------------------
shared_class<object::Text> declare_text(py::module& m, obj_class& obj) {
    auto text = shared_class<object::Text>(m, "Text", obj)
        .def(py::init<const std::string&>(), py::arg("text"))
        .def_property_readonly("text", &object::Text::text);
    return text;
}

//-------------------------------------------------------------------
shared_class<object::Hashtag> declare_hashtag(py::module& m, obj_class& obj) {
    auto hashtag = shared_class<object::Hashtag>(m, "Hashtag", obj)
        .def(py::init<const std::string&>(), py::arg("tag"))
        .def_property_readonly("tag", &object::Hashtag::tag);
    return hashtag;
}

//-------------------------------------------------------------------
shared_class<object::LineBreak> declare_line_break(py::module& m, obj_class& obj) {
    auto line_break = shared_class<object::LineBreak>(m, "LineBreak", obj)
        .def(py::init<>());
    return line_break;
}

//-------------------------------------------------------------------
shared_class<object::Code> declare_code(py::module& m, obj_class& obj) {
    auto code = shared_class<object::Code>(m, "Code", obj)
        .def(py::init<const std::string&>(), py::arg("code"))
        .def_property_readonly("code", &object::Code::code);
    return code;
}

//-------------------------------------------------------------------
shared_class<object::Ref> declare_ref(py::module& m, obj_class& obj) {
    auto ref = shared_class<object::Ref>(m, "Ref", obj)
        .def(py::init<const std::string&, const std::string&>(),
             py::arg("link"), py::arg("text") = "")
        .def_property_readonly("link", &object::Ref::link)
        .def_property_readonly("text", &object::Ref::text);
    return ref;
}

//-------------------------------------------------------------------
shared_class<object::IndexedRef> declare_indexed_ref(py::module& m, obj_class& obj) {
    auto indexed_ref = shared_class<object::IndexedRef>(m, "IndexedRef", obj)
        .def(py::init<const std::string&, const std::string&>(),
             py::arg("text"), py::arg("index_name"))
        .def_property_readonly("index_name", &object::IndexedRef::index_name)
        .def_property_readonly("text", &object::IndexedRef::text);
    return indexed_ref;
}

//-------------------------------------------------------------------
shared_class<object::CodeBlock> declare_code_block(py::module& m, obj_class& obj) {
    auto code_block = shared_class<object::CodeBlock>(m, "CodeBlock", obj)
        .def(py::init<const std::string&, const std::string&>(),
             py::arg("code"), py::arg("language") = "")
        .def_property_readonly("code", &object::CodeBlock::code)
        .def_property_readonly("language", &object::CodeBlock::language);
    return code_block;
}

//-------------------------------------------------------------------
shared_class<object::FrontMatter> declare_front_matter(py::module& m, obj_class& obj) {
    auto code_block = shared_class<object::FrontMatter>(m, "FrontMatter", obj)
        .def(py::init<const std::string&, const std::string&>(),
             py::arg("code"), py::arg("language") = "")
        .def_property_readonly("code", &object::FrontMatter::code)
        .def_property_readonly("language", &object::FrontMatter::language);
    return code_block;
}

//-------------------------------------------------------------------
shared_class<object::RefIndex> declare_ref_index(py::module& m, obj_class& obj) {
    auto ref_index = shared_class<object::RefIndex>(m, "RefIndex", obj)
        .def(py::init<const std::string&, const std::string&>(),
             py::arg("name"), py::arg("link"))
        .def_property_readonly("name", &object::RefIndex::name)
        .def_property_readonly("link", &object::RefIndex::link);
    return ref_index;
}

//-------------------------------------------------------------------
// Declare Python Module
//
PYBIND11_MODULE(jotdown, m) {
    // API declarators: api.cpp
    declare_api(m);

    // Base Object declarators: object.cpp
    declare_object_config(m);
    declare_object_error(m);
    declare_location(m);
    declare_range(m);
    auto obj = declare_object(m);

    // Container declarator: container.cpp
    auto container = declare_container(m, obj);

    // Simple object declarators: simple.cpp
    auto anchor = declare_anchor(m, obj);
    auto text = declare_text(m, obj);
    auto hashtag = declare_hashtag(m, obj);
    auto line_break = declare_line_break(m, obj);
    auto code = declare_code(m, obj);
    auto ref = declare_ref(m, obj);
    auto indexed_ref = declare_indexed_ref(m, obj);
    auto code_block = declare_code_block(m, obj);
    auto front_matter = declare_front_matter(m, obj);
    auto ref_index = declare_ref_index(m, obj);

    // Textblock declarator: textblock.cpp
    auto text_content = declare_text_content(m, container);

    // Base List declarators: list.cpp
    auto list = declare_list(m, container);
    auto li = declare_list_item(m, container);

    // Ordered List declarators: ordered_list.cpp
    auto ol = declare_ordered_list(m, list);
    auto oli = declare_ordered_list_item(m, li);

    // Unordered List declarators: unordered_list.cpp
    auto ul = declare_unordered_list(m, list);
    auto uli = declare_unordered_list_item(m, li);

    // Section declarator: section.cpp
    auto section = declare_section(m, container);

    // Document declarator: document.cpp
    auto document = declare_document(m, container);
}

}
}
