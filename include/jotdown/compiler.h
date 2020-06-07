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

    token_t get() {
        token_t tk;

        if (_buffer.size() > 0) {
            tk = _buffer.front();
            _buffer.pop_front();

        } else if (_current == _end) {
            tk = nullptr;

        } else {
            tk = *(_current++);
        }

        return tk;
    }

    token_t peek(size_t offset = 1) {
        if (offset == 0) {
            return nullptr;
        }

        while (_buffer.size() < offset) {
            if (_current == _end) {
                return nullptr;
            }
            auto tk = *(_current++);
            if (tk == nullptr) {
                return nullptr;
            }
            _buffer.push_back(tk);
        }

        return _buffer[offset-1];
    }

    void advance(size_t offset = 1) {
        for (size_t x = 0; x < offset; x++) {
            get();
        }
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
            tfm::format("Unexpected '%s' token: %s.",
                        tk->type_name(tk->type()),
                        doing),
            tk->location());
    }
};

//-------------------------------------------------------------------
class CompileTextBlock : public CompileState {
public:
    CompileTextBlock(TextBlock* text_block,
                     Token::Type terminal_type = Token::Type::NONE)
    : _text_block(text_block), _terminal_type(terminal_type) { }

    const char* name() const {
        return "TextBlock";
    }

    void run() {
        auto tk = context().tokens.get();

        switch (tk->type()) {
        case Token::Type::TEXT:
            _text_block->add(new Text(tk->content()));
            break;
        case Token::Type::HASHTAG:
            _text_block->add(new Hashtag(tk->content()));
            break;
        case Token::Type::CODE:
            _text_block->add(new Code(tk->content()));
            break;
        case Token::Type::ANCHOR:
            _text_block->add(new Anchor(tk->content()));
            break;
        case Token::Type::REF:
            ingest_ref_token(tk);
            break;
        default:
            if (tk->type() != _terminal_type && _terminal_type != Token::Type::NONE) {
                throw unexpected_token(
                    tk, "expecting " + tk->type_name(_terminal_type));
            }
            pop();
            break;
        }
    }

private:
    void ingest_ref_token(token_t tk) {
        std::shared_ptr<parser::RefToken> ref_tk = (
            dynamic_pointer_cast<parser::RefToken>(tk));
        _text_block->add(new Ref(ref_tk->link(), ref_tk->text()));
    }

    TextBlock* _text_block;
    Token::Type _terminal_type;
};

//-------------------------------------------------------------------
class CompileOrderedList;
class CompileUnorderedList;
class CompileListBase : public CompileState {
protected:
    enum class Result {
        POP,
        RETURN,
        CONTINUE
    };

    Result _compare_item(token_t tk) {
        switch(tk->type()) {
        case Token::Type::OL_ITEM:
        case Token::Type::UL_ITEM:
            break;
        default:
            return Result::POP;
        }

        if (last_token != nullptr && last_item != nullptr &&
            (tk->type() == Token::Type::OL_ITEM || tk->type() == Token::Type::UL_ITEM)) {
            std::shared_ptr<parser::ListItemToken> li_tk = (
                dynamic_pointer_cast<parser::ListItemToken>(tk));
            if (li_tk->level() < last_token->level()) {
                return Result::POP;

            } else if (li_tk->level() > last_token->level()) {
                if (li_tk->type() == Token::Type::OL_ITEM) {
                    auto& ordered_list = last_item->add(new OrderedList());
                    push<CompileOrderedList>(&ordered_list);
                    return Result::RETURN;

                } else {
                    auto& unordered_list = last_item->add(new UnorderedList());
                    push<CompileUnorderedList>(&unordered_list);
                    return Result::RETURN;
                }
            }
        }
        return Result::CONTINUE;
    }

    ListItem* last_item = nullptr;
    parser::ListItemToken* last_token = nullptr;
};

//-------------------------------------------------------------------
class CompileOrderedList : public CompileListBase {
public:
    CompileOrderedList(OrderedList* list) : _list(list) { }

    const char* name() const {
        return "OrderedList";
    }

