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

#include "exceptions.h"
#include "util/tools.h"
#include "util/url_utils.h"

static constexpr auto xspfNamespace = "http://xspf.org/ns/0/";

static std::string namespaceUri(pugi::xml_node node)
{
    std::string name = node.name();
    auto colon = name.find(':');
    auto attribute = colon == std::string::npos ? "xmlns" : "xmlns:" + name.substr(0, colon);
    for (auto parent = node; parent; parent = parent.parent()) {
        if (auto declaration = parent.attribute(attribute.c_str()))
            return declaration.value();
    }
    return {};
}

static bool named(pugi::xml_node node, const std::string& local,
    const std::string& ns = "")
{
    std::string name = node.name();
    auto colon = name.find(':');
    return name.substr(colon == std::string::npos ? 0 : colon + 1) == local && namespaceUri(node) == ns;
}

static std::string nodeBase(pugi::xml_node node, const fs::path& path)
{
    std::vector<std::string> bases;
    for (auto parent = node; parent; parent = parent.parent()) {
        if (auto base = parent.attribute("xml:base"))
            bases.emplace_back(base.value());
    }
    auto result = URLUtils::filePathToUri(path);
    for (auto it = bases.rbegin(); it != bases.rend(); ++it)
        result = URLUtils::resolveUriReference(*it, result);
    return result;
}

std::vector<XmlPlaylistEntry> readXmlPlaylist(const pugi::xml_document& doc,
    const fs::path& path,
    XmlPlaylistType type)
{
    int roots = 0;
    for (auto node : doc.children()) {
        if (node.type() == pugi::node_doctype)
            throw_std_runtime_error("XML playlists must not contain a DOCTYPE");
        if (node.type() == pugi::node_element)
            ++roots;
    }
    if (roots != 1)
        throw_std_runtime_error("XML playlists must have exactly one root element");
    std::vector<XmlPlaylistEntry> entries;
    auto root = doc.document_element();
    if (type == XmlPlaylistType::Wpl) {
        if (!named(root, "smil"))
            throw_std_runtime_error("WPL requires a smil root element");
        auto body = root.child("body");
        auto seq = body.child("seq");
        if (!named(body, "body") || !named(seq, "seq") || body.next_sibling("body") || seq.next_sibling("seq"))
            throw_std_runtime_error("WPL requires body/seq");
        for (auto node : seq.children()) {
            if (named(node, "smartPlaylist"))
                throw_std_runtime_error(
                    "Dynamic WPL smartPlaylist queries are not supported");
            if (!named(node, "media"))
                continue;
            if (auto reference = URLUtils::resolveMediaReference(
                    node.attribute("src").value(), nodeBase(node, path), true))
                entries.push_back({ { *reference }, {} });
        }
    } else {
        if (!named(root, "playlist", xspfNamespace) || std::string(root.attribute("version").value()) != "1")
            throw_std_runtime_error(
                "XSPF requires playlist version 1 in the XSPF namespace");
        pugi::xml_node trackList;
        for (auto node : root.children()) {
            if (named(node, "trackList", xspfNamespace)) {
                if (trackList)
                    throw_std_runtime_error("XSPF requires exactly one trackList");
                trackList = node;
            }
        }
        if (!trackList)
            throw_std_runtime_error("XSPF requires exactly one trackList");
        for (auto track : trackList.children()) {
            if (!named(track, "track", xspfNamespace))
                continue;
            XmlPlaylistEntry entry;
            for (auto node : track.children()) {
                if (named(node, "title", xspfNamespace))
                    entry.title = node.text().as_string();
                if (named(node, "location", xspfNamespace)) {
                    if (auto reference = URLUtils::resolveMediaReference(
                            node.text().as_string(), nodeBase(node, path), false))
                        entry.locations.push_back(*reference);
                }
            }
            if (!entry.locations.empty())
                entries.push_back(std::move(entry));
        }
    }
    return entries;
}

#ifdef HAVE_JS

#include "cds/cds_container.h"
#include "cds/cds_item.h"
#include "config/config.h"
#include "config/config_option_enum.h"
#include "config/config_val.h"
#include "content/autoscan_setting.h"
#include "content/content.h"
#include "database/database.h"
#include "script_property.h"
#include "scripting_runtime.h"
#include "util/generic_task.h"
#include "util/string_converter.h"

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
    duk_push_object(ctx);
    duk_push_string(ctx, entry->get().title.c_str());
    duk_put_prop_string(ctx, -2, "title");
    duk_push_array(ctx);
    for (duk_uarridx_t i = 0; i < entry->get().locations.size(); ++i) {
        duk_push_string(ctx, entry->get().locations[i].c_str());
        duk_put_prop_index(ctx, -2, i);
    }
    duk_put_prop_string(ctx, -2, "locations");
    return 1;
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
    if (xmlPosition >= xmlEntries.size())
        return std::nullopt;
    return std::cref(xmlEntries[xmlPosition++]);
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
    bool isXmlPlaylist = item->getMimeType() == MIME_TYPE_WPL_PLAYLIST || item->getMimeType() == MIME_TYPE_XSPF_PLAYLIST;
    currentTask = std::move(task);
    currentObjectID = item->getID();
    ScriptingRuntime::AutoLock lock(runtime->getMutex());
    xmlEntries.clear();
    xmlPosition = 0;
    std::vector<int> result;
    try {
        if (isXmlPlaylist) {
            prepareXmlFile(item->getLocation(), pugi::parse_default | pugi::parse_doctype);
            xmlEntries = readXmlPlaylist(xmlDoc, item->getLocation(), item->getMimeType() == MIME_TYPE_WPL_PLAYLIST ? XmlPlaylistType::Wpl : XmlPlaylistType::Xspf);
            useUtf8Converter();
        } else if (item->getMimeType() == MIME_TYPE_ASX_PLAYLIST) {
            prepareXmlFile(item->getLocation());
        } else {
            auto mode = item->getMimeType() == "audio/x-mpegurl" ? charsetMode : TextFileCharset::Default;
            if (mode != TextFileCharset::Default && toLower(item->getLocation().extension().string()) == ".m3u8")
                mode = TextFileCharset::Utf8;
            prepareFile(item->getLocation(), mode);
        }

        result = call(obj, nullptr, playlistFunction, rootPath, "", hasResult);
    } catch (const std::runtime_error& e) {
        log_debug("Failed to parse playlist {}: {}", obj->getLocation().string(), EX(e));
        xmlEntries.clear();
        cleanUp();
        throw;
    }

    xmlEntries.clear();
    cleanUp();
    log_debug("Done {} {} ({})...", contextName, obj->getLocation().string(), obj->getID());
    return result;
}

#endif // HAVE_JS
