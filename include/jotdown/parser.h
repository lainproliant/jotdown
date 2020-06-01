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

#include "moonlight/core.h"
#include "moonlight/generator.h"
#include "jotdown/utils.h"
#include "jotdown/interfaces.h"
#include "jotdown/error.h"
#include "tinyformat/tinyformat.h"

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
class Token : public moonlight::file::Writable<Token> {
public:
    enum class Type {
        NONE,
        TEXT,
        REF,
        ANCHOR,
        HASHTAG,
        HEADER_START,
        HEADER_END,
        UL_ITEM,
        OL_ITEM,
        LIST_ITEM_END,
        LANGSPEC,
        CODE_BLOCK,
        NEWLINE,
        END,
        ERROR,
        NUM_TYPES
    };

    Token(Type type, const std::string& content = "",
          const Location& loc = NOWHERE)
    : _type(type), _content(content), _location(loc) { }

    virtual ~Token() { }

    static const std::string& type_name(Type type) {
        static const std::string names[] = {
            "NONE",
            "TEXT",
            "REF",
            "HASHTAG",
            "HEADER_START",
            "HEADER_END",
            "UL_ITEM",
            "OL_ITEM",
            "LIST_ITEM_END",
            "LANGSPEC",
            "CODE_BLOCK",
            "NEWLINE",
            "END",
            "ERROR"
            "NUM_TYPES"
        };

        return names[(unsigned int)type];
    }

    std::string repr() const {
        if (content().size() == 0) {
            return tfm::format("<%s>", type_name(type()));

        } else {
            return tfm::format("<%s '%s'>",
                               type_name(type()), strliteral(content()));
        }
    }

    Type type() const {
        return _type;
    }

    const std::string& content() const {
        return _content;
    }

    const Location& location() const {
        return _location;
    }

private:
    const Type _type;
    const std::string _content;
    const Location _location;
};
typedef std::shared_ptr<Token> token_t;

// ------------------------------------------------------------------
class HeaderStartToken : public Token {
public:
    HeaderStartToken(int level, const Location& loc = NOWHERE)
    : Token(Type::HEADER_START, "", loc), _level(level) { }

    int level() const {
        return _level;
    }

    std::string repr() const {
        return tfm::format("<%s %d>", type_name(type()), _level);
    }

private:
    const int _level;
};

// ------------------------------------------------------------------
class RefToken : public Token {
public:
    RefToken(const std::string& link,
             const std::string& text = "",
             const Location& loc = NOWHERE)
    : Token(Type::REF, link, loc), _text(text.size() > 0 ? text : link) { }

    const std::string& link() const {
        return content();
    }

    const std::string& text() const {
        return _text;
    }

    std::string repr() const {
        if (_text != content()) {
            return tfm::format("<%s %s (%s)>", type_name(type()), content(), _text);
        } else {
            return tfm::format("<%s %s>", type_name(type()), content());
        }
    }

private:
    const std::string& _text;
};

// ------------------------------------------------------------------
class ListItemToken : public Token {
public:
    ListItemToken(Type type, int level, const std::string& content = "",
                  const Location& loc = NOWHERE)
    : Token(type, content, loc), _level(level) { }

    int level() const {
        return _level;
    }

private:
    const int _level;
};

// ------------------------------------------------------------------
class UnorderedListItemToken : public ListItemToken {
public:
    UnorderedListItemToken(int level, const Location& loc = NOWHERE)
    : ListItemToken(Type::UL_ITEM, level, "- ", loc) { }

    std::string repr() const {
        return tfm::format("<%s %d>", type_name(type()), level());
    }
};

// ------------------------------------------------------------------
class OrderedListItemToken : public ListItemToken {
public:
    OrderedListItemToken(int level, const std::string& ordinal,
                         const Location& loc = NOWHERE)
    : ListItemToken(Type::OL_ITEM, level, ordinal, loc) { }

    const std::string& ordinal() const {
        return content();
    }

