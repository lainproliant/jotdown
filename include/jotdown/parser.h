/*
 * parser.h
 *
 * Author: Lain Musgrove (lain.proliant@gmail.com)
 * Date: Tuesday May 26, 2020
 *
 * Distributed under terms of the MIT license.
 */

#ifndef __JOTDOWN_PARSER_H
#define __JOTDOWN_PARSER_H

#include "moonlight/generator.h"
#include "jotdown/utils.h"
#include "jotdown/interfaces.h"
#include "jotdown/error.h"
#include "tinyformat/tinyformat.h"

#ifdef MOONLIGHT_AUTOMATA_DEBUG
#include "moonlight/collect.h"
#endif

#include <istream>

namespace jotdown {
namespace parser {

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
class Token {
public:
    enum class Type {
        NONE,
        ANCHOR,
        CODE,
        CODE_BLOCK,
        END,
        ERROR,
        FRONT_MATTER,
        HASHTAG,
        HEADER_END,
        HEADER_START,
        LIST_ITEM_END,
        NEWLINE,
        OL_ITEM,
        REF,
        INDEX,
        STATUS,
        TEXT,
        UL_ITEM,
        NUM_TYPES
    };

    Token(Type type, const std::string& content = "")
    : _type(type), _content(content) { }

    virtual ~Token() { }

    static const std::string& type_name(Type type) {
        static const std::string names[] = {
            "NONE",
            "ANCHOR",
            "CODE",
            "CODE_BLOCK",
            "END",
            "ERROR",
            "FRONT_MATTER",
            "HASHTAG",
            "HEADER_END",
            "HEADER_START",
            "LIST_ITEM_END",
            "NEWLINE",
            "OL_ITEM",
            "REF",
            "INDEX",
            "STATUS",
            "TEXT",
            "UL_ITEM",
            "NUM_TYPES"
        };

        return names[(unsigned int)type];
    }

    std::string repr() const {
        if (content().size() == 0) {
            return tfm::format("%s", type_name(type()));

        } else {
            return tfm::format("%-16s '%s'",
                               type_name(type()), strliteral(content()));
        }
    }

    Type type() const {
        return _type;
    }

    const std::string& content() const {
        return _content;
    }

    const Location& begin() const {
        return _range.begin;
    }

    const Location& end() const {
        return _range.end;
    }

    const Range& range() const {
        return _range;
    }

    void range(const Range& range) {
        _range = range;
    }

    void begin(const Location& location) {
        _range.begin = location;
    }

    void end(const Location& location) {
        _range.end = location;
    }

private:
    const Type _type;
    const std::string _content;
    Range _range = {NOWHERE, NOWHERE};
};
typedef std::shared_ptr<Token> token_t;

// ------------------------------------------------------------------
class EmbeddedDocumentToken : public Token {
public:
    EmbeddedDocumentToken(Token::Type type, const std::string& langspec, const std::string& code)
    : Token(type, code), _langspec(moonlight::str::trim(langspec)) { }

    const std::string& langspec() const {
        return _langspec;
    }

private:
    const std::string _langspec;
};

// ------------------------------------------------------------------
class HeaderStartToken : public Token {
public:
    HeaderStartToken(int level)
    : Token(Type::HEADER_START, ""), _level(level) { }

    int level() const {
        return _level;
    }

    std::string repr() const {
        return tfm::format("%s[%d]", type_name(type()), _level);
    }

private:
    const int _level;
};

// ------------------------------------------------------------------
class RefToken : public Token {
public:
    RefToken(const std::string& link,
             const std::string& text = "",
             const std::string& index_name = "")
    : Token(Type::REF, link), _text(text.size() > 0 ? text : link), _index_name(index_name) { }

    const std::string& link() const {
        return content();
    }

    const std::string& text() const {
        return _text;
    }

    const std::string& index_name() const {
        return _index_name;
    }

    std::string repr() const {
        if (_text != content()) {
            return tfm::format("%-16s %s (%s)", type_name(type()), content(), _text);
        } else {
            return tfm::format("%-16s %s", type_name(type()), content());
        }
    }

private:
    const std::string _text;
    const std::string _index_name;
};

// ------------------------------------------------------------------
class IndexToken : public Token {
public:
    IndexToken(const std::string& name, const std::string& link)
    : Token(Type::INDEX, link), _name(name), _link(link) { }

