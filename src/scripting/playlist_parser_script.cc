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
#include "js_functions.h"

#define ONE_TEXTLINE_BYTES  1024

using namespace zmm;

extern "C" {

static JSBool
js_readln(JSContext *cx, uintN argc, jsval *vp)
{
    PlaylistParserScript *self = (PlaylistParserScript *)JS_GetContextPrivate(cx);

    String line;
    
    try
    {
        line = self->readln();
    }
    catch (ServerShutdownException se)
    {
        log_warning("Aborting script execution due to server shutdown.\n");
        return JS_FALSE;
    }
    catch (Exception e)
    {
        e.printStackTrace();
        return JS_TRUE;
    }

    JSString *jsline = JS_NewStringCopyZ(cx, line.c_str());

    JS_SET_RVAL(cx, vp, STRING_TO_JSVAL(jsline));
  
    return JS_TRUE;
}
    
} // extern "C"

PlaylistParserScript::PlaylistParserScript(Ref<Runtime> runtime) : Script(runtime)
{
    currentHandle = NULL;
    currentObjectID = INVALID_OBJECT_ID;
    currentLine = NULL;
 
#ifdef JS_THREADSAFE
    JS_SetContextThread(cx);
    JS_BeginRequest(cx);
#endif
  
    try
    {
        defineFunction(_("readln"), js_readln, 0);

        String scriptPath = ConfigManager::getInstance()->getOption(CFG_IMPORT_SCRIPTING_PLAYLIST_SCRIPT); 
        load(scriptPath);
        JS_AddNamedObjectRoot(cx, &script, "PlaylistScript");
    }
    catch (Exception ex)
    {
#ifdef JS_THREADSAFE
        JS_EndRequest(cx);
        JS_ClearContextThread(cx);
#endif
        throw ex;
    }
    
#ifdef JS_THREADSAFE
        JS_EndRequest(cx);
        JS_ClearContextThread(cx);
#endif
}

String PlaylistParserScript::readln()
{
    String ret;
    if (!currentHandle)
        throw _Exception(_("Readline not yet setup for use"));

    if ((currentTask != nil) && (!currentTask->isValid()))
        return nil;

    while (true)
    {
        if(fgets(currentLine, ONE_TEXTLINE_BYTES, currentHandle) == NULL)
            return nil;

        ret = trim_string(currentLine);
        if (string_ok(ret))
            return ret;
    }
}

void PlaylistParserScript::processPlaylistObject(zmm::Ref<CdsObject> obj, Ref<GenericTask> task)
{
#ifdef JS_THREADSAFE
    JS_SetContextThread(cx);
    JS_BeginRequest(cx);
#endif
    if ((currentObjectID != INVALID_OBJECT_ID) || (currentHandle != NULL) ||
            (currentLine != NULL))
    {
#ifdef JS_THREADSAFE
        JS_EndRequest(cx);
        JS_ClearContextThread(cx);
#endif
        throw _Exception(_("recursion not allowed!"));
    }

    if (!IS_CDS_PURE_ITEM(obj->getObjectType()))
    {
#ifdef JS_THREADSAFE
        JS_EndRequest(cx);
        JS_ClearContextThread(cx);
#endif
        throw _Exception(_("only allowed for pure items"));
    }

    currentTask = task;
    currentObjectID = obj->getID();
    currentLine = (char *)MALLOC(ONE_TEXTLINE_BYTES);
    if (!currentLine)
    {
        currentObjectID = INVALID_OBJECT_ID;
        currentTask = nil;
#ifdef JS_THREADSAFE
        JS_EndRequest(cx);
        JS_ClearContextThread(cx);
#endif
        throw _Exception(_("failed to allocate memory for playlist parsing!"));
    }

    currentLine[0] = '\0';

    currentHandle = fopen(obj->getLocation().c_str(), "r");
    if (!currentHandle)
    {
        currentObjectID = INVALID_OBJECT_ID;
        currentTask = nil;
        FREE(currentLine);
#ifdef JS_THREADSAFE
        JS_EndRequest(cx);
        JS_ClearContextThread(cx);
#endif
        throw _Exception(_("failed to open file: ") + obj->getLocation());
    }

    JSObject *playlist = JS_NewObject(cx, NULL, NULL, glob);

    try
    {
        setObjectProperty(glob, _("playlist"), playlist);
        cdsObject2jsObject(obj, playlist);

        execute();
    }

    catch (Exception e)
    {
        fclose(currentHandle);
        currentHandle = NULL;

        FREE(currentLine);
        currentLine = NULL;

        currentObjectID = INVALID_OBJECT_ID;
        currentTask = nil;

#ifdef JS_THREADSAFE
        JS_EndRequest(cx);
        JS_ClearContextThread(cx);
#endif
        throw e;
    }

    fclose(currentHandle);
    currentHandle = NULL;

    FREE(currentLine);
    currentLine = NULL;

    currentObjectID = INVALID_OBJECT_ID;
    currentTask = nil;

    gc_counter++;
    if (gc_counter > JS_CALL_GC_AFTER_NUM)
    {
        JS_MaybeGC(cx);
        gc_counter = 0;
    }

#ifdef JS_THREADSAFE
    JS_EndRequest(cx);
    JS_ClearContextThread(cx);
#endif

}


PlaylistParserScript::~PlaylistParserScript()
{
#ifdef JS_THREADSAFE
    JS_SetContextThread(cx);
    JS_BeginRequest(cx);
#endif

    if (script)
        JS_RemoveObjectRoot(cx, &script);

#ifdef JS_THREADSAFE
    JS_EndRequest(cx);
    JS_ClearContextThread(cx);
#endif

}
#endif // HAVE_JS
