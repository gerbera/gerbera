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

#include "import_script.h"
#include "config_manager.h"
#include "js_functions.h"

using namespace zmm;

ImportScript::ImportScript(Ref<Runtime> runtime) : Script(runtime)
{
    String scriptPath = ConfigManager::getInstance()->getOption(CFG_IMPORT_SCRIPTING_IMPORT_SCRIPT); 

#ifdef JS_THREADSAFE
    JS_SetContextThread(cx);
    JS_BeginRequest(cx);
#endif

    try 
    {
        load(scriptPath);
        JS_AddNamedObjectRoot(cx, &script, "ImportScript");
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

void ImportScript::processCdsObject(Ref<CdsObject> obj, String scriptpath)
{
#ifdef JS_THREADSAFE
    JS_SetContextThread(cx);
    JS_BeginRequest(cx);
#endif
    processed = obj;
    try 
    {
        JSObject *orig = JS_NewObject(cx, NULL, NULL, glob);
        setObjectProperty(glob, _("orig"), orig);
        cdsObject2jsObject(obj, orig);
        setProperty(glob, _("object_script_path"), scriptpath);
        execute();
    }
    catch (Exception ex)
    {
        processed = nil;
#ifdef JS_THREADSAFE
        JS_EndRequest(cx);
        JS_ClearContextThread(cx);
#endif
        throw ex;
    }

    processed = nil;

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

ImportScript::~ImportScript()
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
