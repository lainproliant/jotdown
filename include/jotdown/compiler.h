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
    std::shared_ptr<Document> doc;
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
            tk->begin());
    }
};

//-------------------------------------------------------------------
class CompileTextContent : public CompileState {
public:
    CompileTextContent(std::shared_ptr<TextContent> text_content,
                     Token::Type terminal_type = Token::Type::NONE)
    : _text_content(text_content), _terminal_type(terminal_type) { }

    const char* tracer_name() const {
        return "TextContent";
    }

    void run() {
        auto tk = context().tokens.peek();

        if (_text_content->range().begin == NOWHERE) {
            _text_content->range().begin = tk->begin();
        }

        switch (tk->type()) {
        case Token::Type::TEXT:
            context().tokens.advance();
            _text_content->add(std::make_shared<Text>(tk->content()))->range(tk->range());
            break;
        case Token::Type::HASHTAG:
            context().tokens.advance();
            _text_content->add(std::make_shared<Hashtag>(tk->content()))->range(tk->range());
            break;
        case Token::Type::CODE:
            context().tokens.advance();
            _text_content->add(std::make_shared<Code>(tk->content()))->range(tk->range());
            break;
        case Token::Type::ANCHOR:
            context().tokens.advance();
            _text_content->add(std::make_shared<Anchor>(tk->content()))->range(tk->range());
            break;
        case Token::Type::REF:
            context().tokens.advance();
            ingest_ref_token(tk);
            break;
        case Token::Type::INDEX:
            context().tokens.advance();
            ingest_index_token(tk);
            break;

        default:
            if (tk->type() != _terminal_type && _terminal_type != Token::Type::NONE) {
                throw unexpected_token(
                    tk, "expecting " + tk->type_name(_terminal_type));

            } else if (tk->type() == _terminal_type) {
                context().tokens.advance();
                _text_content->range().end = tk->end();

            } else {
                _text_content->range().end = tk->begin();
            }
            pop();
            break;
        }
    }

private:
    void ingest_ref_token(token_t tk) {
        obj_t obj;
        std::shared_ptr<parser::RefToken> ref_tk = (
            dynamic_pointer_cast<parser::RefToken>(tk));

        if (! ref_tk->index_name().empty()) {
            obj = _text_content->add(
                std::make_shared<IndexedRef>(ref_tk->text(), ref_tk->index_name()));

        } else {
            obj = _text_content->add(
                std::make_shared<Ref>(ref_tk->link(), ref_tk->text()));
        }

        obj->range(tk->range());
    }

    void ingest_index_token(token_t tk) {
        std::shared_ptr<parser::IndexToken> index_tk = (
            dynamic_pointer_cast<parser::IndexToken>(tk));
        auto obj = _text_content->add(
            std::make_shared<RefIndex>(index_tk->name(), index_tk->link()));
        obj->range(tk->range());
    }

    std::shared_ptr<TextContent> _text_content;
    Token::Type _terminal_type;
};

//-------------------------------------------------------------------
class CompileOrderedList;
class CompileUnorderedList;
class CompileListBase : public CompileState {
public:
    CompileListBase(std::shared_ptr<List> list, int level) : list(list), level(level) { }

    void run() {
        auto tk = context().tokens.peek();

        if (tk->type() == Token::Type::UL_ITEM || tk->type() == Token::Type::OL_ITEM) {
            std::shared_ptr<parser::ListItemToken> li_tk = (
                dynamic_pointer_cast<parser::ListItemToken>(tk));
            if (last_token == nullptr ||
                li_tk->level() == last_token->level()) {
                context().tokens.advance();
                _process_list_item(li_tk);

            } else if (last_token->level() > li_tk->level()) {
                pop();

            } else {
                _process_sub_list(li_tk);
            }

        } else {
            list->range().end = last_token->end();
            pop();
        }
    }

protected:
    virtual void _process_list_item(std::shared_ptr<parser::ListItemToken> tk) = 0;

    void _process_sub_list(std::shared_ptr<parser::ListItemToken> tk) {
        if (tk->type() == Token::Type::OL_ITEM) {
            auto new_list = std::make_shared<OrderedList>();
            last_item->add(new_list);
            new_list->range().begin = tk->begin();
            push<CompileOrderedList>(new_list, level + 1);

        } else if (tk->type() == Token::Type::UL_ITEM) {
            auto new_list = std::make_shared<UnorderedList>();
            new_list->range().begin = tk->begin();
            last_item->add(new_list);
            push<CompileUnorderedList>(new_list, level + 1);
        }
    }

    std::shared_ptr<List> list;
    std::shared_ptr<ListItem> last_item = nullptr;
    std::shared_ptr<parser::ListItemToken> last_token = nullptr;
    int level;
};

