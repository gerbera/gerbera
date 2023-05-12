/*MT*

    MediaTomb - http://www.mediatomb.cc/

    playlist_parser_script.cc - this file is part of MediaTomb.

    Copyright (C) 2005 Gena Batyan <bgeradz@mediatomb.cc>,
                       Sergey 'Jin' Bostandzhyan <jin@mediatomb.cc>

    Copyright (C) 2006-2010 Gena Batyan <bgeradz@mediatomb.cc>,
                            Sergey 'Jin' Bostandzhyan <jin@mediatomb.cc>,
                            Leonhard Wimmer <leo@mediatomb.cc>

    Copyright (C) 2016-2023 Gerbera Contributors

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
#include "js_functions.h"
#include "scripting_runtime.h"
#include "util/string_converter.h"
#include "util/tools.h"

#define ONE_TEXTLINE_BYTES 1024

duk_ret_t jsReadXml(duk_context* ctx)
{
    auto self = dynamic_cast<ParserScript*>(Script::getContextScript(ctx));
    if (!self) {
        log_error("readXml can only be called for ParserScript.");
        return 0;
    }

    if (!duk_is_number(ctx, 0)) {
        log_error("readXml must be called with integer argument.");
        return 0;
    }

    int direction = duk_to_int(ctx, 0);
    duk_pop(ctx);

    try {
        auto node = self->readXml(direction);
        if (!node) {
            log_debug("No matching node {} found.", direction);
            return 0;
        }
        duk_push_object(ctx);
        for (auto&& attrib : node.attributes()) {
            duk_push_string(ctx, attrib.value());
            duk_put_prop_string(ctx, -2, toLower(attrib.name()).c_str());
        }

        duk_push_string(ctx, node.text().as_string());
        duk_put_prop_string(ctx, -2, "VALUE");
        duk_push_string(ctx, toLower(node.name()).c_str());
        duk_put_prop_string(ctx, -2, "NAME");
    } catch (const ServerShutdownException&) {
        log_warning("Aborting script execution due to server shutdown.");
        return duk_error(ctx, DUK_ERR_ERROR, "Aborting script execution due to server shutdown.");
    } catch (const std::runtime_error& e) {
        log_error("DUK exception: {}", e.what());
        return 0;
    }

    return 1;
}

duk_ret_t jsReadln(duk_context* ctx)
{
    auto self = dynamic_cast<ParserScript*>(Script::getContextScript(ctx));
    if (!self) {
        log_error("readln can only be called for ParserScript.");
        return 0;
    }

    try {
        auto [line, trimmed] = self->readLine();
        duk_push_string(ctx, line.c_str());
        return trimmed ? 1 : 0;
    } catch (const ServerShutdownException&) {
        log_warning("Aborting script execution due to server shutdown.");
        return duk_error(ctx, DUK_ERR_ERROR, "Aborting script execution due to server shutdown.");
    } catch (const std::runtime_error& e) {
        log_error("DUK exception: {}", e.what());
        return 0;
    }
}

duk_ret_t jsGetCdsObject(duk_context* ctx)
{
    auto self = dynamic_cast<ParserScript*>(Script::getContextScript(ctx));
    if (!self) {
        log_error("getCdsObject can only be called for ParserScript.");
        return 0;
    }

    if (!duk_is_string(ctx, 0))
        return 0;

    std::error_code ec;
    fs::path path = fs::weakly_canonical(duk_to_string(ctx, 0), ec);
    duk_pop(ctx);

    if (ec || path.empty())
        return 0;

    auto database = self->getDatabase();
    auto obj = database->findObjectByPath(path);
    if (!obj) {
        auto cm = self->getContent();
        ec.clear();
        auto dirEnt = fs::directory_entry(path, ec);
        if (!ec) {
            auto adir = cm->findAutoscanDirectory(path);
            obj = cm->createObjectFromFile(adir, dirEnt, false);
        } else {
            log_error("Failed to read {}: {}", path.c_str(), ec.message());
        }
        if (!obj) { // object ignored
            return 0;
        }
    }
    self->cdsObject2dukObject(obj);
    return 1;
}

duk_ret_t jsUpdateCdsObject(duk_context* ctx)
{
    auto self = dynamic_cast<MetafileParserScript*>(Script::getContextScript(ctx));
    if (!self) {
        log_error("updateCdsObject can only be called for MetafileParserScript.");
        return 0;
    }

    if (self->getOrigName().empty())
        duk_push_undefined(ctx);
    else
        duk_get_global_string(ctx, self->getOrigName().c_str());
    // stack: js_cds_obj

    if (duk_is_undefined(ctx, -1)) {
        log_debug("Could not retrieve global {} object", self->getOrigName());
        return 0;
    }
    auto origObject = self->dukObject2cdsObject(self->getProcessedObject());
    if (!origObject) {
        log_debug("Could not load global {} object", self->getOrigName());
        return 0;
    }
    return 1;
}

ParserScript::ParserScript(const std::shared_ptr<ContentManager>& content, const std::string& parent,
    const std::string& name, const std::string& objName)
    : Script(content, parent, name, objName, StringConverter::p2i(content->getContext()->getConfig()))
{
    defineFunction("readln", jsReadln, 0);
    defineFunction("readXml", jsReadXml, 1);
    defineFunction("getCdsObject", jsGetCdsObject, 1);
}

static pugi::xml_node nullNode;

pugi::xml_node& ParserScript::readXml(int direction)
{
    if (!root)
        throw_std_runtime_error("readXml not yet setup for use");

    if (currentTask && !currentTask->isValid())
        return root;

    if (direction > 0 && root.first_child()) {
        root = root.first_child();
        return root;
    }
    if (direction < -1 && root.root()) {
        root = xmlDoc->document_element();
        return root;
    }
    if (direction < 0 && root.parent()) {
        root = root.parent();
        return root;
    }
    if (root.next_sibling()) {
        root = root.next_sibling();
        return root;
    }
    return nullNode;
}

std::pair<std::string, bool> ParserScript::readLine()
{
    if (!currentHandle)
        throw_std_runtime_error("readLine not yet setup for use");

    if (currentTask && !currentTask->isValid())
        return { {}, false };

    while (true) {
        if (!fgets(currentLine, ONE_TEXTLINE_BYTES, currentHandle))
            return { {}, false };
        auto ret = trimString(currentLine);
        if (!ret.empty())
            return { ret, true };
    }
}

PlaylistParserScript::PlaylistParserScript(const std::shared_ptr<ContentManager>& content, const std::string& parent)
    : ParserScript(content, parent, "playlist", "playlist")
{
    std::string scriptPath = config->getOption(CFG_IMPORT_SCRIPTING_PLAYLIST_SCRIPT);
    if (!scriptPath.empty()) {
        load(scriptPath);
        scriptMode = true;
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

    fs::path loc = getProperty("location");
    std::error_code ec;
    auto dirEnt = fs::directory_entry(loc, ec);
    if (!ec) {
        AutoScanSetting asSetting;
        asSetting.followSymlinks = config->getBoolOption(CFG_IMPORT_FOLLOW_SYMLINKS);
        asSetting.recursive = false;
        asSetting.hidden = config->getBoolOption(CFG_IMPORT_HIDDEN_FILES);
        asSetting.rescanResource = false;
        asSetting.mergeOptions(config, loc);

        auto mainObj = content->addFile(dirEnt, rootPath, asSetting, false);
        if (!mainObj) {
            log_error("Failed to add object {}", dirEnt.path().string());
            return { {}, INVALID_OBJECT_ID };
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
    } else if (config->getBoolOption(CFG_IMPORT_SCRIPTING_PLAYLIST_SCRIPT_LINK_OBJECTS)) {
        cdsObj->setFlag(OBJECT_FLAG_PLAYLIST_REF);
        cdsObj->setRefID(origObject->getID());
    }
    return true;
}

void PlaylistParserScript::handleObject2cdsContainer(duk_context* ctx, const std::shared_ptr<CdsObject>& pcd, const std::shared_ptr<CdsContainer>& cont)
{
    if (config->getBoolOption(CFG_IMPORT_SCRIPTING_PLAYLIST_SCRIPT_LINK_OBJECTS) && cont->getRefID() > 0) {
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
        pcd->removeMetaData(M_TITLE);
        setMetaData(pcd, std::static_pointer_cast<CdsItem>(pcd), MetadataHandler::getMetaFieldName(M_TITLE), item->getTitle());
        content->updateObject(pcd);
    }

    item->setTrackNumber(getIntProperty("playlistOrder", 0));
    item->setPartNumber(0);
}

void PlaylistParserScript::processPlaylistObject(const std::shared_ptr<CdsObject>& obj, std::shared_ptr<GenericTask> task, const std::string& rootPath)
{
    if ((currentObjectID != INVALID_OBJECT_ID) || currentHandle || currentLine) {
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
            auto playlistFunction = config->getOption(CFG_IMPORT_SCRIPTING_IMPORT_FUNCTION_PLAYLIST);
            call(obj, playlistFunction, rootPath, "");
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

MetafileParserScript::MetafileParserScript(const std::shared_ptr<ContentManager>& content)
    : ParserScript(content, "", "metafile", "obj")
{
    std::string scriptPath = config->getOption(CFG_IMPORT_SCRIPTING_METAFILE_SCRIPT);
    defineFunction("updateCdsObject", jsUpdateCdsObject, 0);
    if (!scriptPath.empty()) {
        load(scriptPath);
        scriptMode = true;
    }
}

void MetafileParserScript::processObject(const std::shared_ptr<CdsObject>& obj, const fs::path& path)
{
    if ((currentObjectID != INVALID_OBJECT_ID) || currentHandle || currentLine) {
        throw_std_runtime_error("Recursion in playlists not allowed");
    }

    if (!obj->isPureItem()) {
        throw_std_runtime_error("Calling processObject only allowed for pure items");
    }
    auto item = std::static_pointer_cast<CdsItem>(obj);

    log_debug("Checking metafile {} for {}...", path.string(), obj->getLocation().string());
    GrbFile file(path);
    pugi::xml_parse_result result = xmlDoc->load_file(path.c_str());
    if (result.status == pugi::xml_parse_status::status_ok) {
        root = xmlDoc->document_element();
    }
    currentHandle = file.open("r");
    processed = obj;

    currentObjectID = item->getID();
    currentLine = new char[ONE_TEXTLINE_BYTES];
    currentLine[0] = '\0';

    ScriptingRuntime::AutoLock lock(runtime->getMutex());
    try {
        if (scriptMode) {
            execute(obj, path);
        } else {
            auto metafileFunction = config->getOption(CFG_IMPORT_SCRIPTING_IMPORT_FUNCTION_METAFILE);
            call(obj, metafileFunction, path, "");
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

        throw;
    }

    currentHandle = nullptr;

    log_debug("Done metafile {} ...", path.string());

    delete[] currentLine;
    currentLine = nullptr;
    xmlDoc->reset();
    root = nullNode;

    currentObjectID = INVALID_OBJECT_ID;

    gc_counter++;
    if (gc_counter > JS_CALL_GC_AFTER_NUM) {
        duk_gc(ctx, 0);
        gc_counter = 0;
    }
}

std::pair<std::shared_ptr<CdsObject>, int> MetafileParserScript::createObject2cdsObject(const std::shared_ptr<CdsObject>& origObject, const std::string& rootPath)
{
    return { {}, INVALID_OBJECT_ID };
}

bool MetafileParserScript::setRefId(const std::shared_ptr<CdsObject>& cdsObj, const std::shared_ptr<CdsObject>& origObject, int pcdId)
{
    if (!cdsObj->isExternalItem()) {
        cdsObj->setRefID(origObject->getID());
        cdsObj->setFlag(OBJECT_FLAG_USE_RESOURCE_REF);
    }
    return true;
}

void MetafileParserScript::handleObject2cdsItem(duk_context* ctx, const std::shared_ptr<CdsObject>& pcd, const std::shared_ptr<CdsItem>& item)
{
    item->setTrackNumber(getIntProperty("trackNumber", 0));
    item->setPartNumber(getIntProperty("partNumber", 0));
}

std::shared_ptr<CdsObject> MetafileParserScript::createObject(const std::shared_ptr<CdsObject>& pcd)
{
    duk_get_prop_string(ctx, -1, "aux");
    if (!duk_is_null_or_undefined(ctx, -1) && duk_is_object(ctx, -1)) {
        duk_to_object(ctx, -1);
        auto keys = getPropertyNames();
        for (auto&& sym : keys) {
            auto val = getProperty(sym);
            if (!val.empty()) {
                val = sc->convert(val);
                pcd->setAuxData(sym, val);
            }
        }
    }
    duk_pop(ctx); // aux
    return pcd;
}

#endif // HAVE_JS
