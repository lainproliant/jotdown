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

#include "moonlight/generator.h"
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

typedef moonlight::gen::Stream<obj_t> ResultStream;
typedef std::vector<obj_t> ObjectList;
typedef std::set<obj_t> ObjectSet;

namespace gen = moonlight::gen;
namespace str = moonlight::str;

class Query;
typedef std::function<Query(const ResultStream&)> QueryFactory;

class Query {
    Query()
    : _results(ResultStream::empty()) { }

    Query(const std::vector<obj_t>& objects)
    : _results(gen::wrap(objects.begin(), objects.end())) { }

    Query(const ResultStream& results)
    : _results(results) { }

    static Query parse(const std::string& expr) {
        Query query;
        moonlight::Classifier<std::string> classify;

        auto rx = [](const std::string& rx_str) {
            std::regex rx(rx_str);
            return [rx](const std::string& s) {
                return std::regex_match(s, rx);
            };
        };

        classify(rx("=.*")) = [&](const auto& s) {
            query = query.rx_match(moonlight::slice(s, 1, {}));
        };

        classify(rx("~.*")) = [&](const auto& s) {
            query = query.rx_search(moonlight::slice(s, 1, {}));
        };

        classify(rx("level.+")) = [&](const auto& s) {
            auto expr = moonlight::slice(s, 5, {});
            if (expr.starts_with("=")) {
                query = query.level(std::stoi(moonlight::slice(s, 1, {})));
            } else if (expr.starts_with(">=")) {
                query = query.level(std::stoi(moonlight::slice(s, 2, {})),
                                    [](auto a, auto b) { return a >= b; });
            } else if (expr.starts_with("<=")) {
                query = query.level(std::stoi(moonlight::slice(s, 2, {})),
                                    [](auto a, auto b) { return a <= b; });
            } else {
                THROW(QueryError, "Invalid level expression: " + s);
            }
        };

        const auto type_names = gen::stream(Object::type_names_map()).transform<std::string>([](const auto& pair) {
            return pair.first;
        }).collect();

        classify(rx(str::join(moonlight::slice(moonlight::maps::values(Object::type_names_map()).collect(), 1, -1), "|"))) = [](const std::string& s) {

        };

        auto subexprs = str::split(expr, '/');

        return query;
    }

    ResultStream results() const {
        return _results;
    }

    Query types(const std::set<Object::Type>& types) const {
        return results().filter([types](obj_t obj) {
            return types.find(obj->type()) != types.end();
        });
    }

    Query type(Object::Type type) const {
        return types({type});
    }

    Query slice(std::optional<int> begin = {}, std::optional<int> end = {}) const {
        try {
            return gen::stream(moonlight::slice(results().collect(), begin, end));

        } catch (const moonlight::core::IndexError& e) {
            return ResultStream::empty();
        }
    }

    Query offset(int offset) const {
        return slice(offset);
    }

    Query label() const {
        return results().transform<obj_t>([](obj_t obj) -> std::optional<obj_t> {
            if (obj->has_label()) {
                return obj->label();
            }
            return {};
        }).unique();
    }

    Query parent() const {
        return results().transform<obj_t>([](obj_t obj) -> std::optional<obj_t> {
            if (obj->has_parent()) {
                return obj->parent();
            }
            return {};
        }).unique();
    }

    Query children(bool include_labels = false) const {
        return gen::merge(results().transform_split<obj_t>([include_labels](obj_t obj) -> ResultStream {
            std::vector<ResultStream> iterables;

            if (include_labels && obj->has_label()) {
                iterables.push_back(ResultStream::singleton(obj->label()));
            }

            if (obj->is_container()) {
                container_t container = static_pointer_cast<Container>(obj);
                iterables.push_back(gen::wrap(
                    container->contents().begin(),
                    container->contents().end()
                ));
            }

            return gen::merge(gen::stream(iterables));
        }));
    }

    Query decendant(bool include_labels = false) {
        std::vector<ResultStream> result_streams;
        Query child_query = children(include_labels);
        Query recursive_query;

        if (! child_query.results().is_empty()) {
            recursive_query = children(include_labels).decendant(include_labels);
        }

        return ResultStream::merge(std::vector<ResultStream>{
            child_query.results(),
            recursive_query.results()
        });
    }

