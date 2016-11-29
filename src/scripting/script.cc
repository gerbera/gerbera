/*MT*
    
    MediaTomb - http://www.mediatomb.cc/
    
    script.cc - this file is part of MediaTomb.
    
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

/// \file script.cc

#ifdef HAVE_JS

#include "script.h"
#include "tools.h"
#include "metadata_handler.h"
#include "js_functions.h"
#include "config_manager.h"
#ifdef ONLINE_SERVICES
    #include "online_service.h"
#endif

#ifdef YOUTUBE
    #include "youtube_service.h"
    #include "youtube_content_handler.h"
#endif

#ifdef WEBORAMA
    #include "weborama_content_handler.h"
#endif

#ifdef ATRAILERS
    #include "atrailers_content_handler.h"
#endif

#ifdef HAVE_LIBDVDNAV
    #include "metadata/dvd_handler.h"
#endif

using namespace zmm;

static JSFunctionSpec js_global_functions[] =
{
    JS_FS("print",          js_print,          1, 0),
    JS_FS("addCdsObject",   js_addCdsObject,   3, 0),
    JS_FS("copyObject",     js_copyObject,     2, 0),
    JS_FS("f2i",            js_f2i,            1, 0),
    JS_FS("m2i",            js_m2i,            1, 0),
    JS_FS("p2i",            js_m2i,            1, 0),
    JS_FS("j2i",            js_m2i,            1, 0),
    JS_FS_END 
};

String Script::getProperty(JSObject *obj, String name)
{
    jsval val;
    JSString *str;
    String ret;
    if (!JS_GetProperty(cx, obj, name.c_str(), &val))
        return nil;
    if (val == JSVAL_VOID)
        return nil;
    str = JS_ValueToString(cx, val);
    if (!str)
        return nil;

    char *ts = JS_EncodeString(cx, str);
    if (!ts)
    {
        return nil;
    }
    ret = ts;
    JS_free(cx, ts);
    return ret;
}

int Script::getBoolProperty(JSObject *obj, String name)
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

int Script::getIntProperty(JSObject *obj, String name, int def)
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

JSObject *Script::getObjectProperty(JSObject *obj, String name)
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

void Script::setProperty(JSObject *obj, String name, String value)
{
    jsval val;
    JSString *str = JS_NewStringCopyN(cx, value.c_str(), value.length());
    if (!str)
        return;
    val = STRING_TO_JSVAL(str);
    if (!JS_SetProperty(cx, obj, name.c_str(), &val))
        return;
}

void Script::setIntProperty(JSObject *obj, String name, int value)
{
    jsval val;
    if (!JS_NewNumberValue(cx, (jsdouble)value, &val))
        return;
    if (!JS_SetProperty(cx, obj, name.c_str(), &val))
        return;
}

void Script::setObjectProperty(JSObject *parent, String name, JSObject *obj)
{
    jsval val;
    val = OBJECT_TO_JSVAL(obj);
    if (!JS_SetProperty(cx, parent, name.c_str(), &val)) {
        log_error("Failed to set object property %s\n", name.c_str());
    }
}

void Script::deleteProperty(JSObject *obj, String name)
{
    JS_DeleteProperty(cx, obj, name.c_str());
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
    log_js("%s\n", err.c_str());
}

/* **************** */

