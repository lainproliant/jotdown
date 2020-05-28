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
        static std::string names[] = {
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
        return tfm::format("<Token:%s '%s'>",
                           type_name(type()), strliteral(content()));
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

// ------------------------------------------------------------------
class HeaderStartToken : public Token {
public:
    HeaderStartToken(int level, const Location& loc = NOWHERE)
    : Token(Type::HEADER_START, "", loc), _level(level) { }

    int level() const {
        return _level;
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
};

// ------------------------------------------------------------------
struct Context {
    moonlight::file::BufferedInput fb;
    Token::Type terminal = Token::Type::NONE;
    bool newline = false;
    int level = 0;
    bool ol = false;
    bool ul = false;
    int ord = 0;  // TODO: Fix this not actually getting updated!
    std::deque<Token> tokens;

    void push_token(Token::Type type, const std::string& content = "") {
        push_token(Token(type, content));
    }

    void push_token(const Token& tk) {
        tokens.push_back(tk);
    }

    void push_terminal() {
        push_token(terminal);
        terminal = Token::Type::NONE;
    }

    void push_error(const std::string& message) {
        push_token(Token::Type::ERROR, message);
    }

    std::optional<Token> pop_token() {
        if (tokens.size() > 0) {
            Token tk = tokens.front();
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
            sb.push_back(context().fb.getc());
            break;
        default:
            if (line_begin && context().fb.scan_eq("```")) {
                context().push_token(Token::Type::CODE_BLOCK, sb);
                context().fb.advance(3);
                transition<ParsePostCode>();
            }
        }

    }

private:
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
                context().push_token(RefToken(sb, link_text_sb,
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
            context().push_token(RefToken(sb, "", context().location()));
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
                context().push_token(RefToken(sb, "", context().location()));
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

        if (c == '\n' || c == EOF || isspace(c)) {
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
                context().fb.advance();
                push<ParseHashtag>();
            } else {
                new_word = false;
                ingest();
            }
        case '\n':
            new_word = false;
            ingest();
            // fallthrough

        case EOF:
            digest();
            context().push_terminal();
            pop();
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
    ParseMaybeHeader(int level) : level(level) { }

    void run() {
        int c = context().fb.peek(2);

        if (c == '#') {
            context().fb.advance();
            transition<ParseMaybeHeader>(level + 1);

        } else if (isspace(c)) {
            context().fb.advance();
            context().push_token(Token::Type::HEADER_START);
            context().terminal = Token::Type::HEADER_END;
            transition<ParseText>();

        } else {
            std::string sb;
            for (int x = 0; x < level - 1; x++) {
                sb.push_back('#');
            }
            transition<ParseText>(sb);
        }
    }

private:
    int level;
};

// ------------------------------------------------------------------
class ParseOrderedList : public ParseState {
public:
    ParseOrderedList(int level) : level(level) { }

    void run() {
        context().fb.advance();

        while (! ordinal_scanned) {
            int c = context().fb.getc();
            if (isalnum(c)) {
                ordinal.push_back(c);

            } else if (c == '.') {
                ordinal_scanned = true;

            } else {
                // This is an error condition that should not
                // be possible outside of a programming mistake.
                throw ParserError("Unexpected parser state encountered.",
                                  context().location());
            }
        }

        context().push_token(OrderedListItemToken(level, ordinal));
        context().fb.advance(); // Skip the required opening space.
        transition<ParseText>();
    }

private:
    int level;
    bool ordinal_scanned = false;
    std::string ordinal;
};

// ------------------------------------------------------------------
class ParseUnorderedList : public ParseState {
public:
    ParseUnorderedList(int level) : level(level) { }

    void run() {
        if (context().level != 0) {
            context().push_token(Token::Type::LIST_ITEM_END);
        }

        // Skip the indent, slash, and required opening space.
        context().fb.advance(level+1);
        context().push_token(UnorderedListItemToken(level,
                                                    context().location()));
        context().level = level;
        context().newline = false;
        context().ul = true;
        context().ol = false;
        context().ord = 2;
        transition<ParseText>();
    }

private:
    int level;
};

// ------------------------------------------------------------------
class ParseLineScan : public ParseState {
    void run() {
        x++;

        int c = context().fb.peek(x);

        if (isspace(c) && c != '\n') {
            x = 0;
            return;

        } else if (c == '\n' || c == EOF) {
            if (context().level != 0) {
                context().push_token(Token::Type::LIST_ITEM_END);
            }
            context().fb.advance();
            context().push_token(Token::Type::NEWLINE);
            pop();
            return;

        } else if (c == '-' && (context().level != 0 || context().newline)
                   && x != context().level + context().ord) {
            int next_c = context().fb.peek(x+1);
            if (isspace(next_c) && next_c != '\n') {
                transition<ParseUnorderedList>(x);
                return;
            }

        } else if (isalnum(c) && (context().level != 0 || context().newline)
                   && x != context().level + context().ord) {
            for (size_t y = x+1;; y++) {
                int next_c = context().fb.peek(y);
                if (next_c == '.') {
                    int final_c = context().fb.peek(y+1);
                    if (isspace(final_c)) {
                        transition<ParseOrderedList>(x);
                        return;

                    } else {
                        break;
                    }
                } else if (! isalnum(next_c)) {
                    break;
                }
            }
        }

        context().fb.advance(x-1);
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
            push<ParseMaybeHeader>(1);
            break;
        case '`':
            push<ParseMaybeCodeBlock>();
            break;
        default:
            push<ParseLineScan>();
            break;
        }
    }
};

// ------------------------------------------------------------------
class Parser {
public:
    typedef moonlight::gen::Iterator<Token> Iterator;

    Parser(std::istream& input, const std::string& filename = "<input>")
    : ctx({
        .fb = moonlight::file::BufferedInput(input, filename)
    }),
    machine(ParseState::Machine::init<ParseBegin>(ctx)) { }

    static const Iterator end() {
        return moonlight::gen::end<Token>();
    }

    Iterator begin() {
        return moonlight::gen::begin<Token>(std::bind(&Parser::parse_one, this));
    }

private:
    std::optional<Token> parse_one() {
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

#endif /* !__JOTDOWN_PARSER_H */
