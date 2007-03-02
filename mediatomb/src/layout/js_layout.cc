/*MT*
    
    MediaTomb - http://www.mediatomb.cc/
    
    js_layout.cc - this file is part of MediaTomb.
    
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

/// \file js_layout.cc

#ifdef HAVE_CONFIG_H
    #include "autoconfig.h"
#endif

#ifdef HAVE_JS

#include "js_layout.h"
#include "storage.h"
#include "content_manager.h"
#include "metadata_handler.h"
#include "config_manager.h"
#include "tools.h"
#include "string_converter.h"

using namespace zmm;

/* static js stuff */

/*
static JSClass global_class = {
    "global", JSCLASS_NEW_RESOLVE,
    JS_PropertyStub,  JS_PropertyStub,
    JS_PropertyStub,  JS_PropertyStub,
    global_enumerate, (JSResolveOp) global_resolve,
    JS_ConvertStub,   JS_FinalizeStub
};
*/


static String js_get_property(JSContext *cx, JSObject *obj, String name)
{
    jsval val;
    JSString *str;
    if (!JS_GetProperty(cx, obj, name.c_str(), &val))
        return nil;
    if (val == JSVAL_VOID)
        return nil;
    str = JS_ValueToString(cx, val);
    if (! str)
        return nil;
    return String(JS_GetStringBytes(str));
}
static int js_get_bool_property(JSContext *cx, JSObject *obj, String name)
{
    jsval val;
    JSBool boolVal;

    if (!JS_GetProperty(cx, obj, name.c_str(), &val))
        return -1;
    if (val == JSVAL_VOID)
        return -1;
    if (!JS_ValueToBoolean(cx, val, &boolVal))
        return -1;
    return (boolVal ? 1 : 0);
}
static int js_get_int_property(JSContext *cx, JSObject *obj, String name, int def)
{
    jsval val;
    int intVal;

    if (!JS_GetProperty(cx, obj, name.c_str(), &val))
        return def;
    if (val == JSVAL_VOID)
        return def;
    if (!JS_ValueToInt32(cx, val, &intVal))
        return def;
    return intVal;
}

static JSObject *js_get_object_property(JSContext *cx, JSObject *obj, String name)
{
    jsval val;
    JSObject *js_obj;

    if (!JS_GetProperty(cx, obj, name.c_str(), &val))
        return NULL;
    if (val == JSVAL_VOID)
        return NULL;
    if (!JS_ValueToObject(cx, val, &js_obj))
        return NULL;
    return js_obj;
}

static void js_set_property(JSContext *cx, JSObject *obj, char *name, String value)
{
    jsval val;
    JSString *str = JS_NewStringCopyN(cx, value.c_str(), value.length());
	if (!str)
		return;
	val = STRING_TO_JSVAL(str);
    if (!JS_SetProperty(cx, obj, name, &val))
        return;
}

static void js_set_char_property(JSContext *cx, JSObject *obj, char *name, char *value)
{
    jsval val;
    JSString *str = JS_NewStringCopyN(cx, value, strlen(value));
	if (!str)
		return;
	val = STRING_TO_JSVAL(str);
    if (!JS_SetProperty(cx, obj, name, &val))
        return;
}

static void js_set_int_property(JSContext *cx, JSObject *obj, char *name, int value)
{
    jsval val;
    if (!JS_NewNumberValue(cx, (jsdouble)value, &val))
        return;
    if (!JS_SetProperty(cx, obj, name, &val))
        return;
}

static void js_set_object_property(JSContext *cx, JSObject *parent, char *name, JSObject *obj)
{
    jsval val;
	val = OBJECT_TO_JSVAL(obj);
    if (!JS_SetProperty(cx, parent, name, &val))
        return;
}

