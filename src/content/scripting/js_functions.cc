/*MT*

    MediaTomb - http://www.mediatomb.cc/

    js_functions.cc - this file is part of MediaTomb.

    Copyright (C) 2005 Gena Batyan <bgeradz@mediatomb.cc>,
                       Sergey 'Jin' Bostandzhyan <jin@mediatomb.cc>

    Copyright (C) 2006-2010 Gena Batyan <bgeradz@mediatomb.cc>,
                            Sergey 'Jin' Bostandzhyan <jin@mediatomb.cc>,
                            Leonhard Wimmer <leo@mediatomb.cc>

    Copyright (C) 2016-2025 Gerbera Contributors

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
#define GRB_LOG_FAC GrbLogFacility::script
#include "js_functions.h" // API

#include "cds/cds_objects.h"
#include "content/content.h"
#include "database/database.h"
#include "exceptions.h"
#include "script.h"
#include "script_property.h"
#include "util/string_converter.h"
#include "util/tools.h"

duk_ret_t js_print2(duk_context* ctx)
{
    std::string mode = toUpper(duk_get_string(ctx, 0));
    duk_push_string(ctx, " ");
    duk_replace(ctx, 0);
    duk_join(ctx, duk_get_top(ctx) - 1);
    std::string message = duk_get_string(ctx, 0);
    if (mode == "ERROR")
        log_error("{}", message);
    else if (mode == "WARNING")
        log_warning("{}", message);
    else if (mode == "DEBUG")
        log_debug("{}", message);
    else
        log_info("{}", message);
    duk_push_string(ctx, fmt::format("{}: {}", mode, message).c_str());
    return 1;
}

duk_ret_t js_print(duk_context* ctx)
{
    duk_push_string(ctx, " ");
    duk_insert(ctx, 0);
    duk_join(ctx, duk_get_top(ctx) - 1);
    auto message = duk_get_string(ctx, 0);
    log_info("{}", message);
    duk_push_string(ctx, message);
    return 1;
}

duk_ret_t js_copyObject(duk_context* ctx)
{
    auto self = Script::getContextScript(ctx);
    if (!duk_is_object(ctx, 0))
        return duk_error(ctx, DUK_ERR_TYPE_ERROR, "copyObject argument is not an object");
    auto cdsObj = self->dukObject2cdsObject(nullptr);
    self->cdsObject2dukObject(cdsObj);
    return 1;
}

duk_ret_t js_addContainerTree(duk_context* ctx)
{
    auto self = Script::getContextScript(ctx);

    if (!duk_is_array(ctx, 0)) {
        log_debug("No Array");
        return 0;
    }

    std::vector<std::shared_ptr<CdsObject>> chain;
    auto length = duk_get_length(ctx, -1);

    for (duk_uarridx_t i = 0; i < length; i++) {
        if (duk_get_prop_index(ctx, -1, i)) {
            if (!duk_is_object(ctx, -1)) {
                duk_pop(ctx);
                log_debug("no object at {}", i);
                break;
            }
            duk_to_object(ctx, -1);
            auto cdsObj = self->dukObject2cdsObject(nullptr);
            if (cdsObj) {
                chain.push_back(std::move(cdsObj));
            } else {
                log_debug("no CdsObject at {}", i);
            }
        }
        duk_pop(ctx);
    }

    if (!chain.empty()) {
        auto lastItem = chain.at(chain.size() - 1);
        if (lastItem->isContainer()) {
            log_debug("lastContainer {}, resSize {}, refID {}", lastItem->getTitle(), lastItem->getResourceCount(), lastItem->getRefID());
            if (lastItem->getResourceCount() == 0 && lastItem->getRefID() >= CDS_ID_ROOT) {
                lastItem = self->getDatabase()->loadObject(lastItem->getRefID());
                log_debug("lastItem from RefID {}, resSize {}, loc {}", lastItem->getTitle(), lastItem->getResourceCount(), lastItem->getLocation().string());
            }
        } else
            log_debug("lastItem {}, resSize {}, refID {}", lastItem->getTitle(), lastItem->getResourceCount(), lastItem->getRefID());
        auto cm = self->getContent();
        auto containerId = std::get<0>(cm->addContainerTree(chain, lastItem));
        if (containerId != INVALID_OBJECT_ID) {
            /* setting last container ID as return value */
            log_debug("container {}", containerId);
            duk_push_string(ctx, fmt::to_string(containerId).c_str());
            return 1;
        }
    }
    log_debug("Chain empty");

    return 0;
}

