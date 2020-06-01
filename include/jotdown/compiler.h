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

using jotdown::parser::token_t;
using jotdown::parser::Iterator;

//-------------------------------------------------------------------
class BufferedTokens {
public:
    BufferedTokens(Iterator& begin, Iterator& end)
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
    Iterator _current;
    const Iterator _end;
    std::deque<jotdown::parser::token_t> _buffer;
};

//-------------------------------------------------------------------
class Compiler {
public:

private:
    object::Document doc;
};

}
}

#endif /* !__JOTDOWN_COMPILER_H */
