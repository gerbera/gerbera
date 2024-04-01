/*MT*

    MediaTomb - http://www.mediatomb.cc/

    playlist_parser_script.cc - this file is part of MediaTomb.

    Copyright (C) 2005 Gena Batyan <bgeradz@mediatomb.cc>,
                       Sergey 'Jin' Bostandzhyan <jin@mediatomb.cc>

    Copyright (C) 2006-2010 Gena Batyan <bgeradz@mediatomb.cc>,
                            Sergey 'Jin' Bostandzhyan <jin@mediatomb.cc>,
                            Leonhard Wimmer <leo@mediatomb.cc>

    Copyright (C) 2016-2024 Gerbera Contributors

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

/// \file playlist_parser_script.cc

#ifdef HAVE_JS
#define LOG_FAC log_facility_t::script

#include "playlist_parser_script.h" // API

#include "cds/cds_container.h"
#include "cds/cds_item.h"
#include "config/config_manager.h"
#include "content/content_manager.h"
#include "database/database.h"
#include "scripting_runtime.h"
#include "util/string_converter.h"

PlaylistParserScript::PlaylistParserScript(const std::shared_ptr<ContentManager>& content, const std::string& parent)
    : ParserScript(content, parent, "playlist", "playlist")
{
    std::string scriptPath = config->getOption(CFG_IMPORT_SCRIPTING_PLAYLIST_SCRIPT);
    if (!scriptPath.empty()) {
        load(scriptPath);
        scriptMode = true;
    } else {
        playlistFunction = config->getOption(CFG_IMPORT_SCRIPTING_IMPORT_FUNCTION_PLAYLIST);
    }
}

std::pair<std::shared_ptr<CdsObject>, int> PlaylistParserScript::createObject2cdsObject(const std::shared_ptr<CdsObject>& origObject, const std::string& rootPath)
{
    int otype = getIntProperty("objectType", -1);
    if (otype == -1) {
        log_error("missing objectType property");
        return { {}, INVALID_OBJECT_ID };
    }

    if (IS_CDS_ITEM_EXTERNAL_URL(otype))
        return { dukObject2cdsObject(origObject), INVALID_OBJECT_ID };

    fs::path loc = fs::weakly_canonical(getProperty("location")); // make sure relative paths can be retrieved
    std::error_code ec;
    auto dirEnt = fs::directory_entry(loc, ec);
    if (!ec && isRegularFile(dirEnt, ec)) {
        auto mainObj = database->findObjectByPath(dirEnt.path());

        if (!mainObj) {
            AutoScanSetting asSetting;
            asSetting.followSymlinks = config->getBoolOption(CFG_IMPORT_FOLLOW_SYMLINKS);
            asSetting.recursive = false;
            asSetting.hidden = config->getBoolOption(CFG_IMPORT_HIDDEN_FILES);
            asSetting.rescanResource = false;
            asSetting.async = false;
            asSetting.adir = content->findAutoscanDirectory(rootPath);
            asSetting.mergeOptions(config, loc);

            mainObj = content->addFile(dirEnt, rootPath, asSetting, false);

            if (!mainObj) {
                log_error("Failed to add object {}", dirEnt.path().string());
                return { {}, INVALID_OBJECT_ID };
            }
        }
        return { dukObject2cdsObject(mainObj), mainObj->getID() };
    }

    log_error("Failed to read {}: {}", loc.c_str(), ec.message());
    return { {}, INVALID_OBJECT_ID };
}

bool PlaylistParserScript::setRefId(const std::shared_ptr<CdsObject>& cdsObj, const std::shared_ptr<CdsObject>& origObject, int pcdId)
{
    if (!cdsObj->isExternalItem()) {
        /// \todo get hidden file setting from config manager?
        /// what about same stuff in content manager, why is it not used
        /// there?
        if (pcdId == INVALID_OBJECT_ID) {
            return false;
        }
        cdsObj->setRefID(pcdId);
        cdsObj->setFlag(OBJECT_FLAG_USE_RESOURCE_REF);
    } else if (config->getBoolOption(CFG_IMPORT_SCRIPTING_PLAYLIST_SCRIPT_LINK_OBJECTS) || config->getBoolOption(CFG_IMPORT_SCRIPTING_PLAYLIST_LINK_OBJECTS)) {
        cdsObj->setFlag(OBJECT_FLAG_PLAYLIST_REF);
        cdsObj->setRefID(origObject->getID());
    }
    return true;
}

void PlaylistParserScript::handleObject2cdsContainer(duk_context* ctx, const std::shared_ptr<CdsObject>& pcd, const std::shared_ptr<CdsContainer>& cont)
{
    if ((config->getBoolOption(CFG_IMPORT_SCRIPTING_PLAYLIST_SCRIPT_LINK_OBJECTS) || config->getBoolOption(CFG_IMPORT_SCRIPTING_PLAYLIST_LINK_OBJECTS)) && cont->getRefID() > 0) {
        cont->setFlag(OBJECT_FLAG_PLAYLIST_REF);
    }
}

void PlaylistParserScript::handleObject2cdsItem(duk_context* ctx, const std::shared_ptr<CdsObject>& pcd, const std::shared_ptr<CdsItem>& item)
{
    int writeThrough = getIntProperty("writeThrough", -1);
    duk_get_prop_string(ctx, -1, "extra");

    if (!duk_is_null_or_undefined(ctx, -1) && duk_is_object(ctx, -1)) {
        duk_to_object(ctx, -1);
        auto keys = getPropertyNames();
        for (auto&& sym : keys) {
            auto val = getProperty(sym);
            if (!val.empty()) {
                val = sc->convert(val);
                item->addMetaData(sym, val);

                if (writeThrough > 0 && pcd) {
                    pcd->removeMetaData(sym);
                    setMetaData(pcd, std::static_pointer_cast<CdsItem>(pcd), sym, val);
                }
            }
        }
    }
    duk_pop(ctx); // extra

    if (writeThrough > 0 && pcd) {
        pcd->removeMetaData(MetadataFields::M_TITLE);
        setMetaData(pcd, std::static_pointer_cast<CdsItem>(pcd), MetaEnumMapper::getMetaFieldName(MetadataFields::M_TITLE), item->getTitle());
        content->updateObject(pcd);
    }

    item->setTrackNumber(getIntProperty("playlistOrder", 0));
    item->setPartNumber(0);
}

void PlaylistParserScript::processPlaylistObject(const std::shared_ptr<CdsObject>& obj, std::shared_ptr<GenericTask> task, const std::string& rootPath)
{
    if (currentObjectID != INVALID_OBJECT_ID || currentHandle || currentLine) {
        throw_std_runtime_error("Recursion in playlists not allowed");
    }

    if (!obj->isPureItem()) {
        throw_std_runtime_error("Calling processPlaylistObject only allowed for pure items");
    }
    auto item = std::static_pointer_cast<CdsItem>(obj);

    log_debug("Checking playlist {} ...", obj->getLocation().string());
    GrbFile file(item->getLocation());
    if (item->getMimeType() != MIME_TYPE_ASX_PLAYLIST) {
        currentHandle = file.open("r");
    } else {
        pugi::xml_parse_result result = xmlDoc->load_file(item->getLocation().c_str());
        if (result.status != pugi::xml_parse_status::status_ok) {
            throw ConfigParseException(result.description());
        }
        root = xmlDoc->document_element();
    }

    currentTask = std::move(task);
    currentObjectID = item->getID();
    currentLine = new char[ONE_TEXTLINE_BYTES];
    currentLine[0] = '\0';

    ScriptingRuntime::AutoLock lock(runtime->getMutex());
    try {
        if (scriptMode) {
            execute(obj, rootPath);
        } else {
            call(obj, nullptr, playlistFunction, rootPath, "");
        }
    } catch (const std::runtime_error&) {
        if (scriptMode) {
            cleanup();
            duk_pop(ctx);
        }

        currentHandle = nullptr;

        delete[] currentLine;
        currentLine = nullptr;

        currentObjectID = INVALID_OBJECT_ID;
        currentTask = nullptr;

        throw;
    }

    currentHandle = nullptr;

    log_debug("Done playlist {} ...", obj->getLocation().string());

    delete[] currentLine;
    currentLine = nullptr;
    xmlDoc->reset();
    root = nullNode;

    currentObjectID = INVALID_OBJECT_ID;
    currentTask = nullptr;

    gc_counter++;
    if (gc_counter > JS_CALL_GC_AFTER_NUM) {
        duk_gc(ctx, 0);
        gc_counter = 0;
    }
}

#endif // HAVE_JS
