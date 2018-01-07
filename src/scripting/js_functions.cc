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

#ifdef HAVE_JS

#include "js_functions.h"

#include "script.h"
#include <typeinfo>
#include "storage.h"
#include "content_manager.h"
#include "config_manager.h"
#include "metadata_handler.h"

using namespace zmm;

//extern "C" {

duk_ret_t js_print(duk_context *ctx)
{
    duk_push_string(ctx, " ");
    duk_insert(ctx, 0);
    duk_join(ctx, duk_get_top(ctx)-1);
    log_js("%s\n", duk_get_string(ctx, 0));
    return 0;
}

duk_ret_t js_copyObject(duk_context *ctx)
{
    auto *self = Script::getContextScript(ctx);
    if (!duk_is_object(ctx, 0))
        return duk_error(ctx, DUK_ERR_TYPE_ERROR, "copyObject argument is not an object");
    Ref<CdsObject> cds_obj = self->dukObject2cdsObject(nullptr);
    self->cdsObject2dukObject(cds_obj);
    return 1;
}

duk_ret_t js_addCdsObject(duk_context *ctx)
{
    auto *self = Script::getContextScript(ctx);

    if (!duk_is_object(ctx, 0))
        return 0;
    duk_to_object(ctx, 0);
    //stack: js_cds_obj
    const char *ts = duk_to_string(ctx, 1);
    if (!ts)
        ts = "/";
    String path = ts;
    //stack: js_cds_obj path
    String containerclass = nullptr;
    if (!duk_is_null_or_undefined(ctx, 2))
    {
        containerclass = duk_to_string(ctx, 2);
        if (!string_ok(containerclass))
            containerclass = nullptr;
    }
    //stack: js_cds_obj path containerclass

    try
    {
        Ref<CdsObject> orig_object;

        Ref<StringConverter> p2i;
        Ref<StringConverter> i2i;

        if (self->whoami() == S_PLAYLIST)
        {
            p2i = StringConverter::p2i();
        }
        else
        {
            i2i = StringConverter::i2i();
        }

        if (self->whoami() == S_PLAYLIST)
            duk_get_global_string(ctx, "playlist");
        else if (self->whoami() == S_IMPORT)
            duk_get_global_string(ctx, "orig");
        else
            duk_push_undefined(ctx);
        //stack: js_cds_obj path containerclass js_orig_obj

        if (duk_is_undefined(ctx, -1))
        {
            log_debug("Could not retrieve orig/playlist object\n");
            return 0;
        }

        orig_object = self->dukObject2cdsObject(self->getProcessedObject());
        if (orig_object == nullptr)
            return 0;

        Ref<CdsObject> cds_obj;
        Ref<ContentManager> cm = ContentManager::getInstance();
        int pcd_id = INVALID_OBJECT_ID;

        duk_swap_top(ctx, 0);
        //stack: js_orig_obj path containerclass js_cds_obj
        if (self->whoami() == S_PLAYLIST)
        {
            int otype = self->getIntProperty(_("objectType"), -1);
            if (otype == -1)
            {
                log_error("missing objectType property\n");
                return 0;
            }

            if (!IS_CDS_ITEM_EXTERNAL_URL(otype) &&
                !IS_CDS_ITEM_INTERNAL_URL(otype))
            {
                String loc = self->getProperty(_("location"));
                if (string_ok(loc) &&
                   (IS_CDS_PURE_ITEM(otype) || IS_CDS_ACTIVE_ITEM(otype)))
                    loc = normalizePath(loc);

                pcd_id = cm->addFile(loc, false, false, true);
                if (pcd_id == INVALID_OBJECT_ID)
                {
                    return 0;
                }

                Ref<CdsObject> mainObj = Storage::getInstance()->loadObject(pcd_id);
                cds_obj = self->dukObject2cdsObject(mainObj);
            }
            else
                cds_obj = self->dukObject2cdsObject(self->getProcessedObject());
        }
        else
            cds_obj = self->dukObject2cdsObject(orig_object);

        if (cds_obj == nullptr)
        {
            return 0;
        }

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
                {
                    return 0;
                }

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
        duk_push_string(ctx, tmp.c_str());
        return 1;
    }
    catch (const ServerShutdownException & se)
    {
        log_warning("Aborting script execution due to server shutdown.\n");
        return duk_error(ctx, DUK_ERR_ERROR, "Aborting script execution due to server shutdown.\n");
    }
    catch (const Exception & e)
    {
        log_error("%s\n", e.getMessage().c_str());
        e.printStackTrace();
    }
    return 0;
}

static duk_ret_t convert_charset_generic(duk_context *ctx, charset_convert_t chr)
{
    auto *self = Script::getContextScript(ctx);
    if (duk_get_top(ctx) != 1)
        return DUK_RET_SYNTAX_ERROR;
    if (!duk_is_string(ctx, 0))
        return DUK_RET_TYPE_ERROR;
    const char *ts = duk_to_string(ctx, 0);
    duk_pop(ctx);

    try
    {
        String result = self->convertToCharset(_(ts), chr);
        duk_push_lstring(ctx, result.c_str(), result.length());
        return 1;
    }
    catch (const ServerShutdownException & se)
    {
        log_warning("Aborting script execution due to server shutdown.\n");
        return DUK_RET_ERROR;
    }
    catch (const Exception & e)
    {
        log_error("%s\n", e.getMessage().c_str());
        e.printStackTrace();
    }
    return 0;
}


duk_ret_t js_f2i(duk_context *ctx)
{
    return convert_charset_generic(ctx, F2I);
}

duk_ret_t js_m2i(duk_context *ctx)
{
    return convert_charset_generic(ctx, M2I);
}

duk_ret_t js_p2i(duk_context *ctx)
{
    return convert_charset_generic(ctx, P2I);
}

duk_ret_t js_j2i(duk_context *ctx)
{
    return convert_charset_generic(ctx, J2I);
}

//} // extern "C"

#endif //HAVE_JS
