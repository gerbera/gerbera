/*MT*
    
    MediaTomb - http://www.mediatomb.cc/
    
    js_functions.h - this file is part of MediaTomb.
    
    Copyright (C) 2005 Gena Batyan <bgeradz@mediatomb.cc>,
                       Sergey 'Jin' Bostandzhyan <jin@mediatomb.cc>
    
    Copyright (C) 2006-2010 Gena Batyan <bgeradz@mediatomb.cc>,
                            Sergey 'Jin' Bostandzhyan <jin@mediatomb.cc>,
                            Leonhard Wimmer <leo@mediatomb.cc>
    
    MediaTomb is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License version 2
    as published by the Free Software Foundation.
    
    MediaTomb is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.
    
    You should have received a copy of the GNU General Public License
    version 2 along with MediaTomb; if not, write to the Free Software
    Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301, USA.
    
    $Id$
*/

/// \file js_functions.h
/// \brief These functions can be called from scripts.

#ifndef __SCRIPTING_JS_FUNCTIONS_H__
#define __SCRIPTING_JS_FUNCTIONS_H__

#include <duktape.h>

extern "C" {

/// \brief Log output.
duk_ret_t js_print(duk_context* ctx);

/// \brief Adds an object to the database.
duk_ret_t js_addCdsObject(duk_context* ctx);

/// \brief Creates a tree of containers.
duk_ret_t js_addContainerTree(duk_context* ctx);

/// \brief Makes a copy of an CDS object.
duk_ret_t js_copyObject(duk_context* ctx);

/// filesystem charset to internal
duk_ret_t js_f2i(duk_context* ctx);
/// metadata charset to internal
duk_ret_t js_m2i(duk_context* ctx);
/// playlist charset to internal
duk_ret_t js_p2i(duk_context* ctx);
/// js charset to internal
duk_ret_t js_j2i(duk_context* ctx);

#define log_debug_stack(ctx)                            \
    do {                                                \
        duk_push_context_dump(ctx);                     \
        log_debug("{}\n", duk_safe_to_string(ctx, -1)); \
        duk_pop(ctx);                                   \
    } while (0)
} // extern "C"

#endif //__SCRIPTING_JS_FUNCTIONS_H__