    const std::string& name() const {
        return _name;
    }

    const std::string& link() const {
        return _link;
    }

    std::string repr() const {
        return tfm::format("%-16s %s => %s", type_name(type()), name(), link());
    }

private:
    std::string _name;
    std::string _link;
};

// ------------------------------------------------------------------
class ListItemToken : public Token {
public:
    ListItemToken(Type type, int level, const std::string& content = "")
    : Token(type, content), _level(level) { }

    int level() const {
        return _level;
    }

private:
    const int _level;
};

// ------------------------------------------------------------------
class UnorderedListItemToken : public ListItemToken {
public:
    UnorderedListItemToken(int level)
    : ListItemToken(Type::UL_ITEM, level, "- ") { }

    std::string repr() const {
        return tfm::format("%s[%d]", type_name(type()), level());
    }
};

// ------------------------------------------------------------------
class OrderedListItemToken : public ListItemToken {
public:
    OrderedListItemToken(int level, const std::string& ordinal)
    : ListItemToken(Type::OL_ITEM, level, ordinal) { }

    const std::string& ordinal() const {
        return content();
    }

    std::string repr() const {
        return tfm::format("%-16s %s", tfm::format("%s[%d]", type_name(type()), level()), ordinal());
    }
};

// ------------------------------------------------------------------
struct Context {
    moonlight::file::BufferedInput input;
    std::deque<token_t> tokens;
    int tokens_parsed = 0;

    token_t push_token(Token::Type type, const std::string& content = "") {
        auto tk = std::make_shared<Token>(type, content);
        return push_token(tk);
    }

    token_t push_token(token_t tk) {
        tokens.push_back(tk);
        tokens_parsed ++;
        return tk;
    }

    token_t push_token(Token* token) {
        auto tk = std::shared_ptr<Token>(token);
        return push_token(tk);
    }

    std::optional<token_t> pop_token() {
        if (tokens.size() > 0) {
            token_t tk = tokens.front();
            tokens.pop_front();
            return tk;
        } else {
            return {};
        }
    }

    Location location() const {
        return {
            .filename = input.name(),
            .line = input.line(),
            .col = input.col()
        };
    }

    token_t last_token() const {
        if (tokens.size() > 0) {
            return tokens[tokens.size()-1];
        } else {
            return nullptr;
        }
    }

    std::string dbg_cursor() {
        std::string cursor;
        for (int x = 1; x <= 70 && input.peek(x) != EOF; x++) {
            cursor.push_back(input.peek(x));
        }
        return tfm::format("\"%s\"", strliteral(cursor));
    }
};

// ------------------------------------------------------------------
class ParseState : public moonlight::automata::State<Context> {
protected:
    void error(const std::string& message, const Location& begin) {
        auto tk = context().push_token(Token::Type::ERROR, message);
        tk->begin(begin);
        tk->end(context().location());
    }

    void error(const std::string& message) {
        auto tk = context().push_token(Token::Type::ERROR, message);
        tk->begin(context().location());
        tk->end(context().location());
    }

    int scan_para_break() {
        for (int y = 1;; y++) {
            int c = context().input.peek(y);
            if (c == EOF || c == '\n') {
                return y;
            } else if (! isspace(c)) {
                return 0;
            }
        }
    }

    int scan_indent() {
        int y = 1;

        for (;; y++) {
            int c = context().input.peek(y);
            if (!isspace(c) || c == '\n') {
                break;
            }
        }

        return y;
    }

    int scan_end_of_line() {
        int c = context().input.peek();
        return c == '\n' || c == EOF;
    }

    int scan_ordered_list(std::string* ord_out = nullptr) {
        int indent = scan_indent();
        int c = context().input.peek(indent);

        // Did we find an alphanumeric?
        if (isalnum(c)) {
            // Scan forward until we find a period.  If a space follows, we've found
            // an unordered list.
            for (int y = indent;; y++) {
                c = context().input.peek(y);
                if (c == '\n' || c == EOF) {
                    break;

                } else if (! isalnum(c) && c != '.') {
                    break;

                } else if (c == '.') {
                    int c2 = context().input.peek(y+1);
                    if (isspace(c2) && c2 != '\n') {
                        return indent;
                    } else {
                        break;
                    }
                } else if (ord_out != nullptr) {
                    ord_out->push_back(c);
                }
            }
        }

        return 0;
    }

