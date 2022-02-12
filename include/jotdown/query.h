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
#include "moonlight/classify.h"
#include "jotdown/error.h"
#include "jotdown/interfaces.h"
#include "jotdown/object.h"
#include "jotdown/utils.h"
#include "tinyformat/tinyformat.h"

#include <regex>
#include <vector>
#include <set>

namespace jotdown {

EXCEPTION_TYPE(QueryError);

namespace q {

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

    std::vector<obj_t> select(obj_t object) const {
        return select(std::vector<obj_t>{object});
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
class TypeSelect : public Selector {
public:
    TypeSelect(Object::Type type) : type(type) { }

    Selector* clone() const {
        return new TypeSelect(type);
    }

    bool choose(obj_t obj) const {
        return obj->type() == type;
    }

    std::string repr() const {
        return tfm::format("Type<\"%s\">", Object::type_name(type));
    }

private:
    Object::Type type;
};

// ------------------------------------------------------------------
class SliceSelect : public Selector {
public:
    SliceSelect(const std::optional<int>& begin, const std::optional<int>& end)
    : begin(begin), end(end) { }

    Selector* clone() const {
        return new SliceSelect(begin, end);
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
class OffsetSelector : public Selector {
public:
    OffsetSelector(int offset) : offset(offset) { }

    std::vector<obj_t> select(const std::vector<obj_t>& objects) const {
        try {
            return {moonlight::slice(objects, offset)};

        } catch (const moonlight::core::IndexError& e) {
            return {};
        }
    }

    Selector* clone() const {
        return new OffsetSelector(offset);
    }

    std::string repr() const {
        return tfm::format("Offset<%d>", offset);
    }

private:
    int offset;
};

// ------------------------------------------------------------------
class LabelSelector : public Selector {
    Selector* clone() const {
        return new LabelSelector();
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
class ParentSelector : public Selector {
    Selector* clone() const {
        return new ParentSelector();
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
class ChildrenSelector : public Selector {
public:
    ChildrenSelector(bool include_labels = false) : include_labels(include_labels) { }

    Selector* clone() const {
        return new ChildrenSelector(include_labels);
    }

    std::vector<obj_t> select(const std::vector<obj_t>& objects) const {
        std::vector<obj_t> results;

        for (auto obj : objects) {
            if (include_labels && obj->has_label()) {
                auto sub_results = select({obj->label()});
                std::copy(sub_results.begin(), sub_results.end(), std::back_inserter(results));
            }
            if (obj->is_container()) {
                auto container = std::static_pointer_cast<Container>(obj);
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
class DescendantsSelector : public Selector {
public:
    DescendantsSelector(bool include_labels = false) : include_labels(include_labels) { }

    Selector* clone() const {
        return new DescendantsSelector(include_labels);
    }

    std::vector<obj_t> select(const std::vector<obj_t>& objects) const {
        std::vector<obj_t> results;

        for (auto obj : objects) {
            if (include_labels && obj->has_label()) {
                auto sub_results = select({obj->label()});
                std::copy(sub_results.begin(), sub_results.end(), std::back_inserter(results));
            }
            if (obj->is_container()) {
                auto container = std::static_pointer_cast<Container>(obj);
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
class AntecedentsSelector : public Selector {
public:
    Selector* clone() const {
        return new AntecedentsSelector();
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
class LevelSelector : public Selector {
public:
    LevelSelector(int level) : level(level) { }

    Selector* clone() const {
        return new LevelSelector(level);
    }

    bool choose(obj_t obj) const {
        if (obj->type() == Object::Type::ORDERED_LIST_ITEM ||
            obj->type() == Object::Type::UNORDERED_LIST_ITEM) {
            auto list_item = std::static_pointer_cast<ListItem>(obj);
            return list_item->level() == level;

        } else if (obj->type() == Object::Type::SECTION) {
            auto section = std::static_pointer_cast<Section>(obj);
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
class RegexSelector : public Selector {
public:
    RegexSelector(const std::string& rx) : str_rx(rx), rx(rx) { }

    Selector* clone() const {
        return new RegexSelector(str_rx, rx);
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
    RegexSelector(const std::string& str_rx, const std::regex& rx) : str_rx(str_rx), rx(rx) { }

    std::string str_rx;
    std::regex rx;
};

// ------------------------------------------------------------------
class HashtagSelector : public Selector {
public:
    HashtagSelector(const std::string& tag) : tag(tag) { }

    Selector* clone() const {
        return new HashtagSelector(tag);
    }

    bool choose(obj_t obj) const {
        if (obj->type() == Object::Type::HASHTAG) {
            auto hashtag = std::static_pointer_cast<Hashtag>(obj);
            return tag == "" || moonlight::str::to_lower(hashtag->tag()) == moonlight::str::to_lower(tag);
        }
        return false;
    }

    std::string repr() const {
        return tfm::format("HashtagSelector<\"%s\">", tag);
    }

private:
    std::string tag;
};

// ------------------------------------------------------------------
class ListSelector : public Selector {
public:
    ListSelector() : type(Object::Type::NONE) { }

    static Query Ordered() {
        return Query().by(ListSelector(Object::Type::ORDERED_LIST));
    }

    static Query Unordered() {
        return Query().by(ListSelector(Object::Type::UNORDERED_LIST));
    }

    Selector* clone() const {
        return new ListSelector(type);
    }

    bool choose(obj_t obj) const {
        if (type == Object::Type::NONE) {
            return obj->type() == Object::Type::ORDERED_LIST || obj->type() == Object::Type::UNORDERED_LIST;

        } else {
            return obj->type() == type;
        }
    }

    std::string repr() const {
        if (type == Object::Type::NONE) {
            return "ListSelector";
        } else {
            return Object::type_name(type);
        }
    }

private:
    ListSelector(Object::Type type) : type(type) { }
    Object::Type type;
};

// ------------------------------------------------------------------
class ListItemSelector : public Selector {
public:
    ListItemSelector() : type(Object::Type::NONE) { }

    static Query Ordered(const std::string& ordinal = "") {
        return Query().by(ListItemSelector(Object::Type::ORDERED_LIST_ITEM, ordinal));
    }

    static Query Unordered() {
        return Query().by(ListItemSelector(Object::Type::UNORDERED_LIST_ITEM));
    }

    static Query Checklist(const std::string& status = "") {
        return Query().by(ListItemSelector(Object::Type::NONE, "", true, status));
    }

    Selector* clone() const {
        return new ListItemSelector(type, ordinal, checklist_item, status);
    }

    bool choose(obj_t obj) const {
        if (! is_correct_type(obj)) {
            return false;
        }

        if (type == Object::Type::ORDERED_LIST_ITEM && ordinal.size() > 0) {
            auto oli = std::static_pointer_cast<OrderedListItem>(obj);
            if (oli->ordinal() != ordinal) {
                return false;
            }
        }

        if (checklist_item) {
            auto item = std::static_pointer_cast<ListItem>(obj);
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

        if (type == Object::Type::UNORDERED_LIST_ITEM) {
            return "UnorderedListItem";
        }

        if (type == Object::Type::ORDERED_LIST_ITEM) {
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
        if (type == Object::Type::NONE) {
            return obj->type() == Object::Type::ORDERED_LIST_ITEM || obj->type() == Object::Type::UNORDERED_LIST_ITEM;

        } else {
            return obj->type() == type;
        }
    }

    ListItemSelector(Object::Type type, const std::string& ordinal = "", bool checklist_item = false, const std::string& status = "")
    : type(type), ordinal(ordinal), checklist_item(checklist_item), status(status) { }

    Object::Type type;
    std::string ordinal;
    bool checklist_item = false;
    std::string status;
};

// ------------------------------------------------------------------
class AnchorSelector : public Selector {
public:
    AnchorSelector(const std::string& name) : name(name) { }

    Selector* clone() const {
        return new AnchorSelector(name);
    }

    bool choose(obj_t obj) const {
        if (obj->type() == Object::Type::HASHTAG) {
            auto anchor = std::static_pointer_cast<Anchor>(obj);
            return name == "" || anchor->name() == name;
        }
        return false;
    }

    std::string repr() const {
        if (name != "") {
            return tfm::format("AnchorSelector<\"%s\">", name);

        } else {
            return "AnchorSelector";
        }
    }

private:
    std::string name;
};

// ------------------------------------------------------------------
class ReferenceSelector : public Selector {
public:
    ReferenceSelector(const std::string& ref_search) : ref_search(ref_search) { }

    Selector* clone() const {
        return new ReferenceSelector(ref_search);
    }

    bool choose(obj_t obj) const {
        if (obj->type() == Object::Type::REF) {
            auto ref = std::static_pointer_cast<Ref>(obj);
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
class LogicalNot : public Selector {
public:
    LogicalNot(const Query& query) : query(query.clone()) { }

    Selector* clone() const {
        return new LogicalNot(query.get());
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
    LogicalNot(const Selector* selector) : query(selector->clone()) { }
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
            auto container = std::static_pointer_cast<Container>(obj);
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

namespace _private {

// ------------------------------------------------------------------
inline std::vector<std::string> tokenize_query(const std::string& query) {
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
inline RegexSelector build_search_query(std::vector<std::string>& tokens) {
    if (tokens.size() == 0) {
        throw QueryError("/search query requires an argument.");
    }

    auto rx = tokens.back();
    tokens.pop_back();
    return RegexSelector(rx);
}

// ------------------------------------------------------------------
inline LevelSelector build_level_query(std::vector<std::string>& tokens) {
    if (tokens.size() == 0) {
        throw QueryError("/level query requires an argument.");
    }

    try {
        auto level = std::stoi(tokens.back());
        tokens.pop_back();
        return LevelSelector(level);

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

    return ListItemSelector::Ordered(ordinal);
}

// ------------------------------------------------------------------
inline Query build_status_list_item_query(std::vector<std::string>& tokens) {
    if (tokens.size() == 0) {
        throw QueryError("/status query requires an argument.");
    }

    auto status = tokens.back();
    tokens.pop_back();

    return ListItemSelector::Checklist(status);
}

// ------------------------------------------------------------------
inline bool scan_offset_or_slice(Query& result, const std::string& token) {
    auto parts = moonlight::str::split(token, ":");
    switch(parts.size()) {
    case 2:
        try {
            result = Query().by(
                SliceSelect(parts[0] == "" ? std::optional<int>() : std::stoi(parts[0]),
                      parts[1] == "" ? std::optional<int>() : std::stoi(parts[1])));
            return true;
        } catch (...) {
            return false;
        }

    case 1:
        try {
            result = Query().by(OffsetSelector(std::stoi(parts[0])));
            return true;

        } catch (...) {
            return false;
        }

    default:
        return false;
    }
}

// ------------------------------------------------------------------
inline Query parse(std::vector<std::string>& tokens, int depth = -1) {
    Query query;
    Query sub_result;
    moonlight::Classifier<std::string> classify;
    int cycles = 0;

    auto startswith = [](const std::string& s) {
        return [=](const std::string& x) {
            return x.starts_with(s);
        };
    };

    classify("*") = [&]() { query.by(ChildrenSelector(true)); };
    classify("**") = [&]() { query.by(DescendantsSelector(true)); };
    classify(">") = [&]() { query.by(ChildrenSelector(false)); };
    classify(">>") = [&]() { query.by(DescendantsSelector(false)); };
    classify("<") = [&]() { query.by(ParentSelector()); };
    classify("<<") = [&]() { query.by(AntecedentsSelector()); };
    classify("label") = [&]() { query.by(LabelSelector()); };
    classify("contains") = [&]() { query.by(Contains(parse(tokens))); };
    classify("not", "!") = [&]() { query.by(LogicalNot(parse(tokens, 1))); };
    classify("search") = [&]() { query.by(build_search_query(tokens)); };
    classify("level") = [&]() { query.by(build_level_query(tokens)); };
    classify("line_break", "br") = [&]() { query.by(TypeSelect(Object::Type::LINE_BREAK)); };
    classify("text", "t") = [&]() { query.by(TypeSelect(Object::Type::TEXT)); };
    classify("content") = [&]() { query.by(TypeSelect(Object::Type::TEXT_CONTENT)); };
    classify("list") = [&]() { query.by(ListSelector()); };
    classify("ordered_list", "ol") = [&]() { query.by(ListSelector::Ordered()); };
    classify("unordered_list", "ul") = [&]() { query.by(ListSelector::Unordered()); };
    classify("check_list", "task_list") = [&]() { query.by(Contains(ListItemSelector::Checklist())); };
    classify("status") = [&]() { query.by(build_status_list_item_query(tokens)); };
    classify("item", "list_item", "li") = [&]() { query.by(ListItemSelector()); };
    classify("ordinal", "ord") = [&]() { query.by(build_ordinal_list_item_query(tokens)); };
    classify("ordered_list_item", "oli") = [&]() { query.by(ListItemSelector::Ordered()); };
    classify("unordered_list_item", "uli") = [&]() { query.by(ListItemSelector::Unordered()); };
    classify("section", "s") = [&]() { query.by(TypeSelect(Object::Type::SECTION)); };
    classify("task", "check_item", "task_item") = [&]() { query.by(ListItemSelector::Checklist()); };
    classify(startswith("(")) = [&](const std::string& token) {
        auto subquery_tokens = tokenize_query(moonlight::slice(token, 1, -1));
        query.by(Query(parse(subquery_tokens)));
    };
    classify(startswith("~")) = [&](const std::string& token) { query.by(RegexSelector(moonlight::slice(token, 1, {}))); };
    classify(startswith("#")) = [&](const std::string& token) { query.by(HashtagSelector(moonlight::slice(token, 1, {}))); };
    classify(startswith("@")) = [&](const std::string& token) { query.by(ReferenceSelector(moonlight::slice(token, 1, {}))); };
    classify(([&](const std::string& token) { return scan_offset_or_slice(sub_result, token); })) = [&]() { query.by(sub_result); };
    classify.otherwise() = [](const std::string& token) {
        throw QueryError(tfm::format("Unrecognized query token: \"%s\"", strliteral(token)));
    };

    while (tokens.size() > 0 && (depth == -1 || cycles < depth)) {
        cycles++;
        auto token = tokens.back();
        tokens.pop_back();
        classify.apply(token);
    }

    return query;
}

}

}

namespace q {

// ------------------------------------------------------------------
inline Query parse(const std::string& str) {
    auto tokens = _private::tokenize_query(str);
    std::reverse(tokens.begin(), tokens.end());
    return _private::parse(tokens);
}

}

typedef q::Query Query;

}

#endif /* !__JOTDOWN_QUERY_H */
