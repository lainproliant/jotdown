/*
 * compiler.h
 *
 * Author: Lain Musgrove (lain.proliant@gmail.com)
 * Date: Friday May 29, 2020
 *
 * Distributed under terms of the MIT license.
 */

#ifndef __JOTDOWN_OBJECT_H
#define __JOTDOWN_OBJECT_H

#include "jotdown/parser.h"
#include "jotdown/interfaces.h"
#include "tinyformat/tinyformat.h"
#include "moonlight/json.h"

#include <memory>

namespace jotdown {
namespace object {

using jotdown::Location;
using jotdown::NOWHERE;

typedef moonlight::json::Wrapper JSON;

//-------------------------------------------------------------------'
struct Config {
    int list_indent;
};

//-------------------------------------------------------------------
class ObjectError : public moonlight::core::Exception {
public:
    using Exception::Exception;
};

//-------------------------------------------------------------------
class Object;
typedef std::shared_ptr<Object> obj_t;
typedef std::shared_ptr<const Object> cobj_t;

class Object : public std::enable_shared_from_this<Object> {
public:
    enum class Type {
        NONE,
        ANCHOR,
        CODE,
        CODE_BLOCK,
        DOCUMENT,
        HASHTAG,
        LINE_BREAK,
        ORDERED_LIST,
        ORDERED_LIST_ITEM,
        REF,
        SECTION,
        STATUS,
        TEXT,
        TEXT_CONTENT,
        UNORDERED_LIST,
        UNORDERED_LIST_ITEM,
        NUM_TYPES
    };

    Object(Type type) : _type(type) { }

    virtual ~Object() { }

    Type type() const {
        return _type;
    }

    const Range& range() const {
        return _range;
    }

    Range& range() {
        return _range;
    }

    void range(const Range& range) {
        _range = range;
    }

    bool has_parent() const {
        return _parent != nullptr;
    }

    obj_t parent() {
        return _parent;
    }

    cobj_t parent() const {
        return std::const_pointer_cast<Object>(_parent);
    }

    void parent(obj_t obj) {
        _parent = obj;
    }

    static const std::string& type_name(Type type) {
        static const std::string names[] = {
            "NONE",
            "ANCHOR",
            "CODE",
            "CODE_BLOCK",
            "DOCUMENT",
            "HASHTAG",
            "LINE_BREAK",
            "ORDERED_LIST",
            "ORDERED_LIST_ITEM",
            "REF",
            "SECTION",
            "STATUS",
            "TEXT",
            "TEXT_CONTENT",
            "UNORDERED_LIST",
            "UNORDERED_LIST_ITEM",
            "NUM_TYPES"
        };

        return names[(unsigned int)type];
    }

    static Config& config() {
        static Config config {
            .list_indent = 2
        };
        return config;
    }

    virtual JSON to_json() const {
        JSON json;
        json.set<std::string>("type", type_name(type()));
        if (range().begin != NOWHERE && range().end != NOWHERE) {
            json.set_object("range", range().to_json());
        }
        return json;
    }

    virtual obj_t clone() const = 0;
    virtual std::string to_jotdown() const = 0;

private:
    Type _type;
    obj_t _parent = nullptr;
    Range _range = {NOWHERE, NOWHERE};
};

//-------------------------------------------------------------------
class Container : public Object {
private:
    std::vector<obj_t> _contents;

public:
    Container(Type type) : Object(type) { }

    typedef typename decltype(_contents)::iterator iterator;
    typedef typename decltype(_contents)::const_iterator const_iterator;

    void remove(obj_t obj) {
        _contents.erase(std::remove_if(_contents.begin(), _contents.end(), [&](auto& obj_uniq) {
            return obj_uniq == obj;
        }));
    }

    iterator begin() {
        return _contents.begin();
    }

    iterator end() {
        return _contents.end();
    }

    const_iterator begin() const {
        return _contents.begin();
    }

    const_iterator end() const {
        return _contents.end();
    }

    const std::vector<obj_t>& contents() const {
        return _contents;
    }

    JSON to_json() const {
        JSON json = Object::to_json();
        std::vector<JSON> json_contents;
        for (auto& obj : contents()) {
            json_contents.push_back(obj->to_json());
        }
        json.set_object_array("contents", json_contents);
        return json;
    }

protected:
    void _add(obj_t obj) {
        _contents.push_back(obj);
        obj->parent(shared_from_this());
    }

    void _copy_from(std::shared_ptr<const Container> other) {
        for (auto& obj : other->contents()) {
            _add(obj->clone());
        }
    }
};

//-------------------------------------------------------------------
class Anchor : public Object {
public:
    Anchor(const std::string& name) : Object(Type::ANCHOR), _name(name) { }

