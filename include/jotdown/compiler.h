/*
 * compiler.h
 *
 * Author: Lain Musgrove (lain.proliant@gmail.com)
 * Date: Friday May 29, 2020
 *
 * Distributed under terms of the MIT license.
 */

#ifndef __JOTDOWN_COMPILER_H
#define __JOTDOWN_COMPILER_H

#include <deque>
#include "jotdown/parser.h"
#include "jotdown/interfaces.h"
#include "jotdown/object.h"
#include "tinyformat/tinyformat.h"
#include "moonlight/json.h"

namespace jotdown {
namespace compiler {

using namespace object;
using jotdown::parser::Token;
using jotdown::parser::token_t;

//-------------------------------------------------------------------
class CompilerError : public JotdownError {
public:
    CompilerError(const std::string& message, const Location& location)
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

//-------------------------------------------------------------------
class BufferedTokens {
public:
    BufferedTokens(parser::Iterator& begin, parser::Iterator& end)
    : _current(begin), _end(end) { }

    token_t peek(size_t offset = 1) {
        if (offset == 0) {
            return nullptr;
        }

        while (_buffer.size() < offset) {
            auto tk = get();
            if (tk == nullptr) {
                return nullptr;
            }
            _buffer.push_back(get());
        }

        return _buffer[offset-1];
    }

    token_t get() {
        if (_current == _end) {
            return nullptr;
        }
        return *(_current++);
    }

private:
    parser::Iterator _current;
    const parser::Iterator _end;
    std::deque<token_t> _buffer;
};

//-------------------------------------------------------------------
struct Context {
    Document doc;
    BufferedTokens tokens;
};

//-------------------------------------------------------------------
class CompileState : public moonlight::automata::State<Context> {
protected:
    CompilerError unexpected_token(token_t tk, const std::string& doing) {
        return CompilerError(
            tfm::format("Unexpected '%s' token while %s.",
                        tk->type_name(tk->type()),
                        doing),
            tk->location());
    }
};

//-------------------------------------------------------------------
class CompileTextBlock : public CompileState {
public:

};

//-------------------------------------------------------------------
class CompileSection : public CompileState {
public:
    CompileSection(Section& section) : _section(section) { }

    void run() {
        auto tk = context().tokens.peek();

        switch (tk->type()) {
        case Token::Type::REF:
        case Token::Type::TEXT:
        case Token::Type::ANCHOR:
        case Token::Type::HASHTAG:
            init_text_block();
            break;
        default:
            pop();
            break;
        }
    }

private:
    void init_text_block() {
        auto text_block = _section.add(new TextBlock());
        push<CompileTextBlock>(text_block);
    }

    Section& _section;
};

//-------------------------------------------------------------------
class CompileSectionHeaderText : public CompileState {
public:
    CompileSectionHeaderText(Section& section) : _section(section) { }

    void run() {
        auto tk = context().tokens.get();

        switch (tk->type()) {
        case Token::Type::TEXT:
            _section.header().add(new Text(tk->content()));
            break;
        case Token::Type::HASHTAG:
            _section.header().add(new Hashtag(tk->content()));
            break;
        case Token::Type::ANCHOR:
            _section.header().add(new Anchor(tk->content()));
            break;
        case Token::Type::REF:
            ingest_ref_token(tk);
            break;
        case Token::Type::HEADER_END:
            transition<CompileSection>(_section);
            break;
        default:
            throw unexpected_token(tk, "compiling section header");
        }
    }

private:
    void ingest_ref_token(token_t tk) {
        std::shared_ptr<parser::RefToken> ref_tk = (
            dynamic_pointer_cast<parser::RefToken>(tk));
        _section.header().add(new Ref(ref_tk->link(), ref_tk->text()));
    }

    Section& _section;
};

//-------------------------------------------------------------------
class CompileHeader : public CompileState {
public:
    void run() {
        token_t tk = context().tokens.get();
        std::shared_ptr<parser::HeaderStartToken> header_tk = (
            dynamic_pointer_cast<parser::HeaderStartToken>(tk));
        Section& section = context().doc.add(new Section(
            header_tk->level()
        ));

        transition<CompileSectionHeaderText>(section);
    }
};

//-------------------------------------------------------------------
class CompileBegin : public CompileState {
    void run() {
        token_t tk = context().tokens.peek();

        switch (tk->type()) {
        case Token::Type::HEADER_START:
            push<CompileHeader>();
        }
    }
};

//-------------------------------------------------------------------
class Compiler {
public:
    Document compile(parser::Iterator begin, parser::Iterator end) {
        Context ctx {
            .tokens = BufferedTokens(begin, end)
        };

        // TODO: Do stuff here!

        return ctx.doc;
    }
};

}
}

#endif /* !__JOTDOWN_COMPILER_H */
