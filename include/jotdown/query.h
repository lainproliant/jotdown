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
#include "jotdown/utils.h"
#include "tinyformat/tinyformat.h"

#include <regex>
#include <vector>
#include <set>

namespace jotdown {
namespace query {

using Object = object::Object;
using ObjectType = object::Object::Type;
using obj_t = object::obj_t;

// ------------------------------------------------------------------
class QueryError : public moonlight::core::Exception {
    using Exception::Exception;
};

// ------------------------------------------------------------------
class Selector {
public:
    virtual ~Selector() { }

    virtual Selector* clone() const  = 0;
    virtual std::string repr() const = 0;

    virtual std::vector<obj_t> select(const std::vector<obj_t>& objects) const {
        std::vector<obj_t> results;
        std::copy_if(objects.begin(), objects.end(), std::back_inserter(results), [&](auto obj) {
            return choose(obj);
        });
        return results;
    }

    virtual bool choose(obj_t obj) const {
        (void) obj;
        return true;
    }
};

// ------------------------------------------------------------------
class Query : public Selector {
public:
    Query() { }
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

    Query& by(const Selector& query) {
        sequence.emplace_back(query.clone());
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
        for (auto& query : sequence) {
            seq_reprs.push_back(query->repr());
        }
        return tfm::format("Query(%d)<%s>", sequence.size(), moonlight::str::join(seq_reprs, " "));
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
class Type : public Selector {
public:
    Type(ObjectType type) : type(type) { }

    Selector* clone() const {
        return new Type(type);
    }

    bool choose(obj_t obj) const {
        return obj->type() == type;
    }

    std::string repr() const {
        return tfm::format("Type<\"%s\">", Object::type_name(type));
    }

private:
    ObjectType type;
};

// ------------------------------------------------------------------
class Slice : public Selector {
public:
    Slice(const std::optional<int>& begin, const std::optional<int>& end)
    : begin(begin), end(end) { }

    Selector* clone() const {
        return new Slice(begin, end);
    }

    std::vector<obj_t> select(const std::vector<obj_t>& objects) const {
        return moonlight::slice(objects, begin, end);
    }

    std::string repr() const {
        if (begin.has_value() && end.has_value()) {
            return tfm::format("Slice<%d:%d>", *begin, *end);

        } else if (begin.has_value()) {
            return tfm::format("Slice<%d:>", *begin);

        } else if (end.has_value()) {
            return tfm::format("Slice<:%d>", *end);

        } else {
            return "Slice<:>";
        }
    }

private:
    std::optional<int> begin, end;
};

// ------------------------------------------------------------------
class Offset : public Selector {
public:
    Offset(int offset) : offset(offset) { }

    std::vector<obj_t> select(const std::vector<obj_t>& objects) const {
        try {
            return {moonlight::slice(objects, offset)};

        } catch (const moonlight::SliceError& e) {
            return {};
        }
    }

    Selector* clone() const {
        return new Offset(offset);
    }

    std::string repr() const {
        return tfm::format("Offset<%d>", offset);
    }

private:
    int offset;
};

// ------------------------------------------------------------------
class Label : public Selector {
    Selector* clone() const {
        return new Label();
    }

    std::vector<obj_t> select(const std::vector<obj_t>& objects) const {
        std::vector<obj_t> results;

        for (auto obj : objects) {
            if (obj->has_label()) {
                auto sub_results = select({obj->label()});
                std::copy(sub_results.begin(), sub_results.end(), std::back_inserter(results));
            }
        }

        return results;
    }

    std::string repr() const {
        return "Label";
    }
};

// ------------------------------------------------------------------
class Parents : public Selector {
    Selector* clone() const {
        return new Parents();
    }

    std::vector<obj_t> select(const std::vector<obj_t>& objects) const {
        std::vector<obj_t> intermediate_results;
        std::vector<obj_t> final_results;
        std::set<obj_t> final_result_set;

        for (auto obj : objects) {
            if (obj->has_parent()) {
                intermediate_results.push_back(obj->parent());
            }
        }

        for (auto parent : intermediate_results) {
            if (final_result_set.find(parent) == final_result_set.end()) {
                final_results.push_back(parent);
                final_result_set.insert(parent);
            }
        }

        return final_results;
    }

    std::string repr() const {
        return "Parents";
    }
};

// ------------------------------------------------------------------
class Children : public Selector {
public:
    Children(bool include_labels = false) : include_labels(include_labels) { }

    Selector* clone() const {
        return new Children(include_labels);
    }

    std::vector<obj_t> select(const std::vector<obj_t>& objects) const {
        std::vector<obj_t> results;

        for (auto obj : objects) {
            if (include_labels && obj->has_label()) {
                auto sub_results = select({obj->label()});
                std::copy(sub_results.begin(), sub_results.end(), std::back_inserter(results));
            }
            if (obj->is_container()) {
                auto container = std::static_pointer_cast<object::Container>(obj);
                auto& contents = container->contents();
                std::copy(contents.begin(), contents.end(), std::back_inserter(results));
            }
        }

        return results;
    }

    std::string repr() const {
        if (include_labels) {
            return "Children+Labels";
        } else {
            return "Children";
        }
    }

private:
    bool include_labels;
};

// ------------------------------------------------------------------
class Descendants : public Selector {
public:
    Descendants(bool include_labels = false) : include_labels(include_labels) { }

    Selector* clone() const {
        return new Descendants(include_labels);
    }

    std::vector<obj_t> select(const std::vector<obj_t>& objects) const {
        std::vector<obj_t> results;

        for (auto obj : objects) {
            if (include_labels && obj->has_label()) {
                auto sub_results = select({obj->label()});
                std::copy(sub_results.begin(), sub_results.end(), std::back_inserter(results));
            }
            if (obj->is_container()) {
                auto container = std::static_pointer_cast<object::Container>(obj);
                auto& contents = container->contents();
                std::copy(contents.begin(), contents.end(), std::back_inserter(results));
                auto sub_results = select(contents);
                std::copy(sub_results.begin(), sub_results.end(), std::back_inserter(results));
            }
        }

        return results;
    }

    std::string repr() const {
        if (include_labels) {
            return "Descendants+Labels";
        } else {
            return "Descendants";
        }
    }

private:
    bool include_labels = false;
};

// ------------------------------------------------------------------
class Antecedents : public Selector {
public:
    Selector* clone() const {
        return new Antecedents();
    }

    std::vector<obj_t> select(const std::vector<obj_t>& objects) const {
        std::vector<obj_t> results;
        std::vector<obj_t> final_results;
        std::set<obj_t> sieve;
        for (auto obj : objects) {
            if (obj->has_parent() && sieve.find(obj->parent()) == sieve.end()) {
                results.push_back(obj->parent());
                sieve.insert(obj->parent());
            }
        }
        if (results.size() == 0) {
            return {};
        }
        final_results = results;
        for (auto obj : select(results)) {
            if (sieve.find(obj) == sieve.end()) {
                final_results.push_back(obj);
                sieve.insert(obj);
            }
        }
        return final_results;
    }

    std::string repr() const {
        return "Antecedents";
    }
};

// ------------------------------------------------------------------
class Level : public Selector {
public:
    Level(int level) : level(level) { }

    Selector* clone() const {
        return new Level(level);
    }

    bool choose(obj_t obj) const {
        if (obj->type() == ObjectType::ORDERED_LIST_ITEM ||
            obj->type() == ObjectType::UNORDERED_LIST_ITEM) {
            auto list_item = std::static_pointer_cast<object::ListItem>(obj);
            return list_item->level() == level;

        } else if (obj->type() == ObjectType::SECTION) {
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
class Search : public Selector {
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
class Hashtag : public Selector {
public:
    Hashtag(const std::string& tag) : tag(tag) { }

    Selector* clone() const {
        return new Hashtag(tag);
    }

    bool choose(obj_t obj) const {
        if (obj->type() == ObjectType::HASHTAG) {
            auto hashtag = std::static_pointer_cast<object::Hashtag>(obj);
            return tag == "" || moonlight::str::to_lower(hashtag->tag()) == moonlight::str::to_lower(tag);
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
class List : public Selector {
public:
    List() : type(ObjectType::NONE) { }

    static Query Ordered() {
        return Query().by(List(ObjectType::ORDERED_LIST));
    }

    static Query Unordered() {
        return Query().by(List(ObjectType::UNORDERED_LIST));
    }

    Selector* clone() const {
        return new List(type);
    }

    bool choose(obj_t obj) const {
        if (type == ObjectType::NONE) {
            return obj->type() == ObjectType::ORDERED_LIST || obj->type() == ObjectType::UNORDERED_LIST;

        } else {
            return obj->type() == type;
        }
    }

    std::string repr() const {
        if (type == ObjectType::NONE) {
            return "List";
        } else {
            return Object::type_name(type);
        }
    }

private:
    List(ObjectType type) : type(type) { }
    ObjectType type;
};

// ------------------------------------------------------------------
class Item : public Selector {
public:
    Item() : type(ObjectType::NONE) { }

    static Query Ordered(const std::string& ordinal = "") {
        return Query().by(Item(ObjectType::ORDERED_LIST_ITEM, ordinal));
    }

    static Query Unordered() {
        return Query().by(Item(ObjectType::UNORDERED_LIST_ITEM));
    }

    static Query Checklist(const std::string& status = "") {
        return Query().by(Item(ObjectType::NONE, "", true, status));
    }

    Selector* clone() const {
        return new Item(type, ordinal, checklist_item, status);
    }

    bool choose(obj_t obj) const {
        if (! is_correct_type(obj)) {
            return false;
        }

        if (type == ObjectType::ORDERED_LIST_ITEM && ordinal.size() > 0) {
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

        if (type == ObjectType::UNORDERED_LIST_ITEM) {
            return "UnorderedListItem";
        }

        if (type == ObjectType::ORDERED_LIST_ITEM) {
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
        if (type == ObjectType::NONE) {
            return obj->type() == ObjectType::ORDERED_LIST_ITEM || obj->type() == ObjectType::UNORDERED_LIST_ITEM;

        } else {
            return obj->type() == type;
        }
    }

    Item(ObjectType type, const std::string& ordinal = "", bool checklist_item = false, const std::string& status = "")
    : type(type), ordinal(ordinal), checklist_item(checklist_item), status(status) { }

    ObjectType type;
    std::string ordinal;
    bool checklist_item = false;
    std::string status;
};

// ------------------------------------------------------------------
class Anchor : public Selector {
public:
    Anchor(const std::string& name) : name(name) { }

    Selector* clone() const {
        return new Anchor(name);
    }

    bool choose(obj_t obj) const {
        if (obj->type() == ObjectType::HASHTAG) {
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
class Reference : public Selector {
public:
    Reference(const std::string& ref_search) : ref_search(ref_search) { }

    Selector* clone() const {
        return new Reference(ref_search);
    }

    bool choose(obj_t obj) const {
        if (obj->type() == ObjectType::REF) {
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
class Not : public Selector {
public:
    Not(const Query& query) : query(query.clone()) { }

    Selector* clone() const {
        return new Not(query.get());
    }

    std::vector<obj_t> select(const std::vector<obj_t>& objects) const {
        auto results = query->select(objects);
        std::set<obj_t> results_set;
        results_set.insert(results.begin(), results.end());
        results.clear();

        std::copy_if(objects.begin(), objects.end(), std::back_inserter(results),
                     [&](auto obj) { return results_set.find(obj) == results_set.end(); });
        return results;
    }

    std::string repr() const {
        return tfm::format("Not<%s>", query->repr());
    }

private:
    Not(const Selector* selector) : query(selector->clone()) { }
    std::unique_ptr<Selector> query;
};

// ------------------------------------------------------------------
class Contains : public Selector {
public:
    Contains(const Query& query) : query(query.clone()) { }

    Selector* clone() const {
        return new Contains(query.get());
    }

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
    Contains(const Selector* selector) : query(selector->clone()) { }
    std::unique_ptr<Selector> query;
};

// ------------------------------------------------------------------
inline std::vector<std::string> tokenize(const std::string& query) {
    std::istringstream sinput(query);
    moonlight::file::BufferedInput input(sinput);
    std::vector<std::string> tokens;
    std::string token;

    for (;;) {
        int c = input.getc();

        if (c == EOF) {
            if (token.size() > 0) {
                tokens.push_back(token);
            }
            break;
        }

        if (c == '/') {
            if (token.size() > 0) {
                tokens.push_back(token);
                token.clear();
            }

        } else if (c == '\\') {
            int c2 = input.peek();

            if (c != EOF && (c == '/' || c == '(' || c == ')')) {
                token.push_back(c);
                token.push_back(c2);
                input.advance();
            }

        } else if (c == '(') {
            if (token.size() > 0) {
                tokens.push_back(token);
                token.clear();
            }

            token.push_back(c);

            // Scan for the next unescaped ')'
            for (;;) {
                c = input.getc();

                if (c == '\\') {
                    int c2 = input.peek();

                    if (c != EOF && (c == '/' || c == '(' || c == ')')) {
                        token.push_back(c);
                        token.push_back(c2);
                        input.advance();
                    }
                } else if (c == ')') {
                    token.push_back(c);
                    tokens.push_back(token);
                    token.clear();
                    break;
                } else if (c == EOF) {
                    throw QueryError("Unterminated parenthetical grouping in query.");
                } else {
                    token.push_back(c);
                }
            }
        } else {
            token.push_back(c);
        }
    }

    return tokens;
}

// ------------------------------------------------------------------
inline Search build_search_query(std::vector<std::string>& tokens) {
    if (tokens.size() == 0) {
        throw QueryError("/search query requires an argument.");
    }

    auto rx = tokens.back();
    tokens.pop_back();
    return Search(rx);
}

// ------------------------------------------------------------------
inline Level build_level_query(std::vector<std::string>& tokens) {
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
inline Query build_status_list_item_query(std::vector<std::string>& tokens) {
    if (tokens.size() == 0) {
        throw QueryError("/status query requires an argument.");
    }

    auto status = tokens.back();
    tokens.pop_back();

    return Item::Checklist(status);
}

// ------------------------------------------------------------------
inline bool scan_offset_or_slice(Query& result, const std::string& token) {
    auto parts = moonlight::str::split(token, ":");
    switch(parts.size()) {
    case 2:
        try {
            result = Query().by(
                Slice(parts[0] == "" ? std::optional<int>() : std::stoi(parts[0]),
                      parts[1] == "" ? std::optional<int>() : std::stoi(parts[1])));
            return true;
        } catch (...) {
            return false;
        }

    case 1:
        try {
            result = Query().by(Offset(std::stoi(parts[0])));
            return true;

        } catch (...) {
            return false;
        }

    default:
        return false;
    }
}

// ------------------------------------------------------------------
inline Query _parse(std::vector<std::string>& tokens, int depth = -1) {
    Query query;
    Query sub_result;
    Classifier<std::string> classify;
    int cycles = 0;

    auto startswith = [](const std::string& s) {
        return [=](const std::string& x) {
            return x.starts_with(s);
        };
    };

    classify("*") = [&]() { query.by(Children(true)); };
    classify("**") = [&]() { query.by(Descendants(true)); };
    classify(">") = [&]() { query.by(Children(false)); };
    classify(">>") = [&]() { query.by(Descendants(false)); };
    classify("<") = [&]() { query.by(Parents()); };
    classify("<<") = [&]() { query.by(Antecedents()); };
    classify("label") = [&]() { query.by(Label()); };
    classify("contains") = [&]() { query.by(Contains(_parse(tokens))); };
    classify("not", "!") = [&]() { query.by(Not(_parse(tokens, 1))); };
    classify("search") = [&]() { query.by(build_search_query(tokens)); };
    classify("level") = [&]() { query.by(build_level_query(tokens)); };
    classify("line_break", "br") = [&]() { query.by(Type(ObjectType::LINE_BREAK)); };
    classify("text", "t") = [&]() { query.by(Type(ObjectType::TEXT)); };
    classify("content") = [&]() { query.by(Type(ObjectType::TEXT_CONTENT)); };
    classify("list") = [&]() { query.by(List()); };
    classify("ordered_list", "ol") = [&]() { query.by(List::Ordered()); };
    classify("unordered_list", "ul") = [&]() { query.by(List::Unordered()); };
    classify("check_list", "task_list") = [&]() { query.by(Contains(Item::Checklist())); };
    classify("status") = [&]() { query.by(build_status_list_item_query(tokens)); };
    classify("item", "list_item", "li") = [&]() { query.by(Item()); };
    classify("ordinal", "ord") = [&]() { query.by(build_ordinal_list_item_query(tokens)); };
    classify("ordered_list_item", "oli") = [&]() { query.by(Item::Ordered()); };
    classify("unordered_list_item", "uli") = [&]() { query.by(Item::Unordered()); };
    classify("section", "s") = [&]() { query.by(Type(ObjectType::SECTION)); };
    classify("task", "check_item", "task_item") = [&]() { query.by(Item::Checklist()); };
    classify(startswith("(")) = [&](const std::string& token) {
        auto subquery_tokens = tokenize(moonlight::slice(token, 1, -1));
        query.by(Query(_parse(subquery_tokens)));
    };
    classify(startswith("~")) = [&](const std::string& token) { query.by(Search(moonlight::slice(token, 1, {}))); };
    classify(startswith("#")) = [&](const std::string& token) { query.by(Hashtag(moonlight::slice(token, 1, {}))); };
    classify(startswith("@")) = [&](const std::string& token) { query.by(Reference(moonlight::slice(token, 1, {}))); };
    classify(([&](const std::string& token) { return scan_offset_or_slice(sub_result, token); })) = [&]() { query.by(sub_result); };
    classify.otherwise() = [](const std::string& token) {
        throw QueryError(tfm::format("Unrecognized query token: \"%s\"", strliteral(token)));
    };

    while (tokens.size() > 0 && (depth == -1 || cycles < depth)) {
        cycles++;
        auto token = tokens.back();
        tokens.pop_back();
        classify.match(token);
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