    void run() {
        auto tk = context().tokens.peek();

        switch (_compare_item(tk)) {
        case Result::POP:
            pop();
            return;
        case Result::RETURN:
            return;
        case Result::CONTINUE:
            break;
        }

        if (tk->type() == Token::Type::OL_ITEM) {
            std::shared_ptr<parser::OrderedListItemToken> ol_tk = (
                dynamic_pointer_cast<parser::OrderedListItemToken>(tk));
            auto& ordered_list_item = _list->add(new OrderedListItem(ol_tk->ordinal()));
            TextBlock& text_block = ordered_list_item.text();
            context().tokens.advance();
            last_token = ol_tk.get();
            last_item = &ordered_list_item;
            push<CompileTextBlock>(&text_block, Token::Type::LIST_ITEM_END);

        } else if (tk->type() == Token::Type::UL_ITEM) {
            throw unexpected_token(tk, "compiling ordered list at the same level");
        }
    }

private:
    ListItem* last_item = nullptr;
    parser::ListItemToken* last_token = nullptr;
    OrderedList* _list;
};

//-------------------------------------------------------------------
class CompileUnorderedList : public CompileListBase {
public:
    CompileUnorderedList(UnorderedList* list) : _list(list) { }

    const char* name() const {
        return "UnorderedList";
    }

    void run() {
        auto tk = context().tokens.peek();

        switch (_compare_item(tk)) {
        case Result::POP:
            pop();
            return;
        case Result::RETURN:
            return;
        case Result::CONTINUE:
            break;
        }

        if (tk->type() == Token::Type::UL_ITEM) {
            std::shared_ptr<parser::UnorderedListItemToken> ul_tk = (
                dynamic_pointer_cast<parser::UnorderedListItemToken>(tk));
            auto& unordered_list_item = _list->add(new UnorderedListItem());
            TextBlock& text_block = unordered_list_item.text();
            context().tokens.advance();
            last_token = ul_tk.get();
            last_item = &unordered_list_item;
            push<CompileTextBlock>(&text_block, Token::Type::LIST_ITEM_END);

        } else if (tk->type() == Token::Type::OL_ITEM) {
            throw unexpected_token(tk, "compiling unordered list at the same level");
        }
    }

private:
    UnorderedList* _list;
};

//-------------------------------------------------------------------
class CompileTopLevelList : public CompileState {
public:
    CompileTopLevelList(Section* parent) : _parent(parent) { }

    const char* name() const {
        return "TopLevelList";
    }

    void run() {
        auto tk = context().tokens.peek();

        if (tk->type() == Token::Type::OL_ITEM) {
            auto& ordered_list = _parent->add(new OrderedList());
            transition<CompileOrderedList>(&ordered_list);

        } else if (tk->type() == Token::Type::UL_ITEM) {
            auto& unordered_list = _parent->add(new UnorderedList());
            transition<CompileUnorderedList>(&unordered_list);

        } else {
            throw unexpected_token(tk, "parsing top-level list");
        }
    }

private:
    Section* _parent;
};

//-------------------------------------------------------------------
class CompileCodeBlock : public CompileState {
public:
    CompileCodeBlock(Section* section) : _section(section) { }

    const char* name() const {
        return "CodeBlock";
    }

    void run() {
        auto lang_tk = context().tokens.get();
        if (lang_tk == nullptr || lang_tk->type() != Token::Type::LANGSPEC) {
            throw unexpected_token(lang_tk, "compiling code block, expected LANGSPEC");
        }
        auto code_tk = context().tokens.get();
        if (code_tk == nullptr || code_tk->type() != Token::Type::CODE_BLOCK) {
            throw unexpected_token(lang_tk, "compiling code block, expected CODE_BLOCK");
        }

        _section->add(new CodeBlock(code_tk->content(), lang_tk->content()));
        pop();
    }

private:
    Section* _section;
};

//-------------------------------------------------------------------
class CompileSectionHeader;
class CompileSection : public CompileState {
public:
    CompileSection(Section* section) : _section(section) { }

    const char* name() const {
        return "Section";
    }

    void run() {
        auto tk = context().tokens.peek();

        switch (tk->type()) {
        case Token::Type::REF:
        case Token::Type::TEXT:
        case Token::Type::ANCHOR:
        case Token::Type::HASHTAG:
        case Token::Type::CODE:
            init_text_block();
            break;

        case Token::Type::LANGSPEC:
            push<CompileCodeBlock>(_section);
            break;

        case Token::Type::HEADER_START:
            if (is_subsection(tk)) {
                push<CompileSectionHeader>(_section);

            } else {
                pop();
            }
            break;

        case Token::Type::OL_ITEM:
        case Token::Type::UL_ITEM:
            push<CompileTopLevelList>(_section);
            break;

        case Token::Type::NEWLINE:
            context().tokens.advance();
            _section->add(new LineBreak());
            break;

        default:
            pop();
            break;
        }
    }

private:
    bool is_subsection(token_t tk) {
        std::shared_ptr<parser::HeaderStartToken> header_tk = (
            dynamic_pointer_cast<parser::HeaderStartToken>(tk));
        return header_tk->level() < _section->level();
    }

