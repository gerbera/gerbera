/*MT*

    MediaTomb - http://www.mediatomb.cc/

    playlist_parser_script.cc - this file is part of MediaTomb.

    Copyright (C) 2005 Gena Batyan <bgeradz@mediatomb.cc>,
                       Sergey 'Jin' Bostandzhyan <jin@mediatomb.cc>

    Copyright (C) 2006-2010 Gena Batyan <bgeradz@mediatomb.cc>,
                            Sergey 'Jin' Bostandzhyan <jin@mediatomb.cc>,
                            Leonhard Wimmer <leo@mediatomb.cc>

    Copyright (C) 2016-2026 Gerbera Contributors

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

/// @file content/scripting/playlist_parser_script.cc

#define GRB_LOG_FAC GrbLogFacility::script
#include "playlist_parser_script.h" // API

#ifdef HAVE_JS
#include "cds/cds_container.h"
#include "cds/cds_item.h"
#include "config/config.h"
#include "config/config_option_enum.h"
#include "config/config_val.h"
#include "content/autoscan_setting.h"
#include "content/content.h"
#include "database/database.h"
#include "exceptions.h"
#include "script_property.h"
#include "scripting_runtime.h"
#include "util/generic_task.h"
#include "util/logger.h"
#include "util/string_converter.h"
#include "util/tools.h"
#include "util/url_utils.h"

#include <map>
#include <system_error>
#include <utility>

static duk_ret_t jsReadPlaylistEntry(duk_context* ctx)
{
    auto self = dynamic_cast<PlaylistParserScript*>(Script::getContextScript(ctx));
    if (!self)
        return 0;
    std::optional<std::reference_wrapper<const XmlPlaylistEntry>> entry;
    try {
        entry = self->readPlaylistEntry();
    } catch (const std::runtime_error& e) {
        log_error("{}", EX(e));
        return duk_error(ctx, DUK_ERR_ERROR, "Aborting playlist import");
    }
    if (!entry)
        return 0;
    PlaylistParserScript::pushPlaylistEntry(ctx, entry->get());
    return 1;
}

void PlaylistParserScript::pushPlaylistEntry(duk_context* ctx, const XmlPlaylistEntry& entry)
{
    duk_push_object(ctx);
    const std::pair<const char*, const std::string&> properties[] {
        { "title", entry.title }, { "description", entry.description },
        { "mimetype", entry.mimetype }, { "protocol", entry.protocol },
        { "size", entry.size }, { "writeThrough", entry.writeThrough }
    };
    for (const auto& [name, value] : properties) {
        duk_push_lstring(ctx, value.data(), value.size());
        duk_put_prop_string(ctx, -2, name);
    }
    duk_push_array(ctx);
    for (duk_uarridx_t i = 0; i < entry.locations.size(); ++i) {
        duk_push_lstring(ctx, entry.locations[i].data(), entry.locations[i].size());
        duk_put_prop_index(ctx, -2, i);
    }
    duk_put_prop_string(ctx, -2, "locations");
    duk_push_array(ctx);
    for (const auto& [name, value] : entry.extra) {
        duk_push_lstring(ctx, value.data(), value.size());
        duk_put_prop_string(ctx, -2, name.c_str());
    }
    duk_put_prop_string(ctx, -2, "extra");
    duk_push_array(ctx);
    for (duk_uarridx_t i = 0; i < entry.writeThroughs.size(); ++i) {
        duk_push_int(ctx, entry.writeThroughs[i]);
        duk_put_prop_index(ctx, -2, i);
    }
    duk_put_prop_string(ctx, -2, "writeThroughs");
    duk_push_object(ctx);
    for (const auto& [name, value] : entry.defaults) {
        duk_push_lstring(ctx, value.data(), value.size());
        duk_put_prop_string(ctx, -2, name.c_str());
    }
    duk_put_prop_string(ctx, -2, "defaults");
}

PlaylistParserScript::PlaylistParserScript(const std::shared_ptr<Content>& content, const std::string& parent)
    : ParserScript(content, parent, "playlist", "pls", true)
{
    playlistFunction = config->getOption(ConfigVal::IMPORT_SCRIPTING_IMPORT_FUNCTION_PLAYLIST);
    linkObjects = config->getBoolOption(ConfigVal::IMPORT_SCRIPTING_PLAYLIST_LINK_OBJECTS);
    followSymlinks = config->getBoolOption(ConfigVal::IMPORT_FOLLOW_SYMLINKS);
    hidden = config->getBoolOption(ConfigVal::IMPORT_HIDDEN_FILES);
    charsetMode = EnumOption<TextFileCharset>::getEnumOption(config, ConfigVal::IMPORT_PLAYLIST_CHARSET_MODE);
    defineFunction("readPlaylistEntry", jsReadPlaylistEntry, 0);
}

std::optional<std::reference_wrapper<const XmlPlaylistEntry>> PlaylistParserScript::readPlaylistEntry()
{
    if (currentTask && !currentTask->isValid())
        throw_std_runtime_error("Aborting playlist import after cancellation");
    auto entry = xmlPlaylist ? xmlPlaylist->readPlaylistEntry() : std::nullopt;
    // The native iterator may skip several XML nodes before returning, even at EOF.
    if (currentTask && !currentTask->isValid())
        throw_std_runtime_error("Aborting playlist import after cancellation");
    return entry;
}

std::pair<std::shared_ptr<CdsObject>, int> PlaylistParserScript::createObject2cdsObject(
    const std::shared_ptr<CdsObject>& origObject,
    const std::string& rootPath)
{
    int otype = ScriptNamedProperty(ctx, "objectType").getIntValue(-1);
    if (otype == -1) {
        log_error("missing objectType property");
        return { {}, INVALID_OBJECT_ID };
    }

    if (IS_CDS_ITEM_EXTERNAL_URL(otype)) {
        log_debug("Creating external");
        return { dukObject2cdsObject(origObject), INVALID_OBJECT_ID };
    }

    fs::path loc = fs::weakly_canonical(ScriptNamedProperty(ctx, "location").getStringValue()); // make sure relative paths can be retrieved
    std::error_code ec;
    auto dirEnt = fs::directory_entry(loc, ec);
    if (!ec && isRegularFile(dirEnt, ec)) {
        auto mainObj = database->findObjectByPath(dirEnt.path(), UNUSED_CLIENT_GROUP);

        if (!mainObj) {
            AutoScanSetting asSetting;
            asSetting.followSymlinks = followSymlinks;
            asSetting.recursive = false;
            asSetting.hidden = hidden;
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
        if (pcdId == INVALID_OBJECT_ID) {
            return false;
        }
        if (cdsObj->getID() != pcdId) {
            cdsObj->setRefID(pcdId);
            cdsObj->setFlag(ObjectFlag::UseResourceReference);
        }
    } else if (linkObjects) {
        cdsObj->setFlag(ObjectFlag::PlaylistReference);
        cdsObj->setRefID(origObject->getID());
    }
    return true;
}

void PlaylistParserScript::handleObject2cdsContainer(duk_context* ctx, const std::shared_ptr<CdsObject>& pcd, const std::shared_ptr<CdsContainer>& cont)
{
    if (linkObjects && cont->getRefID() > CDS_ID_ROOT) {
        cont->setFlag(ObjectFlag::PlaylistReference);
    }
}

void PlaylistParserScript::handleObject2cdsItem(
    duk_context* ctx,
    const std::shared_ptr<CdsObject>& pcd,
    const std::shared_ptr<CdsItem>& item)
{
    int writeThrough = ScriptNamedProperty(ctx, "writeThrough").getIntValue(-1);
    ScriptNamedProperty(ctx, "extra").getObject([&]() {
        auto keys = ScriptProperty(ctx).getPropertyNames();
        for (auto&& sym : keys) {
            auto val = ScriptNamedProperty(ctx, sym).getStringValue();
            if (!val.empty()) {
                auto [mval, err] = sc->convert(val);
                if (!err.empty()) {
                    log_warning("{}: {}", sym, err);
                }
                item->addMetaData(sym, mval);

                if (writeThrough > 0 && pcd) {
                    pcd->removeMetaData(sym);
                    setMetaData(pcd, std::static_pointer_cast<CdsItem>(pcd), sym, mval);
                }
            }
        }
    });

    if (writeThrough > 0 && pcd) {
        pcd->removeMetaData(MetadataFields::M_TITLE);
        setMetaData(pcd, std::static_pointer_cast<CdsItem>(pcd), MetaEnumMapper::getMetaFieldName(MetadataFields::M_TITLE), item->getTitle());
        content->updateObject(pcd);
    }

    item->setTrackNumber(ScriptNamedProperty(ctx, "playlistOrder").getIntValue(0));
    item->setPartNumber(0);
}

std::vector<int> PlaylistParserScript::processPlaylistObject(
    const std::shared_ptr<CdsObject>& obj,
    std::shared_ptr<GenericTask> task,
    const std::string& rootPath,
    bool& hasResult)
{
    hasResult = false;
    if (currentObjectID != INVALID_OBJECT_ID || currentFile) {
        throw_std_runtime_error("Recursion in playlists not allowed");
    }

    if (!obj->isPureItem()) {
        throw_std_runtime_error("Calling processPlaylistObject only allowed for pure items");
    }
    auto item = std::static_pointer_cast<CdsItem>(obj);

    log_debug("Checking playlist {} ...", obj->getLocation().string());
    static const std::map<std::string, XmlPlaylistType> xmlPlaylistTypes {
        { MIME_TYPE_WPL_PLAYLIST, XmlPlaylistType::Wpl },
        { MIME_TYPE_XSPF_PLAYLIST, XmlPlaylistType::Xspf },
        { MIME_TYPE_ASX_PLAYLIST, XmlPlaylistType::Asx },
    };
    auto xmlPlaylistType = xmlPlaylistTypes.find(item->getMimeType());
    currentTask = std::move(task);
    currentObjectID = item->getID();
    ScriptingRuntime::AutoLock lock(runtime->getMutex());
    xmlPlaylist.reset();
    std::vector<int> result;
    try {
        if (xmlPlaylistType != xmlPlaylistTypes.end()) {
            xmlPlaylist = XmlPlaylist::create(xmlPlaylistType->second, [this](const std::string& href, const fs::path& source) {
                if (currentTask && !currentTask->isValid())
                    throw_std_runtime_error("Aborting playlist import after cancellation");
                auto location = URLUtils::resolvePlaylistReference(href, source);
                if (!location || !fs::path(*location).is_absolute())
                    return AsxPlaylistDocument { {}, {}, "ASX unsupported remote ENTRYREF skipped (" + href + ")" };
                std::error_code ec;
                if (!isRegularFile(*location, ec))
                    return AsxPlaylistDocument { {}, {}, "ASX unavailable ENTRYREF skipped (" + href + ")" };
                auto path = fs::weakly_canonical(*location);
                auto document = std::make_unique<pugi::xml_document>();
                readXmlFile(*document, path);
                return AsxPlaylistDocument { path, std::move(document), {} };
            });
            prepareXmlFile(item->getLocation(), xmlPlaylist->getParseFlags());
            xmlPlaylist->read(xmlDoc, item->getLocation());
            useUtf8Converter();
        } else {
            auto mode = item->getMimeType() == MIME_TYPE_MPEGURL ? charsetMode : TextFileCharset::Default;
            if (mode != TextFileCharset::Default && toLower(item->getLocation().extension().string()) == ".m3u8")
                mode = TextFileCharset::Utf8;
            prepareFile(item->getLocation(), mode);
        }

        result = call(obj, nullptr, playlistFunction, rootPath, "", hasResult);
    } catch (const std::runtime_error& e) {
        log_debug("Failed to parse playlist {}: {}", obj->getLocation().string(), EX(e));
        xmlPlaylist.reset();
        cleanUp();
        throw;
    }

    xmlPlaylist.reset();
    cleanUp();
    log_debug("Done {} {} ({})...", contextName, obj->getLocation().string(), obj->getID());
    return result;
}

#endif // HAVE_JS