    const std::string& name() const {
        return _name;
    }

    obj_t clone() const {
        auto obj = make<Anchor>(name());
        obj->range(range());
        return obj;
    }

    JSON to_json() const {
        JSON json = Object::to_json();
        json.set<std::string>("name", name());
        return json;
    }

    std::string to_jotdown() const {
        return tfm::format("&%s", name());
    }

private:
    std::string _name;
};

//-------------------------------------------------------------------
class Text : public Object {
public:
    Text(const std::string& text = "") : Object(Type::TEXT), _text(text) { }

    const std::string& text() const {
        return _text;
    }

    obj_t clone() const {
        auto obj = make<Text>(text());
        obj->range(range());
        return obj;
    }

    JSON to_json() const {
        JSON json = Object::to_json();
        json.set<std::string>("text", text());
        return json;
    }

    std::string to_jotdown() const {
        return _text;
    }

private:
    std::string _text;
};

//-------------------------------------------------------------------
class Hashtag : public Object {
public:
    Hashtag(const std::string& tag) : Object(Type::HASHTAG), _tag(tag) { }

    const std::string& tag() const {
        return _tag;
    }

    obj_t clone() const {
        auto obj = make<Hashtag>(tag());
        obj->range(range());
        return obj;
    }

    JSON to_json() const {
        JSON json = Object::to_json();
        json.set<std::string>("tag", tag());
        return json;
    }

    std::string to_jotdown() const {
        return tfm::format("#%s", tag());
    }

private:
    std::string _tag;
};

//-------------------------------------------------------------------
class LineBreak : public Object {
public:
    LineBreak() : Object(Type::LINE_BREAK) { }

    obj_t clone() const {
        auto obj = make<LineBreak>();
        obj->range(range());
        return obj;
    }

    std::string to_jotdown() const {
        return "\n";
    }
};

//-------------------------------------------------------------------
class Code : public Object {
public:
    Code(const std::string& code) : Object(Type::CODE), _code(code) { }

    const std::string& code() const {
        return _code;
    }

    obj_t clone() const {
        auto obj = make<Code>(code());
        obj->range(range());
        return obj;
    }

    JSON to_json() const {
        JSON json = Object::to_json();
        json.set<std::string>("code", code());
        return json;
    }

    std::string to_jotdown() const {
        std::ostringstream sb;
        for (auto c : _code) {
            if (c == '`') {
                sb << '\\';
            }
            sb << c;
        }
        return tfm::format("`%s`", sb.str());
    }

private:
    std::string _code;
};

//-------------------------------------------------------------------
class Ref : public Object {
public:
    Ref(const std::string& link, const std::string& text = "")
    : Object(Type::REF), _link(link), _text(text) { }

    const std::string& link() const {
        return _link;
    }

    const std::string& text() const {
        return _text;
    }

    obj_t clone() const {
        auto obj = make<Ref>(link(), text());
        obj->range(range());
        return obj;
    }

    JSON to_json() const {
        JSON json = Object::to_json();
        json.set<std::string>("link", link());
        if (text() != link()) {
            json.set<std::string>("text", text());
        }
        return json;
    }

    std::string to_jotdown() const {
        std::ostringstream sb;
        sb << "@";
        for (auto c : link()) {
            if (c == '[' || c == '\\') {
                sb << '\\';
            }
            sb << c;
        }
        if (link() != text()) {
            sb << "[";
            for (auto c : text()) {
                if (c == ']' || c == '\\') {
                    sb << '\\';
                }
                sb << c;
            }
            sb << "]";
        }
        return sb.str();
    }

private:
    std::string _link;
    std::string _text;
};

//-------------------------------------------------------------------
class TextContent : public Container {
public:
    friend class Section;
    friend class OrderedListItem;
    friend class UnorderedListItem;
    TextContent() : Container(Type::TEXT_CONTENT) { }

    std::shared_ptr<Anchor> add(std::shared_ptr<Anchor> anchor) {
        _add(anchor);
        return anchor;
    }

    std::shared_ptr<Hashtag> add(std::shared_ptr<Hashtag> hashtag) {
        _add(hashtag);
        return hashtag;
    }

    std::shared_ptr<Code> add(std::shared_ptr<Code> code) {
        _add(code);
        return code;
    }

    std::shared_ptr<Ref> add(std::shared_ptr<Ref> ref) {
        _add(ref);
        return ref;
    }

    std::shared_ptr<Text> add(std::shared_ptr<Text> text) {
        _add(text);
        return text;
    }

    obj_t clone() const {
        auto textblock = make<TextContent>();
        textblock->_copy_from(std::static_pointer_cast<const Container>(shared_from_this()));
        textblock->range(range());
        return textblock;
    }

