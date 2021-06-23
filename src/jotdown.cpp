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
namespace json = moonlight::json;

//-------------------------------------------------------------------
// Utilities and Typedefs
//
template<class T, typename... options>
using shared_class = py::class_<T, std::shared_ptr<T>, options...>;

//-------------------------------------------------------------------
inline py::dict json_to_dict(const json::Object& json) {
    auto pyjson = py::module::import("json");
    return pyjson.attr("loads")(json::to_string(json));
}

//-------------------------------------------------------------------
class ObjectTrampoline : public Object {
public:
    using Object::Object;

    obj_t clone() const override {
        PYBIND11_OVERLOAD_PURE(
            obj_t,
            Object,
            clone
        );
    }
};

//-------------------------------------------------------------------
using obj_class = shared_class<Object, ObjectTrampoline>;


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
    m.def("q", [](const std::vector<obj_t>& objects, const std::string& query_str) {
        return q::parse(query_str).select(objects);
    });
}


//-------------------------------------------------------------------
// Object Declaration
//
py::class_<Config> declare_object_config(py::module& m) {
    py::class_<Config> object_config(m, "ObjectConfig");
    object_config
        .def(py::init<>())
        .def_readwrite("list_indent", &Config::list_indent);
    return object_config;
}

//-------------------------------------------------------------------
void declare_object_error(py::module& m) {
    py::register_exception<ObjectError>(m, "ObjectError");
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
    py::enum_<Object::Type>(object, "Type")
        .value("NONE", Object::Type::NONE)
        .value("ANCHOR", Object::Type::ANCHOR)
        .value("CODE", Object::Type::CODE)
        .value("CODE_BLOCK", Object::Type::CODE_BLOCK)
        .value("DOCUMENT", Object::Type::DOCUMENT)
        .value("HASHTAG", Object::Type::HASHTAG)
        .value("LINE_BREAK", Object::Type::LINE_BREAK)
        .value("ORDERED_LIST", Object::Type::ORDERED_LIST)
        .value("ORDERED_LIST_ITEM", Object::Type::ORDERED_LIST_ITEM)
        .value("REF", Object::Type::REF)
        .value("REF_INDEX", Object::Type::REF_INDEX)
        .value("SECTION", Object::Type::SECTION)
        .value("TEXT", Object::Type::TEXT)
        .value("TEXT_BLOCK", Object::Type::TEXT_CONTENT)
        .value("UNORDERED_LIST", Object::Type::UNORDERED_LIST)
        .value("UNORDERED_LIST_ITEM", Object::Type::UNORDERED_LIST_ITEM);

    object.def("type", &Object::type);
    object.def("range", [](const Object& self) {
        return self.range();
    });
    object.def_property(
        "range",
        [](const Object& self) {
            return self.range();
        },
        [](Object& self, const Range& range) {
            self.range(range);
        });
    object.def_property_readonly(
        "parent",
        [](const Object& self) {
            return self.cparent();
        });
    object.def_static("type_name", [](Object::Type type) {
        return Object::type_name(type);
    });
    object.def_property_static(
        "config",
        []() {
            return Object::config();
        },
        [](const Config& config) {
            Object::config() = config;
        });
    object.def("clone", [](const Object& obj) {
        return obj.clone();
    });
    object.def("query", [](std::shared_ptr<Object> object, const std::string& query_str) {
        return q::parse(query_str).select({object});
    });
    object.def("to_json", [](const Object& obj) {
        return json_to_dict(obj.to_json());
    });
    object.def("to_jotdown", [](const Object& obj) {
        return obj.to_jotdown();
    });
    object.def("__repr__", [](const Object& obj) {
        return obj.repr();
    });

    return object;
}


