/*MT*
    
    MediaTomb - http://www.mediatomb.cc/
    
    js_functions.cc - this file is part of MediaTomb.
    
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

/// \file js_functions.cc

#ifdef HAVE_CONFIG_H
    #include "autoconfig.h"
#endif

#ifdef HAVE_JS

#include "js_functions.h"

#include "script.h"
#include <typeinfo>
#include "storage.h"
#include "content_manager.h"
#include "config_manager.h"
#include "metadata_handler.h"

using namespace zmm;

extern "C" {

JSBool 
js_print(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval)
{
    uintN i;
    JSString *str;

    for (i = 0; i < argc; i++) 
    {
        str = JS_ValueToString(cx, argv[i]);
        if (!str)
            return JS_TRUE;
        argv[i] = STRING_TO_JSVAL(str);
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
            return JS_TRUE;

        if (!JS_ValueToObject(cx, arg, &js_cds_obj))
            return JS_TRUE;

        argv[0] = OBJECT_TO_JSVAL(js_cds_obj);

        Ref<CdsObject> cds_obj = self->jsObject2cdsObject(js_cds_obj, nil);
        js_cds_clone_obj = JS_NewObject(cx, NULL, NULL, NULL);
        argv[1] = OBJECT_TO_JSVAL(js_cds_clone_obj);

        self->cdsObject2jsObject(cds_obj, js_cds_clone_obj);

        *rval = OBJECT_TO_JSVAL(js_cds_clone_obj);

        return JS_TRUE;

    }
    catch (ServerShutdownException se)
    {
        log_warning("Aborting script execution due to server shutdown.\n");
        return JS_FALSE;
    }
    catch (Exception e)
    {
        log_error("%s\n", e.getMessage().c_str());
        e.printStackTrace();
    }
    return JS_TRUE;
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
        JSObject *js_orig_obj = NULL;
        Ref<CdsObject> orig_object;

        Ref<StringConverter> p2i;
        Ref<StringConverter> i2i;

        Script *self = (Script *)JS_GetPrivate(cx, obj);

        if (self == NULL)
        {
            log_debug("Could not retrieve class instance from global object\n");
            return JS_FALSE;
        }

        if (self->whoami() == S_PLAYLIST)
        {
            p2i = StringConverter::p2i();
        }
        else
        {
            i2i = StringConverter::i2i();
        }
 
        arg = argv[0];
        if (!JSVAL_IS_OBJECT(arg))
            return JS_TRUE;
        if (!JS_ValueToObject(cx, arg, &js_cds_obj))
            return JS_TRUE;

        // root it
        argv[0] = OBJECT_TO_JSVAL(js_cds_obj);

        str = JS_ValueToString(cx, argv[1]);
        if (!str)
            path = _("/");
        else
            path = JS_GetStringBytes(str);

        JSString *cont = JS_ValueToString(cx, argv[2]);
        if (cont)
        {
            containerclass = JS_GetStringBytes(cont);
            if (!string_ok(containerclass) || containerclass == "undefined")
                containerclass = nil;
        }

        if (self->whoami() == S_PLAYLIST)
            js_orig_obj = self->getObjectProperty(obj, _("playlist"));
        else if (self->whoami() == S_IMPORT)
            js_orig_obj = self->getObjectProperty(obj, _("orig"));
        
        if (js_orig_obj == NULL)
        {
            log_debug("Could not retrieve orig/playlist object\n");
            return JS_TRUE;
        }

        // root it
        argv[1] = OBJECT_TO_JSVAL(js_orig_obj);

        orig_object = self->jsObject2cdsObject(js_orig_obj, self->getProcessedObject());
        if (orig_object == nil)
            return JS_TRUE;

        Ref<CdsObject> cds_obj;
        Ref<ContentManager> cm = ContentManager::getInstance();
        int pcd_id = INVALID_OBJECT_ID;

        if (self->whoami() == S_PLAYLIST)
        {  
            int otype = self->getIntProperty(js_cds_obj, _("objectType"), -1);
            if (otype == -1)
            {
                log_error("missing objectType property\n");
                return JS_TRUE;
            }

            if (!IS_CDS_ITEM_EXTERNAL_URL(otype) &&
                !IS_CDS_ITEM_INTERNAL_URL(otype))
            { 
                String loc = self->getProperty(js_cds_obj, _("location"));
                if (string_ok(loc) && 
                   (IS_CDS_PURE_ITEM(otype) || IS_CDS_ACTIVE_ITEM(otype)))
                    loc = normalizePath(loc);

                pcd_id = cm->addFile(loc, false, false, true);
                if (pcd_id == INVALID_OBJECT_ID)
                    return JS_TRUE;

                Ref<CdsObject> mainObj = Storage::getInstance()->loadObject(pcd_id);
                cds_obj = self->jsObject2cdsObject(js_cds_obj, mainObj);
            }
            else
                cds_obj = self->jsObject2cdsObject(js_cds_obj, self->getProcessedObject());
        }
        else
            cds_obj = self->jsObject2cdsObject(js_cds_obj, orig_object);
        
        if (cds_obj == nil)
            return JS_TRUE;

        int id;

        if ((self->whoami() == S_PLAYLIST) &&
            (ConfigManager::getInstance()->
             getBoolOption(CFG_IMPORT_SCRIPTING_PLAYLIST_SCRIPT_LINK_OBJECTS)))
        {
            path = p2i->convert(path);
            id = cm->addContainerChain(path, containerclass, 
                    orig_object->getID());
        }
        else
        {
            if (self->whoami() == S_PLAYLIST)
                path = p2i->convert(path);
            else
                path = i2i->convert(path);
            
            id = cm->addContainerChain(path, containerclass);
        }

        cds_obj->setParentID(id);
        if (!IS_CDS_ITEM_EXTERNAL_URL(cds_obj->getObjectType()) &&
            !IS_CDS_ITEM_INTERNAL_URL(cds_obj->getObjectType()))
        {
            /// \todo get hidden file setting from config manager?
            /// what about same stuff in content manager, why is it not used
            /// there?

            if (self->whoami() == S_PLAYLIST)
            {
                if (pcd_id == INVALID_OBJECT_ID)
                    return JS_TRUE;

                /// \todo check why this if is needed?
                if (IS_CDS_ACTIVE_ITEM(cds_obj->getObjectType()))
                    cds_obj->setFlag(OBJECT_FLAG_PLAYLIST_REF);
                cds_obj->setRefID(pcd_id);
            }
            else
                cds_obj->setRefID(orig_object->getID());

            cds_obj->setFlag(OBJECT_FLAG_USE_RESOURCE_REF);
        }
        else if (IS_CDS_ITEM_EXTERNAL_URL(cds_obj->getObjectType()) || 
                 IS_CDS_ITEM_INTERNAL_URL(cds_obj->getObjectType()))
        {
            if ((self->whoami() == S_PLAYLIST) &&
            (ConfigManager::getInstance()->
             getBoolOption(CFG_IMPORT_SCRIPTING_PLAYLIST_SCRIPT_LINK_OBJECTS)))
            {
                cds_obj->setFlag(OBJECT_FLAG_PLAYLIST_REF);
                cds_obj->setRefID(orig_object->getID());
            }
        }

        cds_obj->setID(INVALID_OBJECT_ID);
        cm->addObject(cds_obj);

        /* setting object ID as return value */
        String tmp = String::from(id);

        JSString *str2 = JS_NewStringCopyN(cx, tmp.c_str(), tmp.length());
        if (!str2)
            return JS_TRUE;
        *rval = STRING_TO_JSVAL(str2);

        return JS_TRUE;
    }
    catch (ServerShutdownException se)
    {
        log_warning("Aborting script execution due to server shutdown.\n");
        return JS_FALSE;
    }
    catch (Exception e)
    {
        log_error("%s\n", e.getMessage().c_str());
        e.printStackTrace();
    }
    return JS_TRUE;
}

