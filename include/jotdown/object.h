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
        FRONT_MATTER,
        HASHTAG,
        LINE_BREAK,
        ORDERED_LIST,
        ORDERED_LIST_ITEM,
        REF,
        REF_INDEX,
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
            "FrontMatter",
            "Hashtag",
            "LineBreak",
            "OrderedList",
            "OrderedListItem",
            "Ref",
            "RefIndex",
            "Section",
            "Text",
            "TextContent",
            "UnorderedList",
            "UnorderedListItem",
            "NUM_TYPES"
        };

        return names[static_cast<unsigned int>(type)];
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

    virtual std::string repr() const {
        return type_name(type());
    }

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

    bool is_empty() const {
        return contents().size() == 0;
    }

    virtual bool can_contain(cobj_t obj) const = 0;

    template<class T>
    std::shared_ptr<T> add(std::shared_ptr<T> obj) {
        _check_can_contain(obj);
        _reparent(obj);
        _contents.push_back(obj);
        return obj;
    }

    template<class T>
    std::shared_ptr<T> insert_before(cobj_t pivot, std::shared_ptr<T> obj) {
        _check_can_contain(obj);
        auto iter = std::find(_contents.begin(), _contents.end(), pivot);
        if (iter == _contents.end()) {
            throw ObjectError("Pivot object does not exist in container.");
        }
        _contents.insert(iter, obj);
        return obj;
    }

    template<class T>
    std::shared_ptr<T> insert_after(cobj_t pivot, std::shared_ptr<T> obj) {
        _check_can_contain(obj);
        auto iter = std::find(_contents.begin(), _contents.end(), pivot);
        if (iter == _contents.end()) {
            throw ObjectError("Pivot object does not exist in container.");
        }
        _contents.insert(std::next(iter), obj);
        return obj;
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

    std::string repr() const {
        return tfm::format("%s(%d)", type_name(type()), contents().size());
    }

protected:
    void _reparent(obj_t obj) {
        if (obj->has_parent()) {
            obj->parent()->remove(obj);
        }
        obj->parent(std::static_pointer_cast<Container>(shared_from_this()));
    }

    void _copy_from(std::shared_ptr<const Container> other) {
        for (auto& obj : other->contents()) {
            add(obj->clone());
        }
    }

    void _check_can_contain(cobj_t obj) const {
        if (! can_contain(obj)) {
            throw ObjectError(tfm::format("%s cannot contain %s.",
                                          type_name(type()),
                                          obj->type_name(obj->type())));
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
        auto obj = std::make_shared<Anchor>(name());
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

    std::string repr() const {
        return tfm::format("%s<\"%s\">", type_name(type()), strliteral(name()));
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
        auto obj = std::make_shared<Text>(text());
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

    std::string repr() const {
        return tfm::format("%s<\"%s\">", type_name(type()), strliteral(text()));
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
        auto obj = std::make_shared<Hashtag>(tag());
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

    std::string repr() const {
        return tfm::format("%s<\"%s\">", type_name(type()), strliteral(tag()));
    }

private:
    std::string _tag;
};

//-------------------------------------------------------------------
class LineBreak : public Object {
public:
    LineBreak() : Object(Type::LINE_BREAK) { }

    obj_t clone() const {
        auto obj = std::make_shared<LineBreak>();
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
        auto obj = std::make_shared<Code>(code());
        obj->range(range());
        return obj;
    }

    JSON to_json() const {
        JSON json = Object::to_json();
        json.set<std::string>("code", code());
        return json;
    }

    std::string to_jotdown() const {
        return tfm::format("`%s`",
                           strescape(code(), "`"));
    }

    std::string to_search_string() const {
        return to_jotdown();
    }

//-------------------------------------------------------------------
    std::string repr() const {
        return tfm::format("%s<\"%s\">", type_name(type()), strliteral(code()));
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
        auto obj = std::make_shared<Ref>(link(), text());
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
        if (link() == text()) {
            return tfm::format("<%s>",
                               strescape(link(), ">"));
        } else {
            return tfm::format("[%s](%s)",
                               strescape(text(), "]"),
                               strescape(link(), ")"));
        }
    }

    std::string to_search_string() const {
        return tfm::format("%s %s", text(), link());
    }

    std::string repr() const {
        if (link() == text()) {
            return tfm::format("%s<\"%s\">", type_name(type()), strliteral(link()));

        } else {
            return tfm::format("%s<link=\"%s\" text=\"%s\">", type_name(type()), link(), text());
        }
    }

private:
    std::string _link;
    std::string _text;
};

//-------------------------------------------------------------------
class IndexedRef : public Object {
public:
    IndexedRef(const std::string& text, const std::string& index_name) : Object(Type::REF), _text(text), _index_name(index_name) { }

    void link(const std::string& link) {
        _link = link;
    }

    const std::string& link() const {
        return _link;
    }

    const std::string& text() const {
        return _text;
    }

    const std::string& index_name() const {
        return _index_name;
    }

    JSON to_json() const {
        JSON json = Object::to_json();
        json.set<std::string>("text", text());
        json.set<std::string>("index_name", index_name());
        return json;
    }

    obj_t clone() const {
        auto obj = std::make_shared<IndexedRef>(text(), index_name());
        obj->link(link());
        obj->range(range());
        return obj;
    }

    std::string to_jotdown() const {
        return tfm::format("[%s][%s]",
                           strescape(text(), "]"),
                           strescape(index_name(), "]"));
    }

    std::string to_search_string() const {
        return text();
    }

    std::string repr() const {
        return tfm::format("%s<index_name=\"%s\" text=\"%s\">", type_name(type()), index_name(), text());
    }

private:
    std::string _text;
    std::string _index_name;
    std::string _link = "";
};

//-------------------------------------------------------------------
class RefIndex : public Object {
public:
    RefIndex(const std::string& name, const std::string& link) : Object(Type::REF_INDEX), _name(name), _link(link) { }

    const std::string& name() const {
        return _name;
    }

    const std::string& link() const {
        return _link;
    }

    obj_t clone() const {
        auto obj = std::make_shared<RefIndex>(name(), link());
        obj->range(range());
        return obj;
    }

    JSON to_json() const {
        JSON json = Object::to_json();
        json.set<std::string>("name", name());
        json.set<std::string>("link", link());
        return json;
    }

    std::string to_jotdown() const {
        return tfm::format("[%s]: %s",
                           strescape(name(), "]"),
                           link());
    }

    std::string to_search_string() const {
        return tfm::format("%s: %s", name(), link());
    }

    std::string repr() const {
        return tfm::format("%s<name=\"%s\" link=\"%s\">", type_name(type()), name(), link());
    }

private:
    std::string _name;
    std::string _link;
};

//-------------------------------------------------------------------
class TextContent : public Container {
public:
    friend class Section;
    friend class OrderedListItem;
    friend class UnorderedListItem;
    TextContent() : Container(Type::TEXT_CONTENT) { }

    bool can_contain(cobj_t obj) const {
        static const std::vector<Type> cont = {
            Type::ANCHOR,
            Type::CODE,
            Type::HASHTAG,
            Type::REF,
            Type::REF_INDEX,
            Type::TEXT
        };
        return std::find(cont.begin(), cont.end(), obj->type()) != cont.end();
    }

    obj_t clone() const {
        auto textblock = std::make_shared<TextContent>();
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

    std::string repr() const {
        return tfm::format("%s<\"%s\">", Container::repr(), strliteral(to_jotdown()));
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

    int level() const {
        return _level;
    }

    void level(int level) {
        _level = level;
    }

private:
    int _level = 1;
};

//-------------------------------------------------------------------
class ListItem : public Container {
public:
    virtual std::string crown() const = 0;

    bool can_contain(cobj_t obj) const {
        static const std::vector<Type> cont = {
            Type::ORDERED_LIST,
            Type::UNORDERED_LIST
        };
        return std::find(cont.begin(), cont.end(), obj->type()) != cont.end();
    }

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

    int level() const {
        return _level;
    }

    void level(int level) {
        _level = level;
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
            if (obj->type() == Type::TEXT) {
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

    std::string repr() const {
        return tfm::format("%s<\"%s\">", Container::repr(), to_search_string());
    }

protected:
    ListItem(Type type) : Container(type) { }

    static void init(std::shared_ptr<ListItem> item) {
        item->text(std::make_shared<TextContent>());
    }

private:
    std::shared_ptr<TextContent> _text_block = nullptr;
    std::string _status;
    int _level = 1;
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

    bool can_contain(cobj_t obj) const {
        return obj->type() == Type::ORDERED_LIST_ITEM;
    }

    obj_t clone() const {
        auto ol = std::make_shared<OrderedList>();
        ol->_copy_from(std::static_pointer_cast<const List>(shared_from_this()));
        ol->range(range());
        return ol;
    }
};

//-------------------------------------------------------------------
class UnorderedList : public List {
public:
    UnorderedList() : List(Type::UNORDERED_LIST) { }

    bool can_contain(cobj_t obj) const {
        return obj->type() == Type::UNORDERED_LIST_ITEM;
    }

    obj_t clone() const {
        auto ul = std::make_shared<UnorderedList>();
        ul->_copy_from(std::static_pointer_cast<const List>(shared_from_this()));
        ul->range(range());
        return ul;
    }
};

//-------------------------------------------------------------------
class EmbeddedDocument : public Object {
public:
    EmbeddedDocument(Type type, const std::string& code, const std::string& language)
    : Object(type), _code(code), _language(language) { }

    const std::string& code() const {
        return _code;
    }

    const std::string& language() const {
        return _language;
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
        sb << delimiter();
        if (language().size() > 0) {
            sb << " " << language();
        }
        sb << "\n";
        sb << code();
        if (! code().ends_with('\n')) {
            sb << '\n';
        }
        sb << delimiter() << '\n';
        return sb.str();
    }

    std::string to_search_string() const {
        return code();
    }

    std::string repr() const {
        if (language() != "") {
            return tfm::format("%s<lang=\"%s\" code=\"%s\"", type_name(type()), language(), strliteral(code()));

        } else {
            return tfm::format("%s<\"%s\">", type_name(type()), strliteral(code()));
        }
    }

protected:
    virtual const std::string& delimiter() const = 0;

private:
    std::string _code;
    std::string _language;
};

//-------------------------------------------------------------------
class CodeBlock : public EmbeddedDocument {
public:
    CodeBlock(const std::string& code, const std::string& language)
    : EmbeddedDocument(Type::CODE_BLOCK, code, language) { }

    obj_t clone() const {
        auto obj = std::make_shared<CodeBlock>(code(), language());
        obj->range(range());
        return obj;
    }

    const std::string& delimiter() const {
        static const std::string delim = "```";
        return delim;
    }

private:
    std::string _code;
    std::string _language;
};

//-------------------------------------------------------------------
class FrontMatter : public EmbeddedDocument {
public:
    FrontMatter(const std::string& code, const std::string& language)
    : EmbeddedDocument(Type::FRONT_MATTER, code, language) { }

    obj_t clone() const {
        auto obj = std::make_shared<FrontMatter>(code(), language());
        obj->range(range());
        return obj;
    }

    const std::string& delimiter() const {
        static const std::string delim = "---";
        return delim;
    }
};

//-------------------------------------------------------------------
class Section : public Container {
public:
    static std::shared_ptr<Section> create(int level = 0) {
        auto section = std::shared_ptr<Section>(new Section(level));
        section->header(std::make_shared<TextContent>());
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

    bool can_contain(cobj_t obj) const {
        static const std::vector<Type> cont = {
            Type::CODE_BLOCK,
            Type::LINE_BREAK,
            Type::ORDERED_LIST,
            Type::SECTION,
            Type::TEXT_CONTENT,
            Type::UNORDERED_LIST
        };
        return std::find(cont.begin(), cont.end(), obj->type()) != cont.end();
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

    std::string repr() const {
        return tfm::format("%s<level=%d header=\"%s\">", Container::repr(), level(),
                           strliteral(cheader()->to_jotdown()));
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

    bool can_contain(cobj_t obj) const {
        return obj->type() == Type::SECTION;
    }

    std::shared_ptr<FrontMatter> front_matter() const {
        return _front_matter;
    }

    void front_matter(std::shared_ptr<FrontMatter> front_matter) {
        _front_matter = front_matter;
    }

    obj_t clone() const {
        auto doc = std::make_shared<Document>();
        if (front_matter() != nullptr) {
            doc->front_matter(std::static_pointer_cast<FrontMatter>(front_matter()->clone()));
        }
        doc->_copy_from(std::static_pointer_cast<const Container>(shared_from_this()));
        doc->range(range());
        return doc;
    }

    std::string to_jotdown() const {
        std::stringstream sb;
        if (front_matter() != nullptr) {
            sb << front_matter()->to_jotdown();
        }
        for (auto obj : contents()) {
            sb << obj->to_jotdown();
        }
        return sb.str();
    }

private:
    std::shared_ptr<FrontMatter> _front_matter;
};

}
}

#endif /* !__JOTDOWN_OBJECT_H */
