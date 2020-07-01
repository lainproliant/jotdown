/*
 * error.h
 *
 * Author: Lain Musgrove (lain.proliant@gmail.com)
 * Date: Tuesday May 26, 2020
 *
 * Distributed under terms of the MIT license.
 */

#ifndef __JOTDOWN_ERROR_H
#define __JOTDOWN_ERROR_H

#include "moonlight/exceptions.h"

namespace jotdown {

// ------------------------------------------------------------------
class JotdownError : public moonlight::core::Exception {
    using Exception::Exception;
};

}

#endif /* !__JOTDOWN_ERROR_H */
