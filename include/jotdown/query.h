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

#include <regex>
#include <unordered_set>
#include <vector>
#include "jotdown/error.h"
#include "jotdown/interfaces.h"
#include "jotdown/object.h"
#include "tinyformat/tinyformat.h"

namespace jotdown {
namespace query {

using Object = object::Object;
using Type = object::Object::Type;
using obj_t = object::obj_t;

// ------------------------------------------------------------------
class ParserError : public JotdownError {
public:
    ParserError(const std::string& message, const Location& location)
    : JotdownError(format_message(message, location)),
    _location(location) { }

    const Location& location() const {
        return _location;
    }

private:
    static std::string format_message(const std::string& message,
                                      const Location& loc) {
        return tfm::format("%s (at %s line %d col %d)",
                           message,
                           loc.filename, loc.line, loc.col);
    }

    const Location _location;
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

private:
    std::vector<obj_t> objects;
};

// ------------------------------------------------------------------
class TypeSelector : public Selector {
public:
    TypeSelector(Type type) : type(type) { }

    Selector* clone() const {
        return new TypeSelector(type);
    }

    bool choose(obj_t obj) const {
        return obj->type() == type;
    }

private:
    Type type;
};

// ------------------------------------------------------------------
class RecursiveSelector : public Selector {
public:
    RecursiveSelector(const Selector* query) : query(query) { };

    Selector* clone() const {
        return new RecursiveSelector(query->clone());
    }

    std::vector<obj_t> select(const std::vector<obj_t>& objects) const {
        auto results = query->select(objects);
        if (results.size() > 0) {
            auto sub_results = select(results);
            std::copy(sub_results.begin(), sub_results.end(), results.begin());
        }
        return results;
    }

private:
    std::unique_ptr<const Selector> query;
};

// ------------------------------------------------------------------
class LevelSelector : public Selector {
public:
    LevelSelector(int level) : level(level) { }

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

private:
    int level;
};

// ------------------------------------------------------------------
class RegexSelector : public Selector {
public:
    RegexSelector(const std::string& rx) : rx(rx) { }
    RegexSelector(const std::regex& rx) : rx(rx) { }

    Selector* clone() const {
        return new RegexSelector(rx);
    }

    bool choose(obj_t obj) const {
        if (obj->has_label()) {
            return std::regex_search(obj->label()->to_search_string(), rx);
        } else {
            return std::regex_search(obj->to_search_string(), rx);
        }
    }

private:
    std::regex rx;
};

// ------------------------------------------------------------------
class ContentsSubSelector : public Selector {
public:
    ContentsSubSelector(const Selector* query) : query(query) { }

    bool choose(obj_t obj) const {
        return obj->is_container() && query->select(
            std::static_pointer_cast<object::Container>(obj)->contents()
        ).size() > 0;
    }

private:
    std::unique_ptr<const Selector> query;
};

// ------------------------------------------------------------------
class SectionSelector : public Selector {
public:
    SectionSelector(int level = -1) : level(level) { }

    bool choose(obj_t obj) const {
        if (obj->type() == Type::SECTION) {
            auto section = std::static_pointer_cast<object::Section>(obj);
            return level == -1 || section->level() == level;
        }
        return false;
    }

private:
    int level;
};

// ------------------------------------------------------------------
class StatusSelector : public Selector {
public:
    StatusSelector(const std::string& status = "") : status(status) { }

    bool choose(obj_t obj) const {
        if (obj->type() == Type::UNORDERED_LIST_ITEM || obj->type() == Type::ORDERED_LIST_ITEM) {
            auto list_item = std::static_pointer_cast<object::ListItem>(obj);
            return (status.size() == 0 && list_item->status().size() > 0) ||
                   (list_item->status() == status);
        }
        return false;
    }

private:
    std::string status;
};

// ------------------------------------------------------------------
class OrSelector : public Selector {
public:
    OrSelector(const Selector* lhs, const Selector* rhs)
    : lhs(lhs), rhs(rhs) { }

    bool choose(obj_t obj) const {
        return lhs->choose(obj) || rhs->choose(obj);
    }

    std::unique_ptr<const Selector> lhs, rhs;
};

// ------------------------------------------------------------------
class AllSubSelector : public Selector {
public:
    AllSubSelector(const Selector* query) : query(query) { }

    Selector* clone() const {
        return new AllSubSelector(query->clone());
    }

    bool choose(obj_t obj) const {
        return obj->is_container() && query->select(
            std::static_pointer_cast<object::Container>(obj)->contents()
        ).size() == std::static_pointer_cast<object::Container>(obj)->contents().size();
    }

private:
    std::unique_ptr<const Selector> query;
};

// ------------------------------------------------------------------
class NoneSubSelector : public Selector {
public:
    NoneSubSelector(const Selector* query) : query(query) { }

    Selector* clone() const {
        return new NoneSubSelector(query->clone());
    }

    bool choose(obj_t obj) const {
        return obj->is_container() && query->select(
            std::static_pointer_cast<object::Container>(obj)->contents()
        ).size() == 0;
    }

private:
    std::unique_ptr<const Selector> query;
};

// ------------------------------------------------------------------
class NotSelector : public Selector {
public:
    NotSelector(const Selector* query) : query(query) { };

    Selector* clone() const {
        return new NotSelector(query->clone());
    }

