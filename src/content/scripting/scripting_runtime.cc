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

/// \file scripting_runtime.cc

#ifdef HAVE_JS
#define LOG_FAC log_facility_t::script

#include "scripting_runtime.h" // API

ScriptingRuntime::ScriptingRuntime()
    : ctx(duk_create_heap(nullptr, nullptr, nullptr, nullptr, [](auto, auto msg) { log_error("Fatal Duktape error: {}", msg ? msg : "no message"); std::abort(); }))
{
}

ScriptingRuntime::~ScriptingRuntime()
{
    duk_destroy_heap(ctx);
}

duk_context* ScriptingRuntime::createContext(const std::string& name)
{
    duk_push_heap_stash(ctx);
    duk_idx_t threadIdx = duk_push_thread_new_globalenv(ctx);
    duk_context* newctx = duk_get_context(ctx, threadIdx);
    duk_put_prop_string(ctx, -2, name.c_str());
    duk_pop(ctx);
    return newctx;
}

void ScriptingRuntime::destroyContext(const std::string& name)
{
    duk_push_heap_stash(ctx);
    duk_del_prop_string(ctx, -1, name.c_str());
    duk_pop(ctx);
}

#endif // HAVE_JS