    Query antecedent() {
        std::vector<ResultStream> result_streams;
        Query parent_query = parent();
        Query recursive_query;

        if (! parent_query.results().is_empty()) {
            recursive_query = parent().antecedent();
        }

        return gen::merge(parent_query.results(),
                          recursive_query.results()).unique();
    }

    Query level(int level, std::function<bool(const int& a, const int& b)> cmp = std::equal_to<int>()) {
        return results().filter([level, cmp](obj_t obj) {
            auto obj_level = level_of(obj);
            return obj_level.has_value() && cmp(*obj_level, level);
        });
    }

    Query rx_search(const std::regex& rx, bool focus_label = true) {
        return results().filter([rx, focus_label](obj_t obj) {
            if (focus_label && obj->has_label()) {
                return std::regex_search(obj->label()->to_search_string(), rx);
            }

            return std::regex_search(obj->to_search_string(), rx);
        });
    }

    Query rx_search(const std::string& rx) {
        return rx_search(std::regex(rx));
    }

    Query rx_match(const std::regex& rx, bool focus_label = true) {
        return results().filter([rx, focus_label](obj_t obj) {
            if (focus_label && obj->has_label()) {
                return std::regex_match(str::trim(obj->label()->to_search_string()), rx);
            }

            return std::regex_match(str::trim(obj->to_search_string()), rx);
        });
    }

    Query hashtag(const std::string& tag = "") {
        return type(Object::Type::HASHTAG).results().filter([tag](obj_t obj) {
            auto hashtag = std::static_pointer_cast<Hashtag>(obj);
            return tag == "" || str::to_lower(hashtag->tag()) == str::to_lower(tag);
        });
    }

    Query list() const {
        return types({Object::Type::ORDERED_LIST, Object::Type::UNORDERED_LIST});
    }

    Query list_item() const {
        return types({Object::Type::ORDERED_LIST_ITEM, Object::Type::UNORDERED_LIST_ITEM});
    }

    Query checklist() const {
        return Query(list().children().results().filter([](obj_t obj) {
            auto list_item = std::static_pointer_cast<ListItem>(obj);
            return list_item->status() != "";
        })).parent().results().unique();
    }

    Query anchor(const std::string& name = "") {
        return type(Object::Type::ANCHOR).results().transform([name](obj_t obj) -> std::optional<obj_t> {
            if (name == "") {
                return obj;
            }
            auto anchor = std::static_pointer_cast<Anchor>(obj);
            if (anchor->name() == name) {
                return obj;
            }
            return {};
        });
    }

    Query reference(const std::string& url_substr = "") {
        return type(Object::Type::REF).results().transform([url_substr](obj_t obj) -> std::optional<obj_t> {
            if (url_substr == "") {
                return obj;
            }
            auto ref = std::static_pointer_cast<Ref>(obj);
            if (ref->link().find(url_substr) != std::string::npos) {
                return obj;
            }
            return {};
        });
    }

    Query not_in(const QueryFactory& qf) {
        return ResultStream::lazy([qf, this]() {
            auto q = qf(results());
            std::set<obj_t> other_set(q.results().begin(), q.results().end());
            return results().filter([other_set](obj_t obj) {
                return other_set.find(obj) == other_set.end();
            });
        });
    }

    Query or_(const QueryFactory& qfA, const QueryFactory& qfB) {
        return ResultStream::lazy([qfA, qfB, this]() {
            return gen::merge(qfA(results()).results(),
                              qfB(results()).results()).unique();
        });
    }

private:
    static std::optional<int> level_of(obj_t obj) {
        if (obj->is_list_item()) {
            auto list_item = std::static_pointer_cast<ListItem>(obj);
            return list_item->level();
        }

        if (obj->is(Object::Type::SECTION)) {
            auto section = std::static_pointer_cast<Section>(obj);
            return section->level();
        }

        return {};
    }

    ResultStream _results;
};

}

#endif /* !__JOTDOWN_QUERY_H */