    std::string repr() const {
        return tfm::format("<%s %d '%s'>", type_name(type()), level(), ordinal());
    }
};

// ------------------------------------------------------------------
struct Context {
    moonlight::file::BufferedInput fb;
    Token::Type terminal = Token::Type::NONE;
    bool newline = false;
    int level = 0;
    bool is_ol = false;
    bool is_ul = false;
    int li_text_indent = 0;
    std::deque<token_t> tokens;

    void push_token(Token::Type type, const std::string& content = "") {
        push_token(make<Token>(type, content));
    }

    void push_token(token_t tk) {
        tokens.push_back(tk);
    }

    void push_terminal() {
        if (terminal != Token::Type::NONE) {
            push_token(terminal);
        }
        terminal = Token::Type::NONE;
    }

    void push_error(const std::string& message) {
        push_token(Token::Type::ERROR, message);
    }

    void push_newline() {
        push_token(Token::Type::NEWLINE);
        level = 0;
        li_text_indent = 0;
        is_ul = false;
        is_ol = false;
        newline = true;
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
            .filename = fb.name(),
            .line = fb.line(),
            .col = fb.col()
        };
    }

};

// ------------------------------------------------------------------
typedef moonlight::automata::State<Context> ParseState;

// ------------------------------------------------------------------
// We've closed the code block, but other text might be on this line.
// If anything other than whitespace is on this line, we report
// an error and move on until the next line.
// ------------------------------------------------------------------
class ParsePostCode : public ParseState {
    void run() {
        int c = context().fb.getc();

        if (c == '\n' || c == EOF) {
            pop();
        } else if (! isspace(c) && ! error_reported) {
            context().push_error("Invalid content found on the same line after "
                                 "terminated code block.");
            error_reported = true;
        }
    }

private:
    bool error_reported = false;
};

// ------------------------------------------------------------------
class ParseCode : public ParseState {
    void run() {
        switch (context().fb.peek()) {
        case EOF:
            context().push_error("Unterminated code block.");
            pop();
            break;
        case '\n':
            line_begin = true;
            ingest();
            break;
        default:
            if (line_begin && context().fb.scan_eq("```")) {
                context().push_token(Token::Type::CODE_BLOCK, sb);
                context().fb.advance(3);
                transition<ParsePostCode>();
            } else {
                line_begin = false;
                ingest();
            }
        }
    }

private:
    void ingest() {
        sb.push_back(context().fb.getc());
    }

    std::string sb;
    bool line_begin = true;
};

// ------------------------------------------------------------------
class ParseCodeBlockLangspec : public ParseState {
    void run() {
        switch (context().fb.peek()) {
        case '\n':
            context().push_token(Token::Type::LANGSPEC, sb);
            context().fb.advance();
            transition<ParseCode>();
            break;

        case EOF:
            context().push_error("Unterminated code block.");
            pop();
            break;

        default:
            sb.push_back(context().fb.getc());
            break;
        }
    }

private:
    std::string sb;
};

// ------------------------------------------------------------------
class ParseRef : public ParseState {
    void run() {
        int c = context().fb.peek();

        if (has_link_text) {
            if (c == '\\') {
                // If a closing square bracket is contained
                // in the link text itself, it must be escaped
                // with a backslash.
                int c2 = context().fb.peek(2);
                if (c2 == ']') {
                    context().fb.advance();
                    link_text_sb.push_back(context().fb.getc());
                }

            } else if (c == ']') {
                context().push_token(make<RefToken>(sb, link_text_sb,
                                                    context().location()));
                context().fb.advance();
                pop();

            } else {
                link_text_sb.push_back(context().fb.getc());
            }

        } else if (c == '[') {
            context().fb.advance();
            has_link_text = true;

        } else if (c == '\n' || c == EOF || isspace(c)) {
            context().push_token(make<RefToken>(sb, "", context().location()));
            pop();

        } else if (ispunct(c)) {
            // This is the somewhat auduous punctuation case.
            // Punctuation may be part of the link, but if the
            // link ends in punctuation followed by a space,
            // that punctuation shouldn't be included in the
            // link text itself as it is part of the structure
            // of the text.
            // E.g., "This is a sentence ending in a @ref."
            int c2 = context().fb.peek(2);
            if (isspace(c2) || c2 == EOF) {
                // This isn't part of the link, we're done.
                context().push_token(make<RefToken>(sb, "", context().location()));
                pop();

            } else {
                sb.push_back(context().fb.getc());
            }

        } else {
            sb.push_back(context().fb.getc());
        }
    }

private:
    bool has_link_text = false;
    std::string link_text_sb;
    std::string sb;
};

