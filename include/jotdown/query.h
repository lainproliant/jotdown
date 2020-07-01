/*
 * query.h
 *
 * Author: Lain Musgrove (lain.proliant@gmail.com)
 * Date: Wednesday June 17, 2020
 *
 * Distributed under terms of the MIT license.
 */

#ifndef __JOTDOWN_QUERY_H
#define __JOTDOWN_QUERY_H

#include "moonlight/slice.h"
#include "jotdown/error.h"
#include "jotdown/interfaces.h"
#include "jotdown/object.h"
#include "tinyformat/tinyformat.h"

#include <regex>
#include <unordered_set>
#include <vector>
#include <set>

namespace jotdown {
namespace query {

using Object = object::Object;
using Type = object::Object::Type;
using obj_t = object::obj_t;

// ------------------------------------------------------------------
class QueryError : public moonlight::core::Exception {
    using Exception::Exception;
};

// ------------------------------------------------------------------
class Selector {
public:
    virtual ~Selector() { }

    virtual Selector* clone() const {
        return new Selector();
    }

    virtual std::vector<obj_t> select(const std::vector<obj_t>& objects) const {
        std::vector<obj_t> results;
        std::copy_if(objects.begin(), objects.end(), results.begin(), [&](auto obj) {
            return choose(obj);
        });
        return results;
    }

    virtual bool choose(obj_t obj) const {
        (void) obj;
        return true;
    }

    virtual std::string repr() const {
        return "Selector";
    }
};

// ------------------------------------------------------------------
class Query : public Selector {
public:
    Query() { }
    Query(const Selector* query) {
        sequence.emplace_back(query);
    }
    Query(const Query& other) {
        for (auto& query : other.sequence) {
            sequence.emplace_back(query->clone());
        }
    }
    Query& operator=(const Query& other) {
        sequence.clear();
        for (auto& query : other.sequence) {
            sequence.emplace_back(query->clone());
        }
        return *this;
    }

    Query operator|(const Query& query) {
        return _extend(query.clone());
    }

    Query operator|(const Selector* query) {
        return operator|(Query(query));
    }

    Query& operator|=(const Query& query) {
        *this = *this | query;
        return *this;
    }

    Query& operator|=(const Selector* query) {
        *this = *this | query;
        return *this;
    }

    Selector* clone() const {
        return new Query(*this);
    }

    std::vector<obj_t> select(const std::vector<obj_t>& objects) const {
        std::vector<obj_t> results = objects;

        for (auto& query : sequence) {
            if (results.size() == 0) {
                break;
            }
            results = query->select(results);
        }
        return results;
    }

    std::string repr() const {
        std::vector<std::string> seq_reprs;
        std::transform(sequence.begin(), sequence.end(), seq_reprs.end(),
                       [](auto& selector) {
                           return selector->repr();
                       });
        return tfm::format("Query<%s>", moonlight::str::join(seq_reprs, " "));
    }

private:
    Query _extend(const Selector* query) const {
        Query q = *this;
        q.sequence.emplace_back(query);
        return q;
    }

    std::vector<std::unique_ptr<const Selector>> sequence;
};

// ------------------------------------------------------------------
class Children : public Query {
    std::vector<obj_t> select(const std::vector<obj_t>& objects) const {
        std::vector<obj_t> results;

        for (auto obj : objects) {
            if (obj->is_container()) {
                auto container = std::static_pointer_cast<object::Container>(obj);
                auto& contents = container->contents();
                results.insert(results.end(), contents.begin(), contents.end());
            }
        }

        return results;
    }

    std::string repr() const {
        return "Children";
    }
};

// ------------------------------------------------------------------
class Descendants : public Query {
    std::vector<obj_t> select(const std::vector<obj_t>& objects) const {
        std::vector<obj_t> results;

        for (auto obj : objects) {
            if (obj->is_container()) {
                auto container = std::static_pointer_cast<object::Container>(obj);
                auto& contents = container->contents();
                results.insert(results.end(), contents.begin(), contents.end());
                auto sub_results = select(contents);
                results.insert(results.end(), sub_results.begin(), sub_results.end());
            }
        }

        return results;
    }

    std::string repr() const {
        return "Decendants";
    }
};

// ------------------------------------------------------------------
class Level : public Query {
public:
    Level(int level) : level(level) { }

    Selector* clone() const {
        return new Level(level);
    }

    bool choose(obj_t obj) const {
        if (obj->type() == Type::ORDERED_LIST_ITEM ||
            obj->type() == Type::UNORDERED_LIST_ITEM) {
            auto list_item = std::static_pointer_cast<object::ListItem>(obj);
            return list_item->level() == level;

        } else if (obj->type() == Type::SECTION) {
            auto section = std::static_pointer_cast<object::Section>(obj);
            return section->level() == level;
        }

        return false;
    }

    std::string repr() const {
        return tfm::format("Level<%d>", level);
    }

private:
    int level;
};

// ------------------------------------------------------------------
class Search : public Query {
public:
    Search(const std::string& rx) : str_rx(rx), rx(rx) { }

    Selector* clone() const {
        return new Search(str_rx, rx);
    }

