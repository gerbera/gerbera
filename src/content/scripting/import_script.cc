/*MT*
    
    MediaTomb - http://www.mediatomb.cc/
    
    import_script.cc - this file is part of MediaTomb.
    
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

/// \file import_script.cc

#ifdef HAVE_JS
#include "import_script.h" // API

#include "config/config_manager.h"
#include "content/content_manager.h"
#include "js_functions.h"

ImportScript::ImportScript(std::shared_ptr<ContentManager> content,
    const std::shared_ptr<ScriptingRuntime>& runtime)
    : Script(std::move(content), runtime, "import")
{
    std::string scriptPath = config->getOption(CFG_IMPORT_SCRIPTING_IMPORT_SCRIPT);

    try {
        load(scriptPath);
    } catch (const std::runtime_error& ex) {
        throw ex;
    }
}

void ImportScript::processCdsObject(const std::shared_ptr<CdsObject>& obj, const std::string& scriptpath)
{
    processed = obj;
    try {
        cdsObject2dukObject(obj);
        duk_put_global_string(ctx, "orig");
        duk_push_string(ctx, scriptpath.c_str());
        duk_put_global_string(ctx, "object_script_path");
        auto autoScan = content->getAutoscanDirectory(scriptpath);
        if (autoScan && !scriptpath.empty()) {
            duk_push_sprintf(ctx, "%d", autoScan->getScanID());
            duk_put_global_string(ctx, "object_autoscan_id");
        }
        execute();
        duk_push_global_object(ctx);
        duk_del_prop_string(ctx, -1, "orig");
        duk_del_prop_string(ctx, -1, "object_script_path");
        duk_del_prop_string(ctx, -1, "object_autoscan_id");
        duk_pop(ctx);
    } catch (const std::runtime_error& ex) {
        duk_push_global_object(ctx);
        duk_del_prop_string(ctx, -1, "orig");
        duk_del_prop_string(ctx, -1, "object_script_path");
        duk_del_prop_string(ctx, -1, "object_autoscan_id");
        processed = nullptr;
        throw ex;
    }

    processed = nullptr;

    gc_counter++;
    if (gc_counter > JS_CALL_GC_AFTER_NUM) {
        duk_gc(ctx, 0);
        gc_counter = 0;
    }
}
#endif // HAVE_JS