Script::Script(Ref<Runtime> runtime) : Object()
{
    gc_counter = 0;

    this->runtime = runtime;
    rt = runtime->getRT();

    /* create a context and associate it with the JS run time */
    cx = JS_NewContext(rt, 8192);
    if (! cx)
        throw _Exception(_("Scripting: could not initialize js context"));

#ifdef JS_THREADSAFE
    JS_SetContextThread(cx);
    JS_BeginRequest(cx);
#endif
//    JS_SetGCZeal(cx, 2);

    glob = NULL;
    script = NULL;

    _p2i = StringConverter::p2i();
    _j2i = StringConverter::j2i();
    _m2i = StringConverter::m2i();
    _f2i = StringConverter::f2i();
    _i2i = StringConverter::i2i();

    JS_SetErrorReporter(cx, js_error_reporter);
    initGlobalObject();

    JS_SetContextPrivate(cx, this);

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
#ifdef ONLINE_SERVICES 
    setIntProperty(glob, _("ONLINE_SERVICE_NONE"), (int)OS_None);
#ifdef YOUTUBE
    setIntProperty(glob, _("ONLINE_SERVICE_YOUTUBE"), (int)OS_YouTube);

    setProperty(glob, _("YOUTUBE_AUXDATA_KEYWORDS"), 
            _(YOUTUBE_AUXDATA_KEYWORDS));
    setProperty(glob, _("YOUTUBE_AUXDATA_AVG_RATING"), 
            _(YOUTUBE_AUXDATA_AVG_RATING));
    setProperty(glob, _("YOUTUBE_AUXDATA_AUTHOR"), 
            _(YOUTUBE_AUXDATA_AUTHOR));
    setProperty(glob, _("YOUTUBE_AUXDATA_FEED"), 
            _(YOUTUBE_AUXDATA_FEED));
    setProperty(glob, _("YOUTUBE_AUXDATA_VIEW_COUNT"), 
            _(YOUTUBE_AUXDATA_VIEW_COUNT));
    setProperty(glob, _("YOUTUBE_AUXDATA_FAVORITE_COUNT"), 
            _(YOUTUBE_AUXDATA_FAVORITE_COUNT));
    setProperty(glob, _("YOUTUBE_AUXDATA_RATING_COUNT"), 
            _(YOUTUBE_AUXDATA_RATING_COUNT));
    setProperty(glob, _("YOUTUBE_AUXDATA_CATEGORY"), 
            _(YOUTUBE_AUXDATA_CATEGORY));
    setProperty(glob, _("YOUTUBE_AUXDATA_SUBREQUEST_NAME"), 
            _(YOUTUBE_AUXDATA_SUBREQUEST_NAME));
    setProperty(glob, _("YOUTUBE_AUXDATA_REQUEST"), 
            _(YOUTUBE_AUXDATA_REQUEST));
    setProperty(glob, _("YOUTUBE_AUXDATA_REGION"),
            _(YOUTUBE_AUXDATA_REGION));

    setIntProperty(glob, _("YOUTUBE_REQUEST_NONE"), (int)YT_request_none);
    setIntProperty(glob, _("YOUTUBE_REQUEST_VIDEO_SEARCH"), 
                  (int)YT_request_video_search);
    setIntProperty(glob, _("YOUTUBE_REQUEST_STANDARD_FEED"), 
                  (int)YT_request_stdfeed);
    setIntProperty(glob, _("YOUTUBE_REQUEST_USER_FAVORITES"), 
                  (int)YT_request_user_favorites);
    setIntProperty(glob, _("YOUTUBE_REQUEST_USER_PLAYLISTS"), 
                  (int)YT_request_user_playlists);
    setIntProperty(glob, _("YOUTUBE_REQUEST_USER_SUBSCRIPTIONS"), 
                   (int)YT_request_user_subscriptions);
    setIntProperty(glob, _("YOUTUBE_REQUEST_USER_UPLOADS"), 
                   (int)YT_request_user_uploads);
#else
    setIntProperty(glob, _("ONLINE_SERVICE_YOUTUBE"), -1);
#endif//YOUTUBE

#ifdef WEBORAMA
    setIntProperty(glob, _("ONLINE_SERVICE_WEBORAMA"), (int)OS_Weborama);
    setProperty(glob, _("WEBORAMA_AUXDATA_REQUEST_NAME"),
                      _(WEBORAMA_AUXDATA_REQUEST_NAME));
#else
    setIntProperty(glob, _("ONLINE_SERVICE_WEBORAMA"), -1);
#endif//WEBORAMAa

#ifdef ATRAILERS
    setIntProperty(glob, _("ONLINE_SERVICE_APPLE_TRAILERS"), (int)OS_ATrailers);
    setProperty(glob, _("APPLE_TRAILERS_AUXDATA_POST_DATE"),
                      _(ATRAILERS_AUXDATA_POST_DATE));
#else
    setIntProperty(glob, _("ONLINE_SERVICE_APPLE_TRAILERS"), -1);
#endif//ATRAILERS

#ifdef SOPCAST
    setIntProperty(glob, _("ONLINE_SERVICE_SOPCAST"), (int)OS_SopCast);
#else
    setIntProperty(glob, _("ONLINE_SERVICE_SOPCAST"), -1);
#endif//SOPCAST

#else // ONLINE SERVICES
    setIntProperty(glob, _("ONLINE_SERVICE_NONE"), 0);
    setIntProperty(glob, _("ONLINE_SERVICE_YOUTUBE"), -1);
    setIntProperty(glob, _("ONLINE_SERVICE_WEBORAMA"), -1);
    setIntProperty(glob, _("ONLINE_SERVICE_SOPCAST"), -1);
    setIntProperty(glob, _("ONLINE_SERVICE_APPLE_TRAILERS"), -1);
#endif//ONLINE_SERVICES

    for (int i = 0; i < M_MAX; i++)
    {
        setProperty(glob, _(MT_KEYS[i].sym), _(MT_KEYS[i].upnp));
    }
 
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
    setProperty(glob, _("UPNP_CLASS_ITEM_VIDEO"),
            _(UPNP_DEFAULT_CLASS_VIDEO_ITEM));
    setProperty(glob, _("UPNP_CLASS_ITEM_IMAGE"), 
            _(UPNP_DEFAULT_CLASS_IMAGE_ITEM));
    setProperty(glob, _("UPNP_CLASS_PLAYLIST_CONTAINER"),
            _(UPNP_DEFAULT_CLASS_PLAYLIST_CONTAINER));

    defineFunctions(js_global_functions);

    String common_scr_path = ConfigManager::getInstance()->getOption(CFG_IMPORT_SCRIPTING_COMMON_SCRIPT);

    if (!string_ok(common_scr_path))
        log_js("Common script disabled in configuration\n");
    else
    {
        try
        {
            common_script = _load(common_scr_path);
            JS_AddNamedObjectRoot(cx, &common_script, "common-script");
            _execute(common_script);
        }
        catch (Exception e)
        {
            if (common_script)
            {
                JS_RemoveObjectRoot(cx, &common_script);
            }

            log_js("Unable to load %s: %s\n", common_scr_path.c_str(), 
                    e.getMessage().c_str());
        }
    }
#ifdef JS_THREADSAFE
    JS_EndRequest(cx);
    JS_ClearContextThread(cx);
#endif
}

