/*MT*
    
    MediaTomb - http://www.mediatomb.cc/
    
    import_script.cc - this file is part of MediaTomb.
    
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

/// \file import_script.cc

#ifdef HAVE_CONFIG_H
    #include "autoconfig.h"
#endif

#ifdef HAVE_JS

#include "import_script.h"
#include "storage.h"
#include "content_manager.h"
#include "metadata_handler.h"
#include "config_manager.h"
#include "tools.h"
#include "string_converter.h"

using namespace zmm;

/* native js functions */
extern "C" {

static JSBool
js_print(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval)
{
    uintN i;
    JSString *str;

    for (i = 0; i < argc; i++) {
        str = JS_ValueToString(cx, argv[i]);
        if (!str)
            return JS_FALSE;
        log_js("%s\n", JS_GetStringBytes(str));
    }
    return JS_TRUE;
}

static JSBool
js_copyObject(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval)
{
    jsval arg;
    JSObject *js_cds_obj;
    JSObject *js_cds_clone_obj;

    ImportScript *self = (ImportScript *)JS_GetContextPrivate(cx);

    try
    {
        arg = argv[0];
        if (!JSVAL_IS_OBJECT(arg))
            return JS_FALSE;
        if (!JS_ValueToObject(cx, arg, &js_cds_obj))
            return JS_FALSE;

        Ref<CdsObject> cds_obj = self->jsObject2cdsObject(js_cds_obj);
        js_cds_clone_obj = JS_NewObject(cx, NULL, NULL, NULL);
        self->cdsObject2jsObject(cds_obj, js_cds_clone_obj);

        *rval = OBJECT_TO_JSVAL(js_cds_clone_obj);

        return JS_TRUE;

    }
    catch (Exception e)
    {
        e.printStackTrace();
    }
    return JS_FALSE;
}

static JSBool
js_addCdsObject(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval)
{
    try
    {
        jsval arg;
        JSString *str;
        String path;
        String containerclass;

        JSObject *js_cds_obj;

        ImportScript *self = (ImportScript *)JS_GetContextPrivate(cx);

        arg = argv[0];
        if (!JSVAL_IS_OBJECT(arg))
            return JS_FALSE;
        if (!JS_ValueToObject(cx, arg, &js_cds_obj))
            return JS_FALSE;

        str = JS_ValueToString(cx, argv[1]);
        if (!str)
            path = _("/");
        else
            path = String(JS_GetStringBytes(str));

        JSString *cont = JS_ValueToString(cx, argv[2]);
        if (cont)
        {
            containerclass = String(JS_GetStringBytes(cont));
            if (!string_ok(containerclass) || containerclass == "undefined")
                containerclass = nil;
        }

        Ref<CdsObject> cds_obj = self->jsObject2cdsObject(js_cds_obj);
        Ref<ContentManager> cm = ContentManager::getInstance();

        int id = cm->addContainerChain(path, containerclass);
        cds_obj->setParentID(id);
        if (!IS_CDS_ITEM_EXTERNAL_URL(cds_obj->getObjectType()) &&
            !IS_CDS_ITEM_INTERNAL_URL(cds_obj->getObjectType()))
        {
            cds_obj->setFlag(OBJECT_FLAG_USE_RESOURCE_REF);
        }

        cds_obj->setID(INVALID_OBJECT_ID);
        cm->addObject(cds_obj);

        /* setting object ID as return value */
        String tmp = String::from(id);

        JSString *str2 = JS_NewStringCopyN(cx, tmp.c_str(), tmp.length());
        if (!str2)
            return JS_FALSE;
        *rval = STRING_TO_JSVAL(str2);

        return JS_TRUE;
    }
    catch (Exception e)
    {
        e.printStackTrace();
    }
    return JS_FALSE;
}


} // extern "C"

static JSFunctionSpec global_functions[] = {
    {"print",           js_print,          0},
    {"addCdsObject",    js_addCdsObject,   3},
    {"copyObject",      js_copyObject,     1},
    {0}
};


/* **************** */

ImportScript::ImportScript(Ref<Runtime> runtime) : Script(runtime)
{
    JS_SetContextPrivate(cx, this);
    defineFunctions(global_functions);
    
    /* initialize contstants */
    setIntProperty(glob, _("OBJECT_TYPE_CONTAINER"),
                          OBJECT_TYPE_CONTAINER);
    setIntProperty(glob, _("OBJECT_TYPE_ITEM"),
                          OBJECT_TYPE_ITEM);
    setIntProperty(glob, _("OBJECT_TYPE_ACTIVE_ITEM"),
                          OBJECT_TYPE_ACTIVE_ITEM);
    setIntProperty(glob, _("OBJECT_TYPE_ITEM_EXTERNAL_URL"),
                          OBJECT_TYPE_ITEM_EXTERNAL_URL);
    setIntProperty(glob, _("OBJECT_TYPE_ITEM_INTERNAL_URL"),
                          OBJECT_TYPE_ITEM_INTERNAL_URL);


    for (int i = 0; i < M_MAX; i++)
    {
        setProperty(glob, _(MT_KEYS[i].sym), _(MT_KEYS[i].upnp));
    }

    setProperty(glob, _("UPNP_CLASS_CONTAINER_MUSIC"), 
                       _(UPNP_DEFAULT_CLASS_MUSIC_CONT));
    setProperty(glob, _("UPNP_CLASS_CONTAINER_MUSIC_ALBUM"), 
                       _(UPNP_DEFAULT_CLASS_MUSIC_ALBUM));
    setProperty(glob, _("UPNP_CLASS_CONTAINER_MUSIC_ARTIST"), 
                      _(UPNP_DEFAULT_CLASS_MUSIC_ARTIST));
    setProperty(glob, _("UPNP_CLASS_CONTAINER_MUSIC_GENRE"), 
                       _(UPNP_DEFAULT_CLASS_MUSIC_GENRE));
    setProperty(glob, _("UPNP_CLASS_CONTAINER"), 
                      _(UPNP_DEFAULT_CLASS_CONTAINER));
    setProperty(glob, _("UPNP_CLASS_ITEM"), _(UPNP_DEFAULT_CLASS_ITEM));
    setProperty(glob, _("UPNP_CLASS_ITEM_MUSIC_TRACK"), 
                       _(UPNP_DEFAULT_CLASS_MUSIC_TRACK));
    setProperty(glob, _("UPNP_CLASS_PLAYLIST_CONTAINER"),
                       _(UPNP_DEFAULT_CLASS_PLAYLIST_CONTAINER));

    
    String scriptPath = ConfigManager::getInstance()->getOption(_("/import/virtual-layout/script")); 
    load(scriptPath);
}

void ImportScript::processCdsObject(Ref<CdsObject> obj)
{
   JSObject *orig = JS_NewObject(cx, NULL, NULL, glob);
   setObjectProperty(glob, _("orig"), orig);
   cdsObject2jsObject(obj, orig);
   execute();
}

#endif // HAVE_JS