//-------------------------------------------------------------------
class CompileOrderedList : public CompileListBase {
public:
    CompileOrderedList(std::shared_ptr<OrderedList> list, int level = 1)
    : CompileListBase(list, level), _list(list) { }

    const char* tracer_name() const {
        return "OrderedList";
    }

    void _process_list_item(std::shared_ptr<parser::ListItemToken> li_tk) {
        if (li_tk->type() != Token::Type::OL_ITEM) {
            throw unexpected_token(li_tk, "compiling ordered list at the same level");
        }
        std::shared_ptr<parser::OrderedListItemToken> oli_tk = (
            std::dynamic_pointer_cast<parser::OrderedListItemToken>(li_tk));

        auto oli = OrderedListItem::create(oli_tk->ordinal());
        oli->level(level);
        oli->range(oli_tk->range());
        _list->add(oli);
        last_item = oli;
        last_token = li_tk;
        if (context().tokens.peek()->type() == Token::Type::STATUS) {
            auto tk = context().tokens.get();
            oli->status(tk->content());
        }
        push<CompileTextContent>(oli->text(), Token::Type::LIST_ITEM_END);
    }

    std::shared_ptr<OrderedList> _list;
};

//-------------------------------------------------------------------
class CompileUnorderedList : public CompileListBase {
public:
    CompileUnorderedList(std::shared_ptr<UnorderedList> list, int level = 1)
    : CompileListBase(list, level), _list(list) { }

    const char* tracer_name() const {
        return "UnorderedList";
    }

    void _process_list_item(std::shared_ptr<parser::ListItemToken> li_tk) {
        if (li_tk->type() != Token::Type::UL_ITEM) {
            throw unexpected_token(li_tk, "compiling unordered list at the same level");
        }
        auto uli = UnorderedListItem::create();
        uli->level(level);
        uli->range(li_tk->range());
        _list->add(uli);
        last_item = uli;
        last_token = li_tk;
        if (context().tokens.peek()->type() == Token::Type::STATUS) {
            auto tk = context().tokens.get();
            uli->status(tk->content());
        }
        push<CompileTextContent>(uli->text(), Token::Type::LIST_ITEM_END);
    }

    std::shared_ptr<UnorderedList> _list;
};

//-------------------------------------------------------------------
class CompileTopLevelList : public CompileState {
public:
    CompileTopLevelList(std::shared_ptr<Section> parent) : _parent(parent) { }

    const char* tracer_name() const {
        return "TopLevelList";
    }

    void run() {
        auto tk = context().tokens.peek();

        if (tk->type() == Token::Type::OL_ITEM) {
            auto ordered_list = _parent->add(std::make_shared<OrderedList>());
            ordered_list->range().begin = tk->begin();
            transition<CompileOrderedList>(ordered_list);

        } else if (tk->type() == Token::Type::UL_ITEM) {
            auto unordered_list = _parent->add(std::make_shared<UnorderedList>());
            unordered_list->range().begin = tk->begin();
            transition<CompileUnorderedList>(unordered_list);

        } else {
            throw unexpected_token(tk, "parsing top-level list");
        }
    }

private:
    std::shared_ptr<Section> _parent;
};

//-------------------------------------------------------------------
class CompileFrontMatter : public CompileState {
public:
    const char* tracer_name() const {
        return "CompileFrontMatter";
    }

    void run() {
        auto tk = context().tokens.get();
        if (tk == nullptr || tk->type() != Token::Type::FRONT_MATTER) {
            throw unexpected_token(tk, "compiling front matter, expected FRONT_MATTER");
        }

        std::shared_ptr<parser::EmbeddedDocumentToken> front_matter_tk = (
            std::dynamic_pointer_cast<parser::EmbeddedDocumentToken>(tk)
        );

        auto obj = std::make_shared<FrontMatter>(front_matter_tk->content(), front_matter_tk->langspec());
        context().doc->front_matter(obj);
        pop();
    }
};

//-------------------------------------------------------------------
class CompileCodeBlock : public CompileState {
public:
    CompileCodeBlock(std::shared_ptr<Section> section) : _section(section) { }

    const char* tracer_name() const {
        return "CodeBlock";
    }

    void run() {
        auto tk = context().tokens.get();
        if (tk == nullptr || tk->type() != Token::Type::CODE_BLOCK) {
            throw unexpected_token(tk, "compiling code block, expected CODE_BLOCK");
        }
        std::shared_ptr<parser::EmbeddedDocumentToken> code_tk = (
            std::dynamic_pointer_cast<parser::EmbeddedDocumentToken>(tk)
        );

        auto obj = _section->add(
            std::make_shared<CodeBlock>(code_tk->content(), code_tk->langspec()));
        obj->range(tk->range());
        pop();
    }

private:
    std::shared_ptr<Section> _section;
};