    void init_text_block() {
        auto& text_block = _section->add(new TextBlock());
        push<CompileTextBlock>(&text_block);
    }

    Section* _section;
};

//-------------------------------------------------------------------
class CompileSectionHeader : public CompileState {
public:
    CompileSectionHeader(Section* section) : _section(section) { }

    const char* name() const {
        return "SectionHeader";
    }

    void run() {
        if (! header_text_processed) {
            push<CompileTextBlock>(&(_section->header()), Token::Type::HEADER_END);
            header_text_processed = true;

        } else {
            transition<CompileSection>(_section);
        }
    }

private:
    void ingest_ref_token(token_t tk) {
        std::shared_ptr<parser::RefToken> ref_tk = (
            dynamic_pointer_cast<parser::RefToken>(tk));
        _section->header().add(new Ref(ref_tk->link(), ref_tk->text()));
    }

    bool header_text_processed = false;
    Section* _section;
};

//-------------------------------------------------------------------
class CompileSubSection : public CompileState {
public:
    CompileSubSection(Section* parent) : _parent(parent) { }

    const char* name() const {
        return "SubSection";
    }

    void run() {
        token_t tk = context().tokens.get();
        std::shared_ptr<parser::HeaderStartToken> header_tk = (
            dynamic_pointer_cast<parser::HeaderStartToken>(tk));
        Section& section = _parent->add(new Section(
            header_tk->level()
        ));

        transition<CompileSectionHeader>(&section);
    }

private:
    Section* _parent;
};

//-------------------------------------------------------------------
class CompileTopLevelSection : public CompileState {
public:
    const char* name() const {
        return "TopLevelSection";
    }

    void run() {
        token_t tk = context().tokens.get();
        std::shared_ptr<parser::HeaderStartToken> header_tk = (
            dynamic_pointer_cast<parser::HeaderStartToken>(tk));
        Section& section = context().doc.add(new Section(
            header_tk->level()
        ));

        transition<CompileSectionHeader>(&section);
    }
};

//-------------------------------------------------------------------
class CompileBegin : public CompileState {
    const char* name() const {
        return "Begin";
    }

    void run() {
        token_t tk = context().tokens.peek();

        if (tk->type() == Token::Type::HEADER_START) {
            push<CompileTopLevelSection>();

        } else if (tk->type() == Token::Type::END) {
            terminate();

        } else {
            auto& section = context().doc.add(new Section(0));
            push<CompileSection>(&section);
        }
    }
};

//-------------------------------------------------------------------
class Compiler {
public:
    template<class T>
    Document compile(T true_begin, T true_end) {
        auto begin = moonlight::gen::wrap(true_begin, true_end);
        auto end = moonlight::gen::end<typename T::value_type>();

        Context ctx {
            .tokens = BufferedTokens(begin, end)
        };


        auto machine = CompileState::Machine::init<CompileBegin>(ctx);
#ifdef MOONLIGHT_AUTOMATA_DEBUG
        machine.add_tracer([](CompileState::Machine::TraceEvent event,
                              Context& context,
                              const std::string& event_name,
                              const std::vector<CompileState::Pointer>& stack,
                              CompileState::Pointer old_state,
                              CompileState::Pointer new_state) {
            std::vector<const char*> stack_names = moonlight::collect::map<const char*>(
                stack, [](auto state) {
                    return state->name();
                }
            );
            std::vector<std::string> token_names;
            for (size_t x = 1;;x++) {
                auto token = context.tokens.peek(x);
                if (token != nullptr) {
                    token_names.push_back(token->type_name(token->type()));
                } else {
                    break;
                }
            }
            std::string old_state_name = old_state == nullptr ? "(null)" : old_state->name();
            std::string new_state_name = new_state == nullptr ? "(null)" : new_state->name();
            tfm::printf("stack=[%s]\ntokens=[%s]\n%-12s%s -> %s\n",
                        moonlight::str::join(stack_names, ","),
                        moonlight::str::join(token_names, ","),
                        event_name, old_state_name, new_state_name);
        });
#endif
        machine.run_until_complete();

        return ctx.doc;
    }
};

}
}

#endif /* !__JOTDOWN_COMPILER_H */
