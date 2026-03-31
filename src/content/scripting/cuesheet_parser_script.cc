/*GRB*

    Gerbera - https://gerbera.io/

    cuesheet_parser_script.cc - this file is part of Gerbera.

    Copyright (C) 2026 Gerbera Contributors

    Gerbera is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License version 2
    as published by the Free Software Foundation.

    Gerbera is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with Gerbera.  If not, see <http://www.gnu.org/licenses/>.

    $Id$
*/

/// @file content/scripting/cuesheet_parser_script.cc

#ifdef HAVE_JS
#define GRB_LOG_FAC GrbLogFacility::script

#include "cuesheet_parser_script.h" // API

#include "cds/cds_container.h"
#include "cds/cds_item.h"
#include "config/config.h"
#include "config/config_val.h"
#include "content/content.h"
#include "database/database.h"
#include "exceptions.h"
#include "script_property.h"
#include "scripting_runtime.h"
#include "util/string_converter.h"
#include "util/tools.h"

struct CueEntry {
    std::string performer;
    std::string discTitle;
    std::string fileName;
    std::string format;
    std::string track;
    std::string type;
    std::string trackTitle;
    std::string count;
    std::string offset;
};

duk_ret_t jsAddCueObject(duk_context* ctx)
{
    log_debug("start");
    auto self = dynamic_cast<CuesheetParserScript*>(Script::getContextScript(ctx));

    if (!duk_is_object(ctx, 0)) {
        log_debug("No object argument");
        return 0;
    }
    duk_to_object(ctx, 0);
    if (!duk_is_object(ctx, 1)) {
        log_debug("No entry argument");
        return 0;
    }
    duk_to_object(ctx, 1);
    // stack: js_cds_obj cue_entry
    auto containerId = ScriptProperty(ctx, 2, true).getStringValue("-1");
    // stack: js_cds_obj cue_entry containerId rootPath

    try {
        auto rootPath = ScriptProperty(ctx, 3, true).getStringValue("/");
        auto parentId = stoiString(containerId);

        if (parentId <= CDS_ID_ROOT) {
            log_error("Invalid parent id passed to addCueObject: {}", parentId);
            return 0;
        }
        duk_swap_top(ctx, 0);
        // stack: rootPath cue_entry containerId js_cds_obj
        auto cdsObj = self->dukObject2cdsObject(nullptr);
        if (!cdsObj) {
            log_debug("No new object");
            return 0;
        }

        duk_swap_top(ctx, 1);
        // stack: rootPath js_cds_obj containerId cue_entry
        CueEntry entry;
        ScriptProperty(ctx, -1).getObject([&]() {
            entry.performer = ScriptNamedProperty(ctx, "performer").getStringValue();
            entry.discTitle = ScriptNamedProperty(ctx, "discTitle").getStringValue();
            entry.fileName = ScriptNamedProperty(ctx, "location").getStringValue();
            entry.format = ScriptNamedProperty(ctx, "format").getStringValue();
            entry.track = ScriptNamedProperty(ctx, "track").getStringValue();
            entry.type = ScriptNamedProperty(ctx, "type").getStringValue();
            entry.trackTitle = ScriptNamedProperty(ctx, "trackTitle").getStringValue();
            entry.count = ScriptNamedProperty(ctx, "count").getStringValue();
            entry.offset = ScriptNamedProperty(ctx, "offset").getStringValue();
        });

        log_debug("new entry {} {} at {}", entry.track, entry.trackTitle, entry.offset);

        if (self->isHiddenFile(cdsObj, rootPath)) {
            log_debug("Hidden file {} cannot be added", cdsObj->getLocation().c_str());
            return 0;
        }

        if (cdsObj->getLocation().empty())
            cdsObj->setLocation(entry.fileName, CdsEntryType::ExtraFile);
        else
            cdsObj->setEntryType(CdsEntryType::ExtraFile);

        cdsObj->setParentID(parentId);
        cdsObj->setID(INVALID_OBJECT_ID);

        std::shared_ptr<CdsResource> resource = cdsObj->getResource(ContentHandler::DEFAULT);
        if (!resource) {
            resource = std::make_shared<CdsResource>(ContentHandler::DEFAULT, ResourcePurpose::Content);
            cdsObj->addResource(resource);
        }
        long long offset;
        if (parseTime(offset, entry.offset))
            resource->addAttribute(ResourceAttribute::OFFSET, offset);
        if (cdsObj->isItem()) {
            auto item = std::static_pointer_cast<CdsItem>(cdsObj);
            if (item->getTrackNumber() == 0)
                item->setTrackNumber(stoiString(entry.track));
            item->setPartNumber(0);
            log_debug("cues {} {}", item->getTrackNumber(), item->getPartNumber());
        }
        self->getContent()->addObject(cdsObj, false);

        /* setting object ID as return value */
        log_debug("object {} parent {}", cdsObj->getID(), parentId);
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

CuesheetParserScript::CuesheetParserScript(
    const std::shared_ptr<Content>& content,
    const std::string& parent)
    : ParserScript(content, parent, "cuesheet", "cue", false)
{
    doTrim = false;
    defineFunction("updateCdsObject", jsUpdateCdsObject, 1);
    defineFunction("addCueObject", jsAddCueObject, 4);
    cuesheetFunction = config->getOption(ConfigVal::IMPORT_SCRIPTING_IMPORT_FUNCTION_CUESHEET);
}

std::vector<int> CuesheetParserScript::processObject(
    const std::shared_ptr<CdsObject>& obj,
    const fs::path& path)
{
    log_debug("start");
    if (currentObjectID != INVALID_OBJECT_ID || currentHandle || currentLine) {
        throw_std_runtime_error("Recursion in cuesheet not allowed");
    }

    if (!obj->isPureItem()) {
        throw_std_runtime_error("Calling processObject only allowed for pure items");
    }
    auto item = std::static_pointer_cast<CdsItem>(obj);

    log_debug("Checking cuesheet {} ...", obj->getLocation().string());
    GrbFile file(item->getLocation());
    currentHandle = file.open("r");

    currentObjectID = item->getID();
    auto parent = item->getParentID() != INVALID_OBJECT_ID
        ? std::dynamic_pointer_cast<CdsContainer>(database->loadObject(item->getParentID()))
        : nullptr;
    currentLine = new char[ONE_TEXTLINE_BYTES];
    currentLine[0] = '\0';

    ScriptingRuntime::AutoLock lock(runtime->getMutex());
    std::vector<int> result;

    try {
        result = call(obj, parent, cuesheetFunction, path, "");
    } catch (const std::runtime_error&) {
        log_error("Failed cuesheet {} ({})...", obj->getLocation().string(), obj->getID());
        cleanUp();
        throw;
    }
    log_debug("Done cuesheet {} ({})... -> {}", obj->getLocation().string(), obj->getID(), result.size());

    cleanUp();

    return result;
}

std::pair<std::shared_ptr<CdsObject>, int> CuesheetParserScript::createObject2cdsObject(
    const std::shared_ptr<CdsObject>& origObject,
    const std::string& rootPath)
{
    return { {}, INVALID_OBJECT_ID };
}

bool CuesheetParserScript::setRefId(
    const std::shared_ptr<CdsObject>& cdsObj,
    const std::shared_ptr<CdsObject>& origObject,
    int pcdId)
{
    if (!cdsObj->isExternalItem()) {
        if (pcdId == INVALID_OBJECT_ID) {
            return false;
        }
        if (cdsObj->getID() != pcdId) {
            cdsObj->setRefID(pcdId);
            cdsObj->setFlag(ObjectFlag::UseResourceReference);
        }
    }
    return true;
}

void CuesheetParserScript::handleObject2cdsItem(
    duk_context* ctx,
    const std::shared_ptr<CdsObject>& pcd,
    const std::shared_ptr<CdsItem>& item)
{
    item->setEntryType(CdsObject::remapEntryType(ScriptNamedProperty(ctx, "entryType").getStringValue()));
    item->setTrackNumber(ScriptNamedProperty(ctx, "trackNumber").getIntValue(0));
    item->setPartNumber(ScriptNamedProperty(ctx, "partNumber").getIntValue(0));
}

void CuesheetParserScript::handleObject2cdsContainer(
    duk_context* ctx,
    const std::shared_ptr<CdsObject>& pcd,
    const std::shared_ptr<CdsContainer>& cont)
{
    cont->setEntryType(CdsObject::remapEntryType(ScriptNamedProperty(ctx, "entryType").getStringValue()));
}

#endif // HAVE_JS