    int scan_unordered_list() {
        int indent = scan_indent();
        int c = context().input.peek(indent);

        // Is it a dash?
        if (c == '-') {
            // Is the following character a non-newline space?
            int c2 = context().input.peek(indent+1);
            if (isspace(c2) && c2 != '\n') {
                return indent;
            }
        }

        return 0;
    }

    bool scan_status(std::string* status_out = nullptr) {
        int c = context().input.peek();

        if (c == '[') {
            for (int y = 2;; y++) {
                int c2 = context().input.peek(y);
                if (c2 == ']') {
                    return y > 2;
                } else if (c2 == EOF || c2 == '\n') {
                    break;
                } else if (status_out != nullptr) {
                    status_out->push_back(c2);
                }
            }
        }

        return false;
    }

    bool scan_wraplike(char start, char end) {
        return (context().input.peek() == start &&
                context().input.scan_line_eq(moonlight::str::chr(end), 1, "\\"));

    }
};

// ------------------------------------------------------------------
class ParseEmbeddedDocument : public ParseState {
public:
    ParseEmbeddedDocument(Token::Type target_type, const std::string& terminator)
    : _target_type(target_type), _terminator(terminator) {
        _tracer_name = tfm::format("ParseEmbeddedDocument<%s>", Token::type_name(_target_type));
    }

    const char* tracer_name() const {
        return _tracer_name.c_str();
    }

    void run() {
        Location begin = context().location();

        // Skip the terminator, assuming it is also the demarcator.
        context().input.advance(_terminator.size());

        // Scan until a newline to pull the langspec.
        std::string langspec;
        for (;;) {
            int c = context().input.getc();
            if (c == '\n') {
                break;
            }
            langspec.push_back(c);
        }

        langspec = moonlight::str::trim(langspec);

        std::string code;
        bool newline = true;
        for (;;) {
            if (newline && context().input.scan_eq(_terminator)) {
                context().input.advance(_terminator.size() + 1);
                break;
            }
            int c = context().input.getc();
            if (c == '\n') {
                newline = true;
            } else if (c == EOF) {
                error("Unexpected end of file while parsing embedded document.");
                pop();
                return;

            } else {
                newline = false;
            }
            code.push_back(c);
        }

        auto tk = context().push_token(new EmbeddedDocumentToken(_target_type, langspec, code));
        tk->begin(begin);
        tk->end(context().location());
        pop();
    }

private:
    Token::Type _target_type;
    std::string _terminator;
    std::string _tracer_name;
};

// ------------------------------------------------------------------
class ParseCode : public ParseState {
    const char* tracer_name() const {
        return "Code";
    }

    void run() {
        Location begin = context().location();
        std::string code;

        for (;;) {
            int c = context().input.peek();

            if (c == '`') {
                context().input.advance();
                break;

            } else if (c == '\\') {
                int c2 = context().input.peek(2);
                if (c2 == '\\' || c2 == '`') {
                    code.push_back(c2);
                    context().input.advance(2);
                } else {
                    code.push_back(c);
                    context().input.advance();
                }

            } else if (c == EOF) {
                break;

            } else {
                context().input.advance();
                code.push_back(c);
            }
        }

        auto tk = context().push_token(Token::Type::CODE, code);
        tk->begin(begin);
        tk->end(context().location());
        pop();
    }
};

// ------------------------------------------------------------------
class ParseIndexedLink : public ParseState {
public:
    ParseIndexedLink(const Location& begin, const
                                   std::string& text)
    : begin(begin), text(text) { }

    const char* tracer_name() const {
        return "IndexedLink";
    }