// ------------------------------------------------------------------
class ParseHashtag : public ParseState {
    void run() {
        int c = context().fb.peek();

        if (sb.size() == 0 && c == '#') {
            sb.push_back(c);
            context().fb.advance();

        } else if (c == '\n' || c == EOF || isspace(c)) {
            context().push_token(Token::Type::HASHTAG, sb);
            pop();

        } else if (ispunct(c)) {
            // See above in ParseRef for context, this is similar logic.
            int c2 = context().fb.peek(2);
            if (isspace(c2) || c2 == EOF) {
                // This isn't part of the hashtag, we're done.
                context().push_token(Token::Type::HASHTAG, sb);
                pop();

            } else {
                sb.push_back(context().fb.getc());
            }

        } else {
            sb.push_back(context().fb.getc());
        }
    }

private:
    std::string sb;
};

// ------------------------------------------------------------------
class ParseText : public ParseState {
public:
    void run() {
        switch(context().fb.peek()) {
        case '@':
            if (new_word && ! isspace(context().fb.peek(2))) {
                digest();
                push<ParseRef>();
                break;

            } else {
                new_word = false;
                ingest();
            }
            break;

        case '#':
            if (new_word && ! isspace(context().fb.peek(2))) {
                digest();
                push<ParseHashtag>();

            } else {
                new_word = false;
                ingest();
            }
            break;

        case '\n':
            new_word = false;
            context().newline = true;
            ingest();
            // fallthrough

        case EOF:
            digest();
            context().push_terminal();
            pop();
            break;

        default:
            int c = context().fb.peek();
            if (isspace(c)) {
                new_word = true;
            }
            ingest();
            break;
        }
    }

private:
    void ingest() {
        sb.push_back(context().fb.getc());
    }

    void digest() {
        context().push_token(Token::Type::TEXT, sb);
        sb.clear();
    }

    std::string sb;
    bool new_word = true;
};

// ------------------------------------------------------------------
class ParseMaybeCodeBlock : public ParseState {
    void run() {
        if (context().fb.scan_eq("```")) {
            context().fb.advance(3);
            transition<ParseCodeBlockLangspec>();
        } else {
            transition<ParseText>();
        }
    }
};

// ------------------------------------------------------------------
class ParseMaybeHeader : public ParseState {
public:
    ParseMaybeHeader(int level) : level(level) { }

    void run() {
        int c = context().fb.peek(level+1);

        if (c == '#') {
            transition<ParseMaybeHeader>(level+1);

        } else if (isspace(c)) {
            context().push_token(make<HeaderStartToken>(level));
            context().terminal = Token::Type::HEADER_END;
            context().fb.advance(level + 1);
            transition<ParseText>();

        } else {
            transition<ParseText>();
        }
    }

private:
    int level;
};

// ------------------------------------------------------------------
class ParseMaybeOrderedList : public ParseState {
public:
    ParseMaybeOrderedList(int level) : _level(level) { }

