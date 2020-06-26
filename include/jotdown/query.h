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
class Selector;
typedef std::shared_ptr<const Selector> selector_t;

class Selector {
public:
    virtual ~Selector() { }

    virtual Selector* clone() const = 0;

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
};

// ------------------------------------------------------------------
class TypeSelector : public Selector {
public:
    TypeSelector(Type type = Type::NONE) : _type(type) { }

    Selector* clone() const {
        return new TypeSelector(type());
    }

    Type type() const {
        return _type;
    }

    bool any() const {
        return type() == Type::NONE;
    }

    bool choose(obj_t obj) const {
        return any() || obj->type() == type();
    }

private:
    Type _type;
};

// ------------------------------------------------------------------
class RegexSelector : public Selector {
public:
    RegexSelector(const std::string& regex) : _regex(regex) { }
    RegexSelector(const std::regex& regex) : _regex(regex) { }

    Selector* clone() const {
        return new RegexSelector(_regex);
    }

    bool choose(obj_t obj) const {
        if (obj->has_label()) {
            return std::regex_search(obj->label()->to_search_string(), regex());
        } else {
            return std::regex_search(obj->to_search_string(), regex());
        }
    }

private:
    const std::regex& regex() const {
        return _regex;
    }

    const std::regex _regex;
};

// ------------------------------------------------------------------
class OffsetSelector : public Selector {
public:
    OffsetSelector(int offset) : _offset(offset) { }

    Selector* clone() const {
        return new OffsetSelector(offset());
    }

    std::vector<obj_t> select(const std::vector<obj_t>& objects) const {
        auto result = moonlight::splice::at(objects, offset());
        if (result.has_value()) {
            return {*result};
        } else {
            return {};
        }
    }

private:
    int offset() const {
        return _offset;
    }

    int _offset;
};

// ------------------------------------------------------------------
class InverseSelector : public Selector {
public:
    InverseSelector(const Selector* base) : _base(base) { }

    Selector* clone() const {
        return new InverseSelector(base().clone());
    }

    std::vector<obj_t> select(const std::vector<obj_t>& objects) const {
        std::unordered_set<obj_t> selected_set;
        std::vector<obj_t> unselected_objects;
        auto selected_objects = base().select(objects);
        selected_set.insert(selected_objects.begin(), selected_objects.end());
        std::copy_if(objects.begin(), objects.end(), unselected_objects.begin(), [&](auto obj) {
            return selected_set.find(obj) == selected_set.end();
        });
        return unselected_objects;
    }

private:
    const Selector& base() const {
        return *_base;
    }

    std::unique_ptr<const Selector> _base;
};

// ------------------------------------------------------------------
class RecursiveSelector : public Selector {
public:
    RecursiveSelector(const Selector* base) : _base(base) { }

    RecursiveSelector* clone() const {
        return new RecursiveSelector(base().clone());
    }

    std::vector<obj_t> select(const std::vector<obj_t>& objects) const {
        std::vector<obj_t> results;

        for (auto obj : objects) {
            if (choose(obj)) {
                results.push_back(obj);
            }

            if (obj->has_label()) {
                auto sub_results = select(obj->label()->contents());
                results.insert(results.end(), sub_results.begin(), sub_results.end());
            }

            if (obj->is_container()) {
                auto container = std::static_pointer_cast<object::Container>(obj);
                auto sub_results = select(container->contents());
                results.insert(results.end(), sub_results.begin(), sub_results.end());
            }
        }

        return results;
    }

    bool choose(obj_t obj) const {
        return _base->choose(obj);
    }

private:
    const Selector& base() const {
        return *_base;
    }

    std::unique_ptr<const Selector> _base;
};

// ------------------------------------------------------------------
class LabelSelector : public Selector {
public:
    Selector* clone() const {
        return new LabelSelector();
    }

    std::vector<obj_t> select(const std::vector<obj_t>& objects) const {
        std::vector<obj_t> results;

        for (auto obj : objects) {
            if (obj->has_label()) {
                results.push_back(obj->label());
            }
        }

        return results;
    }
};

// ------------------------------------------------------------------
class ContentsSelector : public Selector {
public:
    Selector* clone() const {
        return new ContentsSelector();
    }

    std::vector<obj_t> select(const std::vector<obj_t>& objects) const {
        std::vector<obj_t> results;
        for (auto obj : objects) {
            if (obj->is_container()) {
                auto container = std::static_pointer_cast<object::Container>(obj);
                results.insert(results.begin(), container->begin(), container->end());
            }
        }
        return results;
    }
};

// ------------------------------------------------------------------
class Query : public Selector {
public:
    Query() { }
    Query(const std::vector<selector_t>& selectors) : _selectors(selectors) { }

    Selector* clone() const {
        return new Query(selectors());
    }

    std::vector<obj_t> select(const std::vector<obj_t>& objects) const {
        return cycle(objects);
    }

private:
    std::vector<obj_t> cycle(const std::vector<obj_t>& initial_objects) const {
        std::vector<obj_t> objects = initial_objects;
        std::vector<selector_t> stack;
        std::reverse_copy(selectors().begin(), selectors().end(), stack.begin());

        while (stack.size() > 0 && objects.size() > 0) {
            auto selector = stack.back();
            stack.pop_back();
            objects = selector->select(objects);
        }

        return objects;
    }

private:
    const std::vector<selector_t>& selectors() const {
        return _selectors;
    }

    std::vector<selector_t> _selectors;
};

// ------------------------------------------------------------------
class ContentsQuerySelector : public Selector {
public:
    ContentsQuerySelector(std::shared_ptr<const Selector> query) : query(query) { }

    bool choose(obj_t obj) const {
        if (obj->is_container()) {
            auto container = std::static_pointer_cast<object::Container>(obj);
            return query->select(container->contents()).size() > 0;
        }
        return false;
    }

private:
    std::shared_ptr<const Selector> query;
};


// ------------------------------------------------------------------
struct Context {
    moonlight::file::BufferedInput input;
    Query query;
    bool next_recursive = false;
};

// ------------------------------------------------------------------
typedef moonlight::automata::State<Context> ParseState;

// ------------------------------------------------------------------
class ParseBegin : public ParseState {
    void run() {

    }
};

// ------------------------------------------------------------------
class Parser {
public:
    Parser(std::istream& input)
    : ctx({
        .input=moonlight::file::BufferedInput(input, "<query>")
    }),
    machine(ParseState::Machine::init<ParseBegin>(ctx)) { }

private:
    Context ctx;
    ParseState::Machine machine;
};

}
}

#endif /* !__JOTDOWN_QUERY_H */
