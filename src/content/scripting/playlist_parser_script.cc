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
#include "util/logger.h"
#include "util/tools.h"
#include "util/url_utils.h"

#include <utility>

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
#endif

static constexpr auto xspfNamespace = "http://xspf.org/ns/0/";

std::unique_ptr<XmlPlaylist> XmlPlaylist::create(XmlPlaylistType type)
{
    if (type == XmlPlaylistType::Asx)
        return std::make_unique<AsxPlaylist>();
    if (type == XmlPlaylistType::Wpl)
        return std::make_unique<WplPlaylist>();
    return std::make_unique<XspfPlaylist>();
}

void XmlPlaylist::read(const pugi::xml_document& doc,
    const fs::path& path)
{
    entries.clear();
    entryPosition = 0;
    readComplete = false;
    int roots = 0;
    for (auto node : doc.children()) {
        if (node.type() == pugi::node_doctype)
            throw_std_runtime_error("XML playlists must not contain a DOCTYPE");
        if (node.type() == pugi::node_element)
            ++roots;
    }
    if (roots != 1)
        throw_std_runtime_error("XML playlists must have exactly one root element");
    readEntries(doc.document_element(), path);
    readComplete = true;
}

void WplPlaylist::readEntries(pugi::xml_node root, const fs::path& path)
{
    if (!URLUtils::isNamed(root, "smil"))
        throw_std_runtime_error("WPL requires a smil root element");
    auto body = root.child("body");
    auto seq = body.child("seq");
    if (!URLUtils::isNamed(body, "body") || !URLUtils::isNamed(seq, "seq") || body.next_sibling("body") || seq.next_sibling("seq"))
        throw_std_runtime_error("WPL requires body/seq");
    for (auto node : seq.children()) {
        if (URLUtils::isNamed(node, "smartPlaylist"))
            throw_std_runtime_error(
                "Dynamic WPL smartPlaylist queries are not supported");
        if (!URLUtils::isNamed(node, "media"))
            continue;
        if (auto reference = URLUtils::resolveMediaReference(
                node.attribute("src").value(), URLUtils::nodeBase(node, path), URLUtils::MediaReferenceType::NativePath))
            entries.push_back({ { *reference }, {} });
    }
}

void XspfPlaylist::readEntries(pugi::xml_node root, const fs::path& path)
{
    if (!URLUtils::isNamed(root, "playlist", xspfNamespace) || std::string(root.attribute("version").value()) != "1")
        throw_std_runtime_error(
            "XSPF requires playlist version 1 in the XSPF namespace");
    pugi::xml_node trackList;
    for (auto node : root.children()) {
        if (URLUtils::isNamed(node, "trackList", xspfNamespace)) {
            if (trackList)
                throw_std_runtime_error("XSPF requires exactly one trackList");
            trackList = node;
        }
    }
    if (!trackList)
        throw_std_runtime_error("XSPF requires exactly one trackList");
    for (auto track : trackList.children()) {
        if (!URLUtils::isNamed(track, "track", xspfNamespace))
            continue;
        XmlPlaylistEntry entry;
        for (auto node : track.children()) {
            if (URLUtils::isNamed(node, "title", xspfNamespace))
                entry.title = node.text().as_string();
            if (URLUtils::isNamed(node, "location", xspfNamespace)) {
                if (auto reference = URLUtils::resolveMediaReference(
                        node.text().as_string(), URLUtils::nodeBase(node, path), URLUtils::MediaReferenceType::Uri))
                    entry.locations.push_back(*reference);
            }
        }
        if (!entry.locations.empty())
            entries.push_back(std::move(entry));
    }
}

std::optional<std::reference_wrapper<const XmlPlaylistEntry>> XmlPlaylist::readPlaylistEntry()
{
    if (!readComplete || entryPosition >= entries.size())
        return std::nullopt;
    return std::cref(entries[entryPosition++]);
}

static pugi::xml_attribute asxAttribute(pugi::xml_node node, const char* name)
{
    pugi::xml_attribute result;
    for (auto attribute : node.attributes()) {
        if (toLower(attribute.name()) == name)
            result = attribute;
    }
    return result;
}

