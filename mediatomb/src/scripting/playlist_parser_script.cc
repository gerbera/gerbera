/*MT*
    
    MediaTomb - http://www.mediatomb.cc/
    
    playlist_parser_script.cc - this file is part of MediaTomb.
    
    Copyright (C) 2005 Gena Batyan <bgeradz@mediatomb.cc>,
                       Sergey 'Jin' Bostandzhyan <jin@mediatomb.cc>
    
    Copyright (C) 2006-2007 Gena Batyan <bgeradz@mediatomb.cc>,
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

#ifdef HAVE_CONFIG_H
    #include "autoconfig.h"
#endif

#ifdef HAVE_JS

#include "playlist_parser_script.h"
#include "config_manager.h"
#include "js_functions.h"

#define ONE_TEXTLINE_BYTES  1024

using namespace zmm;

extern "C" {

static JSBool
js_readln(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval)
{
    PlaylistParserScript *self = (PlaylistParserScript *)JS_GetPrivate(cx, obj);

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

    *rval = STRING_TO_JSVAL(jsline);
  
    return JS_TRUE;
}
    
} // extern "C"

PlaylistParserScript::PlaylistParserScript(Ref<Runtime> runtime) : Script(runtime)
{
    currentHandle = NULL;
    currentObjectID = INVALID_OBJECT_ID;
    currentLine = NULL;
    
    defineFunction(_("readln"), js_readln, 0);
    
    String scriptPath = ConfigManager::getInstance()->getOption(CFG_IMPORT_SCRIPTING_PLAYLIST_SCRIPT); 
    load(scriptPath);
    root = JS_NewScriptObject(cx, script);
    JS_AddNamedRoot(cx, &root, "PlaylistScript");
}

String PlaylistParserScript::readln()
{
    String ret;
    if (!currentHandle)
        throw _Exception(_("Readline not yet setup for use"));

    if ((currentTask == nil) || (!currentTask->isValid()))
        return nil;

    while (true)
    {
        if(fgets(currentLine, ONE_TEXTLINE_BYTES, currentHandle) == NULL)
            return nil;

        ret = trim_string(String(currentLine));
        if (string_ok(ret))
            return ret;
    }
}

void PlaylistParserScript::processPlaylistObject(zmm::Ref<CdsObject> obj, Ref<CMTask> task)
{

   if ((currentObjectID != INVALID_OBJECT_ID) || (currentHandle != NULL) ||
       (currentLine != NULL))
       throw _Exception(_("recursion not allowed!"));

   if (!IS_CDS_PURE_ITEM(obj->getObjectType()))
   {
       throw _Exception(_("only allowed for pure items"));
   }

   currentTask = task;
   currentObjectID = obj->getID();
   currentLine = (char *)MALLOC(ONE_TEXTLINE_BYTES);
   if (!currentLine)
   {
       currentObjectID = INVALID_OBJECT_ID;
       currentTask = nil;
       throw _Exception(_("failed to allocate memory for playlist parsing!"));
   }
   currentLine[0] = '\0';

   currentHandle = fopen(obj->getLocation().c_str(), "r");
   if (!currentHandle)
   {
       currentObjectID = INVALID_OBJECT_ID;
       currentTask = nil;
       FREE(currentLine);
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

       throw e;
   }

   fclose(currentHandle);
   currentHandle = NULL;

   FREE(currentLine);
   currentLine = NULL;

   currentObjectID = INVALID_OBJECT_ID;
   currentTask = nil;
}


PlaylistParserScript::~PlaylistParserScript()
{
    if (root)
        JS_RemoveRoot(cx, &root);
}
#endif // HAVE_JS
