/*MT*
    
    MediaTomb - http://www.mediatomb.cc/
    
    playlist_parser_script.cc - this file is part of MediaTomb.
    
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

/// \file playlist_parser_script.cc

#ifdef HAVE_JS
#include "playlist_parser_script.h" // API

#include "config/config_manager.h"
#include "content/content_manager.h"
#include "database/database.h"
#include "js_functions.h"
#include "scripting_runtime.h"

#define ONE_TEXTLINE_BYTES 1024

extern "C" {

static duk_ret_t
jsReadln(duk_context* ctx)
{
    auto self = dynamic_cast<PlaylistParserScript*>(Script::getContextScript(ctx));
    if (!self) {
        return 0;
    }

    std::string line;

    try {
        line = self->readln();
    } catch (const ServerShutdownException& se) {
        log_warning("Aborting script execution due to server shutdown.");
        return duk_error(ctx, DUK_ERR_ERROR, "Aborting script execution due to server shutdown.");
    } catch (const std::runtime_error& e) {
        log_error("DUK exception: {}", e.what());
        return 0;
    }

    duk_push_string(ctx, line.c_str());
    return 1;
}

static duk_ret_t
jsGetCdsObject(duk_context* ctx)
{
    auto self = dynamic_cast<PlaylistParserScript*>(Script::getContextScript(ctx));
    if (!self) {
        return 0;
    }

    if (!duk_is_string(ctx, 0))
        return 0;

    std::error_code ec;
    fs::path path = fs::weakly_canonical(duk_to_string(ctx, 0), ec);
    duk_pop(ctx);

    if (ec || path.empty())
        return 0;

    auto database = self->getDatabase();
    auto obj = database->findObjectByPath(path);
    if (!obj) {
        auto cm = self->getContent();
        ec.clear();
        auto dirEnt = fs::directory_entry(path, ec);
        if (!ec) {
            obj = cm->createObjectFromFile(dirEnt, false);
        } else {
            log_error("Failed to read {}: {}", path.c_str(), ec.message());
        }
        if (!obj) { // object ignored
            return 0;
        }
    }
    self->cdsObject2dukObject(obj);
    return 1;
}

} // extern "C"

PlaylistParserScript::PlaylistParserScript(std::shared_ptr<ContentManager> content,
    const std::shared_ptr<ScriptingRuntime>& runtime)
    : Script(std::move(content), runtime, "playlist")
{
    try {
        ScriptingRuntime::AutoLock lock(runtime->getMutex());
        defineFunction("readln", jsReadln, 0);
        defineFunction("getCdsObject", jsGetCdsObject, 1);

        std::string scriptPath = config->getOption(CFG_IMPORT_SCRIPTING_PLAYLIST_SCRIPT);
        load(scriptPath);
    } catch (const std::runtime_error& ex) {
        throw ex;
    }
}

std::string PlaylistParserScript::readln()
{
    std::string ret;
    if (!currentHandle)
        throw_std_runtime_error("Readline not yet setup for use");

    if ((currentTask) && (!currentTask->isValid()))
        return {};

    while (true) {
        if (!fgets(currentLine, ONE_TEXTLINE_BYTES, currentHandle))
            return {};

        ret = trimString(currentLine);
        if (!ret.empty())
            return ret;
    }
}

void PlaylistParserScript::processPlaylistObject(const std::shared_ptr<CdsObject>& obj, std::shared_ptr<GenericTask> task, const std::string& scriptpath)
{
    if ((currentObjectID != INVALID_OBJECT_ID) || (currentHandle) || (currentLine)) {
        throw_std_runtime_error("recursion not allowed");
    }

    if (!obj->isPureItem()) {
        throw_std_runtime_error("only allowed for pure items");
    }

    currentTask = std::move(task);
    currentObjectID = obj->getID();
    currentLine = new char[ONE_TEXTLINE_BYTES];
    currentLine[0] = '\0';

#ifdef __linux__
    currentHandle = std::fopen(obj->getLocation().c_str(), "re");
#else
    currentHandle = std::fopen(obj->getLocation().c_str(), "r");
#endif
    if (!currentHandle) {
        currentObjectID = INVALID_OBJECT_ID;
        currentTask = nullptr;
        delete[] currentLine;
        throw_std_runtime_error("Failed to open file: {}", obj->getLocation().c_str());
    }

    ScriptingRuntime::AutoLock lock(runtime->getMutex());
    try {
        cdsObject2dukObject(obj);
        duk_put_global_string(ctx, "playlist");
        duk_push_string(ctx, scriptpath.c_str());
        duk_put_global_string(ctx, "object_script_path");
        auto autoScan = content->getAutoscanDirectory(scriptpath);
        if (autoScan && !scriptpath.empty()) {
            duk_push_sprintf(ctx, "%d", autoScan->getScanID());
            duk_put_global_string(ctx, "object_autoscan_id");
        }

        execute();

        duk_push_global_object(ctx);
        duk_del_prop_string(ctx, -1, "playlist");
        duk_del_prop_string(ctx, -1, "object_script_path");
        duk_del_prop_string(ctx, -1, "object_autoscan_id");
        duk_pop(ctx);
    } catch (const std::runtime_error& e) {
        duk_push_global_object(ctx);
        duk_del_prop_string(ctx, -1, "playlist");
        duk_del_prop_string(ctx, -1, "object_script_path");
        duk_del_prop_string(ctx, -1, "object_autoscan_id");
        duk_pop(ctx);

        std::fclose(currentHandle);
        currentHandle = nullptr;

        delete[] currentLine;
        currentLine = nullptr;

        currentObjectID = INVALID_OBJECT_ID;
        currentTask = nullptr;

        throw e;
    }

    std::fclose(currentHandle);
    currentHandle = nullptr;

    delete[] currentLine;
    currentLine = nullptr;

    currentObjectID = INVALID_OBJECT_ID;
    currentTask = nullptr;

    gc_counter++;
    if (gc_counter > JS_CALL_GC_AFTER_NUM) {
        duk_gc(ctx, 0);
        gc_counter = 0;
    }
}
#endif // HAVE_JS