void AsxPlaylist::readEntries(pugi::xml_node root, const fs::path& path)
{
    document.reset();
    node = {};
    parents.clear();
    source = path;
    base.clear();
    pendingEntry = {};
    currentEntry = {};
    if (toLower(root.name()) != "asx")
        throw_std_runtime_error("ASX requires an asx root element");
    // Keep the iterator independent of the caller's pugixml document lifetime.
    node = document.append_copy(root);
    if (!node)
        throw_std_runtime_error("Cannot copy ASX XML document");
}

std::optional<std::reference_wrapper<const XmlPlaylistEntry>> AsxPlaylist::readPlaylistEntry()
{
    currentEntry = {};
    if (!readComplete)
        return std::nullopt;
    while (node || !parents.empty()) {
        if (!node) {
            node = parents.back().next_sibling();
            parents.pop_back();
            currentEntry = std::move(pendingEntry);
            pendingEntry = {};
            if (currentEntry.locations.empty())
                continue;
            // Legacy ASX uses a root BASE prefix and the last REF in an ENTRY.
            auto reference = currentEntry.locations.back();
            if (!base.empty())
                reference = base + '/' + reference;
            std::optional<std::string> location;
            try {
                location = URLUtils::resolvePlaylistReference(reference, source);
            } catch (const std::runtime_error& e) {
                log_warning("Cannot resolve playlist reference {}: {}", reference, EX(e));
                continue;
            }
            if (!location)
                continue;
            currentEntry.locations = { *location };
            return std::cref(currentEntry);
        }
        const auto name = toLower(node.name());
        if (name == "asx" || name == "entry") {
            parents.push_back(node);
            node = node.first_child();
            continue;
        }
        const auto href = asxAttribute(node, "href").as_string();
        if (name == "ref" && *href) {
            pendingEntry.locations = { href };
            pendingEntry.writeThrough = asxAttribute(node, "writethrough").as_string("-1");
        } else if (name == "base" && *href && parents.size() < 2) {
            base = href;
        } else if (name == "title") {
            pendingEntry.title = node.text().as_string();
        } else if (name == "abstract") {
            pendingEntry.description = node.text().as_string();
        } else if (name == "param") {
            const std::string parameter = asxAttribute(node, "name").as_string();
            const std::string value = asxAttribute(node, "value").as_string();
            if (parameter == "size")
                pendingEntry.size = value;
            else if (parameter == "mimetype")
                pendingEntry.mimetype = value;
            else if (parameter == "protocol")
                pendingEntry.protocol = value;
            else
                pendingEntry.extra[parameter] = value;
        }
        node = node.next_sibling();
    }
    return std::nullopt;
}

#ifdef HAVE_JS

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
    bool isXmlPlaylist = item->getMimeType() == MIME_TYPE_WPL_PLAYLIST || item->getMimeType() == MIME_TYPE_XSPF_PLAYLIST;
    currentTask = std::move(task);
    currentObjectID = item->getID();
    ScriptingRuntime::AutoLock lock(runtime->getMutex());
    xmlPlaylist.reset();
    std::vector<int> result;
    try {
        if (isXmlPlaylist) {
            prepareXmlFile(item->getLocation(), pugi::parse_default | pugi::parse_doctype);
            xmlPlaylist = XmlPlaylist::create(item->getMimeType() == MIME_TYPE_WPL_PLAYLIST ? XmlPlaylistType::Wpl : XmlPlaylistType::Xspf);
            xmlPlaylist->read(xmlDoc, item->getLocation());
            useUtf8Converter();
        } else if (item->getMimeType() == MIME_TYPE_ASX_PLAYLIST) {
            prepareXmlFile(item->getLocation());
            xmlPlaylist = XmlPlaylist::create(XmlPlaylistType::Asx);
            xmlPlaylist->read(xmlDoc, item->getLocation());
            useUtf8Converter();
        } else {
            auto mode = item->getMimeType() == "audio/x-mpegurl" ? charsetMode : TextFileCharset::Default;
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