/*
static intN map(void *rp, const char *name, void *data)
{
    return JS_MAP_GCROOT_NEXT;
}
*/

Script::~Script()
{
#ifdef JS_THREADSAFE
    JS_SetContextThread(cx);
    JS_BeginRequest(cx);
#endif
    if (common_script)
        JS_RemoveObjectRoot(cx, &common_script);

/*
 * scripts are unrooted and will be cleaned up by GC
    if (common_script)
        JS_DestroyScript(cx, common_script);

    if (script)
        JS_DestroyScript(cx, script);
*/       
//    JS_MapGCRoots(rt, &map, NULL); // debug stuff
#ifdef JS_THREADSAFE
    JS_EndRequest(cx);
//    JS_ClearContextThread(cx);
#endif
    if (cx)
    {
        JS_DestroyContext(cx);
        cx = NULL;
    }
}

void Script::setGlobalObject(JSObject *glob)
{
    this->glob = glob;
    JS_SetGlobalObject(cx, glob);
}

JSObject *Script::getGlobalObject()
{
    return glob;
}

JSContext *Script::getContext()
{
    return cx;
}

void Script::initGlobalObject()
{
    /* define characteristics of the global class */
    static JSClass global_class =
    {
        "global",                                   /* name */
        JSCLASS_HAS_PRIVATE | JSCLASS_GLOBAL_FLAGS, /* flags */
        JS_PropertyStub,                            /* add property */
        JS_PropertyStub,                            /* del property */
        JS_PropertyStub,                            /* get property */
        JS_StrictPropertyStub,                      /* set property */
        JS_EnumerateStandardClasses,                /* enumerate */
        JS_ResolveStub,                             /* resolve */
        JS_ConvertStub,                             /* convert */
        JS_FinalizeStub,                            /* finalize */
        JSCLASS_NO_OPTIONAL_MEMBERS
    };

    /* create the global object here */
    glob = JS_NewCompartmentAndGlobalObject(cx, &global_class, NULL);
    if (! glob)
        throw _Exception(_("Scripting: could not initialize glboal class"));

    /* initialize the built-in JS objects and the global object */
    if (! JS_InitStandardClasses(cx, glob))
        throw _Exception(_("Scripting: JS_InitStandardClasses failed"));

}

