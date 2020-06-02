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
class ObjectError : public moonlight::core::Exception {
public:
    using Exception::Exception;
};

//-------------------------------------------------------------------
class Object {
public:
    enum class Type {
        NONE,
        ANCHOR,
        CODE,
        CODE_BLOCK,
        DOCUMENT,
        HASHTAG,
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

    const Location& location() const {
        return _location;
    }

    void location(const Location& location) {
        _location = location;
    }

    bool has_parent() const {
        return _parent != nullptr;
    }

    Object& parent() {
        if (! has_parent()) {
            throw ObjectError("Object has no parent.");
        }
        return *_parent;
    }

    const Object& parent() const {
        if (! has_parent()) {
            throw ObjectError("Object has no parent.");
        }
        return *_parent;
    }

    void parent(Object* object) {
        _parent = object;
    }

    static const std::string& type_name(Type type) {
        static const std::string names[] = {
            "NONE",
            "ANCHOR",
            "CODE",
            "CODE_BLOCK",
            "DOCUMENT",
            "HASHTAG",
            "ORDERED_LIST",
            "ORDERED_LIST_ITEM",
            "REF",
            "SECTION",
            "TEXT",
            "TEXT_BLOCK",
            "UNORDERED_LIST",
            "UNORDERED_LIST_ITEM",
            "NUM_TYPES"
        };

        return names[(unsigned int)type];
    }

    virtual JSON to_json() const {
        JSON json;
        json.set<std::string>("__type", type_name(type()));
        return json;
    }

    virtual Object* clone() const = 0;

private:
    Type _type;
    Location _location = NOWHERE;
    Object* _parent = nullptr;
};

//-------------------------------------------------------------------
template<class T>
class Container : public Object {
private:
    std::vector<std::shared_ptr<T>> _contents;

public:
    Container(Type type) : Object(type) { }
    Container(const Container<T>& other) : Object(other) {
        _copy_from(other);
    }

    typedef typename decltype(_contents)::iterator iterator;
    typedef typename decltype(_contents)::const_iterator const_iterator;