    bool choose(obj_t obj) const {
        if (obj->has_label()) {
            return std::regex_search(obj->label()->to_search_string(), rx);
        } else {
            return std::regex_search(obj->to_search_string(), rx);
        }
    }

    std::string repr() const {
        return tfm::format("Search<\"%s\">", strliteral(str_rx));
    }

private:
    Search(const std::string& str_rx, const std::regex& rx) : str_rx(str_rx), rx(rx) { }

    std::string str_rx;
    std::regex rx;
};

// ------------------------------------------------------------------
class Hashtag : public Query {
public:
    Hashtag(const std::string& tag) : tag(tag) { }

    Selector* clone() const {
        return new Hashtag(tag);
    }

    bool choose(obj_t obj) const {
        if (obj->type() == Type::HASHTAG) {
            auto hashtag = std::static_pointer_cast<object::Hashtag>(obj);
            return tag == "" || hashtag->tag() == tag;
        }
        return false;
    }

    std::string repr() const {
        return tfm::format("Hashtag<\"%s\">", tag);
    }

private:
    std::string tag;
};

// ------------------------------------------------------------------
class List : public Query {
public:
    List() : type(Type::NONE) { }

    static List Ordered() {
        return List(Type::ORDERED_LIST);
    }

    static List Unordered() {
        return List(Type::UNORDERED_LIST);
    }

    bool choose(obj_t obj) const {
        if (type == Type::NONE) {
            return obj->type() == Type::ORDERED_LIST || obj->type() == Type::UNORDERED_LIST;

        } else {
            return obj->type() == type;
        }
    }

    std::string repr() const {
        if (type == Type::NONE) {
            return "List";
        } else {
            return Object::type_name(type);
        }
    }

private:
    List(Type type) : type(type) { }
    Type type;
};

// ------------------------------------------------------------------
class Item : public Query {
public:
    Item() : type(Type::NONE) { }

    static Item Ordered(const std::string& ordinal = "") {
        return Item(Type::ORDERED_LIST_ITEM, ordinal);
    }

    static Item Unordered() {
        return Item(Type::UNORDERED_LIST_ITEM);
    }

    static Item Checklist(const std::string& status = "") {
        return Item(Type::NONE, "", true, status);
    }

    bool choose(obj_t obj) const {
        if (! is_correct_type(obj)) {
            return false;
        }

        if (type == Type::ORDERED_LIST_ITEM && ordinal.size() > 0) {
            auto oli = std::static_pointer_cast<object::OrderedListItem>(obj);
            if (oli->ordinal() != ordinal) {
                return false;
            }
        }

        if (checklist_item) {
            auto item = std::static_pointer_cast<object::ListItem>(obj);
            return item->status() != "" && (status == "" || item->status() == status);
        }

        return true;
    }

    std::string repr() const {
        if (checklist_item) {
            if (status != "") {
                return tfm::format("ChecklistItem<\"%s\">", status);
            } else {
                return "ChecklistItem";
            }
        }

        if (type == Type::UNORDERED_LIST_ITEM) {
            return "UnorderedListItem";
        }

        if (type == Type::ORDERED_LIST_ITEM) {
            if (ordinal != "") {
                return "OrderedListItem";
            } else {
                return tfm::format("OrderedListItem<\"%s\">", ordinal);
            }
        }

        return "ListItem";
    }

private:
    bool is_correct_type(obj_t obj) const {
        if (type == Type::NONE) {
            return obj->type() == Type::ORDERED_LIST_ITEM || obj->type() == Type::UNORDERED_LIST_ITEM;

        } else {
            return obj->type() == type;
        }
    }

    Item(Type type, const std::string& ordinal = "", bool checklist_item = false, const std::string& status = "")
    : type(type), ordinal(ordinal), checklist_item(checklist_item), status(status) { }

    Type type;
    std::string ordinal;
    bool checklist_item;
    std::string status;
};

// ------------------------------------------------------------------
class Anchor : public Query {
public:
    Anchor(const std::string& name) : name(name) { }

    Selector* clone() const {
        return new Anchor(name);
    }

    bool choose(obj_t obj) const {
        if (obj->type() == Type::HASHTAG) {
            auto anchor = std::static_pointer_cast<object::Anchor>(obj);
            return name == "" || anchor->name() == name;
        }
        return false;
    }

    std::string repr() const {
        if (name != "") {
            return tfm::format("Anchor<\"%s\">", name);

        } else {
            return "Anchor";
        }
    }

private:
    std::string name;
};

// ------------------------------------------------------------------
class Reference : public Query {
public:
    Reference(const std::string& ref_search) : ref_search(ref_search) { }

    Selector* clone() const {
        return new Reference(ref_search);
    }

    bool choose(obj_t obj) const {
        if (obj->type() == Type::REF) {
            auto ref = std::static_pointer_cast<object::Ref>(obj);
            auto& link = ref->link();
            return ref_search == "" || link.find(ref_search) != std::string::npos;
        }
        return false;
    }