void Script::defineFunction(String name, JSNative function, uint32_t numParams)
{
    if (! JS_DefineFunction(cx, glob, name.c_str(), function, numParams, 0))
        throw _Exception(_("Scripting: JS_DefineFunction failed"));
}

void Script::defineFunctions(JSFunctionSpec *functions)
{
    if (! JS_DefineFunctions(cx, glob, functions))
        throw _Exception(_("Scripting: JS_DefineFunctions failed"));
}

JSObject *Script::_load(zmm::String scriptPath)
{
    if (glob == NULL)
        initGlobalObject();

    JSObject *scr;

    String scriptText = read_text_file(scriptPath);

    if (!string_ok(scriptText))
        throw _Exception(_("empty script"));

    Ref<StringConverter> j2i = StringConverter::j2i();
    try
    {
        scriptText = j2i->convert(scriptText, true);
    }
    catch (Exception e)
    {
        throw _Exception(_("Failed to convert import script:") + e.getMessage().c_str());
    }

    scr = JS_CompileScript(cx, glob, scriptText.c_str(), scriptText.length(),
            scriptPath.c_str(), 1);
    if (! scr)
        throw _Exception(_("Scripting: failed to compile ") + scriptPath);

    return scr;
}

void Script::load(zmm::String scriptPath)
{
    script = _load((scriptPath));
}


void Script::_execute(JSObject *scr)
{
    jsval ret_val;

    if (!JS_ExecuteScript(cx, glob, scr, &ret_val))
        throw _Exception(_("Script: failed to execute script"));
}

void Script::execute()
{
    _execute(script);
}