    void run() {
        std::string index_name;

        for (;;) {
            int c = context().input.peek();

            if (c == '\n') {
                context().input.advance();

            } else if (c == EOF) {
                error("Unexpected end of file while parsing indexed link.");
                pop();
                return;
            }
            else if (c == '\\') {
                int c2 = context().input.peek(2);
                if (c2 == ']') {
                    index_name.push_back(c2);
                    context().input.advance(2);
                }

            } else if (c == ']') {
                context().input.advance();
                break;

            } else {
                context().input.advance();
                index_name.push_back(c);
            }
        }

        auto tk = context().push_token(new RefToken("", text, index_name));
        tk->begin(begin);
        tk->end(context().location());
        pop();
    }

private:
    Location begin;
    std::string text;
};

// ------------------------------------------------------------------
class ParseLinkIndex : public ParseState {
public:
    ParseLinkIndex(const Location& begin, const std::string& index_name)
    : begin(begin), index_name(index_name) { }

    const char* tracer_name() const {
        return "LinkIndex";
    }

    void run() {
        std::string link;

        // Skip leading spaces.
        while (isspace(context().input.peek())) {
            context().input.advance();
        }

        while (! scan_end_of_line()) {
            link.push_back(context().input.getc());
        }

        auto tk = context().push_token(new IndexToken(index_name, link));
        tk->begin(begin);
        tk->end(context().location());
        pop();
    }

private:
    Location begin;
    std::string index_name;
};

// ------------------------------------------------------------------
class ParseLink : public ParseState {
public:
    ParseLink(const Location& begin) : begin(begin) { }

    const char* tracer_name() const {
        return "Link";
    }

    void run() {
        std::string text;

        for (;;) {
            int c = context().input.peek();

            if (c == '\n') {
                context().input.advance();

            } else if (c == EOF) {
                error("Unexpected end of file while parsing link or link index.");
                pop();
                return;

            } else if (c == '\\') {
                int c2 = context().input.peek(2);
                if (c2 == ']') {
                    text.push_back(c2);
                    context().input.advance(2);
                }

            } else if (c == ']') {
                context().input.advance();
                break;

            } else {
                context().input.advance();
                text.push_back(c);
            }
        }

        int c = context().input.peek();
        if (c == '[' && context().input.scan_line_eq("]", 1, "\\")) {
            context().input.advance();
            transition<ParseIndexedLink>(begin, text);

        } else if (c == ':') {
            context().input.advance();
            transition<ParseLinkIndex>(begin, text);

        } else if (scan_wraplike('(', ')')) {
            context().input.advance();
            std::string link;

            for (;;) {
                int c = context().input.getc();
                if (c == ')') {
                    break;
                } else if (c == '\\') {
                    int c2 = context().input.peek();
                    if (c2 == '\\' || c2 == ')') {
                        context().input.advance();
                        link.push_back(c2);
                        continue;
                    }
                }
                link.push_back(c);
            }

            auto tk = context().push_token(new RefToken(link, text));
            tk->begin(begin);
            tk->end(context().location());
            pop();

        } else {
            auto tk = context().push_token(new RefToken(text));
            tk->begin(begin);
            tk->end(context().location());
            pop();
        }
    }

private:
    Location begin;
};

// ------------------------------------------------------------------
class ParseSimpleLink : public ParseState {
public:
    ParseSimpleLink(const Location& begin) : begin(begin) { }

    const char* tracer_name() const {
        return "SimpleLink";
    }

    void run() {
        std::string link;

        for (;;) {
            int c = context().input.peek();

            if (c == '\n') {
                context().input.advance();

            } else if (c == EOF) {
                error("Unexpected end of file while parsing simple link.");
                pop();
                return;

            } else if (c == '\\') {
                int c2 = context().input.peek(2);
                if (c2 == '>' || c2 == '\\') {
                    link.push_back(c2);
                    context().input.advance(2);
                }

            } else if (c == '>') {
                context().input.advance();
                break;

            } else {
                context().input.advance();
                link.push_back(c);
            }
        }

        auto tk = context().push_token(new RefToken(link));
        tk->begin(begin);
        tk->end(context().location());
        pop();
    }

private:
    Location begin;
};

// ------------------------------------------------------------------
class CategorizeLink : public ParseState {
public:
    const char* tracer_name() const {
        return "CategorizeLink";
    }