//-------------------------------------------------------------------
// Container Declaration
//
shared_class<Container> declare_container(
    py::module& m, obj_class& obj) {
    auto container = shared_class<Container>(m, "Container", obj)
        .def_property_readonly("contents", [](const Container& self) {
            return self.contents();
        })
    .def("clear", [](Container& self) {
        self.clear();
    })
    .def("add", [](Container& self, obj_t obj) {
        return self.add(obj);
    })
    .def("insert_before", [](Container& self, cobj_t pivot, obj_t obj) {
        return self.insert_before(pivot, obj);
    })
    .def("insert_after", [](Container& self, cobj_t pivot, obj_t obj) {
        return self.insert_after(pivot, obj);
    })
    .def("remove", [](Container& self, obj_t obj) {
        self.remove(obj);
    }, py::arg("obj"))
    .def("shift_up", [](Container& self, cobj_t obj) {
        self.shift_up(obj);
    }, py::arg("obj"))
    .def("shift_down", [](Container& self, cobj_t obj) {
        self.shift_down(obj);
    }, py::arg("obj"))
    .def("__call__", [](std::shared_ptr<Container> self) {
        return self->contents();
    })
    .def("__call__", [](std::shared_ptr<Container> self, py::args args) {
        for (auto arg : args) {
            self->add(arg.cast<obj_t>());
        }
        return self;
    });

    return container;
}