Ref<CdsObject> Script::jsObject2cdsObject(JSObject *js, zmm::Ref<CdsObject> pcd)
{
    String val;
    int objectType;
    int b;
    int i;
    Ref<StringConverter> sc;

    if (this->whoami() == S_PLAYLIST)
    {
        sc = StringConverter::p2i();
    }
    else
        sc = StringConverter::i2i();

    objectType = getIntProperty(js, _("objectType"), -1);
    if (objectType == -1)
    {
        log_error("missing objectType property\n");
        return nil;
    }

    Ref<CdsObject> obj = CdsObject::createObject(objectType);
    objectType = obj->getObjectType(); // this is important, because the
    // type will be changed appropriately
    // by the create function

    // CdsObject
    obj->setVirtual(1); // JS creates only virtual objects

    i = getIntProperty(js, _("id"), INVALID_OBJECT_ID);
    if (i != INVALID_OBJECT_ID)
        obj->setID(i);
    i = getIntProperty(js, _("refID"), INVALID_OBJECT_ID);
    if (i != INVALID_OBJECT_ID)
        obj->setRefID(i);
    i = getIntProperty(js, _("parentID"), INVALID_OBJECT_ID);
    if (i != INVALID_OBJECT_ID)
        obj->setParentID(i);

    val = getProperty(js, _("title"));
    if (val != nil)
    {
        val = sc->convert(val);
        obj->setTitle(val);
    }
    else
    {
        if (pcd != nil)
            obj->setTitle(pcd->getTitle());
    }

    val = getProperty(js, _("upnpclass"));
    if (val != nil)
    {
        val = sc->convert(val);
        obj->setClass(val);
    }
    else
    {
        if (pcd != nil)
            obj->setClass(pcd->getClass());
    }

    b = getBoolProperty(js, _("restricted"));
    if (b >= 0)
        obj->setRestricted(b);
   
    JSObject *js_meta = getObjectProperty(js, _("meta"));
    if (js_meta)
    {
        JS_AddNamedObjectRoot(cx, &js_meta, "meta");
        /// \todo: only metadata enumerated in MT_KEYS is taken
        for (int i = 0; i < M_MAX; i++)
        {
            val = getProperty(js_meta, _(MT_KEYS[i].upnp));
            if (val != nil)
            {
                if (i == M_TRACKNUMBER)
                {
                    int j = val.toInt();
                    if (j > 0)
                    {
                        obj->setMetadata(MT_KEYS[i].upnp, val);
                        RefCast(obj, CdsItem)->setTrackNumber(j);
                    }
                    else
                        RefCast(obj, CdsItem)->setTrackNumber(0);
                }
                else
                {
                    val = sc->convert(val);
                    obj->setMetadata(MT_KEYS[i].upnp, val);
                }
            }
        }
        JS_RemoveObjectRoot(cx, &js_meta);
    }
    
    // stuff that has not been exported to js
    if (pcd != nil)
    {
        obj->setFlags(pcd->getFlags());
        obj->setResources(pcd->getResources());
        obj->setAuxData(pcd->getAuxData());
    }

    // CdsItem
    if (IS_CDS_ITEM(objectType))
    {
        Ref<CdsItem> item = RefCast(obj, CdsItem);
        Ref<CdsItem> pcd_item;

        if (pcd != nil)
            pcd_item = RefCast(pcd, CdsItem);

        val = getProperty(js, _("mimetype"));
        if (val != nil)
        {
            val = sc->convert(val);
            item->setMimeType(val);
        }
        else
        {
            if (pcd != nil)
                item->setMimeType(pcd_item->getMimeType());
        }

        val = getProperty(js, _("serviceID"));
        if (val != nil)
        {
            val = sc->convert(val);
            item->setServiceID(val);
        }

        /// \todo check what this is doing here, wasn't it already handled
        /// in the MT_KEYS loop?
        val = getProperty(js, _("description"));
        if (val != nil)
        {
            val = sc->convert(val);
            item->setMetadata(MetadataHandler::getMetaFieldName(M_DESCRIPTION), val);
        }
        else
        {
            if (pcd != nil)
                item->setMetadata(MetadataHandler::getMetaFieldName(M_DESCRIPTION),
                    pcd_item->getMetadata(MetadataHandler::getMetaFieldName(M_DESCRIPTION)));
        }
        if (this->whoami() == S_PLAYLIST)
        {
            item->setTrackNumber(getIntProperty(js, _("playlistOrder"), 0));
        }

        // location must not be touched by character conversion!
        val = getProperty(js, _("location"));
        if ((val != nil) && (IS_CDS_PURE_ITEM(objectType) || IS_CDS_ACTIVE_ITEM(objectType)))
            val = normalizePath(val);
        
        if (string_ok(val))
            obj->setLocation(val);
        else
        {
            if (pcd != nil)
                obj->setLocation(pcd->getLocation());
        }

        if (IS_CDS_ACTIVE_ITEM(objectType))
        {
            Ref<CdsActiveItem> aitem = RefCast(obj, CdsActiveItem);
            Ref<CdsActiveItem> pcd_aitem;
            if (pcd != nil)
                pcd_aitem = RefCast(pcd, CdsActiveItem);
          /// \todo what about character conversion for action and state fields?
            val = getProperty(js, _("action"));
            if (val != nil)
                aitem->setAction(val);
            else
            {
                if (pcd != nil)
                    aitem->setAction(pcd_aitem->getAction());
            }

            val = getProperty(js, _("state"));
            if (val != nil)
                aitem->setState(val);
            else
            {
                if (pcd != nil)
                    aitem->setState(pcd_aitem->getState());
            }
        }

        if (IS_CDS_ITEM_EXTERNAL_URL(objectType))
        {
            String protocolInfo;

            obj->setRestricted(true);
            Ref<CdsItemExternalURL> item = RefCast(obj, CdsItemExternalURL);
            val = getProperty(js, _("protocol"));
            if (val != nil)
            {
                val = sc->convert(val);
                protocolInfo = renderProtocolInfo(item->getMimeType(), val);
            }
            else
            {
                protocolInfo = renderProtocolInfo(item->getMimeType(), _(PROTOCOL));
            }

            if (item->getResourceCount() == 0)
            {
                Ref<CdsResource> resource(new CdsResource(CH_DEFAULT));
                resource->addAttribute(MetadataHandler::getResAttrName(
                            R_PROTOCOLINFO), protocolInfo);

                item->addResource(resource);
            }
        }
    }

    // CdsDirectory
    if (IS_CDS_CONTAINER(objectType))
    {
        Ref<CdsContainer> cont = RefCast(obj, CdsContainer);
        i = getIntProperty(js, _("updateID"), -1);
        if (i >= 0)
            cont->setUpdateID(i);

        b = getBoolProperty(js, _("searchable"));
        if (b >= 0)
            cont->setSearchable(b);
    }

    return obj;
}