    void run() {
        Location begin = context().location();
        int c = context().input.getc();

        if (c == '<' && context().input.scan_line_eq(">", 1, "\\")) {
            transition<ParseSimpleLink>(begin);

        } else if (c == '[' && context().input.scan_line_eq("]", 1, "\\")) {
            transition<ParseLink>(begin);

        } else {
            error(tfm::format("Unexpected character '%c' while parsing link.", c));
            pop();
            return;
        }
    }
};

// ------------------------------------------------------------------
class ParseHashtag : public ParseState {
    const char* tracer_name() const {
        return "Hashtag";
    }

    void run() {
        Location begin = context().location();

        int c = context().input.getc();

        if (c != '#') {
            error(tfm::format("Unexpected character '%c' while parsing hashtag.", c));
            pop();
            return;
        }

        std::string hash;

        for (;;) {
            c = context().input.peek();

            if (isspace(c) || c == EOF) {
                break;
            } else if (ispunct(c)) {
                int c2 = context().input.peek(2);
                if (isspace(c2) || c2 == EOF) {
                    break;
                }
            }

            hash.push_back(c);
            context().input.advance();
        }

        auto tk = context().push_token(Token::Type::HASHTAG, hash);
        tk->begin(begin);
        tk->end(context().location());
        pop();
    }
};

// ------------------------------------------------------------------
class ParseAnchor : public ParseState {
    const char* tracer_name() const {
        return "Anchor";
    }

    void run() {
        Location begin = context().location();
        std::string anchor;

        for (;;) {
            int c = context().input.peek();

            if (isspace(c) || c == EOF) {
                break;
            } else if (ispunct(c)) {
                int c2 = context().input.peek(2);
                if (isspace(c2) || c2 == EOF) {
                    break;
                }
            }

            anchor.push_back(c);
            context().input.advance();
        }

        auto tk = context().push_token(Token::Type::ANCHOR, anchor);
        tk->begin(begin);
        tk->end(context().location());
        pop();
    }
};

// ------------------------------------------------------------------
class ParseTextLine : public ParseState {
public:
    ParseTextLine(bool allow_status = false,
                  Token::Type terminal_token = Token::Type::NONE,
                  token_t token_to_end = nullptr)
    : allow_status(allow_status), terminal_token(terminal_token), token_to_end(token_to_end) { }

    const char* tracer_name() const {
        return "TextLine";
    }

    void run() {
        int c = context().input.peek();

        if (! status_scanned && allow_status && scan_status()) {
            std::string status;
            status_scanned = true;
            scan_status(&status);
            auto tk = context().push_token(Token::Type::STATUS, status);
            tk->begin(context().location());
            context().input.advance(status.size() + 2);
            tk->end(context().location());
        }

        if (scan_wraplike('[', ']') ||
            scan_wraplike('<', '>')) {
            digest();
            push<CategorizeLink>();

        } else if (scan_taglike('#')) {
            digest();
            push<ParseHashtag>();

        } else if (scan_taglike('&')) {
            digest();
            context().input.advance();
            push<ParseAnchor>();

        } else if (scan_taglike('`', true)) {
            digest();
            context().input.advance();
            push<ParseCode>();

        } else if (c == '\n' || c == EOF) {
            ingest();
            digest();
            if (terminal_token != Token::Type::NONE) {
                auto tk = context().push_token(terminal_token);
                tk->begin(context().location());
                tk->end(tk->begin());
                if (token_to_end != nullptr) {
                    token_to_end->end(context().location());
                }
            }
            pop();

        } else {
            ingest();
        }
    }

private:
    void ingest() {
        int c = context().input.peek();
        last_char = c;
        if (c != EOF) {
            if (text.size() == 0) {
                begin = context().location();
            }
            text.push_back(context().input.getc());
        }
    }

    void digest() {
        if (text.size() > 0) {
            auto tk = context().push_token(Token::Type::TEXT, text);
            tk->begin(begin);
            tk->end(context().location());
            begin = NOWHERE;
            text.clear();
        }
    }

    bool scan_taglike(char symbol, bool allow_space = false) {
        if (! isspace(last_char) || last_char == 0) {
            return false;
        }

        int c = context().input.peek();
        int c2 = context().input.peek(2);

        if (c != symbol) {
            return false;
        }

        if (c2 == symbol) {
            context().input.advance();
            return false;
        }

        if (c2 == EOF || (!allow_space && isspace(c2))) {
            return false;
        }

        return true;
    }