static Ref<CdsObject> jsObject2cdsObject(JSContext *cx, JSObject *js)
{
    String val;
    int objectType;
    int b;
    int i;

    objectType = js_get_int_property(cx, js, _("objectType"), -1);
    if (objectType == -1)
    {
        log_error("missing objectType property\n");
        return nil;
    }

    Ref<CdsObject> obj = CdsObject::createObject(objectType);

    // CdsObject
    obj->setVirtual(1); // JS creates only virtual objects

    i = js_get_int_property(cx, js, _("id"), INVALID_OBJECT_ID);
    if (i != INVALID_OBJECT_ID)
        obj->setID(i);
    i = js_get_int_property(cx, js, _("refID"), INVALID_OBJECT_ID);
    if (i != INVALID_OBJECT_ID)
        obj->setRefID(i);
    i = js_get_int_property(cx, js, _("parentID"), INVALID_OBJECT_ID);
    if (i != INVALID_OBJECT_ID)
        obj->setParentID(i);
    val = js_get_property(cx, js, _("title"));
    if (val != nil)
        obj->setTitle(val);
    val = js_get_property(cx, js, _("class"));
    if (val != nil)
        obj->setClass(val);
    val = js_get_property(cx, js, _("location"));
    if (val != nil)
        obj->setLocation(val);
    b = js_get_bool_property(cx, js, _("restricted"));
    if (b >= 0)
        obj->setRestricted(b);

    JSObject *js_meta = js_get_object_property(cx, js, _("meta"));
    if (js_meta)
    {
        /// \todo: only metadata enumerated in MT_KEYS is taken
        for (int i = 0; i < M_MAX; i++)
        {
            val = js_get_property(cx, js_meta, _(MT_KEYS[i].upnp));
            if (val != nil)
            {
                if (i == M_TRACKNUMBER)
                {
                    int j = val.toInt();
                    if (j > 0)
                    {
                        obj->setMetadata(String(MT_KEYS[i].upnp), val);
                        RefCast(obj, CdsItem)->setTrackNumber(j);
                    }
                    else
                        RefCast(obj, CdsItem)->setTrackNumber(0);
                }
                else
                    obj->setMetadata(String(MT_KEYS[i].upnp), val);
            }
        }
    }

    // CdsItem
    if (IS_CDS_ITEM(objectType))
    {
        Ref<CdsItem> item = RefCast(obj, CdsItem);

        val = js_get_property(cx, js, _("mimetype"));
        if (val != nil)
            item->setMimeType(val);

        if (IS_CDS_ACTIVE_ITEM(objectType))
        {
            Ref<CdsActiveItem> aitem = RefCast(obj, CdsActiveItem);
            val = js_get_property(cx, js, _("action"));
            if (val != nil)
                aitem->setAction(val);
            val = js_get_property(cx, js, _("state"));
            if (val != nil)
                aitem->setState(val);
        }
    }

    // CdsDirectory
    if (IS_CDS_CONTAINER(objectType))
    {
        Ref<CdsContainer> cont = RefCast(obj, CdsContainer);
        i = js_get_int_property(cx, js, _("updateID"), -1);
        if (i >= 0)
            cont->setUpdateID(i);
        b = js_get_bool_property(cx, js, _("searchable"));
        if (b >= 0)
            cont->setSearchable(b);
    }

    return obj;
}

static void cdsObject2jsObject(JSContext *cx, Ref<CdsObject> obj, JSObject *js)
{
	String val;
	int i;

	int objectType = obj->getObjectType();

    // CdsObject
	js_set_int_property(cx, js, "objectType", objectType);

    i = obj->getID();

    if (i != INVALID_OBJECT_ID)
        js_set_int_property(cx, js, "id", i);

    i = obj->getParentID();
    if (i != INVALID_OBJECT_ID)    
        js_set_int_property(cx, js, "parentID", i);

    val = obj->getTitle();
    if (val != nil)
        js_set_property(cx, js, "title", val);
    val = obj->getClass();
    if (val != nil)
        js_set_property(cx, js, "class", val);
    val = obj->getLocation();
    if (val != nil)
        js_set_property(cx, js, "location", val);
	// TODO: boolean type
	i = obj->isRestricted();
    js_set_int_property(cx, js, "restricted", i);
   
    // setting metadata
    {
    	JSObject *meta_js = JS_NewObject(cx, NULL, NULL, js);
        Ref<Dictionary> meta = obj->getMetadata();
        Ref<Array<DictionaryElement> > elements = meta->getElements();
        int len = elements->size();
        for (int i = 0; i < len; i++)
        {
            Ref<DictionaryElement> el = elements->get(i);
            js_set_property(cx, meta_js, el->getKey().c_str(), el->getValue());
        }
    	js_set_object_property(cx, js, "meta", meta_js);
    }

    // setting auxdata
    {
    	JSObject *aux_js = JS_NewObject(cx, NULL, NULL, js);
        Ref<Dictionary> aux = obj->getAuxData();
        Ref<Array<DictionaryElement> > elements = aux->getElements();
        int len = elements->size();
        for (int i = 0; i < len; i++)
        {
            Ref<DictionaryElement> el = elements->get(i);
            js_set_property(cx, aux_js, el->getKey().c_str(), el->getValue());
        }
    	js_set_object_property(cx, js, "aux", aux_js);
    }

    /// \todo add resources
    
    // CdsItem
    if (IS_CDS_ITEM(objectType))
    {
        Ref<CdsItem> item = RefCast(obj, CdsItem);
		val = item->getMimeType();
		if (val != nil)
			js_set_property(cx, js, "mimetype", val);

        if (IS_CDS_ACTIVE_ITEM(objectType))
        {
            Ref<CdsActiveItem> aitem = RefCast(obj, CdsActiveItem);
			val = aitem->getAction();
			if (val != nil)
				js_set_property(cx, js, "action", val);
			val = aitem->getState();
			if (val != nil)
				js_set_property(cx, js, "state", val);
        }
    }

    // CdsDirectory
    if (IS_CDS_CONTAINER(objectType))
    {
        Ref<CdsContainer> cont = RefCast(obj, CdsContainer);
        // TODO: boolean type, hide updateID
        i = cont->getUpdateID();
        js_set_int_property(cx, js, "updateID", i);
        
        i = cont->isSearchable();
        js_set_int_property(cx, js, "searchable", i);
    }
}

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
        log_debug("%s\n", JS_GetStringBytes(str));
    }
    return JS_TRUE;
}