static JSBool convert_charset_generic(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval, charset_convert_t chr)
{
    try
    {
        JSString *str;
        String result;

        Script *self = (Script *)JS_GetPrivate(cx, obj);

        if (self == NULL)
        {
            log_debug("Could not retrieve class instance from global object\n");
            return JS_FALSE;
        }

        if (JSVAL_IS_STRING(argv[0]))
        {
            str = JS_ValueToString(cx, argv[0]);
            if (str)
                result = JS_GetStringBytes(str);
        }

        if (result != nil)
        {
            result = self->convertToCharset(result, chr);
            JSString *str2 = JS_NewStringCopyN(cx, result.c_str(), result.length());
            if (!str2)
                return JS_TRUE;
            *rval = STRING_TO_JSVAL(str2);
        }
    }
    catch (ServerShutdownException se)
    {
        log_warning("Aborting script execution due to server shutdown.\n");
        return JS_FALSE;
    }
    catch (Exception e)
    {
        log_error("%s\n", e.getMessage().c_str());
        e.printStackTrace();
    }
    return JS_TRUE;
}


JSBool js_f2i(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval)
{
     return convert_charset_generic(cx, obj, argc, argv, rval, F2I);
}

JSBool js_m2i(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval)
{
     return convert_charset_generic(cx, obj, argc, argv, rval, M2I);
}

JSBool js_p2i(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval)
{
     return convert_charset_generic(cx, obj, argc, argv, rval, P2I);
}

JSBool js_j2i(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval)
{
     return convert_charset_generic(cx, obj, argc, argv, rval, J2I);
}

} // extern "C"

#endif //HAVE_JS