    std::string text;
    bool status_scanned = false;
    bool allow_status;
    Token::Type terminal_token;
    token_t token_to_end;
    char last_char = 0;
    Location begin = NOWHERE;
};

// ------------------------------------------------------------------
class ParseSectionHeader : public ParseState {
    const char* tracer_name() const {
        return "SectionHeader";
    }

    void run() {
        Location begin = context().location();

        int y = 1;

        // Scan the section level.
        for (;; y++) {
            if (context().input.peek(y) != '#') {
                // Skip the opening space.
                context().input.advance(y);
                break;
            }
        }

        auto tk = context().push_token(new HeaderStartToken(y-1));
        tk->begin(begin);
        transition<ParseTextLine>(false, Token::Type::HEADER_END, tk);
    }
};

// ------------------------------------------------------------------
struct OrderedListContext {
    OrderedListItemToken* last_item;
};

// ------------------------------------------------------------------
class ParseOrderedListItem : public ParseState {
public:
    ParseOrderedListItem(OrderedListContext* ol_context) : ol_context(ol_context) { };

    const char* tracer_name() const {
        return "OrderedListItem";
    }

    void run() {
        if (ol_item == nullptr) {
            std::string ordinal;
            int indent = scan_ordered_list(&ordinal);
            ol_item = new OrderedListItemToken(indent, ordinal);
            ol_context->last_item = ol_item;
            auto tk = context().push_token(ol_item);
            li_indent = indent;
            ord_length = ordinal.size() + 1;
            context().input.advance(li_indent);
            tk->begin(context().location());
            context().input.advance(ord_length);
            push<ParseTextLine>(true);

        } else if (scan_indent() == li_indent + ord_length + 2 ||
                   (!scan_end_of_line() &&
                    !scan_ordered_list() &&
                    !scan_unordered_list())) {
            context().input.advance(scan_indent() - 1);
            push<ParseTextLine>();

        } else {
            ol_item->end(context().location());
            auto tk = context().push_token(Token::Type::LIST_ITEM_END);
            tk->begin(context().location());
            tk->end(tk->begin());
            pop();
        }
    }

private:
    OrderedListContext* ol_context;
    OrderedListItemToken* ol_item = nullptr;
    int li_indent = 0;
    int ord_length = 0;
};

// ------------------------------------------------------------------
class ParseOrderedList : public ParseState {
    const char* tracer_name() const {
        return "OrderedList";
    }

    void run() {
        int indent = scan_ordered_list();

        if (indent &&
            (ol_context.last_item == nullptr ||
             ol_context.last_item->level() == indent)) {
            push<ParseOrderedListItem>(&ol_context);

        } else {
            pop();
        }
    }

    OrderedListContext ol_context;
};

// ------------------------------------------------------------------
struct UnorderedListContext {
    UnorderedListItemToken* last_item;
};

// ------------------------------------------------------------------
class ParseUnorderedListItem : public ParseState {
public:
    ParseUnorderedListItem(UnorderedListContext* ul_context) : ul_context(ul_context) { }

    const char* tracer_name() const {
        return "UnorderedListItem";
    }

    void run() {
        if (ul_item == nullptr) {
            int indent = scan_unordered_list();
            ul_item = new UnorderedListItemToken(indent);
            ul_context->last_item = ul_item;
            auto tk = context().push_token(ul_item);
            li_indent = indent;
            context().input.advance(li_indent);
            tk->begin(context().location());
            context().input.advance();
            push<ParseTextLine>(true);
        } else if (scan_indent() == li_indent + 3 ||
                   (!scan_end_of_line() &&
                    !scan_ordered_list() &&
                    !scan_unordered_list())) {
            context().input.advance(scan_indent() - 1);
            push<ParseTextLine>();
        } else {
            ul_item->end(context().location());
            auto tk = context().push_token(Token::Type::LIST_ITEM_END);
            tk->begin(context().location());
            tk->end(tk->begin());
            pop();
        }
    }

private:
    UnorderedListContext* ul_context;
    UnorderedListItemToken* ul_item = nullptr;
    int li_indent = 0;
};

// ------------------------------------------------------------------
class ParseUnorderedList : public ParseState {
    const char* tracer_name() const {
        return "UnorderedList";
    }

