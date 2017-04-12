/*MT*
    
    MediaTomb - http://www.mediatomb.cc/
    
    runtime.cc - this file is part of MediaTomb.
    
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

/// \file runtime.cc

#ifdef HAVE_JS

#include "runtime.h"

using namespace zmm;
using namespace std;

static void fatal_handler(void *udata, const char *msg)
{
    log_error("Fatal Duktape error: %s\n", msg ? msg : "no message");
    abort();
}

Runtime::Runtime() {
    ctx = duk_create_heap(nullptr, nullptr, nullptr, nullptr, fatal_handler);
}
Runtime::~Runtime()
{
    duk_destroy_heap(ctx);
}

duk_context *Runtime::createContext()
{
    duk_idx_t thread_idx = duk_push_thread_new_globalenv(ctx);
    return duk_get_context(ctx, thread_idx);
    /* we're leaving the thread on the stack of the main context so
       it stays alive */
}

#endif // HAVE_JS