//-------------------------------------------------------------------
// Document Declaration
//
shared_class<Document> declare_document(
    py::module& m,
    shared_class<Container>& container) {

    auto document = shared_class<Document>(m, "Document", container)
        .def(py::init<>())
        .def_property(
            "front_matter",
            [](const Document& self) {
                return self.front_matter();
            },
            [](Document& self, std::shared_ptr<FrontMatter> front_matter) {
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
shared_class<Section> declare_section(
    py::module& m,
    shared_class<Container>& container) {

    auto section = shared_class<Section>(m, "Section", container)
    .def(py::init([](int level) {
        return Section::create(level);
    }), py::arg("level") = 1)
    .def(py::init([](const std::string& header_text, int level) {
        auto header = TextContent::create();
        header->add(Text::create(header_text));
        auto section = Section::create(level);
        section->header(header);
        return section;
    }), py::arg("header_text"), py::arg("level") = 1)
    .def(py::init([](std::shared_ptr<TextContent> header, int level) {
        auto section = Section::create(level);
        section->header(header);
        return section;
    }), py::arg("header"), py::arg("level") = 1)
    .def_property(
        "header",
        [](const Section& self) {
            return self.cheader();
        },
        [&](Section& self, std::shared_ptr<TextContent> header) {
            self.header(header);
        });

    return section;
}


//-------------------------------------------------------------------
// List Declaration
//
shared_class<List> declare_list(
    py::module& m,
    shared_class<Container>& container) {

    auto list = shared_class<List>(m, "List", container)
        .def_property_readonly("level", [](const List& list) {
            return list.level();
        });
    return list;
}

//-------------------------------------------------------------------
shared_class<ListItem> declare_list_item(
    py::module& m,
    shared_class<Container>& container) {

    auto li = shared_class<ListItem>(m, "ListItem", container)
        .def_property(
            "text",
            [](const ListItem& li) {
                return li.ctext();
            },
            [](ListItem& li,
               std::shared_ptr<TextContent> text) {
                li.text(text);
            }
        )
        .def_property_readonly(
            "level",
            [](const ListItem& li) {
                return li.level();
            }
        )
        .def_property(
            "status",
            [](const ListItem& li) {
                return li.status();
            },
            [](ListItem& li, const std::string& status) {
                li.status(status);
            });
    return li;
}


//-------------------------------------------------------------------
// Declare OrderedList
//
shared_class<OrderedList> declare_ordered_list(
    py::module& m,
    shared_class<List>& list) {

    auto ol = shared_class<OrderedList>(m, "OrderedList", list)
        .def(py::init<>());
    return ol;
}

//-------------------------------------------------------------------
shared_class<OrderedListItem> declare_ordered_list_item(
    py::module& m,
    shared_class<ListItem>& li) {

    auto oli = shared_class<OrderedListItem>(m, "OrderedListItem", li)
    .def(py::init([](const std::string& ordinal) {
        return OrderedListItem::create(ordinal);
    }), py::arg("ordinal"));
    return oli;
}


//-------------------------------------------------------------------
// Declare UnorderedList
//
shared_class<UnorderedList> declare_unordered_list(
    py::module& m,
    shared_class<List>& list) {

    auto ul = shared_class<UnorderedList>(m, "UnorderedList", list)
        .def(py::init<>());
    return ul;
}

//-------------------------------------------------------------------
shared_class<UnorderedListItem> declare_unordered_list_item(
    py::module& m,
    shared_class<ListItem>& li) {

    auto uli = shared_class<UnorderedListItem>(
        m, "UnorderedListItem", li)
        .def(py::init([]() {
            return UnorderedListItem::create();
        }));
    return uli;
}


//-------------------------------------------------------------------
// Declare TextContent
//
shared_class<TextContent> declare_text_content(
    py::module& m,
    shared_class<Container>& container) {

    auto text_content = shared_class<TextContent>(
        m, "TextContent", container)
        .def(py::init<>())
        .def(py::init([](const std::string& text) {
            auto content = TextContent::create();
            content->add(Text::create(text));
            return content;
        }));

    return text_content;
}


//-------------------------------------------------------------------
// Declare Simple Types
//
shared_class<Anchor> declare_anchor(py::module& m, obj_class& obj) {
    auto anchor = shared_class<Anchor>(m, "Anchor", obj)
        .def(py::init<const std::string&>(), py::arg("name"))
        .def_property_readonly("name", &Anchor::name);
    return anchor;
}

//-------------------------------------------------------------------
shared_class<Text> declare_text(py::module& m, obj_class& obj) {
    auto text = shared_class<Text>(m, "Text", obj)
        .def(py::init<const std::string&>(), py::arg("text"))
        .def_property_readonly("text", &Text::text);
    return text;
}

//-------------------------------------------------------------------
shared_class<Hashtag> declare_hashtag(py::module& m, obj_class& obj) {
    auto hashtag = shared_class<Hashtag>(m, "Hashtag", obj)
        .def(py::init<const std::string&>(), py::arg("tag"))
        .def_property_readonly("tag", &Hashtag::tag);
    return hashtag;
}

//-------------------------------------------------------------------
shared_class<LineBreak> declare_line_break(py::module& m, obj_class& obj) {
    auto line_break = shared_class<LineBreak>(m, "LineBreak", obj)
        .def(py::init<>());
    return line_break;
}

//-------------------------------------------------------------------
shared_class<Code> declare_code(py::module& m, obj_class& obj) {
    auto code = shared_class<Code>(m, "Code", obj)
        .def(py::init<const std::string&>(), py::arg("code"))
        .def_property_readonly("code", &Code::code);
    return code;
}

//-------------------------------------------------------------------
shared_class<Ref> declare_ref(py::module& m, obj_class& obj) {
    auto ref = shared_class<Ref>(m, "Ref", obj)
        .def(py::init<const std::string&, const std::string&>(),
             py::arg("link"), py::arg("text") = "")
        .def_property_readonly("link", &Ref::link)
        .def_property_readonly("text", &Ref::text);
    return ref;
}

//-------------------------------------------------------------------
shared_class<IndexedRef> declare_indexed_ref(py::module& m, obj_class& obj) {
    auto indexed_ref = shared_class<IndexedRef>(m, "IndexedRef", obj)
        .def(py::init<const std::string&, const std::string&>(),
             py::arg("text"), py::arg("index_name"))
        .def_property_readonly("index_name", &IndexedRef::index_name)
        .def_property_readonly("text", &IndexedRef::text);
    return indexed_ref;
}

//-------------------------------------------------------------------
shared_class<CodeBlock> declare_code_block(py::module& m, obj_class& obj) {
    auto code_block = shared_class<CodeBlock>(m, "CodeBlock", obj)
        .def(py::init<const std::string&, const std::string&>(),
             py::arg("code"), py::arg("language") = "")
        .def_property_readonly("code", &CodeBlock::code)
        .def_property_readonly("language", &CodeBlock::language);
    return code_block;
}

//-------------------------------------------------------------------
shared_class<FrontMatter> declare_front_matter(py::module& m, obj_class& obj) {
    auto code_block = shared_class<FrontMatter>(m, "FrontMatter", obj)
        .def(py::init<const std::string&, const std::string&>(),
             py::arg("code"), py::arg("language") = "")
        .def_property_readonly("code", &FrontMatter::code)
        .def_property_readonly("language", &FrontMatter::language);
    return code_block;
}

//-------------------------------------------------------------------
shared_class<RefIndex> declare_ref_index(py::module& m, obj_class& obj) {
    auto ref_index = shared_class<RefIndex>(m, "RefIndex", obj)
        .def(py::init<const std::string&, const std::string&>(),
             py::arg("name"), py::arg("link"))
        .def_property_readonly("name", &RefIndex::name)
        .def_property_readonly("link", &RefIndex::link);
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