void Script::cdsObject2jsObject(Ref<CdsObject> obj, JSObject *js)
{
    String val;
    int i;

    int objectType = obj->getObjectType();

    // CdsObject
    setIntProperty(js, _("objectType"), objectType);

    i = obj->getID();

    if (i != INVALID_OBJECT_ID)
        setIntProperty(js, _("id"), i);

    i = obj->getParentID();
    if (i != INVALID_OBJECT_ID)
        setIntProperty(js, _("parentID"), i);

    val = obj->getTitle();
    if (val != nil)
        setProperty(js, _("title"), val);

    val = obj->getClass();
    if (val != nil)
        setProperty(js, _("upnpclass"), val);

    val = obj->getLocation();
    if (val != nil)
        setProperty(js, _("location"), val);

    setIntProperty(js, _("mtime"), (int)obj->getMTime());
    setIntProperty(js, _("sizeOnDisk"), (int)obj->getSizeOnDisk());

    // TODO: boolean type
    i = obj->isRestricted();
    setIntProperty(js, _("restricted"), i);

    if (obj->getFlag(OBJECT_FLAG_OGG_THEORA))
        setIntProperty(js, _("theora"), 1);
    else
        setIntProperty(js, _("theora"), 0);

#ifdef ONLINE_SERVICES
    if (obj->getFlag(OBJECT_FLAG_ONLINE_SERVICE))
    {
         service_type_t service = (service_type_t)(obj->getAuxData(_(ONLINE_SERVICE_AUX_ID)).toInt());
        setIntProperty(js, _("onlineservice"), (int)service);
    }
    else
#endif
        setIntProperty(js, _("onlineservice"), 0);

    // setting metadata
    {
        JSObject *meta_js = JS_NewObject(cx, NULL, NULL, js);
        setObjectProperty(js, _("meta"), meta_js);
        Ref<Dictionary> meta = obj->getMetadata();
        Ref<Array<DictionaryElement> > elements = meta->getElements();
        int len = elements->size();
        for (int i = 0; i < len; i++)
        {
            Ref<DictionaryElement> el = elements->get(i);
            setProperty(meta_js, el->getKey(), el->getValue());
        }

        if (RefCast(obj, CdsItem)->getTrackNumber() > 0)
            setProperty(meta_js, MetadataHandler::getMetaFieldName(M_TRACKNUMBER), String::from(RefCast(obj, CdsItem)->getTrackNumber())); 
    }

    // setting auxdata
    {
        JSObject *aux_js = JS_NewObject(cx, NULL, NULL, js);
        setObjectProperty(js, _("aux"), aux_js);
        Ref<Dictionary> aux = obj->getAuxData();

#ifdef HAVE_LIBDVDNAV
        if (obj->getFlag(OBJECT_FLAG_DVD_IMAGE))
        {
            JSObject *aux_dvd = JS_NewObject(cx, NULL, NULL, js);
            setObjectProperty(aux_js, _("DVD"), aux_dvd);

            int title_count = obj->getAuxData(
                                DVDHandler::renderKey(DVD_TitleCount)).toInt();

            JSObject *titles = JS_NewArrayObject(cx, 0, NULL);
            setObjectProperty(aux_dvd, _("titles"), titles);

            for (int t = 0; t < title_count; t++)
            {
                JSObject *title = JS_NewObject(cx, NULL, NULL, js);
                jsval val = OBJECT_TO_JSVAL(title);
                JS_SetElement(cx, titles, t, &val);

                setProperty(title, _("duration"), 
                        obj->getAuxData(DVDHandler::renderKey(DVD_TitleDuration,
                                        t)));

                JSObject *audio_tracks = JS_NewArrayObject(cx, 0, NULL);
                setObjectProperty(title, _("audio_tracks"), audio_tracks);

                int audio_track_count = obj->getAuxData(
                        DVDHandler::renderKey(DVD_AudioTrackCount, t)).toInt();

                for (int a = 0; a < audio_track_count; a++)
                {
                    JSObject *track = JS_NewObject(cx, NULL, NULL, js);
                    jsval val = OBJECT_TO_JSVAL(track);
                    JS_SetElement(cx, audio_tracks, a, &val);

                    setProperty(track, _("language"), obj->getAuxData(
                                DVDHandler::renderKey(DVD_AudioTrackLanguage,
                                    t, 0, a)));

                    setProperty(track, _("format"), obj->getAuxData(
                                DVDHandler::renderKey(DVD_AudioTrackFormat, 
                                    t, 0, a)));
                }

                JSObject *chapters = JS_NewArrayObject(cx, 0, NULL);
                setObjectProperty(title, _("chapters"), chapters);

                int chapter_count = obj->getAuxData(DVDHandler::renderKey(DVD_ChapterCount, t)).toInt();

                for (int c = 0; c < chapter_count; c++)
                {
                    JSObject *chapter = JS_NewObject(cx, NULL, NULL, js);
                    jsval val = OBJECT_TO_JSVAL(chapter);
                    JS_SetElement(cx, chapters, c, &val);

                    setProperty(chapter, _("duration"), obj->getAuxData(
                                DVDHandler::renderKey(DVD_ChapterRestDuration,
                                    t, c)));
                }
            }
        }

#endif
#ifdef YOUTUBE
        // put in meaningful names for YouTube specific enum values
        String tmp = obj->getAuxData(_(YOUTUBE_AUXDATA_AVG_RATING));
        if (string_ok(tmp))
            aux->put(_(YOUTUBE_AUXDATA_AVG_RATING), tmp);

        tmp = obj->getAuxData(_(YOUTUBE_AUXDATA_KEYWORDS));
        if (string_ok(tmp))
            aux->put(_(YOUTUBE_AUXDATA_KEYWORDS), tmp);

        tmp = obj->getAuxData(_(YOUTUBE_AUXDATA_AUTHOR));
        if (string_ok(tmp))
            aux->put(_(YOUTUBE_AUXDATA_AUTHOR), tmp);

        tmp = obj->getAuxData(_(YOUTUBE_AUXDATA_FAVORITE_COUNT));
        if (string_ok(tmp))
            aux->put(_(YOUTUBE_AUXDATA_FAVORITE_COUNT), tmp);

        tmp = obj->getAuxData(_(YOUTUBE_AUXDATA_VIEW_COUNT));
        if (string_ok(tmp))
            aux->put(_(YOUTUBE_AUXDATA_VIEW_COUNT), tmp);

        tmp = obj->getAuxData(_(YOUTUBE_AUXDATA_RATING_COUNT));
        if (string_ok(tmp))
            aux->put(_(YOUTUBE_AUXDATA_RATING_COUNT), tmp);

        tmp = obj->getAuxData(_(YOUTUBE_AUXDATA_FEED));
        if (string_ok(tmp))
            aux->put(_(YOUTUBE_AUXDATA_FEED), tmp);

        tmp = obj->getAuxData(_(YOUTUBE_AUXDATA_SUBREQUEST_NAME));
        if (string_ok(tmp))
            aux->put(_(YOUTUBE_AUXDATA_SUBREQUEST_NAME), tmp);

        tmp = obj->getAuxData(_(YOUTUBE_AUXDATA_CATEGORY));
        if (string_ok(tmp))
            aux->put(_(YOUTUBE_AUXDATA_CATEGORY), tmp);

        tmp = obj->getAuxData(_(YOUTUBE_AUXDATA_REQUEST));
        if (string_ok(tmp))
        {
            yt_requests_t req = (yt_requests_t)tmp.toInt();

            // since subrequests do not actually produce any items they
            // should not be visible to js
            if (req == YT_subrequest_playlists)
                req = YT_request_user_playlists;
            else if (req == YT_subrequest_subscriptions)
                req = YT_request_user_subscriptions;

            setIntProperty(js, _("yt_request"), (int)req);
            tmp = YouTubeService::getRequestName(req);
            if (string_ok(tmp))
                aux->put(_(YOUTUBE_AUXDATA_REQUEST), tmp);
        }

        tmp = obj->getAuxData(_(YOUTUBE_AUXDATA_REGION));
        if (string_ok(tmp))
        {
            yt_regions_t reg = (yt_regions_t)tmp.toInt();
            if (reg != YT_region_none)
            {
                tmp = YouTubeService::getRegionName(reg);
                if (string_ok(tmp))
                    aux->put(_(YOUTUBE_AUXDATA_REGION), tmp);
            }
        }
#endif // YouTube
#ifdef HAVE_ATRAILERSSSS
        tmp = obj->getAuxData(_(ATRAILERS_AUXDATA_POST_DATE));
        if (string_ok(tmp))
            aux->put(_(ATRAILERS_AUXDATA_POST_DATE), tmp);
#endif

        Ref<Array<DictionaryElement> > elements = aux->getElements();
        int len = elements->size();
        for (int i = 0; i < len; i++)
        {
            Ref<DictionaryElement> el = elements->get(i);
            setProperty(aux_js, el->getKey(), el->getValue());
        }
    }


    /// \todo add resources

    // CdsItem
    if (IS_CDS_ITEM(objectType))
    {
        Ref<CdsItem> item = RefCast(obj, CdsItem);
        val = item->getMimeType();
        if (val != nil)
            setProperty(js, _("mimetype"), val);

        val = item->getServiceID();
        if (val != nil)
            setProperty(js, _("serviceID"), val);

        if (IS_CDS_ACTIVE_ITEM(objectType))
        {
            Ref<CdsActiveItem> aitem = RefCast(obj, CdsActiveItem);
            val = aitem->getAction();
            if (val != nil)
                setProperty(js, _("action"), val);
            val = aitem->getState();
            if (val != nil)
                setProperty(js, _("state"), val);
        }
    }

    // CdsDirectory
    if (IS_CDS_CONTAINER(objectType))
    {
        Ref<CdsContainer> cont = RefCast(obj, CdsContainer);
        // TODO: boolean type, hide updateID
        i = cont->getUpdateID();
        setIntProperty(js, _("updateID"), i);

        i = cont->isSearchable();
        setIntProperty(js, _("searchable"), i);
    }
}

String Script::convertToCharset(String str, charset_convert_t chr)
{
    switch (chr)
    {
        case P2I:
            return _p2i->convert(str);
        case M2I:
            return _m2i->convert(str);
        case F2I:
            return _f2i->convert(str);
        case J2I:
            return _j2i->convert(str);
        default:
            return _i2i->convert(str);
    }

    return nil;
}

Ref<CdsObject> Script::getProcessedObject()
{
    return processed;
}

#endif // HAVE_JS