//-------------------------------------------------------------------
class CompileSectionHeader;
class CompileSubSection;
class CompileSection : public CompileState {
public:
    CompileSection(std::shared_ptr<Section> section) : _section(section) { }

    const char* tracer_name() const {
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
        case Token::Type::INDEX:
            init_text_content();
            break;

        case Token::Type::CODE_BLOCK:
            push<CompileCodeBlock>(_section);
            break;

        case Token::Type::HEADER_START:
            if (is_subsection(tk)) {
                push<CompileSubSection>(_section);

            } else {
                _section->range().end = tk->begin();
                pop();
            }
            break;

        case Token::Type::OL_ITEM:
        case Token::Type::UL_ITEM:
            push<CompileTopLevelList>(_section);
            break;

        case Token::Type::NEWLINE:
            context().tokens.advance();
            _section->add(std::make_shared<LineBreak>())->range(tk->range());
            break;

        case Token::Type::END:
            _section->range().end = tk->end();
            pop();
            break;

        default:
            throw unexpected_token(tk, "parsing section");
        }
    }

private:
    bool is_subsection(token_t tk) {
        std::shared_ptr<parser::HeaderStartToken> header_tk = (
            dynamic_pointer_cast<parser::HeaderStartToken>(tk));
        return header_tk->level() > _section->level();
    }

    void init_text_content() {
        auto text_content = _section->add(std::make_shared<TextContent>());
        push<CompileTextContent>(text_content);
    }

    std::shared_ptr<Section> _section;
};

//-------------------------------------------------------------------
class CompileSectionHeader : public CompileState {
public:
    CompileSectionHeader(std::shared_ptr<Section> section) : _section(section) { }

    const char* tracer_name() const {
        return "SectionHeader";
    }

    void run() {
        if (! header_text_processed) {
            push<CompileTextContent>(_section->header(), Token::Type::HEADER_END);
            header_text_processed = true;

        } else {
            transition<CompileSection>(_section);
        }
    }

private:
    bool header_text_processed = false;
    std::shared_ptr<Section> _section;
};

//-------------------------------------------------------------------
class CompileSubSection : public CompileState {
public:
    CompileSubSection(std::shared_ptr<Section> parent) : _parent(parent) { }

    const char* tracer_name() const {
        return "SubSection";
    }

    void run() {
        token_t tk = context().tokens.get();
        std::shared_ptr<parser::HeaderStartToken> header_tk = (
            dynamic_pointer_cast<parser::HeaderStartToken>(tk));
        auto section = _parent->add(Section::create(
            header_tk->level()
        ));
        section->range().begin = tk->begin();

        transition<CompileSectionHeader>(section);
    }

private:
    std::shared_ptr<Section> _parent;
};

//-------------------------------------------------------------------
class CompileTopLevelSection : public CompileState {
public:
    const char* tracer_name() const {
        return "TopLevelSection";
    }

    void run() {
        token_t tk = context().tokens.get();
        std::shared_ptr<parser::HeaderStartToken> header_tk = (
            dynamic_pointer_cast<parser::HeaderStartToken>(tk));
        auto section = context().doc->add(Section::create(
            header_tk->level()
        ));
        section->range().begin = tk->begin();

        transition<CompileSectionHeader>(section);
    }
};

//-------------------------------------------------------------------
class CompileBegin : public CompileState {
    const char* tracer_name() const {
        return "Begin";
    }

    void run() {
        token_t tk = context().tokens.peek();

        if (context().doc->range().begin == NOWHERE) {
            context().doc->range().begin = Location{.filename = tk->begin().filename, .line = 0, .col = 0};
        }

        if (tk->type() == Token::Type::FRONT_MATTER) {
            push<CompileFrontMatter>();

        } else if (tk->type() == Token::Type::HEADER_START) {
            push<CompileTopLevelSection>();

        } else if (tk->type() == Token::Type::END) {
            context().doc->range().end = tk->end();
            terminate();

        } else {
            auto section = context().doc->add(Section::create());
            section->range().begin = tk->begin();
            push<CompileSection>(section);
        }
    }
};

//-------------------------------------------------------------------
class Compiler {
public:
    template<class T>
    std::shared_ptr<Document> compile(T true_begin, T true_end) {
        auto begin = moonlight::gen::wrap(true_begin, true_end);
        auto end = moonlight::gen::end<typename T::value_type>();

        Context ctx {
            .doc = std::make_shared<Document>(),
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
            (void) event;
            std::vector<const char*> stack_names = moonlight::collect::map<const char*>(
                stack, [](auto state) {
                    return state->tracer_name();
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
            std::string old_state_name = old_state == nullptr ? "(null)" : old_state->tracer_name();
            std::string new_state_name = new_state == nullptr ? "(null)" : new_state->tracer_name();
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