    std::string repr() const {
        if (ref_search != "") {
            return tfm::format("RefSearch<\"%s\">", strliteral(ref_search));
        } else {
            return "Reference";
        }
    }

private:
    std::string ref_search;
};

// ------------------------------------------------------------------
class Contains : public Query {
public:
    Contains(const Query& query) : query(query.clone()) { }

    bool choose(obj_t obj) const {
        if (obj->is_container()) {
            auto container = std::static_pointer_cast<object::Container>(obj);
            return query->select(container->contents()).size() > 0;
        }
        return false;
    }

    std::string repr() const {
        return tfm::format("Contains<%s>", query->repr());
    }

private:
    std::unique_ptr<Selector> query;
};

// ------------------------------------------------------------------
class Or : public Query {
public:
    Or(const Query& lhs, const Query& rhs) : lhs(lhs.clone()), rhs(rhs.clone()) { }

    std::vector<obj_t> select(const std::vector<obj_t>& objects) const {
        auto lhs_results = lhs->select(objects);
        auto rhs_results = rhs->select(objects);
        std::set<obj_t> result_set;
        std::vector<obj_t> results;
        result_set.insert(lhs_results.begin(), lhs_results.end());
        result_set.insert(rhs_results.begin(), rhs_results.end());
        results.insert(results.end(), result_set.begin(), result_set.end());
        return results;
    }

    std::string repr() const {
        return tfm::format("Or<%s %s>", lhs->repr(), rhs->repr());
    }

private:
    std::unique_ptr<Selector> lhs, rhs;
};

// ------------------------------------------------------------------
inline std::vector<std::string> tokenize(const std::string& query) {
    std::vector<std::string> tokens;
    std::string token;

    for (auto iter = query.begin();
         iter != query.end();
         iter++) {

        if (*iter == '/' && token.size() > 0) {
            tokens.push_back(token);
            token.clear();

        } else if (*iter == '\\') {
            if (std::next(iter) != query.end() && *std::next(iter) == '/') {
                token.push_back(*(++iter));
            }
        } else {
            token.push_back(*iter);
        }
    }

    if (token.size() > 0) {
        tokens.push_back(token);
    }

    return tokens;
}

// ------------------------------------------------------------------
inline Query build_search_query(std::vector<std::string>& tokens) {
    if (tokens.size() == 0) {
        throw QueryError("/search query requires an argument.");
    }

    auto rx = tokens.back();
    tokens.pop_back();
    return Search(rx);
}

// ------------------------------------------------------------------
inline Query build_level_query(std::vector<std::string>& tokens) {
    if (tokens.size() == 0) {
        throw QueryError("/level query requires an argument.");
    }

    try {
        auto level = std::stoi(tokens.back());
        tokens.pop_back();
        return Level(level);

    } catch (const std::exception& e) {
        throw QueryError("Failed to parse /level query parameter as integer.");
    }
}

// ------------------------------------------------------------------
inline Query build_ordinal_list_item_query(std::vector<std::string>& tokens) {
    if (tokens.size() == 0) {
        throw QueryError("/ordinal query requires an argument.");
    }

    auto ordinal = tokens.back();
    tokens.pop_back();

    return Item::Ordered(ordinal);
}

// ------------------------------------------------------------------
inline Query _parse(std::vector<std::string>& tokens) {
    Query query;

    while (tokens.size() > 0) {
        auto token = tokens.back();
        tokens.pop_back();

        if (token == "*") {
            query = Children() | query;

        } else if (token == "**") {
            query = Descendants() | query;

        } else if (token == "contains" || token == "<") {
            query = Contains(_parse(tokens));
            break;

        } else if (token == "..") {
            query = Contains(query);

        } else if (token == "search") {
            query = build_search_query(tokens) | query;

        } else if (token == "level") {
            query = build_level_query(tokens) | query;

        } else if (token == "list") {
            query = List() | query;

        } else if (token == "ordered_list" || token == "ol") {
            query = List::Ordered() | query;

        } else if (token == "unordered_list" || token == "ul") {
            query = List::Unordered() | query;

        } else if (token == "checklist") {
            query = Contains(Item::Checklist()) | query;

        } else if (token == "item" || token == "list_item") {
            query = Item() | query;

        } else if (token == "ordinal" || token == "ord") {
            query = build_ordinal_list_item_query(tokens) | query;

        } else if (token == "ordered_list_item" || token == "oli") {
            query = Item::Ordered() | query;

        } else if (token == "unordered_list_item" || token == "uli") {
            query = Item::Unordered() | query;

        } else if (token.starts_with("~")) {
            query = Search(moonlight::slice(token, 1, {})) | query;

        } else if (token.starts_with("#")) {
            query = Hashtag(moonlight::slice(token, 1, {})) | query;

        } else if (token.starts_with("&")) {
            query = Anchor(moonlight::slice(token, 1, {})) | query;

        } else if (token.starts_with("@")) {
            query = Reference(moonlight::slice(token, 1, {})) | query;
        }
    }

    return query;
}

// ------------------------------------------------------------------
inline Query parse(const std::string& str) {
    auto tokens = tokenize(str);
    std::reverse(tokens.begin(), tokens.end());
    return _parse(tokens);
}

}
}

#endif /* !__JOTDOWN_QUERY_H */