duk_ret_t js_addCdsObject(duk_context* ctx)
{
    auto self = Script::getContextScript(ctx);

    if (!duk_is_object(ctx, 0)) {
        log_debug("No object argument");
        return 0;
    }
    duk_to_object(ctx, 0);
    // stack: js_cds_obj
    const char* containerId = duk_to_string(ctx, 1);
    if (!containerId)
        containerId = "-1";
    // stack: js_cds_obj containerId

    try {
        std::string rootPath = ScriptGlobalProperty(ctx, "object_script_path").getStringValue();

        if (self->getOrigName().empty())
            duk_push_undefined(ctx);
        else
            duk_get_global_string(ctx, self->getOrigName().c_str());
        // stack: js_cds_obj containerId js_orig_obj

        if (duk_is_undefined(ctx, -1)) {
            log_debug("Could not retrieve global {} object", self->getOrigName());
            return 0;
        }

        auto origObject = self->dukObject2cdsObject(self->getProcessedObject());
        if (!origObject) {
            log_debug("No orig object");
            return 0;
        }

        duk_swap_top(ctx, 0);
        // stack: js_orig_obj containerId js_cds_obj
        auto [cdsObj, pcdId] = self->createObject2cdsObject(origObject, rootPath);
        if (!cdsObj) {
            log_debug("No content object");
            return 0;
        }

        auto parentId = stoiString(containerId);

        if (parentId <= 0) {
            log_error("Invalid parent id passed to addCdsObject: {}", parentId);
            return 0;
        }

        cdsObj->setParentID(parentId);
        if (!self->setRefId(cdsObj, origObject, pcdId)) {
            log_debug("No ref object");
            return 0;
        }

        if (self->isHiddenFile(cdsObj, rootPath)) {
            log_debug("Hidden file {} cannot be added", cdsObj->getLocation().c_str());
            return 0;
        }
        cdsObj->setID(INVALID_OBJECT_ID);
        self->getContent()->addObject(cdsObj, false);

        /* setting object ID as return value */
        duk_push_string(ctx, fmt::to_string(cdsObj->getID()).c_str());
        return 1;
    } catch (const ServerShutdownException&) {
        log_warning("Aborting script execution due to server shutdown.");
        return duk_error(ctx, DUK_ERR_ERROR, "Aborting script execution due to server shutdown.\n");
    } catch (const std::runtime_error& e) {
        log_error("{}", e.what());
    }
    return 0;
}

static duk_ret_t convertCharsetGeneric(duk_context* ctx, CharsetConversion chr)
{
    auto self = Script::getContextScript(ctx);
    if (duk_get_top(ctx) != 1)
        return DUK_RET_SYNTAX_ERROR;
    if (!duk_is_string(ctx, 0))
        return DUK_RET_TYPE_ERROR;
    const char* ts = duk_to_string(ctx, 0);
    duk_pop(ctx);

    try {
        std::string result = self->convertToCharset(ts, chr);
        duk_push_lstring(ctx, result.c_str(), result.length());
        return 1;
    } catch (const ServerShutdownException&) {
        log_warning("Aborting script execution due to server shutdown.");
        return DUK_RET_ERROR;
    } catch (const std::runtime_error& e) {
        log_error("{}", e.what());
    }
    return 0;
}

duk_ret_t js_f2i(duk_context* ctx)
{
    return convertCharsetGeneric(ctx, CharsetConversion::F2I);
}

duk_ret_t js_m2i(duk_context* ctx)
{
    return convertCharsetGeneric(ctx, CharsetConversion::M2I);
}

duk_ret_t js_p2i(duk_context* ctx)
{
    return convertCharsetGeneric(ctx, CharsetConversion::P2I);
}

duk_ret_t js_j2i(duk_context* ctx)
{
    return convertCharsetGeneric(ctx, CharsetConversion::J2I);
}

#endif // HAVE_JS
