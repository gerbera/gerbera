/*MT*
    
    MediaTomb - http://www.mediatomb.cc/
    
    js_functions.cc - this file is part of MediaTomb.
    
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

#ifdef HAVE_CONFIG_H
    #include "autoconfig.h"
#endif

#ifdef HAVE_JS

#include "js_functions.h"

#include "script.h"
#include "storage.h"
#include "content_manager.h"
#include "metadata_handler.h"

using namespace zmm;

extern "C" {

JSBool 
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

JSBool
js_copyObject(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval)
{
    jsval arg;
    JSObject *js_cds_obj;
    JSObject *js_cds_clone_obj;

    Script *self = (Script *)JS_GetPrivate(cx, obj);

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

JSBool
js_addCdsObject(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval)
{
    try
    {
        jsval arg;
        JSString *str;
        String path;
        String containerclass;

        JSObject *js_cds_obj;

        Script *self = (Script *)JS_GetPrivate(cx, obj);

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

 JSBool
js_readln(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval)
{
/*    if (!currentHandle)
    {
        if (currentObject == -1)
        {
            log_js("can not determine current object\n");
            return JS_FALSE;
        }

        try
        {
            Ref<Storage> storage = Storage::getInstance();
            Ref<CdsObject> obj = storage->loadObject(currentObject);

            if (!IS_CDS_PURE_ITEM(obj->getObjectType()))
            {
                log_js("read opearation is only allowed on items\n");
                return JS_FALSE;
            }

            Ref<CdsItem> item = RefCast(obj, CdsItem);
            currentHandle = fopen(item->getLocation().c_str(), "r");

            if (!currentHandle)
            {
                log_js("Could not open \"%s\": %s\n",
                        item->getLocation().c_str(), strerror(errno));
                return JS_FALSE;
            }
        }
        catch (Exception e)
        {
            e.printStackTrace();
            return JS_FALSE;
        }
    }

    if (!currentLine)
        currentLine = (char *)MALLOC(1024);

    currentLine[0] = '\0';

    fgets(currentLine, 1024, currentHandle);

    JSString *line = JS_NewStringCopyZ(cx, currentLine);

    if (!line)
        return JS_FALSE;

    *rval = STRING_TO_JSVAL(line);
*/
    return JS_TRUE;
}


} // extern "C"

#endif //HAVE_JS