    void remove(const T& obj) {
        _contents.erase(std::remove_if(_contents.begin(), _contents.end(), [&](auto& obj_uniq) {
            return obj_uniq.get() == &obj;
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

    const std::vector<std::shared_ptr<T>>& contents() const {
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
    void _add(T* obj) {
        _contents.push_back(std::shared_ptr<T>(obj));
        obj->parent(this);
    }

    void _copy_from(const Container<T>& other) {
        for (auto& obj : other.contents()) {
            _add((T*)obj->clone());
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

    Object* clone() const {
        return new Anchor(name());
    }

    JSON to_json() const {
        JSON json = Object::to_json();
        json.set<std::string>("name", name());
        return json;
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

    Object* clone() const {
        return new Text(text());
    }

    JSON to_json() const {
        JSON json = Object::to_json();
        json.set<std::string>("text", text());
        return json;
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

    Object* clone() const {
        return new Hashtag(tag());
    }

    JSON to_json() const {
        JSON json = Object::to_json();
        json.set<std::string>("tag", tag());
        return json;
    }

private:
    std::string _tag;
};

//-------------------------------------------------------------------
class Code : public Object {
public:
    Code(const std::string& code) : Object(Type::CODE), _code(code) { }

    const std::string& code() const {
        return _code;
    }

    Object* clone() const {
        return new Code(code());
    }

    JSON to_json() const {
        JSON json = Object::to_json();
        json.set<std::string>("code", code());
        return json;
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

    Object* clone() const {
        return new Ref(link(), text());
    }

    JSON to_json() const {
        JSON json = Object::to_json();
        json.set<std::string>("link", link());
        if (text() != link()) {
            json.set<std::string>("text", text());
        }
        return json;
    }

private:
    std::string _link;
    std::string _text;
};

//-------------------------------------------------------------------
class TextBlock : public Container<Object> {
public:
    friend class Section;
    TextBlock() : Container(Type::TEXT_CONTENT) { }

    Anchor& add(Anchor* anchor) {
        _add(anchor);
        return *anchor;
    }

    Hashtag& add(Hashtag* hashtag) {
        _add(hashtag);
        return *hashtag;
    }

    Code& add(Code* code) {
        _add(code);
        return *code;
    }

    Ref& add(Ref* ref) {
        _add(ref);
        return *ref;
    }

    Text& add(Text* text) {
        _add(text);
        return *text;
    }

    Object* clone() const {
        auto textblock = new TextBlock();
        textblock->_copy_from(*this);
        return textblock;
    }
};

//-------------------------------------------------------------------
class List;
class ListItem;
class OrderedList;
class UnorderedList;

//-------------------------------------------------------------------
class List : public Container<ListItem> {
public:
    List(Type type) : Container(type) { }
};

//-------------------------------------------------------------------
class ListItem : public Container<List> {
public:
    ListItem(Type type) : Container(type) { }

    TextBlock& text() {
        return _text_block;
    }

    const TextBlock& text() const {
        return _text_block;
    }

    OrderedList& add(OrderedList* list) {
        _add((List*)list);
        return *list;
    }

    UnorderedList& add(UnorderedList* list) {
        _add((List*)list);
        return *list;
    }

    virtual JSON to_json() const {
        JSON json = Container::to_json();
        json.set_object("text", text().to_json());
        return json;
    }

private:
    TextBlock _text_block;
};

//-------------------------------------------------------------------
class OrderedListItem : public ListItem {
public:
    OrderedListItem(const std::string& ordinal)
    : ListItem(Type::ORDERED_LIST_ITEM), _ordinal(ordinal) { }

    const std::string& ordinal() const {
        return _ordinal;
    }

    Object* clone() const {
        auto oli = new OrderedListItem(ordinal());
        oli->_copy_from(*this);
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

    Object* clone() const {
        auto uli = new UnorderedListItem();
        uli->_copy_from(*this);
        return uli;
    }
};

//-------------------------------------------------------------------
class OrderedList : public List {
public:
    OrderedList() : List(Type::ORDERED_LIST) { }

    OrderedListItem& add(OrderedListItem* item) {
        _add(item);
        return *item;
    }

    Object* clone() const {
        auto ol = new OrderedList();
        ol->_copy_from(*this);
        return ol;
    }
};

//-------------------------------------------------------------------
class UnorderedList : public List {
public:
    UnorderedList() : List(Type::UNORDERED_LIST) { }

    UnorderedListItem& add(UnorderedListItem* item) {
        _add(item);
        return *item;
    }

    Object* clone() const {
        auto ul = new UnorderedList();
        ul->_copy_from(*this);
        return ul;
    }
};

//-------------------------------------------------------------------
class CodeBlock : public Object {
public:
    CodeBlock(const std::string& code) : Object(Type::CODE_BLOCK), _code(code) { }

    const std::string& code() const {
        return _code;
    }

    Object* clone() const {
        return new CodeBlock(code());
    }

    JSON to_json() const {
        JSON json = Object::to_json();
        json.set<std::string>("code", code());
        return json;
    }

private:
    std::string _code;
};

//-------------------------------------------------------------------
class Section : public Container<Object> {
public:
    Section(int level = 0) : Container(Type::SECTION), _level(level) { }

    int level() const {
        return _level;
    }

    TextBlock& header() {
        return _header;
    }

    const TextBlock& header() const {
        return _header;
    }

    CodeBlock& add(CodeBlock* code_block) {
        _add(code_block);
        return *code_block;
    }

    TextBlock& add(TextBlock* text_block) {
        _add(text_block);
        return *text_block;
    }

    OrderedList& add(OrderedList* list) {
        _add(list);
        return *list;
    }

    UnorderedList& add(UnorderedList* list) {
        _add(list);
        return *list;
    }

    Section& add(Section* section) {
        if (level() <= section->level()) {
            throw ObjectError("Sections must be added to parent sections of upper level.");
        }
        _add(section);
        return *section;
    }

    Object* clone() const {
        auto section = new Section(level());
        section->_copy_from(*this);
        section->header()._copy_from(header());
        return section;
    }

    JSON to_json() const {
        JSON json = Container::to_json();
        json.set<double>("__level", level());
        json.set_object("_header", header().to_json());
        return json;
    }

private:
    int _level;
    TextBlock _header;
};

//-------------------------------------------------------------------
class Document : public Container<Section> {
public:
    Document() : Container(Type::DOCUMENT) { }

    Section& add(Section* section) {
        _add(section);
        return *section;
    }

    Object* clone() const {
        auto doc = new Document();
        doc->_copy_from(*this);
        return doc;
    }
};

}
}

#endif /* !__JOTDOWN_OBJECT_H */
