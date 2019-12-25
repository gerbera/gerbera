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

#include "playlist_parser_script.h"
#include "config_manager.h"
#include "content_manager.h"
#include "js_functions.h"

#define ONE_TEXTLINE_BYTES  1024

using namespace zmm;

extern "C" {

static duk_ret_t
js_readln(duk_context *ctx)
{
    auto *self = (PlaylistParserScript *)Script::getContextScript(ctx);

    std::string line;
    
    try
    {
        line = self->readln();
    }
    catch (const ServerShutdownException & se)
    {
        log_warning("Aborting script execution due to server shutdown.\n");
        return duk_error(ctx, DUK_ERR_ERROR, "Aborting script execution due to server shutdown.");
    }
    catch (const Exception & e)
    {
        e.printStackTrace();
        return 0;
    }

    duk_push_string(ctx, line.c_str());
    return 1;
}

static duk_ret_t
js_getCdsObject(duk_context *ctx)
{
    auto *self = (PlaylistParserScript *)Script::getContextScript(ctx);

    if (!duk_is_string(ctx, 0))
        return 0;

    std::string path = duk_to_string(ctx, 0);
    duk_pop(ctx);

    if (!string_ok(path))
        return 0;

    Ref<Storage> storage = Storage::getInstance();
    Ref<CdsObject> obj = storage->findObjectByPath(path);
    if (obj == nullptr) {
        Ref<ContentManager> cm = ContentManager::getInstance();
        obj = cm->createObjectFromFile(path);
        if (obj == nullptr) // object ignored
            return 0;
    }
    self->cdsObject2dukObject(obj);
    return 1;
}

} // extern "C"

PlaylistParserScript::PlaylistParserScript(Ref<Runtime> runtime) : Script(runtime, "playlist")
{
    currentHandle = nullptr;
    currentObjectID = INVALID_OBJECT_ID;
    currentLine = nullptr;
 
    try
    {
        AutoLock lock(runtime->getMutex());
        defineFunction("readln", js_readln, 0);
        defineFunction("getCdsObject", js_getCdsObject, 1);

        std::string scriptPath = ConfigManager::getInstance()->getOption(CFG_IMPORT_SCRIPTING_PLAYLIST_SCRIPT); 
        load(scriptPath);
    }
    catch (const Exception & ex)
    {
        throw ex;
    }
    
}

std::string PlaylistParserScript::readln()
{
    std::string ret;
    if (!currentHandle)
        throw _Exception("Readline not yet setup for use");

    if ((currentTask != nullptr) && (!currentTask->isValid()))
        return nullptr;

    while (true)
    {
        if(fgets(currentLine, ONE_TEXTLINE_BYTES, currentHandle) == nullptr)
            return nullptr;

        ret = trim_string(currentLine);
        if (string_ok(ret))
            return ret;
    }
}

void PlaylistParserScript::processPlaylistObject(zmm::Ref<CdsObject> obj, Ref<GenericTask> task)
{
    if ((currentObjectID != INVALID_OBJECT_ID) || (currentHandle != nullptr) ||
            (currentLine != nullptr))
    {
        throw _Exception("recursion not allowed!");
    }

    if (!IS_CDS_PURE_ITEM(obj->getObjectType()))
    {
        throw _Exception("only allowed for pure items");
    }

    currentTask = task;
    currentObjectID = obj->getID();
    currentLine = (char *)MALLOC(ONE_TEXTLINE_BYTES);
    if (!currentLine)
    {
        currentObjectID = INVALID_OBJECT_ID;
        currentTask = nullptr;
        throw _Exception("failed to allocate memory for playlist parsing!");
    }

    currentLine[0] = '\0';

    currentHandle = fopen(obj->getLocation().c_str(), "r");
    if (!currentHandle)
    {
        currentObjectID = INVALID_OBJECT_ID;
        currentTask = nullptr;
        FREE(currentLine);
        throw _Exception("failed to open file: " + obj->getLocation());
    }

    AutoLock lock(runtime->getMutex());

    try
    {
        cdsObject2dukObject(obj);
        duk_put_global_string(ctx, "playlist");

        execute();

        duk_push_global_object(ctx);
        duk_del_prop_string(ctx, -1, "playlist");
        duk_pop(ctx);
    }

    catch (const Exception & e)
    {
        duk_push_global_object(ctx);
        duk_del_prop_string(ctx, -1, "playlist");
        duk_pop(ctx);

        fclose(currentHandle);
        currentHandle = nullptr;

        FREE(currentLine);
        currentLine = nullptr;

        currentObjectID = INVALID_OBJECT_ID;
        currentTask = nullptr;

        throw e;
    }

    fclose(currentHandle);
    currentHandle = nullptr;

    FREE(currentLine);
    currentLine = nullptr;

    currentObjectID = INVALID_OBJECT_ID;
    currentTask = nullptr;

    gc_counter++;
    if (gc_counter > JS_CALL_GC_AFTER_NUM)
    {
        duk_gc(ctx, 0);
        gc_counter = 0;
    }

}


PlaylistParserScript::~PlaylistParserScript()
{
}
#endif // HAVE_JS