    std::vector<obj_t> select(const std::vector<obj_t>& objects) const {
        std::unordered_set<obj_t> anti_result_set;
        for (auto obj : query->select(objects)) {
            anti_result_set.insert(obj);
        }
        std::vector<obj_t> results;
        std::copy_if(objects.begin(), objects.end(), results.begin(), [&](auto obj) {
            return anti_result_set.find(obj) == anti_result_set.end();
        });
        return results;
    }

private:
    std::unique_ptr<const Selector> query;
};

// ------------------------------------------------------------------
class ParentSelector : public Selector {
public:
    Selector* clone() const {
        return new ParentSelector();
    }

    std::vector<obj_t> select(const std::vector<obj_t>& objects) const {
        std::unordered_set<obj_t> result_set;
        for (auto obj : objects) {
            if (obj->has_parent()) {
                result_set.insert(obj->parent());
            }
        }
        std::vector<obj_t> results;
        results.insert(results.begin(), result_set.begin(), result_set.end());
        return results;
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

    Selector* clone() const {
        return new Query(*this);
    }

    Query by(const Query& query) {
        return _extend(query.clone());
    }

    Query or_by(const Query& query) {
        return Query(new OrSelector(clone(), query.clone()));
    }

    static Query type(Type type) {
        return Query(new TypeSelector(type));
    }

    static Query regex(const std::string& rx) {
        return Query(new RegexSelector(rx));
    }

    static Query level(int level) {
        return Query(new LevelSelector(level));
    }

    static Query parents() {
        return Query(new ParentSelector());
    }

    static Query contents(const Query& content_query) {
        return Query(new ContentsSubSelector(content_query.clone()));
    }

    static Query all(const Query& query) {
        return Query(new AllSubSelector(query.clone()));
    }

    static Query none(const Query& query) {
        return Query(new NoneSubSelector(query.clone()));
    }

    static Query recursive(const Query& query) {
        return Query(new RecursiveSelector(query.clone()));
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

private:
    Query _extend(const Selector* query) const {
        Query q = *this;
        q.sequence.emplace_back(query);
        return q;
    }

    std::vector<std::unique_ptr<const Selector>> sequence;
};

// ------------------------------------------------------------------
struct Context {
    moonlight::file::BufferedInput input;
    Query query;
    std::optional<std::function<Query(const Query&)>> transformer;

    Location location() const {
        return (Location){
            .line = input.line(),
            .col = input.col()
        };
    }
};

// ------------------------------------------------------------------
class ParseBegin;
class ParseState : public moonlight::automata::State<Context> {
protected:
    ParserError unexpected_char() {
        return ParserError(tfm::format(
            "Unexpected character: '%c'", context().input.peek()
        ), context().location());
    }
};

// ------------------------------------------------------------------
class PostByContentsSubExpression : public ParseState {
    PostByContentsSubExpression(Query& query, const Query& sub_query)
    : query(query), sub_query(sub_query) { }

    void run() {
        query = query.by(Query::contents(sub_query));
    }

    Query& query;
    const Query& sub_query;
};

// ------------------------------------------------------------------
class SetupByContentsSubExpression : public ParseState {
public:
    SetupByContentsSubExpression(Query& query) : query(query) { }

    void run() {
        context().transformer = Query::contents;
        push<PostByContentsSubExpression>(query, sub_query);
        push<ParseBegin>(sub_query);
    }

    Query& query;
    Query sub_query;
};

// ------------------------------------------------------------------
class ParseBegin : public ParseState {
public:
    ParseBegin(Query& query) : query(query) { };

    void run() {
        int c = context().input.peek();

        if (c == '<') {
            context().input.advance();
            push<SetupByContentsSubExpression>(query);
            return;

        } else if (c == '>') {
            if (parent() == nullptr) {
                throw unexpected_char();
            }
            pop();
            return;
        }

        if (scan("anchor", "&")) {
            query = query.by(Query::type(Type::ANCHOR));
            return;
        }

        if (scan("code", "c")) {
            query = query.by(Query::type(Type::CODE));
            return;
        }

        if (scan("codeblock", "C")) {
            query = query.by(Query::type(Type::CODE_BLOCK));
            return;
        }

        if (scan("hashtag", "#")) {
            query = query.by(Query::type(Type::HASHTAG));
            return;
        }

        if (scan("list", "l")) {
            query = query.by(Query::type(Type::ORDERED_LIST))
            .or_by(Query::type(Type::UNORDERED_LIST));
            return;
        }

        if (scan("item", "i")) {
            query = query.by(Query::type(Type::ORDERED_LIST_ITEM))
            .or_by(Query::type(Type::UNORDERED_LIST_ITEM));
            return;
        }

        if (scan("ordered_list", "ol")) {
            query = query.by(Query::type(Type::ORDERED_LIST));
            return;
        }

        if (scan("unordered_list", "ul")) {
            query = query.by(Query::type(Type::UNORDERED_LIST));
            return;
        }

        if (scan("ref", "@")) {
            query = query.by(Query::type(Type::REF));
            return;
        }

        if (scan("section", "s")) {
            query = query.by(Query::type(Type::SECTION));
            return;
        }

        throw unexpected_char();
    }

private:
    bool scan(const std::string& long_form, const std::string& short_form) {
        return (context().input.scan_eq_advance(long_form) ||
                context().input.scan_eq_advance(short_form));
    }

    Query& query;
};

// ------------------------------------------------------------------
class Parser {
public:
    Parser(std::istream& input)
    : ctx({
        .input=moonlight::file::BufferedInput(input, "<query>")
    }),
    machine(ParseState::Machine::init<ParseBegin>(ctx, ctx.query)) { }

private:
    Context ctx;
    ParseState::Machine machine;
};

}
}

#endif /* !__JOTDOWN_QUERY_H */
