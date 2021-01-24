/*
 * utils.h
 *
 * Author: Lain Musgrove (lain.proliant@gmail.com)
 * Date: Wednesday May 27, 2020
 *
 * Distributed under terms of the MIT license.
 */

#ifndef __JOTDOWN_UTILS_H
#define __JOTDOWN_UTILS_H

#include <string>
#include <vector>
#include <map>
#include <sstream>
#include <memory>

#include "tinyformat/tinyformat.h"

namespace jotdown {

// ------------------------------------------------------------------
inline std::string make_search_string(const std::string& src) {
    std::string str;

    for (char c : src) {
        if (isspace(c)) {
            if (str.size() == 0 || !isspace(str.back())) {
                str.push_back(' ');
            }
        } else {
            str.push_back(c);
        }
    }

    return str;
}

// ------------------------------------------------------------------
inline std::string strescape(const std::string& str, const std::string& escapes) {
    std::ostringstream sb;
    for (auto c : str) {
        if (c == '\\' || escapes.find(c) != std::string::npos) {
            sb << '\\';
        }
        sb << c;
    }
    return sb.str();
}

// ------------------------------------------------------------------
inline std::string strliteral(const std::string& str) {
    static const std::map<char, std::string> ESCAPE_SEQUENCES = {
        {'\a', "\\a"},
        {'\b', "\\b"},
        {'\e', "\\e"},
        {'\f', "\\f"},
        {'\n', "\\n"},
        {'\r', "\\r"},
        {'\t', "\\t"},
        {'\v', "\\v"},
        {'\\', "\\\\"},
        {'"', "\\\""}
    };

    std::stringstream sb;

    for (size_t x = 0; str[x] != '\0'; x++) {
        char c = str[x];
        std::string repr = "";

        auto iter = ESCAPE_SEQUENCES.find(c);
        if (iter != ESCAPE_SEQUENCES.end()) {
            repr = iter->second;
        }

        if (repr == "" && !isprint(c)) {
            repr = tfm::format("\\x%x", c);
        }

        if (repr != "") {
            sb << repr;
        } else {
            sb << c;
        }
    }

    return sb.str();
}

// ------------------------------------------------------------------
template<class T>
class Classifier {
public:
    typedef std::function<bool(const T&)> MatchFunction;
    typedef std::function<void()> NullaryAction;
    typedef std::function<void(const T&)> UnaryAction;

    class Action {
    public:
        Action(NullaryAction action) : _nullary(action), _unary({}) { }
        Action(UnaryAction action) : _nullary({}), _unary(action) { }

        void nullary() const {
            _nullary.value()();
        }

        void unary(const T& value) const {
            _unary.value()(value);
        }

        bool is_unary() const {
            return _unary.has_value();
        }

    private:
        std::optional<NullaryAction> _nullary;
        std::optional<UnaryAction> _unary;
    };

    class Case {
    public:
        template<class ActionImpl>
        Case(MatchFunction match, ActionImpl action) : _match(match), _action(Action(action)) { }

        bool match(const T& value) const {
            if (_match(value)) {
                if (_action.is_unary()) {
                    _action.unary(value);
                } else {
                    _action.nullary();
                }
                return true;
            }
            return false;
        }

    private:
        MatchFunction _match;
        Action _action;
    };

    class CaseAssigner {
    public:
        CaseAssigner(Classifier& switch_, MatchFunction match) : _switch(switch_), _match(match) { }

        template<class ActionType>
        void operator=(ActionType action) {
            _switch._cases.push_back(Case(_match, action));
        }

    private:
        Classifier& _switch;
        MatchFunction _match;
    };

    template<class... MatchType>
    CaseAssigner operator()(const MatchType&... matches) {
        return CaseAssigner(*this, f_or_join(matches...));
    }

    CaseAssigner otherwise() {
        return CaseAssigner(*this, [](const T& value) { (void)value; return true; });
    }

    bool match(const T& value) const {
        for (auto case_ : _cases) {
            if (case_.match(value)) {
                return true;
            }
        }
        return false;
    }

private:
    static MatchFunction f_eq(const T& value) {
        return [=](const T& x) {
            return x == value;
        };
    }

    static MatchFunction f_or(MatchFunction fA, MatchFunction fB) {
        return [=](const T& x) {
            return fA(x) || fB(x);
        };
    }

    MatchFunction f_or_join(MatchFunction f) {
        return f;
    }

    MatchFunction f_or_join(const T& value) {
        return f_eq(value);
    }

    template<class... MatchTypePack>
    MatchFunction f_or_join(MatchFunction f, MatchTypePack... pack) {
        return f_or(f, f_or_join(pack...));
    }

    template<class... MatchTypePack>
    MatchFunction f_or_join(const T& value, MatchTypePack... pack) {
        return f_or(f_eq(value), f_or_join(pack...));
    }

    std::vector<Case> _cases;
};

}

#endif /* !__JOTDOWN_UTILS_H */
