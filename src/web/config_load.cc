/*GRB*

    Gerbera - https://gerbera.io/

    web/config_load.cc - this file is part of Gerbera.

    Copyright (C) 2020-2025 Gerbera Contributors

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

/// @file web/config_load.cc
#define GRB_LOG_FAC GrbLogFacility::web

#include "pages.h" // API

#include "cds/cds_container.h"
#include "config/config_definition.h"
#include "config/config_option_enum.h"
#include "config/config_setup.h"
#include "config/result/autoscan.h"
#include "config/result/box_layout.h"
#include "config/result/client_config.h"
#include "config/result/directory_tweak.h"
#include "config/result/dynamic_content.h"
#include "config/result/transcoding.h"
#include "config/setup/config_setup_array.h"
#include "config/setup/config_setup_autoscan.h"
#include "config/setup/config_setup_boxlayout.h"
#include "config/setup/config_setup_dictionary.h"
#include "config/setup/config_setup_enum.h"
#include "config/setup/config_setup_vector.h"
#include "content/content.h"
#include "context.h"
#include "database/database.h"
#include "database/db_param.h"

#include <fmt/chrono.h>
#include <numeric>

#define CONFIG_LOAD_AID "aid"
#define CONFIG_LOAD_DEFAULTVALUE "defaultValue"
#define CONFIG_LOAD_HELP "help"
#define CONFIG_LOAD_ID "id"
#define CONFIG_LOAD_ITEM "item"
#define CONFIG_LOAD_ORIGVALUE "origValue"
#define CONFIG_LOAD_SOURCE "source"
#define CONFIG_LOAD_SOURCE_CONFIG "config.xml"
#define CONFIG_LOAD_SOURCE_DATABASE "database"
#define CONFIG_LOAD_SOURCE_DEFAULT "default"
#define CONFIG_LOAD_SOURCE_UNCHANGED "unchanged"
#define CONFIG_LOAD_STATUS "status"
#define CONFIG_LOAD_TYPE "type"
#define CONFIG_LOAD_VALUE "value"

static std::vector<std::size_t> ITEM_PATH_NEW = {};

const std::string_view Web::ConfigLoad::PAGE = "config_load";

Web::ConfigLoad::ConfigLoad(const std::shared_ptr<Content>& content,
    const std::shared_ptr<Server>& server,
    const std::shared_ptr<UpnpXMLBuilder>& xmlBuilder,
    const std::shared_ptr<Quirks>& quirks)
    : PageRequest(content, server, xmlBuilder, quirks)
    , definition(content->getContext()->getDefinition())
{
    try {
        if (this->database) {
            dbEntries = this->database->getConfigValues();
        } else {
            log_error("configLoad database missing");
        }
    } catch (const std::runtime_error& e) {
        log_error("configLoad {}", e.what());
    }
}

void Web::ConfigLoad::addTypeMeta(Json::Value& meta, const std::shared_ptr<ConfigSetup>& cs)
{
    Json::Value info;
    info[CONFIG_LOAD_ITEM] = cs->getUniquePath();
    info[CONFIG_LOAD_ID] = fmt::format("{:03d}", cs->option);
    info[CONFIG_LOAD_TYPE] = cs->getTypeString();
    info[CONFIG_LOAD_VALUE] = cs->getDefaultValue();
    info[CONFIG_LOAD_HELP] = cs->getHelp();
    meta.append(info);
}

void Web::ConfigLoad::createItem(
    Json::Value& item,
    const std::string& name,
    ConfigVal id,
    ConfigVal aid,
    const std::shared_ptr<ConfigSetup>& cs)
{
    allItems[name] = allItems.size() + 1;
    item[CONFIG_LOAD_ITEM] = name;
    item[CONFIG_LOAD_ID] = fmt::format("{:03d}", id);
    item[CONFIG_LOAD_AID] = fmt::format("{:03d}", aid);
    if (std::any_of(dbEntries.begin(), dbEntries.end(), [=](auto&& s) { return s.item == name; })) {
        item[CONFIG_LOAD_STATUS] = CONFIG_LOAD_SOURCE_UNCHANGED;
        item[CONFIG_LOAD_SOURCE] = CONFIG_LOAD_SOURCE_DATABASE;
    } else {
        item[CONFIG_LOAD_STATUS] = !cs || !cs->isDefaultValueUsed() ? CONFIG_LOAD_SOURCE_UNCHANGED : CONFIG_LOAD_SOURCE_DEFAULT;
        item[CONFIG_LOAD_SOURCE] = !cs || !cs->isDefaultValueUsed() ? CONFIG_LOAD_SOURCE_CONFIG : CONFIG_LOAD_SOURCE_DEFAULT;
    }
    item[CONFIG_LOAD_ORIGVALUE] = config->getOrigValue(name);
    item[CONFIG_LOAD_DEFAULTVALUE] = cs ? cs->getDefaultValue() : "";
}

template <typename T>
void Web::ConfigLoad::addValue(
    Json::Value& parent,
    const std::string& name,
    ConfigVal id,
    ConfigVal aid,
    const T& value,
    const std::shared_ptr<ConfigSetup>& cs)
{
    Json::Value item;
    createItem(item, name, id, aid, cs);
    setValue(item, value);
    parent.append(item);
}

void Web::ConfigLoad::addNewValue(
    Json::Value& parent,
    const std::string& name,
    ConfigVal id,
    ConfigVal aid,
    const std::shared_ptr<ConfigSetup>& cs)
{
    Json::Value item;
    createItem(item, name, id, aid, cs);
    parent.append(item);
}

template <typename T>
void Web::ConfigLoad::setValue(Json::Value& item, const T& value)
{
#if FMT_VERSION >= 120000
    static_assert(fmt::is_formattable<T, fmt::format_context>::value, "T must be formattable");
#else
    static_assert(fmt::has_formatter<T, fmt::format_context>::value, "T must be formattable");
#endif
    item[CONFIG_LOAD_VALUE] = fmt::to_string(value);
}

template <>
void Web::ConfigLoad::setValue(Json::Value& item, const std::string& value)
{
    item[CONFIG_LOAD_VALUE] = value;
}

template <>
void Web::ConfigLoad::setValue(Json::Value& item, const std::string_view& value)
{
    item[CONFIG_LOAD_VALUE] = value.data();
}

template <>
void Web::ConfigLoad::setValue(Json::Value& item, const fs::path& value)
{
    item[CONFIG_LOAD_VALUE] = value.string();
}

/// @brief process config_load request
bool Web::ConfigLoad::processPageAction(Json::Value& element, const std::string& action)
{
    Json::Value values;
    Json::Value valueArray(Json::arrayValue);

    log_debug("Sending Config to web!");

    writeDatabaseStatus(valueArray);

    if (action == "status") {
        values[CONFIG_LOAD_ITEM] = valueArray;
        element["values"] = values;
        return true;
    }

    // generate meta info for ui
    Json::Value meta;
    Json::Value metaArray(Json::arrayValue);
    for (auto&& cs : definition->getOptionList()) {
        addTypeMeta(metaArray, cs);
    }

    writeShortcuts(valueArray);
    writeSimpleProperties(valueArray);
    writeClientConfig(valueArray);
    writeImportTweaks(valueArray);
    writeDynamicContent(valueArray);
    writeBoxLayout(valueArray);
    writeTranscoding(valueArray);
    writeAutoscan(valueArray);
    writeDictionaries(valueArray);
    writeVectors(valueArray);
    writeArrays(valueArray);
    updateEntriesFromDatabase(element, valueArray);

    meta[CONFIG_LOAD_ITEM] = metaArray;
    values[CONFIG_LOAD_ITEM] = valueArray;
    element["types"] = meta;
    element["values"] = values;
    return true;
}

/// @brief write database status
void Web::ConfigLoad::writeDatabaseStatus(Json::Value& values)
{
    {
        StatsParam stats(StatsParam::StatsMode::Count, "", "", false);
        addValue(values, "/status/attribute::totalCount", ConfigVal::MAX, ConfigVal::MAX, database->getFileStats(stats));
    }
    {
        StatsParam stats(StatsParam::StatsMode::Size, "", "", false);
        addValue(values, "/status/attribute::totalSize", ConfigVal::MAX, ConfigVal::MAX, CdsResource::formatSizeValue(database->getFileStats(stats)));
    }
    {
        StatsParam stats(StatsParam::StatsMode::Count, "", "", true);
        addValue(values, "/status/attribute::virtual", ConfigVal::MAX, ConfigVal::MAX, database->getFileStats(stats));
    }
    {
        StatsParam stats(StatsParam::StatsMode::Count, "audio", "", true);
        addValue(values, "/status/attribute::audioVirtual", ConfigVal::MAX, ConfigVal::MAX, database->getFileStats(stats));
    }
    {
        StatsParam stats(StatsParam::StatsMode::Count, "video", "", true);
        addValue(values, "/status/attribute::videoVirtual", ConfigVal::MAX, ConfigVal::MAX, database->getFileStats(stats));
    }
    {
        StatsParam stats(StatsParam::StatsMode::Count, "image", "", true);
        addValue(values, "/status/attribute::imageVirtual", ConfigVal::MAX, ConfigVal::MAX, database->getFileStats(stats));
    }

    StatsParam statc(StatsParam::StatsMode::Count, "", "", false);
    auto cnt = database->getGroupStats(statc);
    StatsParam stats(StatsParam::StatsMode::Size, "", "", false);
    auto siz = database->getGroupStats(stats);

    static std::map<std::string, std::string> statMapBase {
        { UPNP_CLASS_ITEM, CONFIG_LOAD_ITEM },
    };
    for (auto&& [cls, attr] : statMapBase) {
        if (cnt[cls] > 0) {
            addValue(values, fmt::format("/status/attribute::{}Count", attr), ConfigVal::MAX, ConfigVal::MAX, cnt[cls]);
            addValue(values, fmt::format("/status/attribute::{}Size", attr), ConfigVal::MAX, ConfigVal::MAX, CdsResource::formatSizeValue(siz[cls]));
            addValue(values, fmt::format("/status/attribute::{}Bytes", attr), ConfigVal::MAX, ConfigVal::MAX, siz[cls]);
        }
    }
    static std::map<std::string, std::string> statMap {
        { UPNP_CLASS_AUDIO_ITEM, "audio" },
        { UPNP_CLASS_MUSIC_TRACK, "audioMusic" },
        { UPNP_CLASS_AUDIO_BOOK, "audioBook" },
        { UPNP_CLASS_AUDIO_BROADCAST, "audioBroadcast" },
        { UPNP_CLASS_VIDEO_ITEM, "video" },
        { UPNP_CLASS_VIDEO_MOVIE, "videoMovie" },
        { UPNP_CLASS_VIDEO_BROADCAST, "videoBroadcast" },
        { UPNP_CLASS_VIDEO_MUSICVIDEOCLIP, "videoMusicVideoClip" },
        { UPNP_CLASS_IMAGE_ITEM, "image" },
        { UPNP_CLASS_IMAGE_PHOTO, "imagePhoto" },
        { UPNP_CLASS_TEXT_ITEM, "text" },
    };
    for (auto&& [cls, attr] : statMap) {
        long long totalCnt = 0;
        for (auto&& [cls2, valu] : cnt) {
            if (startswith(cls2, cls))
                totalCnt += valu;
        }
        if (totalCnt > 0) {
            long long totalSize = 0;
            for (auto&& [cls2, valu] : siz) {
                if (startswith(cls2, cls))
                    totalSize += valu;
            }
            addValue(values, fmt::format("/status/attribute::{}Count", attr), ConfigVal::MAX, ConfigVal::MAX, totalCnt);
            addValue(values, fmt::format("/status/attribute::{}Size", attr), ConfigVal::MAX, ConfigVal::MAX, CdsResource::formatSizeValue(totalSize));
            addValue(values, fmt::format("/status/attribute::{}Bytes", attr), ConfigVal::MAX, ConfigVal::MAX, totalSize);
        }
    }
}

/// @brief write upnp shortcuts
void Web::ConfigLoad::writeShortcuts(Json::Value& values)
{
    auto shortcuts = database->getShortcuts();
    std::size_t idx = 0;
    for (auto&& [shortcut, cont] : shortcuts) {
        if (!shortcut.empty()) {
            log_debug("shortcut {}={}", shortcut, cont->getID());
            addValue(values, fmt::format("/status/shortcut[{}]/attribute::name", idx), ConfigVal::MAX, ConfigVal::MAX, shortcut);
            addValue(values, fmt::format("/status/shortcut[{}]/attribute::id", idx), ConfigVal::MAX, ConfigVal::MAX, cont->getID());
            addValue(values, fmt::format("/status/shortcut[{}]/attribute::location", idx), ConfigVal::MAX, ConfigVal::MAX, cont->getLocation());
            idx++;
        }
    }
}

/// @brief write all values with simple type (string, int, bool)
void Web::ConfigLoad::writeSimpleProperties(Json::Value& values)
{
    for (auto&& option : ConfigOptionIterator()) {
        try {
            auto scs = definition->findConfigSetup(option);
            addValue(values, scs->getItemPathRoot(), option, option, scs->getCurrentValue(), scs);
            log_debug("    Option {:03d} {} = {}", option, scs->getItemPath({ 0 }, {}), scs->getCurrentValue());
        } catch (const std::runtime_error& e) {
            log_warning("Option {:03d} {}", option, e.what());
        }
    }
}

/// @brief write client configuration
void Web::ConfigLoad::writeClientConfig(Json::Value& values)
{
    // Output Clients
    auto cs = definition->findConfigSetup(ConfigVal::CLIENTS_LIST);
    auto clientConfig = cs->getValue()->getClientConfigListOption();
    for (std::size_t i = 0; i < EDIT_CAST(EditHelperClientConfig, clientConfig)->size(); i++) {
        auto client = EDIT_CAST(EditHelperClientConfig, clientConfig)->get(i);
        std::vector<std::size_t> indexList = { i };

        addValue(values,
            cs->getItemPath(indexList, { ConfigVal::A_CLIENTS_CLIENT, ConfigVal::A_CLIENTS_CLIENT_FLAGS }),
            cs->option, ConfigVal::A_CLIENTS_CLIENT_FLAGS, ClientConfig::mapFlags(client->getFlags()), cs);
        addValue(values,
            cs->getItemPath(indexList, { ConfigVal::A_CLIENTS_CLIENT, ConfigVal::A_CLIENTS_CLIENT_IP }),
            cs->option, ConfigVal::A_CLIENTS_CLIENT_IP, client->getIp(), cs);
        addValue(values,
            cs->getItemPath(indexList, { ConfigVal::A_CLIENTS_CLIENT, ConfigVal::A_CLIENTS_CLIENT_USERAGENT }),
            cs->option, ConfigVal::A_CLIENTS_CLIENT_USERAGENT, client->getUserAgent(), cs);
        addValue(values,
            cs->getItemPath(indexList, { ConfigVal::A_CLIENTS_CLIENT, ConfigVal::A_CLIENTS_CLIENT_GROUP }),
            cs->option, ConfigVal::A_CLIENTS_CLIENT_GROUP, client->getGroup(), cs);
        addValue(values,
            cs->getItemPath(indexList, { ConfigVal::A_CLIENTS_CLIENT, ConfigVal::A_CLIENTS_CLIENT_ALLOWED }),
            cs->option, ConfigVal::A_CLIENTS_CLIENT_ALLOWED, client->getAllowed(), cs);
        addValue(values,
            cs->getItemPath(indexList, { ConfigVal::A_CLIENTS_CLIENT, ConfigVal::A_CLIENTS_UPNP_CAPTION_COUNT }),
            cs->option, ConfigVal::A_CLIENTS_UPNP_CAPTION_COUNT, client->getCaptionInfoCount(), cs);
        addValue(values,
            cs->getItemPath(indexList, { ConfigVal::A_CLIENTS_CLIENT, ConfigVal::A_CLIENTS_UPNP_STRING_LIMIT }),
            cs->option, ConfigVal::A_CLIENTS_UPNP_STRING_LIMIT, client->getStringLimit(), cs);
        addValue(values,
            cs->getItemPath(indexList, { ConfigVal::A_CLIENTS_CLIENT, ConfigVal::A_CLIENTS_UPNP_MULTI_VALUE }),
            cs->option, ConfigVal::A_CLIENTS_UPNP_MULTI_VALUE, client->getMultiValue(), cs);

        {
            // Sub Dictionary ConfigVal::A_CLIENTS_UPNP_MAP_MIMETYPE
            std::size_t j = 0;
            for (auto&& [from, to] : client->getMimeMappings(true)) {
                addValue(values,
                    cs->getItemPath({ i, j }, { ConfigVal::A_CLIENTS_CLIENT, ConfigVal::A_IMPORT_MAPPINGS_MIMETYPE_MAP, ConfigVal::A_IMPORT_MAPPINGS_MIMETYPE_FROM }),
                    cs->option, ConfigVal::A_IMPORT_MAPPINGS_MIMETYPE_FROM, from, cs);
                addValue(values,
                    cs->getItemPath({ i, j }, { ConfigVal::A_CLIENTS_CLIENT, ConfigVal::A_IMPORT_MAPPINGS_MIMETYPE_MAP, ConfigVal::A_IMPORT_MAPPINGS_MIMETYPE_TO }),
                    cs->option, ConfigVal::A_IMPORT_MAPPINGS_MIMETYPE_TO, to, cs);
            }
        }
        {
            // Sub Dictionary ConfigVal::A_CLIENTS_UPNP_HEADERS
            std::size_t j = 0;
            for (auto&& [key, value] : client->getHeaders(true)) {
                addValue(values,
                    cs->getItemPath({ i, j }, { ConfigVal::A_CLIENTS_CLIENT, ConfigVal::A_CLIENTS_UPNP_HEADERS_HEADER, ConfigVal::A_CLIENTS_UPNP_HEADERS_KEY }),
                    cs->option, ConfigVal::A_CLIENTS_UPNP_HEADERS_KEY, key, cs);
                addValue(values,
                    cs->getItemPath({ i, j }, { ConfigVal::A_CLIENTS_CLIENT, ConfigVal::A_CLIENTS_UPNP_HEADERS_HEADER, ConfigVal::A_CLIENTS_UPNP_HEADERS_VALUE }),
                    cs->option, ConfigVal::A_CLIENTS_UPNP_HEADERS_VALUE, value, cs);
            }
        }
        {
            // Sub Dictionary ConfigVal::A_CLIENTS_UPNP_MAP_DLNAPROFILE
            std::size_t j = 0;
            auto vcs = definition->findConfigSetup<ConfigVectorSetup>(ConfigVal::A_CLIENTS_UPNP_MAP_DLNAPROFILE);
            for (auto&& map : client->getDlnaMappings(true)) {
                std::vector<std::string> attrList;
                attrList.reserve(vcs->optionList.size());
                for (const auto& opt : vcs->optionList) {
                    attrList.push_back(definition->removeAttribute(opt));
                }
                for (auto&& [key, val] : map) {
                    std::size_t k = 0;
                    auto pos = std::find(attrList.begin(), attrList.end(), key);
                    if (pos != attrList.end())
                        k = std::distance(attrList.begin(), pos);
                    addValue(values,
                        cs->getItemPath({ i, j }, { ConfigVal::A_CLIENTS_CLIENT, ConfigVal::A_CLIENTS_UPNP_MAP_DLNAPROFILE_PROFILE }, key),
                        cs->option, vcs->optionList[k], val, cs);
                }
                j++;
            }
        }
    }
    // Allow creation of clients in blank config
    {
        addNewValue(values,
            cs->getItemPath(ITEM_PATH_NEW, { ConfigVal::A_CLIENTS_CLIENT, ConfigVal::A_CLIENTS_CLIENT_FLAGS }),
            cs->option, ConfigVal::A_CLIENTS_CLIENT_FLAGS, definition->findConfigSetup(ConfigVal::A_CLIENTS_CLIENT_FLAGS));
        addNewValue(values,
            cs->getItemPath(ITEM_PATH_NEW, { ConfigVal::A_CLIENTS_CLIENT, ConfigVal::A_CLIENTS_CLIENT_IP }),
            cs->option, ConfigVal::A_CLIENTS_CLIENT_IP, definition->findConfigSetup(ConfigVal::A_CLIENTS_CLIENT_IP));
        addNewValue(values,
            cs->getItemPath(ITEM_PATH_NEW, { ConfigVal::A_CLIENTS_CLIENT, ConfigVal::A_CLIENTS_CLIENT_USERAGENT }),
            cs->option, ConfigVal::A_CLIENTS_CLIENT_USERAGENT, definition->findConfigSetup(ConfigVal::A_CLIENTS_CLIENT_USERAGENT));
        addNewValue(values,
            cs->getItemPath(ITEM_PATH_NEW, { ConfigVal::A_CLIENTS_CLIENT, ConfigVal::A_CLIENTS_CLIENT_GROUP }),
            cs->option, ConfigVal::A_CLIENTS_CLIENT_GROUP, definition->findConfigSetup(ConfigVal::A_CLIENTS_CLIENT_GROUP));
        addNewValue(values,
            cs->getItemPath(ITEM_PATH_NEW, { ConfigVal::A_CLIENTS_CLIENT, ConfigVal::A_CLIENTS_UPNP_CAPTION_COUNT }),
            cs->option, ConfigVal::A_CLIENTS_UPNP_CAPTION_COUNT, definition->findConfigSetup(ConfigVal::A_CLIENTS_UPNP_CAPTION_COUNT));
        addNewValue(values,
            cs->getItemPath(ITEM_PATH_NEW, { ConfigVal::A_CLIENTS_CLIENT, ConfigVal::A_CLIENTS_UPNP_STRING_LIMIT }),
            cs->option, ConfigVal::A_CLIENTS_UPNP_STRING_LIMIT, definition->findConfigSetup(ConfigVal::A_CLIENTS_UPNP_STRING_LIMIT));
        addNewValue(values,
            cs->getItemPath(ITEM_PATH_NEW, { ConfigVal::A_CLIENTS_CLIENT, ConfigVal::A_CLIENTS_UPNP_MULTI_VALUE }),
            cs->option, ConfigVal::A_CLIENTS_UPNP_MULTI_VALUE, definition->findConfigSetup(ConfigVal::A_CLIENTS_UPNP_MULTI_VALUE));
        addNewValue(values,
            cs->getItemPath(ITEM_PATH_NEW, { ConfigVal::A_CLIENTS_CLIENT, ConfigVal::A_IMPORT_MAPPINGS_MIMETYPE_MAP, ConfigVal::A_IMPORT_MAPPINGS_MIMETYPE_FROM }),
            cs->option, ConfigVal::A_IMPORT_MAPPINGS_MIMETYPE_FROM, cs);
        addNewValue(values,
            cs->getItemPath(ITEM_PATH_NEW, { ConfigVal::A_CLIENTS_CLIENT, ConfigVal::A_IMPORT_MAPPINGS_MIMETYPE_MAP, ConfigVal::A_IMPORT_MAPPINGS_MIMETYPE_TO }),
            cs->option, ConfigVal::A_IMPORT_MAPPINGS_MIMETYPE_TO, cs);
        addNewValue(values,
            cs->getItemPath(ITEM_PATH_NEW, { ConfigVal::A_CLIENTS_CLIENT, ConfigVal::A_CLIENTS_UPNP_HEADERS_HEADER, ConfigVal::A_CLIENTS_UPNP_HEADERS_KEY }),
            cs->option, ConfigVal::A_CLIENTS_UPNP_HEADERS_KEY, cs);
        addNewValue(values,
            cs->getItemPath(ITEM_PATH_NEW, { ConfigVal::A_CLIENTS_CLIENT, ConfigVal::A_CLIENTS_UPNP_HEADERS_HEADER, ConfigVal::A_CLIENTS_UPNP_HEADERS_VALUE }),
            cs->option, ConfigVal::A_CLIENTS_UPNP_HEADERS_VALUE, cs);
        addNewValue(values,
            cs->getItemPath(ITEM_PATH_NEW, { ConfigVal::A_CLIENTS_CLIENT, ConfigVal::A_CLIENTS_UPNP_MAP_DLNAPROFILE_PROFILE, ConfigVal::A_IMPORT_MAPPINGS_MIMETYPE_FROM }),
            cs->option, ConfigVal::A_IMPORT_MAPPINGS_MIMETYPE_FROM, cs);
        addNewValue(values,
            cs->getItemPath(ITEM_PATH_NEW, { ConfigVal::A_CLIENTS_CLIENT, ConfigVal::A_CLIENTS_UPNP_MAP_DLNAPROFILE_PROFILE, ConfigVal::A_IMPORT_MAPPINGS_MIMETYPE_TO }),
            cs->option, ConfigVal::A_IMPORT_MAPPINGS_MIMETYPE_TO, cs);

        // special treatment for vector
        auto vcs = definition->findConfigSetup<ConfigVectorSetup>(ConfigVal::A_CLIENTS_UPNP_MAP_DLNAPROFILE);
        std::vector<std::string> attrList;
        attrList.reserve(vcs->optionList.size());
        for (const auto& opt : vcs->optionList) {
            attrList.push_back(definition->removeAttribute(opt));
        }
        for (auto&& key : { "audioCodec", "videoCodec" }) {
            std::size_t k = 0;
            auto pos = std::find(attrList.begin(), attrList.end(), key);
            if (pos != attrList.end())
                k = std::distance(attrList.begin(), pos);
            addNewValue(values,
                cs->getItemPath(ITEM_PATH_NEW, { ConfigVal::A_CLIENTS_CLIENT, ConfigVal::A_CLIENTS_UPNP_MAP_DLNAPROFILE_PROFILE }, key),
                cs->option, vcs->optionList[k], cs);
        }
    }

    // Output Groups
    for (std::size_t i = 0; i < EDIT_CAST(EditHelperClientGroupConfig, clientConfig)->size(); i++) {
        auto group = EDIT_CAST(EditHelperClientGroupConfig, clientConfig)->get(i);
        std::vector<std::size_t> indexList = { i };
        {
            addValue(values,
                cs->getItemPath(indexList, { ConfigVal::A_CLIENTS_GROUP, ConfigVal::A_CLIENTS_GROUP_NAME }),
                cs->option, ConfigVal::A_CLIENTS_GROUP_NAME, group->getGroupName(), cs);
        }
        auto forbiddenDirs = group->getForbiddenDirectories();
        for (std::size_t j = 0; j < forbiddenDirs.size(); j++) {
            std::vector<std::size_t> subIndexList = { i, j };
            addValue(values,
                cs->getItemPath(subIndexList, { ConfigVal::A_CLIENTS_GROUP, ConfigVal::A_CLIENTS_GROUP_HIDE, ConfigVal::A_CLIENTS_GROUP_LOCATION }),
                cs->option, ConfigVal::A_CLIENTS_GROUP_LOCATION, forbiddenDirs.at(j), cs);
        }
    }
    // Allow creation of groups in blank config
    addNewValue(values,
        cs->getItemPath(ITEM_PATH_NEW, { ConfigVal::A_CLIENTS_GROUP, ConfigVal::A_CLIENTS_GROUP_NAME }),
        cs->option, ConfigVal::A_CLIENTS_GROUP_NAME, definition->findConfigSetup(ConfigVal::A_CLIENTS_GROUP_NAME));
    addNewValue(values,
        cs->getItemPath(ITEM_PATH_NEW, { ConfigVal::A_CLIENTS_GROUP, ConfigVal::A_CLIENTS_GROUP_LOCATION }),
        cs->option, ConfigVal::A_CLIENTS_GROUP_LOCATION, definition->findConfigSetup(ConfigVal::A_CLIENTS_GROUP_LOCATION));
}

/// @brief write import tweaks
void Web::ConfigLoad::writeImportTweaks(Json::Value& values)
{
    auto cs = definition->findConfigSetup(ConfigVal::IMPORT_DIRECTORIES_LIST);
    auto directoryConfig = cs->getValue()->getDirectoryTweakOption();
    for (std::size_t i = 0; i < directoryConfig->size(); i++) {
        auto dir = directoryConfig->get(i);
        std::vector<std::size_t> indexList = { i };

        addValue(values,
            cs->getItemPath(indexList, { ConfigVal::A_DIRECTORIES_TWEAK_LOCATION }),
            cs->option, ConfigVal::A_DIRECTORIES_TWEAK_LOCATION, dir->getLocation());
        addValue(values,
            cs->getItemPath(indexList, { ConfigVal::A_DIRECTORIES_TWEAK_INHERIT }),
            cs->option, ConfigVal::A_DIRECTORIES_TWEAK_INHERIT, dir->getInherit());
        addValue(values,
            cs->getItemPath(indexList, { ConfigVal::A_DIRECTORIES_TWEAK_RECURSIVE }),
            cs->option, ConfigVal::A_DIRECTORIES_TWEAK_RECURSIVE, dir->getRecursive());
        addValue(values,
            cs->getItemPath(indexList, { ConfigVal::A_DIRECTORIES_TWEAK_HIDDEN }),
            cs->option, ConfigVal::A_DIRECTORIES_TWEAK_HIDDEN, dir->getHidden());
        addValue(values,
            cs->getItemPath(indexList, { ConfigVal::A_DIRECTORIES_TWEAK_CASE_SENSITIVE }),
            cs->option, ConfigVal::A_DIRECTORIES_TWEAK_CASE_SENSITIVE, dir->getCaseSensitive());
        addValue(values,
            cs->getItemPath(indexList, { ConfigVal::A_DIRECTORIES_TWEAK_FOLLOW_SYMLINKS }),
            cs->option, ConfigVal::A_DIRECTORIES_TWEAK_FOLLOW_SYMLINKS, dir->getFollowSymlinks());
        addValue(values,
            cs->getItemPath(indexList, { ConfigVal::A_DIRECTORIES_TWEAK_META_CHARSET }),
            cs->option, ConfigVal::A_DIRECTORIES_TWEAK_META_CHARSET, dir->hasMetaCharset() ? dir->getMetaCharset() : "");
        addValue(values,
            cs->getItemPath(indexList, { ConfigVal::A_DIRECTORIES_TWEAK_FANART_FILE }),
            cs->option, ConfigVal::A_DIRECTORIES_TWEAK_FANART_FILE, dir->hasFanArtFile() ? dir->getFanArtFile() : "");
        addValue(values,
            cs->getItemPath(indexList, { ConfigVal::A_DIRECTORIES_TWEAK_RESOURCE_FILE }),
            cs->option, ConfigVal::A_DIRECTORIES_TWEAK_RESOURCE_FILE, dir->hasResourceFile() ? dir->getResourceFile() : "");
        addValue(values,
            cs->getItemPath(indexList, { ConfigVal::A_DIRECTORIES_TWEAK_SUBTITLE_FILE }),
            cs->option, ConfigVal::A_DIRECTORIES_TWEAK_SUBTITLE_FILE, dir->hasSubTitleFile() ? dir->getSubTitleFile() : "");
        addValue(values,
            cs->getItemPath(indexList, { ConfigVal::A_DIRECTORIES_TWEAK_METAFILE_FILE }),
            cs->option, ConfigVal::A_DIRECTORIES_TWEAK_METAFILE_FILE, dir->hasMetafile() ? dir->getMetafile() : "");
    }
    // Allow creation of entry in blank config
    addNewValue(values,
        cs->getItemPath(ITEM_PATH_NEW, { ConfigVal::A_DIRECTORIES_TWEAK_LOCATION }),
        cs->option, ConfigVal::A_DIRECTORIES_TWEAK_LOCATION);
    addNewValue(values,
        cs->getItemPath(ITEM_PATH_NEW, { ConfigVal::A_DIRECTORIES_TWEAK_INHERIT }),
        cs->option, ConfigVal::A_DIRECTORIES_TWEAK_INHERIT);
    addNewValue(values,
        cs->getItemPath(ITEM_PATH_NEW, { ConfigVal::A_DIRECTORIES_TWEAK_RECURSIVE }),
        cs->option, ConfigVal::A_DIRECTORIES_TWEAK_RECURSIVE);
    addNewValue(values,
        cs->getItemPath(ITEM_PATH_NEW, { ConfigVal::A_DIRECTORIES_TWEAK_HIDDEN }),
        cs->option, ConfigVal::A_DIRECTORIES_TWEAK_HIDDEN);
    addNewValue(values,
        cs->getItemPath(ITEM_PATH_NEW, { ConfigVal::A_DIRECTORIES_TWEAK_CASE_SENSITIVE }),
        cs->option, ConfigVal::A_DIRECTORIES_TWEAK_CASE_SENSITIVE);
    addNewValue(values,
        cs->getItemPath(ITEM_PATH_NEW, { ConfigVal::A_DIRECTORIES_TWEAK_FOLLOW_SYMLINKS }),
        cs->option, ConfigVal::A_DIRECTORIES_TWEAK_FOLLOW_SYMLINKS);
    addNewValue(values,
        cs->getItemPath(ITEM_PATH_NEW, { ConfigVal::A_DIRECTORIES_TWEAK_META_CHARSET }),
        cs->option, ConfigVal::A_DIRECTORIES_TWEAK_META_CHARSET);
    addNewValue(values,
        cs->getItemPath(ITEM_PATH_NEW, { ConfigVal::A_DIRECTORIES_TWEAK_FANART_FILE }),
        cs->option, ConfigVal::A_DIRECTORIES_TWEAK_FANART_FILE);
    addNewValue(values,
        cs->getItemPath(ITEM_PATH_NEW, { ConfigVal::A_DIRECTORIES_TWEAK_RESOURCE_FILE }),
        cs->option, ConfigVal::A_DIRECTORIES_TWEAK_RESOURCE_FILE);
    addNewValue(values,
        cs->getItemPath(ITEM_PATH_NEW, { ConfigVal::A_DIRECTORIES_TWEAK_SUBTITLE_FILE }),
        cs->option, ConfigVal::A_DIRECTORIES_TWEAK_SUBTITLE_FILE);
    addNewValue(values,
        cs->getItemPath(ITEM_PATH_NEW, { ConfigVal::A_DIRECTORIES_TWEAK_METAFILE_FILE }),
        cs->option, ConfigVal::A_DIRECTORIES_TWEAK_METAFILE_FILE);
}

/// @brief write dynamic content
void Web::ConfigLoad::writeDynamicContent(Json::Value& values)
{
    auto cs = definition->findConfigSetup(ConfigVal::SERVER_DYNAMIC_CONTENT_LIST);
    auto dynContent = cs->getValue()->getDynamicContentListOption();
    for (std::size_t i = 0; i < dynContent->size(); i++) {
        auto cont = dynContent->get(i);
        std::vector<std::size_t> indexList = { i };

        addValue(values,
            cs->getItemPath(indexList, { ConfigVal::A_DYNAMIC_CONTAINER_LOCATION }),
            cs->option, ConfigVal::A_DYNAMIC_CONTAINER_LOCATION, cont->getLocation());
        addValue(values,
            cs->getItemPath(indexList, { ConfigVal::A_DYNAMIC_CONTAINER_IMAGE }),
            cs->option, ConfigVal::A_DYNAMIC_CONTAINER_IMAGE, cont->getImage());
        addValue(values,
            cs->getItemPath(indexList, { ConfigVal::A_DYNAMIC_CONTAINER_TITLE }),
            cs->option, ConfigVal::A_DYNAMIC_CONTAINER_TITLE, cont->getTitle());
        addValue(values,
            cs->getItemPath(indexList, { ConfigVal::A_DYNAMIC_CONTAINER_FILTER }),
            cs->option, ConfigVal::A_DYNAMIC_CONTAINER_FILTER, cont->getFilter());
        addValue(values,
            cs->getItemPath(indexList, { ConfigVal::A_DYNAMIC_CONTAINER_SORT }),
            cs->option, ConfigVal::A_DYNAMIC_CONTAINER_SORT, cont->getSort());
        addValue(values,
            cs->getItemPath(indexList, { ConfigVal::A_DYNAMIC_CONTAINER_MAXCOUNT }),
            cs->option, ConfigVal::A_DYNAMIC_CONTAINER_MAXCOUNT, cont->getMaxCount());
        addValue(values,
            cs->getItemPath(indexList, { ConfigVal::A_DYNAMIC_CONTAINER_UPNP_SHORTCUT }),
            cs->option, ConfigVal::A_DYNAMIC_CONTAINER_UPNP_SHORTCUT, cont->getUpnpShortcut());
    }
    // Allow creation of entry in blank config
    addNewValue(values,
        cs->getItemPath(ITEM_PATH_NEW, { ConfigVal::A_DYNAMIC_CONTAINER_LOCATION }),
        cs->option, ConfigVal::A_DYNAMIC_CONTAINER_LOCATION, definition->findConfigSetup(ConfigVal::A_DYNAMIC_CONTAINER_LOCATION));
    addNewValue(values,
        cs->getItemPath(ITEM_PATH_NEW, { ConfigVal::A_DYNAMIC_CONTAINER_IMAGE }),
        cs->option, ConfigVal::A_DYNAMIC_CONTAINER_IMAGE, definition->findConfigSetup(ConfigVal::A_DYNAMIC_CONTAINER_IMAGE));
    addNewValue(values,
        cs->getItemPath(ITEM_PATH_NEW, { ConfigVal::A_DYNAMIC_CONTAINER_TITLE }),
        cs->option, ConfigVal::A_DYNAMIC_CONTAINER_TITLE, definition->findConfigSetup(ConfigVal::A_DYNAMIC_CONTAINER_TITLE));
    addNewValue(values,
        cs->getItemPath(ITEM_PATH_NEW, { ConfigVal::A_DYNAMIC_CONTAINER_FILTER }),
        cs->option, ConfigVal::A_DYNAMIC_CONTAINER_FILTER, definition->findConfigSetup(ConfigVal::A_DYNAMIC_CONTAINER_FILTER));
    addNewValue(values,
        cs->getItemPath(ITEM_PATH_NEW, { ConfigVal::A_DYNAMIC_CONTAINER_SORT }),
        cs->option, ConfigVal::A_DYNAMIC_CONTAINER_SORT, definition->findConfigSetup(ConfigVal::A_DYNAMIC_CONTAINER_SORT));
    addNewValue(values,
        cs->getItemPath(ITEM_PATH_NEW, { ConfigVal::A_DYNAMIC_CONTAINER_MAXCOUNT }),
        cs->option, ConfigVal::A_DYNAMIC_CONTAINER_MAXCOUNT, definition->findConfigSetup(ConfigVal::A_DYNAMIC_CONTAINER_MAXCOUNT));
    addNewValue(values,
        cs->getItemPath(ITEM_PATH_NEW, { ConfigVal::A_DYNAMIC_CONTAINER_UPNP_SHORTCUT }),
        cs->option, ConfigVal::A_DYNAMIC_CONTAINER_UPNP_SHORTCUT, definition->findConfigSetup(ConfigVal::A_DYNAMIC_CONTAINER_UPNP_SHORTCUT));
}

/// @brief write box layout
void Web::ConfigLoad::writeBoxLayout(Json::Value& values)
{
    auto cs = definition->findConfigSetup(ConfigVal::BOXLAYOUT_LIST);
    auto ctEnumSetup = definition->findConfigSetup<ConfigEnumSetup<AutoscanMediaMode>>(ConfigVal::A_BOXLAYOUT_CHAIN_TYPE);
    auto boxlayoutContent = EDIT_CAST(EditHelperBoxLayout, cs->getValue()->getBoxLayoutListOption());
    for (std::size_t i = 0; i < boxlayoutContent->size(); i++) {
        auto cont = boxlayoutContent->get(i);
        std::vector<std::size_t> indexList = { i };

        addValue(values,
            cs->getItemPath(indexList, { ConfigVal::A_BOXLAYOUT_BOX, ConfigVal::A_BOXLAYOUT_BOX_KEY }),
            cs->option, ConfigVal::A_BOXLAYOUT_BOX_KEY, cont->getKey());
        addValue(values,
            cs->getItemPath(indexList, { ConfigVal::A_BOXLAYOUT_BOX, ConfigVal::A_BOXLAYOUT_BOX_TITLE }),
            cs->option, ConfigVal::A_BOXLAYOUT_BOX_TITLE, cont->getTitle());
        addValue(values,
            cs->getItemPath(indexList, { ConfigVal::A_BOXLAYOUT_BOX, ConfigVal::A_BOXLAYOUT_BOX_CLASS }),
            cs->option, ConfigVal::A_BOXLAYOUT_BOX_CLASS, cont->getClass());
        addValue(values,
            cs->getItemPath(indexList, { ConfigVal::A_BOXLAYOUT_BOX, ConfigVal::A_BOXLAYOUT_BOX_SIZE }),
            cs->option, ConfigVal::A_BOXLAYOUT_BOX_SIZE, cont->getSize());
        addValue(values,
            cs->getItemPath(indexList, { ConfigVal::A_BOXLAYOUT_BOX, ConfigVal::A_BOXLAYOUT_BOX_ENABLED }),
            cs->option, ConfigVal::A_BOXLAYOUT_BOX_ENABLED, cont->getEnabled());
        addValue(values,
            cs->getItemPath(indexList, { ConfigVal::A_BOXLAYOUT_BOX, ConfigVal::A_BOXLAYOUT_BOX_UPNP_SHORTCUT }),
            cs->option, ConfigVal::A_BOXLAYOUT_BOX_UPNP_SHORTCUT, cont->getUpnpShortcut());
        addValue(values,
            cs->getItemPath(indexList, { ConfigVal::A_BOXLAYOUT_BOX, ConfigVal::A_BOXLAYOUT_BOX_SORT_KEY }),
            cs->option, ConfigVal::A_BOXLAYOUT_BOX_SORT_KEY, cont->getSortKey());
    }
    auto chainlayoutContent = EDIT_CAST(EditHelperBoxChain, cs->getValue()->getBoxLayoutListOption());
    for (std::size_t i = 0; i < chainlayoutContent->size(); i++) {
        auto cont = chainlayoutContent->get(i);
        std::vector<std::size_t> indexList = { i };
        addValue(values,
            cs->getItemPath(indexList, { ConfigVal::A_BOXLAYOUT_CHAIN, ConfigVal::A_BOXLAYOUT_CHAIN_TYPE }),
            cs->option, ConfigVal::A_BOXLAYOUT_CHAIN_TYPE, ctEnumSetup->mapEnumValue(cont->getType()));
        std::size_t j = 0;
        for (auto&& link : cont->getLinks(true)) {
            std::size_t k = 0;
            for (auto&& [key, value] : link) {
                indexList = { i, j, k };
                addValue(values,
                    cs->getItemPath(indexList, { ConfigVal::A_BOXLAYOUT_CHAIN, ConfigVal::A_BOXLAYOUT_CHAIN_LINKS, ConfigVal::A_BOXLAYOUT_CHAIN_LINK }, ConfigBoxLayoutSetup::linkKey.data()),
                    cs->option, ConfigVal::A_BOXLAYOUT_CHAIN_LINK, key);
                addValue(values,
                    cs->getItemPath(indexList, { ConfigVal::A_BOXLAYOUT_CHAIN, ConfigVal::A_BOXLAYOUT_CHAIN_LINKS, ConfigVal::A_BOXLAYOUT_CHAIN_LINK }, ConfigBoxLayoutSetup::linkValue.data()),
                    cs->option, ConfigVal::A_BOXLAYOUT_CHAIN_LINK, value);
                k++;
            }
            j++;
        }
    }
    // Allow creation of entry in blank config
    addNewValue(values,
        cs->getItemPath(ITEM_PATH_NEW, { ConfigVal::A_BOXLAYOUT_BOX, ConfigVal::A_BOXLAYOUT_BOX_KEY }),
        cs->option, ConfigVal::A_BOXLAYOUT_BOX_KEY, definition->findConfigSetup(ConfigVal::A_BOXLAYOUT_BOX_KEY));
    addNewValue(values,
        cs->getItemPath(ITEM_PATH_NEW, { ConfigVal::A_BOXLAYOUT_BOX, ConfigVal::A_BOXLAYOUT_BOX_TITLE }),
        cs->option, ConfigVal::A_BOXLAYOUT_BOX_TITLE, definition->findConfigSetup(ConfigVal::A_BOXLAYOUT_BOX_TITLE));
    addNewValue(values,
        cs->getItemPath(ITEM_PATH_NEW, { ConfigVal::A_BOXLAYOUT_BOX, ConfigVal::A_BOXLAYOUT_BOX_CLASS }),
        cs->option, ConfigVal::A_BOXLAYOUT_BOX_CLASS, definition->findConfigSetup(ConfigVal::A_BOXLAYOUT_BOX_CLASS));
    addNewValue(values,
        cs->getItemPath(ITEM_PATH_NEW, { ConfigVal::A_BOXLAYOUT_BOX, ConfigVal::A_BOXLAYOUT_BOX_SIZE }),
        cs->option, ConfigVal::A_BOXLAYOUT_BOX_SIZE, definition->findConfigSetup(ConfigVal::A_BOXLAYOUT_BOX_SIZE));
    addNewValue(values,
        cs->getItemPath(ITEM_PATH_NEW, { ConfigVal::A_BOXLAYOUT_BOX, ConfigVal::A_BOXLAYOUT_BOX_ENABLED }),
        cs->option, ConfigVal::A_BOXLAYOUT_BOX_ENABLED, definition->findConfigSetup(ConfigVal::A_BOXLAYOUT_BOX_ENABLED));
    addNewValue(values,
        cs->getItemPath(ITEM_PATH_NEW, { ConfigVal::A_BOXLAYOUT_BOX, ConfigVal::A_BOXLAYOUT_BOX_UPNP_SHORTCUT }),
        cs->option, ConfigVal::A_BOXLAYOUT_BOX_UPNP_SHORTCUT, definition->findConfigSetup(ConfigVal::A_BOXLAYOUT_BOX_UPNP_SHORTCUT));

    addNewValue(values,
        cs->getItemPath(ITEM_PATH_NEW, { ConfigVal::A_BOXLAYOUT_CHAIN, ConfigVal::A_BOXLAYOUT_CHAIN_TYPE }),
        cs->option, ConfigVal::A_BOXLAYOUT_CHAIN_TYPE, definition->findConfigSetup(ConfigVal::A_BOXLAYOUT_CHAIN_TYPE));

    addNewValue(values,
        cs->getItemPath(ITEM_PATH_NEW, { ConfigVal::A_BOXLAYOUT_CHAIN, ConfigVal::A_BOXLAYOUT_CHAIN_LINKS, ConfigVal::A_BOXLAYOUT_CHAIN_LINK }, ConfigBoxLayoutSetup::linkKey.data()),
        cs->option, ConfigVal::A_BOXLAYOUT_CHAIN_LINK, definition->findConfigSetup(ConfigVal::A_BOXLAYOUT_CHAIN_LINK));

    addNewValue(values,
        cs->getItemPath(ITEM_PATH_NEW, { ConfigVal::A_BOXLAYOUT_CHAIN, ConfigVal::A_BOXLAYOUT_CHAIN_LINKS, ConfigVal::A_BOXLAYOUT_CHAIN_LINK }, ConfigBoxLayoutSetup::linkValue.data()),
        cs->option, ConfigVal::A_BOXLAYOUT_CHAIN_LINK, definition->findConfigSetup(ConfigVal::A_BOXLAYOUT_CHAIN_LINK));
}

/// @brief write transconding configuration
void Web::ConfigLoad::writeTranscoding(Json::Value& values)
{
    auto cs = definition->findConfigSetup(ConfigVal::TRANSCODING_PROFILE_LIST);
    auto transcoding = cs->getValue()->getTranscodingProfileListOption();
    std::size_t pr = 0;
    std::map<std::string, std::shared_ptr<TranscodingProfile>> profiles;
    // write filter list and collect required profiles
    for (auto&& filter : transcoding->getFilterList()) {
        std::vector<std::size_t> indexList = { pr };

        addValue(values,
            cs->getItemPath(indexList, { ConfigVal::A_TRANSCODING_MIMETYPE_FILTER, ConfigVal::A_TRANSCODING_MIMETYPE_PROF_MAP_MIMETYPE }),
            cs->option, ConfigVal::A_TRANSCODING_MIMETYPE_PROF_MAP_MIMETYPE, filter->getMimeType(), cs);
        addValue(values,
            cs->getItemPath(indexList, { ConfigVal::A_TRANSCODING_MIMETYPE_FILTER, ConfigVal::A_TRANSCODING_MIMETYPE_PROF_MAP_USING }),
            cs->option, ConfigVal::A_TRANSCODING_MIMETYPE_PROF_MAP_USING, filter->getTranscoderName(), cs);
        addValue(values,
            cs->getItemPath(indexList, { ConfigVal::A_TRANSCODING_MIMETYPE_FILTER, ConfigVal::A_TRANSCODING_PROFILES_PROFLE_SRCDLNA }),
            cs->option, ConfigVal::A_TRANSCODING_PROFILES_PROFLE_SRCDLNA, filter->getSourceProfile(), cs);
        addValue(values,
            cs->getItemPath(indexList, { ConfigVal::A_TRANSCODING_MIMETYPE_FILTER, ConfigVal::A_TRANSCODING_PROFILES_PROFLE_NOTRANSCODING }),
            cs->option, ConfigVal::A_TRANSCODING_PROFILES_PROFLE_NOTRANSCODING, fmt::to_string(fmt::join(filter->getNoTranscodingMimeTypes(), ",")), cs);
        addValue(values,
            cs->getItemPath(indexList, { ConfigVal::A_TRANSCODING_MIMETYPE_FILTER, ConfigVal::A_TRANSCODING_PROFILES_PROFLE_CLIENTFLAGS }),
            cs->option, ConfigVal::A_TRANSCODING_PROFILES_PROFLE_CLIENTFLAGS, ClientConfig::mapFlags(filter->getClientFlags()), cs);

        if (filter->getTranscodingProfile())
            profiles[filter->getTranscodingProfile()->getName()] = filter->getTranscodingProfile();

        pr++;
    }

    pr = 0;
    // write profile list
    // profiles can only be exported when linked to at least one filter
    for (auto&& [name, entry] : profiles) {
        std::vector<std::size_t> indexList = { pr };

        addValue(values,
            cs->getItemPath(indexList, { ConfigVal::A_TRANSCODING_PROFILES_PROFLE, ConfigVal::A_TRANSCODING_PROFILES_PROFLE_NAME }),
            cs->option, ConfigVal::A_TRANSCODING_PROFILES_PROFLE_NAME, name);
        addValue(values,
            cs->getItemPath(indexList, { ConfigVal::A_TRANSCODING_PROFILES_PROFLE, ConfigVal::A_TRANSCODING_PROFILES_PROFLE_CLIENTFLAGS }),
            cs->option, ConfigVal::A_TRANSCODING_PROFILES_PROFLE_CLIENTFLAGS, ClientConfig::mapFlags(entry->getClientFlags()));
        addValue(values,
            cs->getItemPath(indexList, { ConfigVal::A_TRANSCODING_PROFILES_PROFLE, ConfigVal::A_TRANSCODING_PROFILES_PROFLE_ENABLED }),
            cs->option, ConfigVal::A_TRANSCODING_PROFILES_PROFLE_ENABLED, entry->isEnabled());
        auto ttEnumSetup = definition->findConfigSetup<ConfigEnumSetup<TranscodingType>>(ConfigVal::A_TRANSCODING_PROFILES_PROFLE_TYPE);
        addValue(values,
            cs->getItemPath(indexList, { ConfigVal::A_TRANSCODING_PROFILES_PROFLE, ConfigVal::A_TRANSCODING_PROFILES_PROFLE_TYPE }),
            cs->option, ConfigVal::A_TRANSCODING_PROFILES_PROFLE_TYPE, ttEnumSetup->mapEnumValue(entry->getType()));
        addValue(values,
            cs->getItemPath(indexList, { ConfigVal::A_TRANSCODING_PROFILES_PROFLE, ConfigVal::A_TRANSCODING_PROFILES_PROFLE_DLNAPROF }),
            cs->option, ConfigVal::A_TRANSCODING_PROFILES_PROFLE_DLNAPROF, entry->getDlnaProfile());
        addValue(values,
            cs->getItemPath(indexList, { ConfigVal::A_TRANSCODING_PROFILES_PROFLE, ConfigVal::A_TRANSCODING_PROFILES_PROFLE_MIMETYPE }),
            cs->option, ConfigVal::A_TRANSCODING_PROFILES_PROFLE_MIMETYPE, entry->getTargetMimeType());
        addValue(values,
            cs->getItemPath(indexList, { ConfigVal::A_TRANSCODING_PROFILES_PROFLE, ConfigVal::A_TRANSCODING_PROFILES_PROFLE_RESOLUTION }),
            cs->option, ConfigVal::A_TRANSCODING_PROFILES_PROFLE_RESOLUTION, entry->getAttributeOverride(ResourceAttribute::RESOLUTION));
        addValue(values,
            cs->getItemPath(indexList, { ConfigVal::A_TRANSCODING_PROFILES_PROFLE, ConfigVal::A_TRANSCODING_PROFILES_PROFLE_ACCURL }),
            cs->option, ConfigVal::A_TRANSCODING_PROFILES_PROFLE_ACCURL, entry->getAcceptURL());
        addValue(values,
            cs->getItemPath(indexList, { ConfigVal::A_TRANSCODING_PROFILES_PROFLE, ConfigVal::A_TRANSCODING_PROFILES_PROFLE_SAMPFREQ }),
            cs->option, ConfigVal::A_TRANSCODING_PROFILES_PROFLE_SAMPFREQ, entry->getSampleFreq());
        addValue(values,
            cs->getItemPath(indexList, { ConfigVal::A_TRANSCODING_PROFILES_PROFLE, ConfigVal::A_TRANSCODING_PROFILES_PROFLE_NRCHAN }),
            cs->option, ConfigVal::A_TRANSCODING_PROFILES_PROFLE_NRCHAN, entry->getNumChannels());
        addValue(values,
            cs->getItemPath(indexList, { ConfigVal::A_TRANSCODING_PROFILES_PROFLE, ConfigVal::A_TRANSCODING_PROFILES_PROFLE_HIDEORIG }),
            cs->option, ConfigVal::A_TRANSCODING_PROFILES_PROFLE_HIDEORIG, entry->hideOriginalResource());
        addValue(values,
            cs->getItemPath({ pr }, { ConfigVal::A_TRANSCODING_PROFILES_PROFLE, ConfigVal::A_TRANSCODING_PROFILES_PROFLE_THUMB }),
            cs->option, ConfigVal::A_TRANSCODING_PROFILES_PROFLE_THUMB, entry->isThumbnail());
        addValue(values,
            cs->getItemPath(indexList, { ConfigVal::A_TRANSCODING_PROFILES_PROFLE, ConfigVal::A_TRANSCODING_PROFILES_PROFLE_FIRST }),
            cs->option, ConfigVal::A_TRANSCODING_PROFILES_PROFLE_FIRST, entry->getFirstResource());
        addValue(values,
            cs->getItemPath(indexList, { ConfigVal::A_TRANSCODING_PROFILES_PROFLE, ConfigVal::A_TRANSCODING_PROFILES_PROFLE_ACCOGG }),
            cs->option, ConfigVal::A_TRANSCODING_PROFILES_PROFLE_ACCOGG, entry->isTheora());
        addValue(values,
            cs->getItemPath(indexList, { ConfigVal::A_TRANSCODING_PROFILES_PROFLE, ConfigVal::A_TRANSCODING_PROFILES_PROFLE_AGENT, ConfigVal::A_TRANSCODING_PROFILES_PROFLE_AGENT_COMMAND }),
            cs->option, ConfigVal::A_TRANSCODING_PROFILES_PROFLE_AGENT_COMMAND, entry->getCommand());
        addValue(values,
            cs->getItemPath(indexList, { ConfigVal::A_TRANSCODING_PROFILES_PROFLE, ConfigVal::A_TRANSCODING_PROFILES_PROFLE_AGENT, ConfigVal::A_TRANSCODING_PROFILES_PROFLE_AGENT_ARGS }),
            cs->option, ConfigVal::A_TRANSCODING_PROFILES_PROFLE_AGENT_ARGS, entry->getArguments());
        addValue(values,
            cs->getItemPath(indexList, { ConfigVal::A_TRANSCODING_PROFILES_PROFLE, ConfigVal::A_TRANSCODING_PROFILES_PROFLE_BUFFER, ConfigVal::A_TRANSCODING_PROFILES_PROFLE_BUFFER_SIZE }),
            cs->option, ConfigVal::A_TRANSCODING_PROFILES_PROFLE_BUFFER_SIZE, entry->getBufferSize());
        addValue(values,
            cs->getItemPath(indexList, { ConfigVal::A_TRANSCODING_PROFILES_PROFLE, ConfigVal::A_TRANSCODING_PROFILES_PROFLE_BUFFER, ConfigVal::A_TRANSCODING_PROFILES_PROFLE_BUFFER_CHUNK }),
            cs->option, ConfigVal::A_TRANSCODING_PROFILES_PROFLE_BUFFER_CHUNK, entry->getBufferChunkSize());
        addValue(values,
            cs->getItemPath(indexList, { ConfigVal::A_TRANSCODING_PROFILES_PROFLE, ConfigVal::A_TRANSCODING_PROFILES_PROFLE_BUFFER, ConfigVal::A_TRANSCODING_PROFILES_PROFLE_BUFFER_FILL }),
            cs->option, ConfigVal::A_TRANSCODING_PROFILES_PROFLE_BUFFER_FILL, entry->getBufferInitialFillSize());

        auto fourCCMode = entry->getAVIFourCCListMode();
        if (fourCCMode != AviFourccListmode::None) {
            auto fccEnumSetup = definition->findConfigSetup<ConfigEnumSetup<AviFourccListmode>>(ConfigVal::A_TRANSCODING_PROFILES_PROFLE_AVI4CC_MODE);
            addValue(values,
                cs->getItemPath(indexList, { ConfigVal::A_TRANSCODING_PROFILES_PROFLE, ConfigVal::A_TRANSCODING_PROFILES_PROFLE_AVI4CC, ConfigVal::A_TRANSCODING_PROFILES_PROFLE_AVI4CC_MODE }),
                cs->option, ConfigVal::A_TRANSCODING_PROFILES_PROFLE_AVI4CC_MODE,
                fccEnumSetup->mapEnumValue(fourCCMode));

            const auto& fourCCList = entry->getAVIFourCCList();
            if (!fourCCList.empty()) {
                addValue(values,
                    cs->getItemPath(indexList, { ConfigVal::A_TRANSCODING_PROFILES_PROFLE, ConfigVal::A_TRANSCODING_PROFILES_PROFLE_AVI4CC, ConfigVal::A_TRANSCODING_PROFILES_PROFLE_AVI4CC_4CC }),
                    cs->option, ConfigVal::A_TRANSCODING_PROFILES_PROFLE_AVI4CC_4CC,
                    std::accumulate(next(fourCCList.begin()), fourCCList.end(), fourCCList[0], [](auto&& a, auto&& b) { return fmt::format("{}, {}", a, b); }));
            }
        }
        pr++;
    }
}

/// @brief write autoscan configuration
void Web::ConfigLoad::writeAutoscan(Json::Value& values)
{
    for (auto&& ascs : definition->getConfigSetupList<ConfigAutoscanSetup>()) {
        auto autoscan = ascs->getValue()->getAutoscanListOption();
        // create separate entries for timed and inotify
        for (std::size_t i = 0; i < autoscan.size(); i++) {
            auto&& entry = autoscan.at(i);
            std::vector<std::size_t> indexList = { i };
            auto&& adir = content->getAutoscanDirectory(entry->getLocation());

            // path
            addValue(values,
                ascs->getItemPath(indexList, { ConfigVal::A_AUTOSCAN_DIRECTORY_LOCATION }),
                ascs->option, ConfigVal::A_AUTOSCAN_DIRECTORY_LOCATION, adir->getLocation());
            // scan mode (timed|inotify)
            addValue(values,
                ascs->getItemPath(indexList, { ConfigVal::A_AUTOSCAN_DIRECTORY_MODE }),
                ascs->option, ConfigVal::A_AUTOSCAN_DIRECTORY_MODE, fmt::to_string(AutoscanDirectory::mapScanmode(adir->getScanMode())));
            // interval for timed
            addValue(values,
                ascs->getItemPath(indexList, { ConfigVal::A_AUTOSCAN_DIRECTORY_INTERVAL }),
                ascs->option, ConfigVal::A_AUTOSCAN_DIRECTORY_INTERVAL, adir->getInterval().count());
            // counter for retries
            addValue(values,
                ascs->getItemPath(indexList, { ConfigVal::A_AUTOSCAN_DIRECTORY_RETRYCOUNT }),
                ascs->option, ConfigVal::A_AUTOSCAN_DIRECTORY_RETRYCOUNT, adir->getInterval().count());
            // recursively import files
            addValue(values,
                ascs->getItemPath(indexList, { ConfigVal::A_AUTOSCAN_DIRECTORY_RECURSIVE }),
                ascs->option, ConfigVal::A_AUTOSCAN_DIRECTORY_RECURSIVE, adir->getRecursive());
            // directory types
            addValue(values,
                ascs->getItemPath(indexList, { ConfigVal::A_AUTOSCAN_DIRECTORY_DIRTYPES }),
                ascs->option, ConfigVal::A_AUTOSCAN_DIRECTORY_DIRTYPES, adir->hasDirTypes());
            // media types imported from directory
            addValue(values,
                ascs->getItemPath(indexList, { ConfigVal::A_AUTOSCAN_DIRECTORY_MEDIATYPE }),
                ascs->option, ConfigVal::A_AUTOSCAN_DIRECTORY_MEDIATYPE, AutoscanDirectory::mapMediaType(adir->getMediaType()));
            // import hidden files
            addValue(values,
                ascs->getItemPath(indexList, { ConfigVal::A_AUTOSCAN_DIRECTORY_HIDDENFILES }),
                ascs->option, ConfigVal::A_AUTOSCAN_DIRECTORY_HIDDENFILES, adir->getHidden());
            // follow symbolic links
            addValue(values,
                ascs->getItemPath(indexList, { ConfigVal::A_AUTOSCAN_DIRECTORY_FOLLOWSYMLINKS }),
                ascs->option, ConfigVal::A_AUTOSCAN_DIRECTORY_FOLLOWSYMLINKS, adir->getFollowSymlinks());
            // container type for audio
            addValue(values,
                ascs->getItemPath(indexList, { ConfigVal::A_AUTOSCAN_CONTAINER_TYPE_AUDIO }),
                ascs->option, ConfigVal::A_AUTOSCAN_CONTAINER_TYPE_AUDIO, adir->getContainerTypes().at(AutoscanMediaMode::Audio));
            // container type for images
            addValue(values,
                ascs->getItemPath(indexList, { ConfigVal::A_AUTOSCAN_CONTAINER_TYPE_IMAGE }),
                ascs->option, ConfigVal::A_AUTOSCAN_CONTAINER_TYPE_IMAGE, adir->getContainerTypes().at(AutoscanMediaMode::Image));
            // container type for videos
            addValue(values,
                ascs->getItemPath(indexList, { ConfigVal::A_AUTOSCAN_CONTAINER_TYPE_VIDEO }),
                ascs->option, ConfigVal::A_AUTOSCAN_CONTAINER_TYPE_VIDEO, adir->getContainerTypes().at(AutoscanMediaMode::Video));
            // active scan count
            addValue(values,
                ascs->getItemPath(indexList, { ConfigVal::A_AUTOSCAN_DIRECTORY_SCANCOUNT }),
                ascs->option, ConfigVal::A_AUTOSCAN_DIRECTORY_SCANCOUNT, adir->getActiveScanCount());
            // active task count
            addValue(values,
                ascs->getItemPath(indexList, { ConfigVal::A_AUTOSCAN_DIRECTORY_TASKCOUNT }),
                ascs->option, ConfigVal::A_AUTOSCAN_DIRECTORY_TASKCOUNT, adir->getTaskCount());
            // Last modified
            addValue(values,
                ascs->getItemPath(indexList, { ConfigVal::A_AUTOSCAN_DIRECTORY_LMT }),
                ascs->option, ConfigVal::A_AUTOSCAN_DIRECTORY_LMT, grbLocaltime("{:%Y-%m-%d %H:%M:%S}", adir->getPreviousLMT()));
        }
        // Allow creation of entry in blank config
        // path
        addNewValue(values,
            ascs->getItemPath(ITEM_PATH_NEW, { ConfigVal::A_AUTOSCAN_DIRECTORY_LOCATION }),
            ascs->option, ConfigVal::A_AUTOSCAN_DIRECTORY_LOCATION, definition->findConfigSetup(ConfigVal::A_AUTOSCAN_DIRECTORY_LOCATION));
        // scan mode (timed|inotify)
        addNewValue(values,
            ascs->getItemPath(ITEM_PATH_NEW, { ConfigVal::A_AUTOSCAN_DIRECTORY_MODE }),
            ascs->option, ConfigVal::A_AUTOSCAN_DIRECTORY_MODE, definition->findConfigSetup(ConfigVal::A_AUTOSCAN_DIRECTORY_MODE));
        // interval for timed
        addNewValue(values,
            ascs->getItemPath(ITEM_PATH_NEW, { ConfigVal::A_AUTOSCAN_DIRECTORY_INTERVAL }),
            ascs->option, ConfigVal::A_AUTOSCAN_DIRECTORY_INTERVAL, definition->findConfigSetup(ConfigVal::A_AUTOSCAN_DIRECTORY_INTERVAL));
        // counter for retries
        addNewValue(values,
            ascs->getItemPath(ITEM_PATH_NEW, { ConfigVal::A_AUTOSCAN_DIRECTORY_RETRYCOUNT }),
            ascs->option, ConfigVal::A_AUTOSCAN_DIRECTORY_RETRYCOUNT);
        // recursively import files
        addNewValue(values,
            ascs->getItemPath(ITEM_PATH_NEW, { ConfigVal::A_AUTOSCAN_DIRECTORY_RECURSIVE }),
            ascs->option, ConfigVal::A_AUTOSCAN_DIRECTORY_RECURSIVE, definition->findConfigSetup(ConfigVal::A_AUTOSCAN_DIRECTORY_RECURSIVE));
        // directory types
        addNewValue(values,
            ascs->getItemPath(ITEM_PATH_NEW, { ConfigVal::A_AUTOSCAN_DIRECTORY_DIRTYPES }),
            ascs->option, ConfigVal::A_AUTOSCAN_DIRECTORY_DIRTYPES);
        // media types imported from directory
        addNewValue(values,
            ascs->getItemPath(ITEM_PATH_NEW, { ConfigVal::A_AUTOSCAN_DIRECTORY_MEDIATYPE }),
            ascs->option, ConfigVal::A_AUTOSCAN_DIRECTORY_MEDIATYPE, definition->findConfigSetup(ConfigVal::A_AUTOSCAN_DIRECTORY_MEDIATYPE));
        // import hidden files
        addNewValue(values,
            ascs->getItemPath(ITEM_PATH_NEW, { ConfigVal::A_AUTOSCAN_DIRECTORY_HIDDENFILES }),
            ascs->option, ConfigVal::A_AUTOSCAN_DIRECTORY_HIDDENFILES, definition->findConfigSetup(ConfigVal::A_AUTOSCAN_DIRECTORY_HIDDENFILES));
        // follow symbolic links
        addNewValue(values,
            ascs->getItemPath(ITEM_PATH_NEW, { ConfigVal::A_AUTOSCAN_DIRECTORY_FOLLOWSYMLINKS }),
            ascs->option, ConfigVal::A_AUTOSCAN_DIRECTORY_FOLLOWSYMLINKS);
        // container type for audio
        addNewValue(values,
            ascs->getItemPath(ITEM_PATH_NEW, { ConfigVal::A_AUTOSCAN_CONTAINER_TYPE_AUDIO }),
            ascs->option, ConfigVal::A_AUTOSCAN_CONTAINER_TYPE_AUDIO, definition->findConfigSetup(ConfigVal::A_AUTOSCAN_CONTAINER_TYPE_AUDIO));
        // container type for images
        addNewValue(values,
            ascs->getItemPath(ITEM_PATH_NEW, { ConfigVal::A_AUTOSCAN_CONTAINER_TYPE_IMAGE }),
            ascs->option, ConfigVal::A_AUTOSCAN_CONTAINER_TYPE_IMAGE, definition->findConfigSetup(ConfigVal::A_AUTOSCAN_CONTAINER_TYPE_IMAGE));
        // container type for videos
        addNewValue(values,
            ascs->getItemPath(ITEM_PATH_NEW, { ConfigVal::A_AUTOSCAN_CONTAINER_TYPE_VIDEO }),
            ascs->option, ConfigVal::A_AUTOSCAN_CONTAINER_TYPE_VIDEO, definition->findConfigSetup(ConfigVal::A_AUTOSCAN_CONTAINER_TYPE_VIDEO));
        // active scan count
        addNewValue(values,
            ascs->getItemPath(ITEM_PATH_NEW, { ConfigVal::A_AUTOSCAN_DIRECTORY_SCANCOUNT }),
            ascs->option, ConfigVal::A_AUTOSCAN_DIRECTORY_SCANCOUNT, definition->findConfigSetup(ConfigVal::A_AUTOSCAN_DIRECTORY_SCANCOUNT));
        // active task count
        addNewValue(values,
            ascs->getItemPath(ITEM_PATH_NEW, { ConfigVal::A_AUTOSCAN_DIRECTORY_TASKCOUNT }),
            ascs->option, ConfigVal::A_AUTOSCAN_DIRECTORY_TASKCOUNT, definition->findConfigSetup(ConfigVal::A_AUTOSCAN_DIRECTORY_TASKCOUNT));
        // Last modified
        addNewValue(values,
            ascs->getItemPath(ITEM_PATH_NEW, { ConfigVal::A_AUTOSCAN_DIRECTORY_LMT }),
            ascs->option, ConfigVal::A_AUTOSCAN_DIRECTORY_LMT, definition->findConfigSetup(ConfigVal::A_AUTOSCAN_DIRECTORY_LMT));
    }
}

/// @brief write content of all dictionaries
void Web::ConfigLoad::writeDictionaries(Json::Value& values)
{
    for (auto&& dcs : definition->getConfigSetupList<ConfigDictionarySetup>()) {
        std::size_t i = 0;
        auto dictionary = dcs->getValue()->getDictionaryOption(true);
        for (auto&& [key, val] : dictionary) {
            std::vector<std::size_t> indexList = { i };

            addValue(values, dcs->getItemPath(indexList, { dcs->keyOption }), dcs->option, dcs->keyOption, key.substr(5), dcs);
            addValue(values, dcs->getItemPath(indexList, { dcs->valOption }), dcs->option, dcs->valOption, val, dcs);
            i++;
        }
        addNewValue(values, dcs->getItemPath(ITEM_PATH_NEW, { dcs->keyOption }), dcs->option, dcs->keyOption, dcs);
        addNewValue(values, dcs->getItemPath(ITEM_PATH_NEW, { dcs->valOption }), dcs->option, dcs->valOption, dcs);
    }
}

/// @brief write content of all vectors
void Web::ConfigLoad::writeVectors(Json::Value& values)
{
    for (auto&& vcs : definition->getConfigSetupList<ConfigVectorSetup>()) {
        std::size_t i = 0;
        auto vectorOption = vcs->getValue()->getVectorOption(true);
        std::vector<std::string> attrList;
        attrList.reserve(vcs->optionList.size());
        for (const auto& opt : vcs->optionList) {
            attrList.push_back(definition->removeAttribute(opt));
        }
        for (auto&& vector : vectorOption) {
            for (auto&& [key, val] : vector) {
                std::size_t j = 0;
                auto pos = std::find(attrList.begin(), attrList.end(), key);
                if (pos != attrList.end())
                    j = std::distance(attrList.begin(), pos);
                addValue(values, vcs->getItemPath({ i }, {}, { key }), vcs->option, vcs->optionList[j], val, vcs);
            }
            i++;
        }
    }
}

/// @brief write content of all arrays
void Web::ConfigLoad::writeArrays(Json::Value& values)
{
    for (auto&& acs : definition->getConfigSetupList<ConfigArraySetup>()) {
        auto array = acs->getValue()->getArrayOption(true);
        for (std::size_t i = 0; i < array.size(); i++) {
            auto&& entry = array[i];
            addValue(values,
                acs->getItemPath({ i }, {}),
                acs->option, acs->attrOption != ConfigVal::MAX ? acs->attrOption : acs->nodeOption, entry, acs);
        }
        addNewValue(values,
            acs->getItemPath(ITEM_PATH_NEW, {}),
            acs->option, acs->attrOption != ConfigVal::MAX ? acs->attrOption : acs->nodeOption, acs);
    }
}

/// @brief update entries with datebase values
void Web::ConfigLoad::updateEntriesFromDatabase(Json::Value& element, Json::Value& values)
{
    for (auto&& entry : dbEntries) {
        auto exItem = allItems.find(entry.item);
        if (exItem != allItems.end()) {
            values[exItem->second][CONFIG_LOAD_SOURCE] = CONFIG_LOAD_SOURCE_DATABASE;
            values[exItem->second][CONFIG_LOAD_STATUS] = entry.status;
        } else {
            auto cs = definition->findConfigSetupByPath(entry.item, true);
            auto acs = definition->findConfigSetupByPath(entry.item, true, cs);
            if (cs) {
                Json::Value item;
                createItem(item, entry.item, cs->option, acs ? acs->option : ConfigVal::MAX);
                setValue(item, entry.value);
                item[CONFIG_LOAD_STATUS] = entry.status;
                item[CONFIG_LOAD_ORIGVALUE] = config->getOrigValue(entry.item);
                item[CONFIG_LOAD_SOURCE] = CONFIG_LOAD_SOURCE_DATABASE;
                values.append(item);
            }
        }
    }
}