static JSBool
js_copyObject(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval)
{
    jsval arg;
    JSObject *js_cds_obj;
    JSObject *js_cds_clone_obj;

    try
    {
        arg = argv[0];
        if (!JSVAL_IS_OBJECT(arg))
            return JS_FALSE;
        if (!JS_ValueToObject(cx, arg, &js_cds_obj))
            return JS_FALSE;

        Ref<CdsObject> cds_obj = jsObject2cdsObject(cx, js_cds_obj);
        js_cds_clone_obj = JS_NewObject(cx, NULL, NULL, NULL);
        cdsObject2jsObject(cx, cds_obj, js_cds_clone_obj);

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

        Ref<CdsObject> cds_obj = jsObject2cdsObject(cx, js_cds_obj);
        Ref<ContentManager> cm = ContentManager::getInstance();
       
        int id = cm->addContainerChain(path, containerclass);
        cds_obj->setParentID(id);
        cds_obj->setFlag(OBJECT_FLAG_USE_RESOURCE_REF);
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

static void
js_error_reporter(JSContext *cx, const char *message, JSErrorReport *report)
{
    int n;
    const char *ctmp;

    int reportWarnings = 1; // TODO move to object field

    Ref<StringBuffer> buf(new StringBuffer());

    do
    {
        if (!report)
        {
            *buf << (char *)message;
            break;
        }

        // Conditionally ignore reported warnings.
        if (JSREPORT_IS_WARNING(report->flags) && !reportWarnings)
            return;

        String prefix;
        Ref<StringBuffer> prefix_buf(new StringBuffer());

        if (report->filename)
            *prefix_buf << (char *)report->filename << ":";

        if (report->lineno)
        {
            *prefix_buf << (int)report->lineno << ": ";
        }
        if (JSREPORT_IS_WARNING(report->flags))
        {
            if (JSREPORT_IS_STRICT(report->flags))
                *prefix_buf << "(STRICT WARN)";
            else
                *prefix_buf << "(WARN)";
        }

        prefix = prefix_buf->toString();

        // embedded newlines
        while ((ctmp = strchr(message, '\n')) != 0)
        {
            ctmp++;
            if (prefix.length())
                *buf << prefix;
            *buf << String((char *)message, ctmp - message);
            message = ctmp;
        }

        // If there were no filename or lineno, the prefix might be empty
        if (prefix.length())
            *buf << prefix;
        *buf << (char *)message << "\n";

        if (report->linebuf)
        {
            // report->linebuf usually ends with a newline.
            n = strlen(report->linebuf);
            *buf << prefix << (char *)report->linebuf;
            *buf << (char *)((n > 0 && report->linebuf[n-1] == '\n') ? "" : "\n");
            *buf << prefix;
            /*
            n = PTRDIFF(report->tokenptr, report->linebuf, char);
            for (i = j = 0; i < n; i++)
            {
                if (report->linebuf[i] == '\t')
                {
                    for (k = (j + 8) & ~7; j < k; j++)
                    {
                        fputc('.', gErrFile);
                    }
                    continue;
                }
                fputc('.', stdout);
                j++;
            }
            fputs("^\n", stdout);
            */
        }
    }
    while (0);

    String err = buf->toString();
    log_debug("%s\n", err.c_str());
}

} // extern "C"

static JSFunctionSpec global_functions[] = {
//   js name            native name        number of arguments
    {"print",           js_print,          0},
    {"addCdsObject",    js_addCdsObject,   3},
    {"copyObject",      js_copyObject,     1},
    {0}
};


/* **************** */

JSLayout::JSLayout() : Layout()
{
    rt = NULL;
    cx = NULL;
    glob = NULL;
	script = NULL;
}

void JSLayout::init()
{
    if (script)
    {
        throw _Exception(_("JSLayout already initialized!"));
    }

    String scriptPath = ConfigManager::getInstance()->getOption(_("/import/virtual-layout/script"));
    if (!string_ok(scriptPath))
    {
        throw _Exception(_("JSLayout is turned off."));
    }

    log_info("Read import script: %s\n", scriptPath.c_str());
    
	String scriptText = read_text_file(scriptPath);
	if (scriptText == nil)
    {
        throw _Exception(String("Could not read script ") + scriptPath.c_str());
    }

    Ref<StringConverter> j2i = StringConverter::j2i();
    try
    {
        scriptText = j2i->convert(scriptText, true);
    }
    catch (Exception e)
    {
        throw _Exception(String("Failed to convert import script:") + e.getMessage().c_str());
    }

    /* initialize the JS run time, and return result in rt */
    rt = JS_NewRuntime(DEFAULT_JS_RUNTIME_MEM);

    if (!rt)
        throw _Exception(_("JSLayout: could not initialize js runtime"));

    /* create a context and associate it with the JS run time */
    cx = JS_NewContext(rt, 8192);
    if (! cx)
        throw _Exception(_("JSLayout: could not initialize js context"));

    /* create the global object here */
    glob = JS_NewObject(cx, /* global_class */ NULL, NULL, NULL);
    if (! glob)
        throw _Exception(_("JSLayout: could not initialize glboal class"));

    /* initialize the built-in JS objects and the global object */
    if (! JS_InitStandardClasses(cx, glob))
        throw _Exception(_("JSLayout: JS_InitStandardClasses failed"));

    if (!JS_DefineFunctions(cx, glob, global_functions))
        throw _Exception(_("JSLayout: JS_DefineFunctions on global object failed"));

    /* initialize contstants */
    js_set_int_property(cx, glob, "OBJECT_TYPE_CONTAINER",
                                   OBJECT_TYPE_CONTAINER);
    js_set_int_property(cx, glob, "OBJECT_TYPE_ITEM",
                                   OBJECT_TYPE_ITEM);
    js_set_int_property(cx, glob, "OBJECT_TYPE_ACTIVE_ITEM",
                                   OBJECT_TYPE_ACTIVE_ITEM);
    js_set_int_property(cx, glob, "OBJECT_TYPE_ITEM_EXTERNAL_URL",
                                   OBJECT_TYPE_ITEM_EXTERNAL_URL);

    for (int i = 0; i < M_MAX; i++)
    {
        js_set_property(cx, glob, MT_KEYS[i].sym, String(MT_KEYS[i].upnp));
    }
   
    js_set_char_property(cx, glob, "UPNP_CLASS_CONTAINER_MUSIC", UPNP_DEFAULT_CLASS_MUSIC_CONT);
    js_set_char_property(cx, glob, "UPNP_CLASS_CONTAINER_MUSIC_ALBUM", UPNP_DEFAULT_CLASS_MUSIC_ALBUM);
    js_set_char_property(cx, glob, "UPNP_CLASS_CONTAINER_MUSIC_ARTIST", UPNP_DEFAULT_CLASS_MUSIC_ARTIST);
    js_set_char_property(cx, glob, "UPNP_CLASS_CONTAINER_MUSIC_GENRE", UPNP_DEFAULT_CLASS_MUSIC_GENRE);
    js_set_char_property(cx, glob, "UPNP_CLASS_CONTAINER", UPNP_DEFAULT_CLASS_CONTAINER);
    js_set_char_property(cx, glob, "UPNP_CLASS_ITEM", UPNP_DEFAULT_CLASS_ITEM);
    js_set_char_property(cx, glob, "UPNP_CLASS_ITEM_MUSIC_TRACK", UPNP_DEFAULT_CLASS_MUSIC_TRACK);
    JS_SetErrorReporter(cx, js_error_reporter);

/*
	if (!JS_EvaluateScript(cx, glob, script.c_str(), script.length(),
		scriptPath.c_str(), 0, &ret_val))
	{
             throw _Exception("JSLayout: failed to evaluate script");
	}
*/

    script = JS_CompileScript(cx, glob, scriptText.c_str(), scriptText.length(),
                              scriptPath.c_str(), 1);
    if (! script)
    {
        log_error("JSLayout: failed to compile script\n");
    }
}
JSLayout::~JSLayout()
{
    log_debug("destoying scripting\n");
    if (script)
        JS_DestroyScript(cx, script);
    if (cx)
		JS_DestroyContext(cx);
	if (rt)
		JS_DestroyRuntime(rt);
}

void JSLayout::processCdsObject(Ref<CdsObject> obj)
{
    if (! script)
        return;

    jsval ret_val;

    JSObject *orig = JS_NewObject(cx, NULL, NULL, glob);
    cdsObject2jsObject(cx, obj, orig);
    js_set_object_property(cx, glob, "orig", orig);
    if (!JS_ExecuteScript(cx, glob, script, &ret_val))
    {
        throw _Exception(_("JSLayout: failed to execute script"));
    }
    log_debug("Script executed successfully\n");

    /*
      if (!JS_EvaluateScript(cx, glob, script.c_str(), script.length(),
		"test-script", 0, &ret_val))
	{
		throw _Exception("JSLayout: failed to evaluate script");
	}
*/
}


#endif // HAVE_JS