    void run() {
        int indent = scan_unordered_list();

        if (indent &&
            (ul_context.last_item == nullptr ||
             ul_context.last_item->level() == indent)) {
            push<ParseUnorderedListItem>(&ul_context);

        } else {
            pop();
        }
    }

    UnorderedListContext ul_context;
};

// ------------------------------------------------------------------
class ParseTextBlock : public ParseState {
    const char* tracer_name() const {
        return "TextBlock";
    }

    void run() {
        int break_length = scan_para_break();
        if (break_length != 0) {
            context().input.advance(break_length - 1);
            pop();

        } else {
            int c = context().input.peek();
            if (c == '`' && context().input.scan_eq("```")) {
                transition<ParseEmbeddedDocument>(Token::Type::CODE_BLOCK, "```");

            } else {
                push<ParseTextLine>();
            }
        }
    }
};

// ------------------------------------------------------------------
class ParseBegin : public ParseState {
    const char* tracer_name() const {
        return "Begin";
    }

    void run() {
        int c = context().input.peek();

        if (c == '\n') {
            auto tk = context().push_token(Token::Type::NEWLINE);
            tk->begin(context().location());
            context().input.advance();
            tk->end(context().location());

        } else if (c == '#') {
            push<ParseSectionHeader>();

        } else if (context().tokens_parsed == 0 && context().input.scan_eq("---")) {
            push<ParseEmbeddedDocument>(Token::Type::FRONT_MATTER, "---");

        } else if (context().input.scan_eq("```")) {
            push<ParseEmbeddedDocument>(Token::Type::CODE_BLOCK, "```");

        } else if (scan_ordered_list() != 0) {
            push<ParseOrderedList>();

        } else if (scan_unordered_list() != 0) {
            push<ParseUnorderedList>();

        } else if (c == EOF) {
            auto tk = context().push_token(Token::Type::END);
            tk->begin(context().location());
            tk->end(tk->begin());
            pop();

        } else {
            push<ParseTextBlock>();
        }
    }
};

// ------------------------------------------------------------------
typedef moonlight::gen::Iterator<token_t> Iterator;

// ------------------------------------------------------------------
class Parser {
public:
    Parser(std::istream& input, const std::string& filename = "<input>")
    : ctx({
        .input = moonlight::file::BufferedInput(input, filename)
    }),
    machine(ParseState::Machine::init<ParseBegin>(ctx)) {
#ifdef MOONLIGHT_AUTOMATA_DEBUG
        machine.add_tracer([](ParseState::Machine::TraceEvent event,
                              Context& context,
                              const std::string& event_name,
                              const std::vector<ParseState::Pointer>& stack,
                              ParseState::Pointer old_state,
                              ParseState::Pointer new_state) {
            (void) event;
            std::vector<const char*> stack_names = moonlight::collect::map<const char*>(
                stack, [](auto state) {
                    return state->tracer_name();
                }
            );
            std::string old_state_name = old_state == nullptr ? "(null)" : old_state->tracer_name();
            std::string new_state_name = new_state == nullptr ? "(null)" : new_state->tracer_name();
            tfm::printf("cursor=%s\nstack=[%s]\n%-12s%s -> %s\n",
                        context.dbg_cursor(),
                        moonlight::str::join(stack_names, ","),
                        event_name, old_state_name, new_state_name);
        });
#endif
    }

    static const Iterator end() {
        return moonlight::gen::end<token_t>();
    }

    Iterator begin() {
        return moonlight::gen::begin<token_t>(
            std::bind(&Parser::parse_one, this));
    }

private:
    std::optional<token_t> parse_one() {
        auto token = ctx.pop_token();
        if (token) {
            return token;
        }

        while (machine.update()) {
            auto token = ctx.pop_token();
            if (token) {
                return token;
            }
        }

        return {};
    }

    Context ctx;
    ParseState::Machine machine;
};

}
}

#endif /* JOTDOWN_PARSER_H */