    void run() {
        // If the given level matches li_text_indent,
        // this should be treated as text contents of a list item
        // instead of a new list item.
        if (_level != 0 && _level == context().li_text_indent) {
            context().fb.advance(context().li_text_indent);
            transition<ParseText>();
            return;
        }

        // If we are at the same level as an unordered list being parsed,
        // this should be treated as text belonging to the unordered list
        // as lists must be homogenous between ordered and unordered.
        if (_level == context().level && context().is_ul) {
            context().fb.advance(_level);
            transition<ParseText>();
            return;
        }

        // Scan forward from the first alphanumeric character until we
        // reach a period.  If the following character is a space,
        // we can treat this as an ordered list item.
        int y = 1;
        for (;; y++) {
            int c = context().fb.peek(_level + y);
            if (isalnum(c)) {
                _ordinal.push_back(c);
                continue;

            } else if (c == '.') {
                int c2 = context().fb.peek(_level + y + 1);
                if (c2 == ' ' || c2 == '\t') {
                    break;
                }
            }

            context().fb.advance(_level);
            transition<ParseText>();
            return;
        }

        context().is_ul = false;
        context().is_ol = true;
        context().level = _level;
        context().li_text_indent = _level + _ordinal.size() + 2;
        context().push_token(make<OrderedListItemToken>(_level, _ordinal, context().location()));
        context().fb.advance(context().li_text_indent);
        transition<ParseText>();
    }

private:
    std::string _ordinal;
    int _level;
};

// ------------------------------------------------------------------
class ParseMaybeUnorderedList : public ParseState {
public:
    ParseMaybeUnorderedList(int level) : _level(level) { }

    void run() {
        // If the given level matches li_text_indent,
        // this should be treated as text contents of a list item
        // instead of a new list item.
        if (_level != 0 && _level == context().li_text_indent) {
            context().fb.advance(_level);
            transition<ParseText>();
            return;
        }

        // If we are at the same level as an ordered list being parsed,
        // this should be treated as text belonging to the ordered list
        // as lists must be homogenous between ordered and unordered.
        if (_level == context().level && context().is_ol) {
            context().fb.advance(_level);
            transition<ParseText>();
            return;
        }

        // If the character following the initial dash is not a space,
        // this should not be treated as an unordered list item.
        if (context().fb.peek(_level+1) != '-' || context().fb.peek(_level+2) != ' ') {
            context().fb.advance(_level);
            transition<ParseText>();
            return;
        }

        // Skip forward and parse the unordered list item.
        context().is_ul = true;
        context().is_ol = false;
        context().level = _level;
        context().li_text_indent = _level + 2;
        context().push_token(make<UnorderedListItemToken>(_level, context().location()));
        context().fb.advance(context().li_text_indent);
        transition<ParseText>();
        return;
    }

private:
    int _level;
};

// ------------------------------------------------------------------
class ParseLineScan : public ParseState {
    void run() {
        int c = context().fb.peek(x+1);

        if (isspace(c) && c != '\n') {
            x++;
            return;

        } else if (c == '\n' || c == EOF) {
            if (context().level != 0) {
                context().push_token(Token::Type::LIST_ITEM_END);
            }
            context().fb.advance();
            context().push_newline();
            pop();
            return;

        } else if (c == '-') {
            transition<ParseMaybeUnorderedList>(x);
            return;

        } else if (isalnum(c)) {
            transition<ParseMaybeOrderedList>(x);
            return;
        }

        context().fb.advance(x);
        transition<ParseText>();
    }

private:
    int x = 0;
};

// ------------------------------------------------------------------
class ParseBegin : public ParseState {
    void run() {
        int c = context().fb.peek();

        switch (c) {
        case '#':
            push<ParseMaybeHeader>(0);
            break;
        case '`':
            push<ParseMaybeCodeBlock>();
            break;
        case EOF:
            if (context().level != 0) {
                context().push_token(Token::Type::LIST_ITEM_END);
            }
            context().push_token(Token::Type::END);
            pop();
            break;
        default:
            push<ParseLineScan>();
            break;
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
        .fb = moonlight::file::BufferedInput(input, filename)
    }),
    machine(ParseState::Machine::init<ParseBegin>(ctx)) { }

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

#endif /* !__JOTDOWN_PARSER_H */
