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

//-------------------------------------------------------------------
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
class Container;
class TextContent;
typedef std::shared_ptr<Object> obj_t;
typedef std::shared_ptr<const Object> cobj_t;
typedef std::shared_ptr<Container> container_t;
typedef std::shared_ptr<const Container> ccontainer_t;
typedef std::shared_ptr<TextContent> text_t;
typedef std::shared_ptr<const TextContent> ctext_t;

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

    container_t parent() {
        return _parent;
    }

    ccontainer_t cparent() const {
        return std::const_pointer_cast<Container>(_parent);
    }

    void parent(container_t obj) {
        _parent = obj;
    }

    bool has_label() const {
        return label() != nullptr;
    }

    virtual text_t label() const {
        return nullptr;
    }

    virtual ctext_t clabel() const {
        return nullptr;
    }

    virtual bool is_container() const {
        return false;
    }

    static const std::string& type_name(Type type) {
        static const std::string names[] = {
            "NONE",
            "Anchor",
            "Code",
            "CodeBlock",
            "Document",
            "Hashtag",
            "LineBreak",
            "OrderedList",
            "OrderedListItem",
            "Ref",
            "Section",
            "Text",
            "TextContent",
            "UnorderedList",
            "UnorderedListItem",
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
    virtual std::string to_search_string() const = 0;

private:
    Type _type;
    container_t _parent = nullptr;
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

    bool is_container() const {
        return true;
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

    void shift_up(cobj_t obj) {
        auto iter = std::find(_contents.begin(), _contents.end(), obj);
        if (iter == _contents.end()) {
            throw ObjectError("Object does not exist in this container.");
        }
        if (iter == _contents.begin()) {
            throw ObjectError("Object is already the first in the container.");
        }
        std::iter_swap(std::prev(iter), iter);
    }

    void shift_down(cobj_t obj) {
        auto iter = std::find(_contents.begin(), _contents.end(), obj);
        if (iter == _contents.end()) {
            throw ObjectError("Object does not exist in this container.");
        }
        if (std::next(iter) == _contents.end()) {
            throw ObjectError("Object is already the last in the container.");
        }
        std::iter_swap(std::next(iter), iter);
    }

    void remove(obj_t obj) {
        auto iter = std::find(_contents.begin(), _contents.end(), obj);
        if (iter == _contents.end()) {
            throw ObjectError("Object is not in the container.");
        }
        _contents.erase(iter);
        if ((*iter)->has_parent() && (*iter)->parent() == shared_from_this()) {
            (*iter)->parent(nullptr);
        }
    }

    void clear() {
        for (auto obj : _contents) {
            if (obj->has_parent() && obj->parent() == shared_from_this()) {
                obj->parent(nullptr);
            }
        }
        _contents.clear();
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

    std::string to_search_string() const {
        std::ostringstream sb;
        for (auto obj : contents()) {
            sb << obj->to_search_string();
        }
        return sb.str();
    }

protected:
    void _add(obj_t obj) {
        if (obj->has_parent()) {
            obj->parent()->remove(obj);
        }
        _contents.push_back(obj);
        obj->parent(std::static_pointer_cast<Container>(shared_from_this()));
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

    std::string to_search_string() const {
        return name();
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

    std::string to_search_string() const {
        return make_search_string(text());
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

    std::string to_search_string() const {
        return tag();
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

    std::string to_search_string() const {
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

    std::string to_search_string() const {
        return to_jotdown();
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

    std::string to_search_string() const {
        return text();
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
        auto str = sb.str();
        if (! str.ends_with('\n')) {
            str.push_back('\n');
        }
        return str;
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
    virtual std::string crown() const = 0;

    std::shared_ptr<TextContent> text() {
        return _text_block;
    }

    void text(std::shared_ptr<TextContent> text) {
        if (_text_block != nullptr && _text_block->has_parent() && _text_block->parent() == shared_from_this()) {
            _text_block->parent(nullptr);
        }
        _text_block = text;
        text->parent(std::static_pointer_cast<Container>(shared_from_this()));
    }

    std::shared_ptr<const TextContent> ctext() const {
        return _text_block;
    }

    text_t label() const {
        return _text_block;
    }

    ctext_t clabel() const {
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

    std::string to_search_string() const {
        std::ostringstream sb;

        sb << crown() << " ";
        if (status().size() > 0) {
            sb << "[" << status() << "]" << " ";
        }

        sb << ctext()->to_search_string();
        return sb.str();
    }

protected:
    ListItem(Object::Type type) : Container(type) { }

    static void init(std::shared_ptr<ListItem> item) {
        item->text(make<TextContent>());
    }

private:
    std::shared_ptr<TextContent> _text_block = nullptr;
    std::string _status;
};

//-------------------------------------------------------------------
class OrderedListItem : public ListItem {
public:
    static std::shared_ptr<OrderedListItem> create(const std::string& ordinal) {
        auto oli = std::shared_ptr<OrderedListItem>(new OrderedListItem(ordinal));
        ListItem::init(oli);
        return oli;
    }

    const std::string& ordinal() const {
        return _ordinal;
    }

    std::string crown() const {
        return ordinal() + ".";
    }

    obj_t clone() const {
        auto oli = OrderedListItem::create(ordinal());
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
    OrderedListItem(const std::string& ordinal)
    : ListItem(Type::ORDERED_LIST_ITEM), _ordinal(ordinal) { }

    std::string _ordinal;
};

//-------------------------------------------------------------------
class UnorderedListItem : public ListItem {
public:
    static std::shared_ptr<UnorderedListItem> create() {
        auto uli = std::shared_ptr<UnorderedListItem>(new UnorderedListItem());
        ListItem::init(uli);
        return uli;
    }

    std::string crown() const {
        return "-";
    }

    obj_t clone() const {
        auto uli = UnorderedListItem::create();
        uli->_copy_from(std::static_pointer_cast<const ListItem>(shared_from_this()));
        uli->range(range());
        uli->text()->_copy_from(std::static_pointer_cast<const Container>(ctext()));
        uli->text()->range(ctext()->range());
        return uli;
    }

private:
    UnorderedListItem() : ListItem(Type::UNORDERED_LIST_ITEM) { }
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
        if (! code().ends_with('\n')) {
            sb << '\n';
        }
        sb << "```\n";
        return sb.str();
    }

    std::string to_search_string() const {
        return code();
    }

private:
    std::string _code;
    std::string _language;
};

//-------------------------------------------------------------------
class Section : public Container {
public:
    static std::shared_ptr<Section> create(int level = 0) {
        auto section = std::shared_ptr<Section>(new Section(level));
        section->header(make<TextContent>());
        return section;
    }

    int level() const {
        return _level;
    }

    std::shared_ptr<TextContent> header() {
        return _header;
    }

    void header(std::shared_ptr<TextContent> header) {
        if (_header != nullptr && _header->has_parent() && _header->parent() == shared_from_this()) {
            _header->parent(nullptr);
        }
        _header = header;
        _header->parent(std::static_pointer_cast<Container>(shared_from_this()));
    }

    std::shared_ptr<const TextContent> cheader() const {
        return _header;
    }

    text_t label() const {
        return _header;
    }

    ctext_t clabel() const {
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
        auto section = Section::create(level());
        section->_copy_from(std::static_pointer_cast<const Section>(shared_from_this()));
        section->range(range());
        section->header()->_copy_from(std::static_pointer_cast<const Container>(cheader()));
        section->header()->range(cheader()->range());
        return section;
    }

    JSON to_json() const {
        JSON json = Container::to_json();
        json.set<double>("level", level());
        json.set_object("header", cheader()->to_json());
        return json;
    }

    std::string to_jotdown() const {
        std::stringstream sb;
        sb << std::string(level(), '#') << " ";
        sb << cheader()->to_jotdown();
        for (auto obj : contents()) {
            sb << obj->to_jotdown();
        }
        return sb.str();
    }

    std::string to_search_string() const {
        return cheader()->to_search_string();
    }

private:
    Section(int level = 0) : Container(Type::SECTION), _level(level) { }

    int _level;
    std::shared_ptr<TextContent> _header = nullptr;
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
