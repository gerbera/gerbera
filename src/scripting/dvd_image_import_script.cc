/*MT*
    
    MediaTomb - http://www.mediatomb.cc/
    
    dvd_image_import_script.cc - this file is part of MediaTomb.
    
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

/// \file dvd_image_import_script.cc

#if defined(HAVE_JS) && defined(HAVE_LIBDVDNAV)

#include <js/jsscript.h>

#include "dvd_image_import_script.h"
#include "config_manager.h"
#include "js_functions.h"
#include "metadata/dvd_handler.h"
using namespace zmm;

extern "C" {

static JSBool js_addDVDObject(JSContext *cx, uint32_t argc, jsval *vp)
{
    try
    {
        jsval arg;
        JSObject *js_cds_obj;
        Ref<CdsObject> cds_obj;
        Ref<CdsObject> processed;
        int title;
        int chapter;
        int audio_track;
        String chain;
        String containerclass;
        JSString *str;


        DVDImportScript *self = (DVDImportScript *)JS_GetPrivate(cx, JS_THIS_OBJECT(cx, vp));

        if (self->whoami() != S_DVD)
        {
            log_error("Function must be called from the DVD import script!\n");
            return JS_TRUE;
        }

        arg = JS_ARGV(cx, vp)[0];
        if (!JSVAL_IS_OBJECT(arg))
        {
            log_error("Invalid argument!");
            return JS_TRUE;
        }

        if (!JS_ValueToObject(cx, arg, &js_cds_obj))
        {
            log_error("Could not convert object!");
            return JS_TRUE;
        }

        // root it
        JS_ARGV(cx, vp)[0] = OBJECT_TO_JSVAL(js_cds_obj);

        if (!JS_ValueToInt32(cx, JS_ARGV(cx, vp)[1], &title))
        {
            log_error("addDVDObject: Invalid DVD title number given!\n");
            return JS_TRUE;
        }

        if (!JS_ValueToInt32(cx, JS_ARGV(cx, vp)[2], &chapter))
        {
            log_error("addDVDObject: Invalid DVD chapter number given!\n");
            return JS_TRUE;
        }

        if (!JS_ValueToInt32(cx, JS_ARGV(cx, vp)[3], &audio_track))
        {
            log_error("addDVDObject: Invalid DVD audio track number given!\n");
            return JS_TRUE;
        }

        str = JS_ValueToString(cx, JS_ARGV(cx, vp)[4]);
        if (!str)
        {
            log_error("addDVDObject: Invalid DVD container chain given!\n");
            return JS_TRUE;
        }

        char *ts = JS_EncodeString(cx, str);
        if (ts) {
            chain = ts;
            JS_free(cx, ts);
        }

        if (!string_ok(chain) || chain == "undefined")
        {
            log_error("addDVDObject: Invalid DVD container chain given!\n");
            return JS_TRUE;
        }

        JSString *cont = JS_ValueToString(cx, JS_ARGV(cx, vp)[5]);
        if (cont) {
            char *ts = JS_EncodeString(cx, cont);
            if (ts) {
                containerclass = ts;
                JS_free(cx, ts);
            }

            if (!string_ok(containerclass) || containerclass == "undefined")
                containerclass = nil;
        }

        processed = self->getProcessedObject();
        if (processed == nil)
        {
            log_error("addDVDObject: could not retrieve original object!\n");
            return JS_TRUE;
        }


        // convert incoming object
        cds_obj = self->jsObject2cdsObject(js_cds_obj, processed);
        if (cds_obj == nil)
        {
            log_error("addDVDObject: could not convert js object!\n");
            return JS_TRUE;
        }

        self->addDVDObject(cds_obj, title, chapter, audio_track, chain,
                           containerclass);

    }
    catch (ServerShutdownException se)
    {
        log_warning("Aborting script execution due to server shutdown.\n");
        return JS_FALSE;
    }
    catch (Exception ex)
    {
        log_error("%s\n", ex.getMessage().c_str());
        ex.printStackTrace();
    }

    return JS_TRUE;
}

} // extern "C" 

void DVDImportScript::addDVDObject(Ref<CdsObject> obj, int title,
                                  int chapter, int audio_track, String chain,
                                  String containerclass)
{

    if (processed == nil)
        throw _Exception(_("Invalid original object!"));

    if (processed->getID() == INVALID_OBJECT_ID)
        throw _Exception(_("Original object not yet in database!"));

    obj->setRefID(processed->getID());
    obj->setID(INVALID_OBJECT_ID);
    obj->setVirtual(1);
    obj->clearFlag(OBJECT_FLAG_USE_RESOURCE_REF);

    Ref<ContentManager> cm = ContentManager::getInstance();

    int id = cm->addContainerChain(chain, containerclass, processed->getID());
    obj->setParentID(id);

    RefCast(obj, CdsItem)->setMimeType(mimetype);
    obj->getResource(0)->addAttribute(MetadataHandler::getResAttrName(R_PROTOCOLINFO), renderProtocolInfo(mimetype));

    /// \todo this has to be changed once we add seeking
    obj->getResource(0)->removeAttribute(MetadataHandler::getResAttrName(R_SIZE));
    obj->getResource(0)->addParameter(DVDHandler::renderKey(DVD_AudioStreamID), processed->getAuxData(DVDHandler::renderKey(DVD_AudioTrackStreamID, title, 0,
                    audio_track)));

    String tmp = processed->getAuxData(DVDHandler::renderKey(DVD_AudioTrackChannels, title, 0, audio_track));
    if (string_ok(tmp))
    {
        obj->getResource(0)->addAttribute(MetadataHandler::getResAttrName(R_NRAUDIOCHANNELS), tmp);
        log_debug("Setting Audio Channels, object %s -  num: %s\n",
                obj->getLocation().c_str(), tmp.c_str());
    }

    tmp = processed->getAuxData(DVDHandler::renderKey(DVD_AudioTrackSampleFreq, title, 0, audio_track));
    if (string_ok(tmp))
        obj->getResource(0)->addAttribute(MetadataHandler::getResAttrName(R_SAMPLEFREQUENCY), tmp);

    obj->getResource(0)->addParameter(DVDHandler::renderKey(DVD_Chapter),
            String::from(chapter));

//    tmp = processed->getAuxData(DVDHandler::renderKey(DVD_ChapterRestDuration,
//                title, chapter));
//    if (string_ok(tmp))
//        obj->getResource(0)->addAttribute(MetadataHandler::getResAttrName(R_DURATION), tmp);

    obj->getResource(0)->addParameter(DVDHandler::renderKey(DVD_Title),
            String::from(title));

    cm->addObject(obj);
}

DVDImportScript::DVDImportScript(Ref<Runtime> runtime) : Script(runtime)
{

#ifdef JS_THREADSAFE
    JS_SetContextThread(cx);
    JS_BeginRequest(cx);
#endif

    try
    {
        defineFunction(_("addDVDObject"), js_addDVDObject, 7);

        setProperty(glob, _("DVD"), _("DVD"));

        String scriptPath = ConfigManager::getInstance()->getOption(CFG_IMPORT_SCRIPTING_DVD_SCRIPT);
        load(scriptPath);
        JS_AddNamedObjectRoot(cx, &root, "DVDImportScript");
        log_info("Loaded %s\n", scriptPath.c_str());

         Ref<Dictionary> mappings =
                         ConfigManager::getInstance()->getDictionaryOption(
                              CFG_IMPORT_MAPPINGS_MIMETYPE_TO_CONTENTTYPE_LIST);

         mimetype = mappings->get(_(CONTENT_TYPE_MPEG));
         if (!string_ok(mimetype))
             mimetype = _("video/mpeg");
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

void DVDImportScript::processDVDObject(Ref<CdsObject> obj)
{
#ifdef JS_THREADSAFE
    JS_SetContextThread(cx);
    JS_BeginRequest(cx);
#endif
    processed = obj;
    try
    {
        JSObject *orig = JS_NewObject(cx, NULL, NULL, glob);
        setObjectProperty(glob, _("dvd"), orig);
        cdsObject2jsObject(obj, orig);
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

DVDImportScript::~DVDImportScript()
{
#ifdef JS_THREADSAFE
    JS_SetContextThread(cx);
    JS_BeginRequest(cx);
#endif

    if (root)
        JS_RemoveObjectRoot(cx, &script);

#ifdef JS_THREADSAFE
    JS_EndRequest(cx);
    JS_ClearContextThread(cx);
#endif

}

#endif // HAVE_JS