    std::string to_jotdown() const {
        std::ostringstream sb;
        for (auto obj : contents()) {
            sb << obj->to_jotdown();
        }
        return sb.str();
    }
};

//-------------------------------------------------------------------
class ListItem;
class OrderedList;
class UnorderedList;

//-------------------------------------------------------------------
class List : public Container {
public:
    List(Type type) : Container(type) { }

    std::string to_jotdown() const {
        std::stringstream sb;
        for (auto obj : contents()) {
            sb << std::static_pointer_cast<Object>(obj)->to_jotdown();
        }
        return sb.str();
    }
};

//-------------------------------------------------------------------
class ListItem : public Container {
public:
    ListItem(Type type) : Container(type) { }

    virtual std::string crown() const = 0;

    std::shared_ptr<TextContent> text() {
        return _text_block;
    }

    std::shared_ptr<const TextContent> ctext() const {
        return _text_block;
    }

    std::shared_ptr<OrderedList> add(std::shared_ptr<OrderedList> list) {
        _add(std::static_pointer_cast<List>(list));
        return list;
    }

    std::shared_ptr<UnorderedList> add(std::shared_ptr<UnorderedList> list) {
        _add(std::static_pointer_cast<List>(list));
        return list;
    }

    const std::string& status() const {
        return _status;
    }

    void status(const std::string& status) {
        _status = status;
    }

    virtual JSON to_json() const {
        JSON json = Container::to_json();
        json.set_object("text", ctext()->to_json());
        if (status().size() > 0) {
            json.set<std::string>("status", status());
        }
        return json;
    }

    std::string to_jotdown() const {
        std::ostringstream sb;

        // Create the crown and first line of text.
        sb << crown() << " ";
        size_t y = 0;
        if (status().size() > 0) {
            sb << "[" << status() << "]";
        }
        for (; y < ctext()->contents().size(); y++) {
            auto obj = ctext()->contents()[y];
            if (obj->type() == Object::Type::TEXT) {
                std::shared_ptr<Text> text_obj = (
                    std::dynamic_pointer_cast<Text>(obj));
                if (text_obj->text().ends_with("\n")) {
                    sb << obj->to_jotdown();
                    y++;
                    break;
                }
            }
            sb << obj->to_jotdown();
        }

        // Any further lines of text are indented to match the crown.
        std::ostringstream sb_addl_text;
        for (; y < ctext()->contents().size(); y++) {
            auto obj = ctext()->contents()[y];
            sb_addl_text << obj->to_jotdown();
        }
        std::string addl_text = sb_addl_text.str();
        if (addl_text.size() > 0) {
            for (auto line : moonlight::str::split(addl_text, "\n")) {
                sb << std::string(crown().size() + 1, ' ') << line << "\n";
            }
        }

        // Collect and indent the text from the children.
        std::ostringstream sb_contents_jd;
        for (auto obj : contents()) {
            sb_contents_jd << obj->to_jotdown();
        }
        std::string contents_jd = sb_contents_jd.str();
        if (contents_jd.size() > 0) {
            for (auto line : moonlight::str::split(contents_jd, "\n")) {
                sb << std::string(config().list_indent, ' ') << line << "\n";
            }
        }

        // Done!
        return sb.str();
    }

private:
    std::shared_ptr<TextContent> _text_block = make<TextContent>();
    std::string _status;
};

//-------------------------------------------------------------------
class OrderedListItem : public ListItem {
public:
    OrderedListItem(const std::string& ordinal)
    : ListItem(Type::ORDERED_LIST_ITEM), _ordinal(ordinal) { }

    const std::string& ordinal() const {
        return _ordinal;
    }

    std::string crown() const {
        return ordinal() + ".";
    }

    obj_t clone() const {
        auto oli = make<OrderedListItem>(ordinal());
        oli->_copy_from(std::static_pointer_cast<const ListItem>(shared_from_this()));
        oli->range(range());
        oli->text()->_copy_from(std::static_pointer_cast<const Container>(ctext()));
        oli->text()->range(ctext()->range());
        return oli;
    }

    JSON to_json() const {
        JSON json = ListItem::to_json();
        json.set<std::string>("ordinal", ordinal());
        return json;
    }

private:
    std::string _ordinal;
};

//-------------------------------------------------------------------
class UnorderedListItem : public ListItem {
public:
    UnorderedListItem() : ListItem(Type::UNORDERED_LIST_ITEM) { }

    std::string crown() const {
        return "-";
    }

    obj_t clone() const {
        auto uli = make<UnorderedListItem>();
        uli->_copy_from(std::static_pointer_cast<const ListItem>(shared_from_this()));
        uli->range(range());
        uli->text()->_copy_from(std::static_pointer_cast<const Container>(ctext()));
        uli->text()->range(ctext()->range());
        return uli;
    }
};

//-------------------------------------------------------------------
class OrderedList : public List {
public:
    OrderedList() : List(Type::ORDERED_LIST) { }

    std::shared_ptr<OrderedListItem> add(std::shared_ptr<OrderedListItem> item) {
        _add(item);
        return item;
    }

    obj_t clone() const {
        auto ol = make<OrderedList>();
        ol->_copy_from(std::static_pointer_cast<const List>(shared_from_this()));
        ol->range(range());
        return ol;
    }
};

//-------------------------------------------------------------------
class UnorderedList : public List {
public:
    UnorderedList() : List(Type::UNORDERED_LIST) { }

    std::shared_ptr<UnorderedListItem> add(std::shared_ptr<UnorderedListItem> item) {
        _add(item);
        return item;
    }

    obj_t clone() const {
        auto ul = make<UnorderedList>();
        ul->_copy_from(std::static_pointer_cast<const List>(shared_from_this()));
        ul->range(range());
        return ul;
    }
};

//-------------------------------------------------------------------
class CodeBlock : public Object {
public:
    CodeBlock(const std::string& code, const std::string& language)
    : Object(Type::CODE_BLOCK), _code(code), _language(language) { }

    const std::string& code() const {
        return _code;
    }

    const std::string& language() const {
        return _language;
    }

    obj_t clone() const {
        auto obj = make<CodeBlock>(code(), language());
        obj->range(range());
        return obj;
    }

    JSON to_json() const {
        JSON json = Object::to_json();
        json.set<std::string>("code", code());
        if (_language.size() != 0) {
            json.set<std::string>("language", language());
        }
        return json;
    }

    std::string to_jotdown() const {
        std::ostringstream sb;
        sb << "```";
        if (language().size() > 0) {
            sb << " " << language();
        }
        sb << "\n";
        sb << code();
        sb << "```\n";
        return sb.str();
    }

private:
    std::string _code;
    std::string _language;
};

//-------------------------------------------------------------------
class Section : public Container {
public:
    Section(int level = 0) : Container(Type::SECTION), _level(level) { }

    int level() const {
        return _level;
    }

    std::shared_ptr<TextContent> header() {
        return _header;
    }

    std::shared_ptr<const TextContent> header() const {
        return _header;
    }

    std::shared_ptr<CodeBlock> add(std::shared_ptr<CodeBlock> code_block) {
        _add(code_block);
        return code_block;
    }

    std::shared_ptr<TextContent> add(std::shared_ptr<TextContent> text_block) {
        _add(text_block);
        return text_block;
    }

    std::shared_ptr<OrderedList> add(std::shared_ptr<OrderedList> list) {
        _add(list);
        return list;
    }

    std::shared_ptr<UnorderedList> add(std::shared_ptr<UnorderedList> list) {
        _add(list);
        return list;
    }

    std::shared_ptr<LineBreak> add(std::shared_ptr<LineBreak> line_break) {
        _add(line_break);
        return line_break;
    }

    std::shared_ptr<Section> add(std::shared_ptr<Section> section) {
        if (section->level() <= level()) {
            throw ObjectError("Sections must be added to parent sections of upper level.");
        }
        _add(section);
        return section;
    }

    obj_t clone() const {
        auto section = make<Section>(level());
        section->_copy_from(std::static_pointer_cast<const Section>(shared_from_this()));
        section->range(range());
        section->header()->_copy_from(std::static_pointer_cast<const Container>(header()));
        section->header()->range(header()->range());
        return section;
    }

    JSON to_json() const {
        JSON json = Container::to_json();
        json.set<double>("level", level());
        json.set_object("header", header()->to_json());
        return json;
    }

    std::string to_jotdown() const {
        std::stringstream sb;
        sb << std::string(level(), '#') << " ";
        sb << header()->to_jotdown();
        for (auto obj : contents()) {
            sb << obj->to_jotdown();
        }
        return sb.str();
    }

private:
    int _level;
    std::shared_ptr<TextContent> _header = make<TextContent>();
};

//-------------------------------------------------------------------
class Document : public Container {
public:
    Document() : Container(Type::DOCUMENT) { }

    std::shared_ptr<Section> add(std::shared_ptr<Section> section) {
        _add(section);
        return section;
    }

    obj_t clone() const {
        auto doc = make<Document>();
        doc->_copy_from(std::static_pointer_cast<const Container>(shared_from_this()));
        doc->range(range());
        return doc;
    }

    std::string to_jotdown() const {
        std::stringstream sb;
        for (auto obj : contents()) {
            sb << obj->to_jotdown();
        }
        return sb.str();
    }
};

}
}

#endif /* !__JOTDOWN_OBJECT_H */
