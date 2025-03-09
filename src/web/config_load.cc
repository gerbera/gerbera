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

/// \file web/config_load.cc
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

void Web::ConfigLoad::createItem(Json::Value& item, const std::string& name, ConfigVal id, ConfigVal aid, const std::shared_ptr<ConfigSetup>& cs)
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
void Web::ConfigLoad::setValue(Json::Value& item, const T& value)
{
    static_assert(fmt::has_formatter<T, fmt::format_context>::value, "T must be formattable");
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

/// \brief: process config_load request
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

/// \brief: write database status
void Web::ConfigLoad::writeDatabaseStatus(Json::Value& values)
{
    {
        StatsParam stats(StatsParam::StatsMode::Count, "", "", false);
        Json::Value item;
        createItem(item, "/status/attribute::totalCount", ConfigVal::MAX, ConfigVal::MAX);
        setValue(item, database->getFileStats(stats));
        values.append(item);
    }
    {
        StatsParam stats(StatsParam::StatsMode::Size, "", "", false);
        Json::Value item;
        createItem(item, "/status/attribute::totalSize", ConfigVal::MAX, ConfigVal::MAX);
        setValue(item, CdsResource::formatSizeValue(database->getFileStats(stats)));
        values.append(item);
    }
    {
        StatsParam stats(StatsParam::StatsMode::Count, "", "", true);
        Json::Value item;
        createItem(item, "/status/attribute::virtual", ConfigVal::MAX, ConfigVal::MAX);
        setValue(item, database->getFileStats(stats));
        values.append(item);
    }

    {
        StatsParam stats(StatsParam::StatsMode::Count, "audio", "", true);
        Json::Value item;
        createItem(item, "/status/attribute::audioVirtual", ConfigVal::MAX, ConfigVal::MAX);
        setValue(item, database->getFileStats(stats));
        values.append(item);
    }
    {
        StatsParam stats(StatsParam::StatsMode::Count, "video", "", true);
        Json::Value item;
        createItem(item, "/status/attribute::videoVirtual", ConfigVal::MAX, ConfigVal::MAX);
        setValue(item, database->getFileStats(stats));
        values.append(item);
    }
    {
        StatsParam stats(StatsParam::StatsMode::Count, "image", "", true);
        Json::Value item;
        createItem(item, "/status/attribute::imageVirtual", ConfigVal::MAX, ConfigVal::MAX);
        setValue(item, database->getFileStats(stats));
        values.append(item);
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
            {
                Json::Value item;
                createItem(item, fmt::format("/status/attribute::{}Count", attr), ConfigVal::MAX, ConfigVal::MAX);
                setValue(item, cnt[cls]);
                values.append(item);
            }
            {
                Json::Value item;
                createItem(item, fmt::format("/status/attribute::{}Size", attr), ConfigVal::MAX, ConfigVal::MAX);
                setValue(item, CdsResource::formatSizeValue(siz[cls]));
                values.append(item);
            }
            {
                Json::Value item;
                createItem(item, fmt::format("/status/attribute::{}Bytes", attr), ConfigVal::MAX, ConfigVal::MAX);
                setValue(item, siz[cls]);
                values.append(item);
            }
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
            {
                Json::Value item;
                createItem(item, fmt::format("/status/attribute::{}Count", attr), ConfigVal::MAX, ConfigVal::MAX);
                setValue(item, totalCnt);
                values.append(item);
            }
            {
                Json::Value item;
                createItem(item, fmt::format("/status/attribute::{}Size", attr), ConfigVal::MAX, ConfigVal::MAX);
                setValue(item, CdsResource::formatSizeValue(totalSize));
                values.append(item);
            }
            {
                Json::Value item;
                createItem(item, fmt::format("/status/attribute::{}Bytes", attr), ConfigVal::MAX, ConfigVal::MAX);
                setValue(item, totalSize);
                values.append(item);
            }
        }
    }
}

/// \brief: write upnp shortcuts
void Web::ConfigLoad::writeShortcuts(Json::Value& values)
{
    auto shortcuts = database->getShortcuts();
    std::size_t idx = 0;
    for (auto&& [shortcut, cont] : shortcuts) {
        if (!shortcut.empty()) {
            log_debug("shortcut {}={}", shortcut, cont->getID());
            {
                Json::Value item;
                createItem(item, fmt::format("/status/shortcut[{}]/attribute::name", idx), ConfigVal::MAX, ConfigVal::MAX);
                setValue(item, shortcut);
                values.append(item);
            }
            {
                Json::Value item;
                createItem(item, fmt::format("/status/shortcut[{}]/attribute::id", idx), ConfigVal::MAX, ConfigVal::MAX);
                setValue(item, cont->getID());
                values.append(item);
            }
            {
                Json::Value item;
                createItem(item, fmt::format("/status/shortcut[{}]/attribute::location", idx), ConfigVal::MAX, ConfigVal::MAX);
                setValue(item, cont->getLocation());
                values.append(item);
            }
            idx++;
        }
    }
}

/// \brief: write all values with simple type (string, int, bool)
void Web::ConfigLoad::writeSimpleProperties(Json::Value& values)
{
    for (auto&& option : ConfigOptionIterator()) {
        try {
            auto scs = definition->findConfigSetup(option);
            Json::Value item;
            createItem(item, scs->getItemPathRoot(), option, option, scs);

            log_debug("    Option {:03d} {} = {}", option, scs->getItemPath({ 0 }, {}), scs->getCurrentValue().c_str());
            setValue(item, scs->getCurrentValue());
            values.append(item);
        } catch (const std::runtime_error& e) {
            log_warning("Option {:03d} {}", option, e.what());
        }
    }
}

/// \brief: write client configuration
void Web::ConfigLoad::writeClientConfig(Json::Value& values)
{
    auto cs = definition->findConfigSetup(ConfigVal::CLIENTS_LIST);
    auto clientConfig = cs->getValue()->getClientConfigListOption();
    for (std::size_t i = 0; i < EDIT_CAST(EditHelperClientConfig, clientConfig)->size(); i++) {
        auto client = EDIT_CAST(EditHelperClientConfig, clientConfig)->get(i);
        std::vector<std::size_t> indexList = { i };

        {
            Json::Value item;
            createItem(item, cs->getItemPath(indexList, { ConfigVal::A_CLIENTS_CLIENT, ConfigVal::A_CLIENTS_CLIENT_FLAGS }), cs->option, ConfigVal::A_CLIENTS_CLIENT_FLAGS, cs);
            setValue(item, ClientConfig::mapFlags(client->getFlags()));
            values.append(item);
        }
        {
            Json::Value item;
            createItem(item, cs->getItemPath(indexList, { ConfigVal::A_CLIENTS_CLIENT, ConfigVal::A_CLIENTS_CLIENT_IP }), cs->option, ConfigVal::A_CLIENTS_CLIENT_IP, cs);
            setValue(item, client->getIp());
            values.append(item);
        }
        {
            Json::Value item;
            createItem(item, cs->getItemPath(indexList, { ConfigVal::A_CLIENTS_CLIENT, ConfigVal::A_CLIENTS_CLIENT_USERAGENT }), cs->option, ConfigVal::A_CLIENTS_CLIENT_USERAGENT, cs);
            setValue(item, client->getUserAgent());
            values.append(item);
        }
        {
            Json::Value item;
            createItem(item, cs->getItemPath(indexList, { ConfigVal::A_CLIENTS_CLIENT, ConfigVal::A_CLIENTS_CLIENT_GROUP }), cs->option, ConfigVal::A_CLIENTS_CLIENT_GROUP, cs);
            setValue(item, client->getGroup());
            values.append(item);
        }
        {
            Json::Value item;
            createItem(item, cs->getItemPath(indexList, { ConfigVal::A_CLIENTS_CLIENT, ConfigVal::A_CLIENTS_CLIENT_ALLOWED }), cs->option, ConfigVal::A_CLIENTS_CLIENT_ALLOWED, cs);
            setValue(item, client->getAllowed());
            values.append(item);
        }
        {
            Json::Value item;
            createItem(item, cs->getItemPath(indexList, { ConfigVal::A_CLIENTS_CLIENT, ConfigVal::A_CLIENTS_UPNP_CAPTION_COUNT }), cs->option, ConfigVal::A_CLIENTS_UPNP_CAPTION_COUNT, cs);
            setValue(item, client->getCaptionInfoCount());
            values.append(item);
        }
        {
            Json::Value item;
            createItem(item, cs->getItemPath(indexList, { ConfigVal::A_CLIENTS_CLIENT, ConfigVal::A_CLIENTS_UPNP_STRING_LIMIT }), cs->option, ConfigVal::A_CLIENTS_UPNP_STRING_LIMIT, cs);
            setValue(item, client->getStringLimit());
            values.append(item);
        }
        {
            Json::Value item;
            createItem(item, cs->getItemPath(indexList, { ConfigVal::A_CLIENTS_CLIENT, ConfigVal::A_CLIENTS_UPNP_MULTI_VALUE }), cs->option, ConfigVal::A_CLIENTS_UPNP_MULTI_VALUE, cs);
            setValue(item, client->getMultiValue());
            values.append(item);
        }

        {
            // Sub Dictionary ConfigVal::A_CLIENTS_UPNP_MAP_MIMETYPE
            std::size_t j = 0;
            for (auto&& [from, to] : client->getMimeMappings(true)) {
                {
                    Json::Value item;
                    createItem(item, cs->getItemPath({ i, j }, { ConfigVal::A_CLIENTS_CLIENT, ConfigVal::A_IMPORT_MAPPINGS_MIMETYPE_MAP, ConfigVal::A_IMPORT_MAPPINGS_MIMETYPE_FROM }), cs->option, ConfigVal::A_IMPORT_MAPPINGS_MIMETYPE_FROM, cs);
                    setValue(item, from);
                    values.append(item);
                }
                {
                    Json::Value item;
                    createItem(item, cs->getItemPath({ i, j }, { ConfigVal::A_CLIENTS_CLIENT, ConfigVal::A_IMPORT_MAPPINGS_MIMETYPE_MAP, ConfigVal::A_IMPORT_MAPPINGS_MIMETYPE_TO }), cs->option, ConfigVal::A_IMPORT_MAPPINGS_MIMETYPE_TO, cs);
                    setValue(item, to);
                    values.append(item);
                }
            }
        }
        {
            // Sub Dictionary ConfigVal::A_CLIENTS_UPNP_HEADERS
            std::size_t j = 0;
            for (auto&& [key, value] : client->getHeaders(true)) {
                {
                    Json::Value item;
                    createItem(item, cs->getItemPath({ i, j }, { ConfigVal::A_CLIENTS_CLIENT, ConfigVal::A_CLIENTS_UPNP_HEADERS_HEADER, ConfigVal::A_CLIENTS_UPNP_HEADERS_KEY }), cs->option, ConfigVal::A_CLIENTS_UPNP_HEADERS_KEY, cs);
                    setValue(item, key);
                    values.append(item);
                }
                {
                    Json::Value item;
                    createItem(item, cs->getItemPath({ i, j }, { ConfigVal::A_CLIENTS_CLIENT, ConfigVal::A_CLIENTS_UPNP_HEADERS_HEADER, ConfigVal::A_CLIENTS_UPNP_HEADERS_VALUE }), cs->option, ConfigVal::A_CLIENTS_UPNP_HEADERS_VALUE, cs);
                    setValue(item, value);
                    values.append(item);
                }
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
                    Json::Value item;
                    std::size_t k = 0;
                    auto pos = std::find(attrList.begin(), attrList.end(), key);
                    if (pos != attrList.end())
                        k = std::distance(attrList.begin(), pos);
                    createItem(item, cs->getItemPath({ i, j }, { ConfigVal::A_CLIENTS_CLIENT, ConfigVal::A_CLIENTS_UPNP_MAP_DLNAPROFILE_PROFILE }, key), cs->option, vcs->optionList[k], cs);
                    setValue(item, val);
                    values.append(item);
                }
                j++;
            }
        }
    }
    // Allow creation of entry in blank config
    {
        {
            Json::Value item;
            createItem(item, cs->getItemPath(ITEM_PATH_NEW, { ConfigVal::A_CLIENTS_CLIENT, ConfigVal::A_CLIENTS_CLIENT_FLAGS }), cs->option, ConfigVal::A_CLIENTS_CLIENT_FLAGS, definition->findConfigSetup(ConfigVal::A_CLIENTS_CLIENT_FLAGS));
            values.append(item);
        }
        {
            Json::Value item;
            createItem(item, cs->getItemPath(ITEM_PATH_NEW, { ConfigVal::A_CLIENTS_CLIENT, ConfigVal::A_CLIENTS_CLIENT_IP }), cs->option, ConfigVal::A_CLIENTS_CLIENT_IP, definition->findConfigSetup(ConfigVal::A_CLIENTS_CLIENT_IP));
            values.append(item);
        }
        {
            Json::Value item;
            createItem(item, cs->getItemPath(ITEM_PATH_NEW, { ConfigVal::A_CLIENTS_CLIENT, ConfigVal::A_CLIENTS_CLIENT_USERAGENT }), cs->option, ConfigVal::A_CLIENTS_CLIENT_USERAGENT, definition->findConfigSetup(ConfigVal::A_CLIENTS_CLIENT_USERAGENT));
            values.append(item);
        }
        {
            Json::Value item;
            createItem(item, cs->getItemPath(ITEM_PATH_NEW, { ConfigVal::A_CLIENTS_CLIENT, ConfigVal::A_CLIENTS_CLIENT_GROUP }), cs->option, ConfigVal::A_CLIENTS_CLIENT_GROUP, definition->findConfigSetup(ConfigVal::A_CLIENTS_CLIENT_GROUP));
            values.append(item);
        }
        {
            Json::Value item;
            createItem(item, cs->getItemPath(ITEM_PATH_NEW, { ConfigVal::A_CLIENTS_CLIENT, ConfigVal::A_CLIENTS_UPNP_CAPTION_COUNT }), cs->option, ConfigVal::A_CLIENTS_UPNP_CAPTION_COUNT, definition->findConfigSetup(ConfigVal::A_CLIENTS_UPNP_CAPTION_COUNT));
            values.append(item);
        }
        {
            Json::Value item;
            createItem(item, cs->getItemPath(ITEM_PATH_NEW, { ConfigVal::A_CLIENTS_CLIENT, ConfigVal::A_CLIENTS_UPNP_STRING_LIMIT }), cs->option, ConfigVal::A_CLIENTS_UPNP_STRING_LIMIT, definition->findConfigSetup(ConfigVal::A_CLIENTS_UPNP_STRING_LIMIT));
            values.append(item);
        }
        {
            Json::Value item;
            createItem(item, cs->getItemPath(ITEM_PATH_NEW, { ConfigVal::A_CLIENTS_CLIENT, ConfigVal::A_CLIENTS_UPNP_MULTI_VALUE }), cs->option, ConfigVal::A_CLIENTS_UPNP_MULTI_VALUE, definition->findConfigSetup(ConfigVal::A_CLIENTS_UPNP_MULTI_VALUE));
            values.append(item);
        }
        {
            Json::Value item;
            createItem(item, cs->getItemPath(ITEM_PATH_NEW, { ConfigVal::A_CLIENTS_CLIENT, ConfigVal::A_IMPORT_MAPPINGS_MIMETYPE_MAP, ConfigVal::A_IMPORT_MAPPINGS_MIMETYPE_FROM }), cs->option, ConfigVal::A_IMPORT_MAPPINGS_MIMETYPE_FROM, cs);
            values.append(item);
        }
        {
            Json::Value item;
            createItem(item, cs->getItemPath(ITEM_PATH_NEW, { ConfigVal::A_CLIENTS_CLIENT, ConfigVal::A_IMPORT_MAPPINGS_MIMETYPE_MAP, ConfigVal::A_IMPORT_MAPPINGS_MIMETYPE_TO }), cs->option, ConfigVal::A_IMPORT_MAPPINGS_MIMETYPE_TO, cs);
            values.append(item);
        }
        {
            Json::Value item;
            createItem(item, cs->getItemPath(ITEM_PATH_NEW, { ConfigVal::A_CLIENTS_CLIENT, ConfigVal::A_CLIENTS_UPNP_HEADERS_HEADER, ConfigVal::A_CLIENTS_UPNP_HEADERS_KEY }), cs->option, ConfigVal::A_CLIENTS_UPNP_HEADERS_KEY, cs);
            values.append(item);
        }
        {
            Json::Value item;
            createItem(item, cs->getItemPath(ITEM_PATH_NEW, { ConfigVal::A_CLIENTS_CLIENT, ConfigVal::A_CLIENTS_UPNP_HEADERS_HEADER, ConfigVal::A_CLIENTS_UPNP_HEADERS_VALUE }), cs->option, ConfigVal::A_CLIENTS_UPNP_HEADERS_VALUE, cs);
            values.append(item);
        }
        {
            Json::Value item;
            createItem(item, cs->getItemPath(ITEM_PATH_NEW, { ConfigVal::A_CLIENTS_CLIENT, ConfigVal::A_CLIENTS_UPNP_MAP_DLNAPROFILE_PROFILE, ConfigVal::A_IMPORT_MAPPINGS_MIMETYPE_FROM }), cs->option, ConfigVal::A_IMPORT_MAPPINGS_MIMETYPE_FROM, cs);
            values.append(item);
        }
        {
            Json::Value item;
            createItem(item, cs->getItemPath(ITEM_PATH_NEW, { ConfigVal::A_CLIENTS_CLIENT, ConfigVal::A_CLIENTS_UPNP_MAP_DLNAPROFILE_PROFILE, ConfigVal::A_IMPORT_MAPPINGS_MIMETYPE_TO }), cs->option, ConfigVal::A_IMPORT_MAPPINGS_MIMETYPE_TO, cs);
            values.append(item);
        }

        // special treatment for vector
        auto vcs = definition->findConfigSetup<ConfigVectorSetup>(ConfigVal::A_CLIENTS_UPNP_MAP_DLNAPROFILE);
        std::vector<std::string> attrList;
        attrList.reserve(vcs->optionList.size());
        for (const auto& opt : vcs->optionList) {
            attrList.push_back(definition->removeAttribute(opt));
        }
        for (auto&& key : { "audioCodec", "videoCodec" }) {
            Json::Value item;
            std::size_t k = 0;
            auto pos = std::find(attrList.begin(), attrList.end(), key);
            if (pos != attrList.end())
                k = std::distance(attrList.begin(), pos);
            createItem(item, cs->getItemPath(ITEM_PATH_NEW, { ConfigVal::A_CLIENTS_CLIENT, ConfigVal::A_CLIENTS_UPNP_MAP_DLNAPROFILE_PROFILE }, key), cs->option, vcs->optionList[k], cs);
            values.append(item);
        }
    }
    for (std::size_t i = 0; i < EDIT_CAST(EditHelperClientGroupConfig, clientConfig)->size(); i++) {
        auto group = EDIT_CAST(EditHelperClientGroupConfig, clientConfig)->get(i);
        std::vector<std::size_t> indexList = { i };
        {
            Json::Value item;
            createItem(item, cs->getItemPath(indexList, { ConfigVal::A_CLIENTS_GROUP, ConfigVal::A_CLIENTS_GROUP_NAME }), cs->option, ConfigVal::A_CLIENTS_GROUP_NAME, cs);
            setValue(item, group->getGroupName());
            values.append(item);
        }
        auto forbiddenDirs = group->getForbiddenDirectories();
        for (std::size_t j = 0; j < forbiddenDirs.size(); j++) {
            std::vector<std::size_t> subIndexList = { i, j };
            Json::Value item;
            createItem(item, cs->getItemPath(subIndexList, { ConfigVal::A_CLIENTS_GROUP, ConfigVal::A_CLIENTS_GROUP_HIDE, ConfigVal::A_CLIENTS_GROUP_LOCATION }), cs->option, ConfigVal::A_CLIENTS_GROUP_LOCATION, cs);
            setValue(item, forbiddenDirs.at(j));
            values.append(item);
        }
    }
    // Allow creation of entry in blank config
    {
        {
            Json::Value item;
            createItem(item, cs->getItemPath(ITEM_PATH_NEW, { ConfigVal::A_CLIENTS_GROUP, ConfigVal::A_CLIENTS_GROUP_NAME }), cs->option, ConfigVal::A_CLIENTS_GROUP_NAME, definition->findConfigSetup(ConfigVal::A_CLIENTS_GROUP_NAME));
            values.append(item);
        }
        {
            Json::Value item;
            createItem(item, cs->getItemPath(ITEM_PATH_NEW, { ConfigVal::A_CLIENTS_GROUP, ConfigVal::A_CLIENTS_GROUP_LOCATION }), cs->option, ConfigVal::A_CLIENTS_GROUP_LOCATION, definition->findConfigSetup(ConfigVal::A_CLIENTS_GROUP_LOCATION));
            values.append(item);
        }
    }
}

/// \brief: write import tweaks
void Web::ConfigLoad::writeImportTweaks(Json::Value& values)
{
    auto cs = definition->findConfigSetup(ConfigVal::IMPORT_DIRECTORIES_LIST);
    auto directoryConfig = cs->getValue()->getDirectoryTweakOption();
    for (std::size_t i = 0; i < directoryConfig->size(); i++) {
        auto dir = directoryConfig->get(i);
        std::vector<std::size_t> indexList = { i };
        {
            Json::Value item;
            createItem(item, cs->getItemPath(indexList, { ConfigVal::A_DIRECTORIES_TWEAK_LOCATION }), cs->option, ConfigVal::A_DIRECTORIES_TWEAK_LOCATION);
            setValue(item, dir->getLocation());
            values.append(item);
        }
        {
            Json::Value item;
            createItem(item, cs->getItemPath(indexList, { ConfigVal::A_DIRECTORIES_TWEAK_INHERIT }), cs->option, ConfigVal::A_DIRECTORIES_TWEAK_INHERIT);
            setValue(item, dir->getInherit());
            values.append(item);
        }
        {
            Json::Value item;
            createItem(item, cs->getItemPath(indexList, { ConfigVal::A_DIRECTORIES_TWEAK_RECURSIVE }), cs->option, ConfigVal::A_DIRECTORIES_TWEAK_RECURSIVE);
            setValue(item, dir->getRecursive());
            values.append(item);
        }
        {
            Json::Value item;
            createItem(item, cs->getItemPath(indexList, { ConfigVal::A_DIRECTORIES_TWEAK_HIDDEN }), cs->option, ConfigVal::A_DIRECTORIES_TWEAK_HIDDEN);
            setValue(item, dir->getHidden());
            values.append(item);
        }
        {
            Json::Value item;
            createItem(item, cs->getItemPath(indexList, { ConfigVal::A_DIRECTORIES_TWEAK_CASE_SENSITIVE }), cs->option, ConfigVal::A_DIRECTORIES_TWEAK_CASE_SENSITIVE);
            setValue(item, dir->getCaseSensitive());
            values.append(item);
        }
        {
            Json::Value item;
            createItem(item, cs->getItemPath(indexList, { ConfigVal::A_DIRECTORIES_TWEAK_FOLLOW_SYMLINKS }), cs->option, ConfigVal::A_DIRECTORIES_TWEAK_FOLLOW_SYMLINKS);
            setValue(item, dir->getFollowSymlinks());
            values.append(item);
        }
        {
            Json::Value item;
            createItem(item, cs->getItemPath(indexList, { ConfigVal::A_DIRECTORIES_TWEAK_META_CHARSET }), cs->option, ConfigVal::A_DIRECTORIES_TWEAK_META_CHARSET);
            setValue(item, dir->hasMetaCharset() ? dir->getMetaCharset() : "");
            values.append(item);
        }
        {
            Json::Value item;
            createItem(item, cs->getItemPath(indexList, { ConfigVal::A_DIRECTORIES_TWEAK_FANART_FILE }), cs->option, ConfigVal::A_DIRECTORIES_TWEAK_FANART_FILE);
            setValue(item, dir->hasFanArtFile() ? dir->getFanArtFile() : "");
            values.append(item);
        }
        {
            Json::Value item;
            createItem(item, cs->getItemPath(indexList, { ConfigVal::A_DIRECTORIES_TWEAK_RESOURCE_FILE }), cs->option, ConfigVal::A_DIRECTORIES_TWEAK_RESOURCE_FILE);
            setValue(item, dir->hasResourceFile() ? dir->getResourceFile() : "");
            values.append(item);
        }
        {
            Json::Value item;
            createItem(item, cs->getItemPath(indexList, { ConfigVal::A_DIRECTORIES_TWEAK_SUBTITLE_FILE }), cs->option, ConfigVal::A_DIRECTORIES_TWEAK_SUBTITLE_FILE);
            setValue(item, dir->hasSubTitleFile() ? dir->getSubTitleFile() : "");
            values.append(item);
        }
        {
            Json::Value item;
            createItem(item, cs->getItemPath(indexList, { ConfigVal::A_DIRECTORIES_TWEAK_METAFILE_FILE }), cs->option, ConfigVal::A_DIRECTORIES_TWEAK_METAFILE_FILE);
            setValue(item, dir->hasMetafile() ? dir->getMetafile() : "");
            values.append(item);
        }
    }
    // Allow creation of entry in blank config
    {
        {
            Json::Value item;
            createItem(item, cs->getItemPath(ITEM_PATH_NEW, { ConfigVal::A_DIRECTORIES_TWEAK_LOCATION }), cs->option, ConfigVal::A_DIRECTORIES_TWEAK_LOCATION);
            values.append(item);
        }
        {
            Json::Value item;
            createItem(item, cs->getItemPath(ITEM_PATH_NEW, { ConfigVal::A_DIRECTORIES_TWEAK_INHERIT }), cs->option, ConfigVal::A_DIRECTORIES_TWEAK_INHERIT);
            values.append(item);
        }
        {
            Json::Value item;
            createItem(item, cs->getItemPath(ITEM_PATH_NEW, { ConfigVal::A_DIRECTORIES_TWEAK_RECURSIVE }), cs->option, ConfigVal::A_DIRECTORIES_TWEAK_RECURSIVE);
            values.append(item);
        }
        {
            Json::Value item;
            createItem(item, cs->getItemPath(ITEM_PATH_NEW, { ConfigVal::A_DIRECTORIES_TWEAK_HIDDEN }), cs->option, ConfigVal::A_DIRECTORIES_TWEAK_HIDDEN);
            values.append(item);
        }
        {
            Json::Value item;
            createItem(item, cs->getItemPath(ITEM_PATH_NEW, { ConfigVal::A_DIRECTORIES_TWEAK_CASE_SENSITIVE }), cs->option, ConfigVal::A_DIRECTORIES_TWEAK_CASE_SENSITIVE);
            values.append(item);
        }
        {
            Json::Value item;
            createItem(item, cs->getItemPath(ITEM_PATH_NEW, { ConfigVal::A_DIRECTORIES_TWEAK_FOLLOW_SYMLINKS }), cs->option, ConfigVal::A_DIRECTORIES_TWEAK_FOLLOW_SYMLINKS);
            values.append(item);
        }
        {
            Json::Value item;
            createItem(item, cs->getItemPath(ITEM_PATH_NEW, { ConfigVal::A_DIRECTORIES_TWEAK_META_CHARSET }), cs->option, ConfigVal::A_DIRECTORIES_TWEAK_META_CHARSET);
            values.append(item);
        }
        {
            Json::Value item;
            createItem(item, cs->getItemPath(ITEM_PATH_NEW, { ConfigVal::A_DIRECTORIES_TWEAK_FANART_FILE }), cs->option, ConfigVal::A_DIRECTORIES_TWEAK_FANART_FILE);
            values.append(item);
        }
        {
            Json::Value item;
            createItem(item, cs->getItemPath(ITEM_PATH_NEW, { ConfigVal::A_DIRECTORIES_TWEAK_RESOURCE_FILE }), cs->option, ConfigVal::A_DIRECTORIES_TWEAK_RESOURCE_FILE);
            values.append(item);
        }
        {
            Json::Value item;
            createItem(item, cs->getItemPath(ITEM_PATH_NEW, { ConfigVal::A_DIRECTORIES_TWEAK_SUBTITLE_FILE }), cs->option, ConfigVal::A_DIRECTORIES_TWEAK_SUBTITLE_FILE);
            values.append(item);
        }
        {
            Json::Value item;
            createItem(item, cs->getItemPath(ITEM_PATH_NEW, { ConfigVal::A_DIRECTORIES_TWEAK_METAFILE_FILE }), cs->option, ConfigVal::A_DIRECTORIES_TWEAK_METAFILE_FILE);
            values.append(item);
        }
    }
}

/// \brief: write dynamic content
void Web::ConfigLoad::writeDynamicContent(Json::Value& values)
{
    auto cs = definition->findConfigSetup(ConfigVal::SERVER_DYNAMIC_CONTENT_LIST);
    auto dynContent = cs->getValue()->getDynamicContentListOption();
    for (std::size_t i = 0; i < dynContent->size(); i++) {
        auto cont = dynContent->get(i);
        std::vector<std::size_t> indexList = { i };

        {
            Json::Value item;
            createItem(item, cs->getItemPath(indexList, { ConfigVal::A_DYNAMIC_CONTAINER_LOCATION }), cs->option, ConfigVal::A_DYNAMIC_CONTAINER_LOCATION);
            setValue(item, cont->getLocation());
            values.append(item);
        }
        {
            Json::Value item;
            createItem(item, cs->getItemPath(indexList, { ConfigVal::A_DYNAMIC_CONTAINER_IMAGE }), cs->option, ConfigVal::A_DYNAMIC_CONTAINER_IMAGE);
            setValue(item, cont->getImage());
            values.append(item);
        }
        {
            Json::Value item;
            createItem(item, cs->getItemPath(indexList, { ConfigVal::A_DYNAMIC_CONTAINER_TITLE }), cs->option, ConfigVal::A_DYNAMIC_CONTAINER_TITLE);
            setValue(item, cont->getTitle());
            values.append(item);
        }
        {
            Json::Value item;
            createItem(item, cs->getItemPath(indexList, { ConfigVal::A_DYNAMIC_CONTAINER_FILTER }), cs->option, ConfigVal::A_DYNAMIC_CONTAINER_FILTER);
            setValue(item, cont->getFilter());
            values.append(item);
        }
        {
            Json::Value item;
            createItem(item, cs->getItemPath(indexList, { ConfigVal::A_DYNAMIC_CONTAINER_SORT }), cs->option, ConfigVal::A_DYNAMIC_CONTAINER_SORT);
            setValue(item, cont->getSort());
            values.append(item);
        }
        {
            Json::Value item;
            createItem(item, cs->getItemPath(indexList, { ConfigVal::A_DYNAMIC_CONTAINER_MAXCOUNT }), cs->option, ConfigVal::A_DYNAMIC_CONTAINER_MAXCOUNT);
            setValue(item, cont->getMaxCount());
            values.append(item);
        }
        {
            Json::Value item;
            createItem(item, cs->getItemPath(indexList, { ConfigVal::A_DYNAMIC_CONTAINER_UPNP_SHORTCUT }), cs->option, ConfigVal::A_DYNAMIC_CONTAINER_UPNP_SHORTCUT);
            setValue(item, cont->getUpnpShortcut());
            values.append(item);
        }
    }
    // Allow creation of entry in blank config
    {
        {
            Json::Value item;
            createItem(item, cs->getItemPath(ITEM_PATH_NEW, { ConfigVal::A_DYNAMIC_CONTAINER_LOCATION }), cs->option, ConfigVal::A_DYNAMIC_CONTAINER_LOCATION, definition->findConfigSetup(ConfigVal::A_DYNAMIC_CONTAINER_LOCATION));
            values.append(item);
        }
        {
            Json::Value item;
            createItem(item, cs->getItemPath(ITEM_PATH_NEW, { ConfigVal::A_DYNAMIC_CONTAINER_IMAGE }), cs->option, ConfigVal::A_DYNAMIC_CONTAINER_IMAGE, definition->findConfigSetup(ConfigVal::A_DYNAMIC_CONTAINER_IMAGE));
            values.append(item);
        }
        {
            Json::Value item;
            createItem(item, cs->getItemPath(ITEM_PATH_NEW, { ConfigVal::A_DYNAMIC_CONTAINER_TITLE }), cs->option, ConfigVal::A_DYNAMIC_CONTAINER_TITLE, definition->findConfigSetup(ConfigVal::A_DYNAMIC_CONTAINER_TITLE));
            values.append(item);
        }
        {
            Json::Value item;
            createItem(item, cs->getItemPath(ITEM_PATH_NEW, { ConfigVal::A_DYNAMIC_CONTAINER_FILTER }), cs->option, ConfigVal::A_DYNAMIC_CONTAINER_FILTER, definition->findConfigSetup(ConfigVal::A_DYNAMIC_CONTAINER_FILTER));
            values.append(item);
        }
        {
            Json::Value item;
            createItem(item, cs->getItemPath(ITEM_PATH_NEW, { ConfigVal::A_DYNAMIC_CONTAINER_SORT }), cs->option, ConfigVal::A_DYNAMIC_CONTAINER_SORT, definition->findConfigSetup(ConfigVal::A_DYNAMIC_CONTAINER_SORT));
            values.append(item);
        }
        {
            Json::Value item;
            createItem(item, cs->getItemPath(ITEM_PATH_NEW, { ConfigVal::A_DYNAMIC_CONTAINER_MAXCOUNT }), cs->option, ConfigVal::A_DYNAMIC_CONTAINER_MAXCOUNT, definition->findConfigSetup(ConfigVal::A_DYNAMIC_CONTAINER_MAXCOUNT));
            values.append(item);
        }
        {
            Json::Value item;
            createItem(item, cs->getItemPath(ITEM_PATH_NEW, { ConfigVal::A_DYNAMIC_CONTAINER_UPNP_SHORTCUT }), cs->option, ConfigVal::A_DYNAMIC_CONTAINER_UPNP_SHORTCUT, definition->findConfigSetup(ConfigVal::A_DYNAMIC_CONTAINER_UPNP_SHORTCUT));
            values.append(item);
        }
    }
}

/// \brief: write box layout
void Web::ConfigLoad::writeBoxLayout(Json::Value& values)
{
    auto cs = definition->findConfigSetup(ConfigVal::BOXLAYOUT_BOX);
    auto boxlayoutContent = cs->getValue()->getBoxLayoutListOption();
    for (std::size_t i = 0; i < boxlayoutContent->size(); i++) {
        auto cont = boxlayoutContent->get(i);
        std::vector<std::size_t> indexList = { i };
        {
            Json::Value item;
            createItem(item, cs->getItemPath(indexList, { ConfigVal::A_BOXLAYOUT_BOX_KEY }), cs->option, ConfigVal::A_BOXLAYOUT_BOX_KEY);
            setValue(item, cont->getKey());
            values.append(item);
        }
        {
            Json::Value item;
            createItem(item, cs->getItemPath(indexList, { ConfigVal::A_BOXLAYOUT_BOX_TITLE }), cs->option, ConfigVal::A_BOXLAYOUT_BOX_TITLE);
            setValue(item, cont->getTitle());
            values.append(item);
        }
        {
            Json::Value item;
            createItem(item, cs->getItemPath(indexList, { ConfigVal::A_BOXLAYOUT_BOX_CLASS }), cs->option, ConfigVal::A_BOXLAYOUT_BOX_CLASS);
            setValue(item, cont->getClass());
            values.append(item);
        }
        {
            Json::Value item;
            createItem(item, cs->getItemPath(indexList, { ConfigVal::A_BOXLAYOUT_BOX_SIZE }), cs->option, ConfigVal::A_BOXLAYOUT_BOX_SIZE);
            setValue(item, cont->getSize());
            values.append(item);
        }
        {
            Json::Value item;
            createItem(item, cs->getItemPath(indexList, { ConfigVal::A_BOXLAYOUT_BOX_ENABLED }), cs->option, ConfigVal::A_BOXLAYOUT_BOX_ENABLED);
            setValue(item, cont->getEnabled());
            values.append(item);
        }
        {
            Json::Value item;
            createItem(item, cs->getItemPath(indexList, { ConfigVal::A_BOXLAYOUT_BOX_UPNP_SHORTCUT }), cs->option, ConfigVal::A_BOXLAYOUT_BOX_UPNP_SHORTCUT);
            setValue(item, cont->getUpnpShortcut());
            values.append(item);
        }
    }
    // Allow creation of entry in blank config
    {
        {
            Json::Value item;
            createItem(item, cs->getItemPath(ITEM_PATH_NEW, { ConfigVal::A_BOXLAYOUT_BOX_KEY }), cs->option, ConfigVal::A_BOXLAYOUT_BOX_KEY, definition->findConfigSetup(ConfigVal::A_BOXLAYOUT_BOX_KEY));
            values.append(item);
        }
        {
            Json::Value item;
            createItem(item, cs->getItemPath(ITEM_PATH_NEW, { ConfigVal::A_BOXLAYOUT_BOX_TITLE }), cs->option, ConfigVal::A_BOXLAYOUT_BOX_TITLE, definition->findConfigSetup(ConfigVal::A_BOXLAYOUT_BOX_TITLE));
            values.append(item);
        }
        {
            Json::Value item;
            createItem(item, cs->getItemPath(ITEM_PATH_NEW, { ConfigVal::A_BOXLAYOUT_BOX_CLASS }), cs->option, ConfigVal::A_BOXLAYOUT_BOX_CLASS, definition->findConfigSetup(ConfigVal::A_BOXLAYOUT_BOX_CLASS));
            values.append(item);
        }
        {
            Json::Value item;
            createItem(item, cs->getItemPath(ITEM_PATH_NEW, { ConfigVal::A_BOXLAYOUT_BOX_SIZE }), cs->option, ConfigVal::A_BOXLAYOUT_BOX_SIZE, definition->findConfigSetup(ConfigVal::A_BOXLAYOUT_BOX_SIZE));
            values.append(item);
        }
        {
            Json::Value item;
            createItem(item, cs->getItemPath(ITEM_PATH_NEW, { ConfigVal::A_BOXLAYOUT_BOX_ENABLED }), cs->option, ConfigVal::A_BOXLAYOUT_BOX_ENABLED, definition->findConfigSetup(ConfigVal::A_BOXLAYOUT_BOX_ENABLED));
            values.append(item);
        }
        {
            Json::Value item;
            createItem(item, cs->getItemPath(ITEM_PATH_NEW, { ConfigVal::A_BOXLAYOUT_BOX_UPNP_SHORTCUT }), cs->option, ConfigVal::A_BOXLAYOUT_BOX_UPNP_SHORTCUT, definition->findConfigSetup(ConfigVal::A_BOXLAYOUT_BOX_UPNP_SHORTCUT));
            values.append(item);
        }
    }
}

/// \brief: write transconding configuration
void Web::ConfigLoad::writeTranscoding(Json::Value& values)
{
    auto cs = definition->findConfigSetup(ConfigVal::TRANSCODING_PROFILE_LIST);
    auto transcoding = cs->getValue()->getTranscodingProfileListOption();
    std::size_t pr = 0;
    std::map<std::string, std::shared_ptr<TranscodingProfile>> profiles;
    // write filter list and collect required profiles
    for (auto&& filter : transcoding->getFilterList()) {
        std::vector<std::size_t> indexList = { pr };
        {
            Json::Value item;
            createItem(item, cs->getItemPath(indexList, { ConfigVal::A_TRANSCODING_MIMETYPE_FILTER, ConfigVal::A_TRANSCODING_MIMETYPE_PROF_MAP_MIMETYPE }), cs->option, ConfigVal::A_TRANSCODING_MIMETYPE_PROF_MAP_MIMETYPE, cs);
            setValue(item, filter->getMimeType());
            values.append(item);
        }
        {
            Json::Value item;
            createItem(item, cs->getItemPath(indexList, { ConfigVal::A_TRANSCODING_MIMETYPE_FILTER, ConfigVal::A_TRANSCODING_MIMETYPE_PROF_MAP_USING }), cs->option, ConfigVal::A_TRANSCODING_MIMETYPE_PROF_MAP_USING, cs);
            setValue(item, filter->getTranscoderName());
            values.append(item);
        }
        {
            Json::Value item;
            createItem(item, cs->getItemPath(indexList, { ConfigVal::A_TRANSCODING_MIMETYPE_FILTER, ConfigVal::A_TRANSCODING_PROFILES_PROFLE_SRCDLNA }), cs->option, ConfigVal::A_TRANSCODING_PROFILES_PROFLE_SRCDLNA, cs);
            setValue(item, filter->getSourceProfile());
            values.append(item);
        }
        {
            Json::Value item;
            createItem(item, cs->getItemPath(indexList, { ConfigVal::A_TRANSCODING_MIMETYPE_FILTER, ConfigVal::A_TRANSCODING_PROFILES_PROFLE_NOTRANSCODING }), cs->option, ConfigVal::A_TRANSCODING_PROFILES_PROFLE_NOTRANSCODING, cs);
            setValue(item, fmt::to_string(fmt::join(filter->getNoTranscodingMimeTypes(), ",")));
            values.append(item);
        }
        {
            Json::Value item;
            createItem(item, cs->getItemPath(indexList, { ConfigVal::A_TRANSCODING_MIMETYPE_FILTER, ConfigVal::A_TRANSCODING_PROFILES_PROFLE_CLIENTFLAGS }), cs->option, ConfigVal::A_TRANSCODING_PROFILES_PROFLE_CLIENTFLAGS, cs);
            setValue(item, ClientConfig::mapFlags(filter->getClientFlags()));
            values.append(item);
        }

        if (filter->getTranscodingProfile())
            profiles[filter->getTranscodingProfile()->getName()] = filter->getTranscodingProfile();

        pr++;
    }

    pr = 0;
    // write profile list
    // profiles can only be exported when linked to at least one filter
    for (auto&& [name, entry] : profiles) {
        std::vector<std::size_t> indexList = { pr };
        {
            Json::Value item;
            createItem(item, cs->getItemPath(indexList, { ConfigVal::A_TRANSCODING_PROFILES_PROFLE, ConfigVal::A_TRANSCODING_PROFILES_PROFLE_NAME }), cs->option, ConfigVal::A_TRANSCODING_PROFILES_PROFLE_NAME);
            setValue(item, name);
            values.append(item);
        }
        {
            Json::Value item;
            createItem(item, cs->getItemPath(indexList, { ConfigVal::A_TRANSCODING_PROFILES_PROFLE, ConfigVal::A_TRANSCODING_PROFILES_PROFLE_CLIENTFLAGS }), cs->option, ConfigVal::A_TRANSCODING_PROFILES_PROFLE_CLIENTFLAGS);
            setValue(item, ClientConfig::mapFlags(entry->getClientFlags()));
            values.append(item);
        }
        {
            Json::Value item;
            createItem(item, cs->getItemPath(indexList, { ConfigVal::A_TRANSCODING_PROFILES_PROFLE, ConfigVal::A_TRANSCODING_PROFILES_PROFLE_ENABLED }), cs->option, ConfigVal::A_TRANSCODING_PROFILES_PROFLE_ENABLED);
            setValue(item, entry->isEnabled());
            values.append(item);
        }
        {
            Json::Value item;
            auto ttEnumSetup = definition->findConfigSetup<ConfigEnumSetup<TranscodingType>>(ConfigVal::A_TRANSCODING_PROFILES_PROFLE_TYPE);
            createItem(item, cs->getItemPath(indexList, { ConfigVal::A_TRANSCODING_PROFILES_PROFLE, ConfigVal::A_TRANSCODING_PROFILES_PROFLE_TYPE }), cs->option, ConfigVal::A_TRANSCODING_PROFILES_PROFLE_TYPE);
            setValue(item, ttEnumSetup->mapEnumValue(entry->getType()));
            values.append(item);
        }
        {
            Json::Value item;
            createItem(item, cs->getItemPath(indexList, { ConfigVal::A_TRANSCODING_PROFILES_PROFLE, ConfigVal::A_TRANSCODING_PROFILES_PROFLE_DLNAPROF }), cs->option, ConfigVal::A_TRANSCODING_PROFILES_PROFLE_DLNAPROF);
            setValue(item, entry->getDlnaProfile());
            values.append(item);
        }
        {
            Json::Value item;
            createItem(item, cs->getItemPath(indexList, { ConfigVal::A_TRANSCODING_PROFILES_PROFLE, ConfigVal::A_TRANSCODING_PROFILES_PROFLE_MIMETYPE }), cs->option, ConfigVal::A_TRANSCODING_PROFILES_PROFLE_MIMETYPE);
            setValue(item, entry->getTargetMimeType());
            values.append(item);
        }
        {
            Json::Value item;
            createItem(item, cs->getItemPath(indexList, { ConfigVal::A_TRANSCODING_PROFILES_PROFLE, ConfigVal::A_TRANSCODING_PROFILES_PROFLE_RESOLUTION }), cs->option, ConfigVal::A_TRANSCODING_PROFILES_PROFLE_RESOLUTION);
            setValue(item, entry->getAttributeOverride(ResourceAttribute::RESOLUTION));
            values.append(item);
        }
        {
            Json::Value item;
            createItem(item, cs->getItemPath(indexList, { ConfigVal::A_TRANSCODING_PROFILES_PROFLE, ConfigVal::A_TRANSCODING_PROFILES_PROFLE_ACCURL }), cs->option, ConfigVal::A_TRANSCODING_PROFILES_PROFLE_ACCURL);
            setValue(item, entry->getAcceptURL());
            values.append(item);
        }
        {
            Json::Value item;
            createItem(item, cs->getItemPath(indexList, { ConfigVal::A_TRANSCODING_PROFILES_PROFLE, ConfigVal::A_TRANSCODING_PROFILES_PROFLE_SAMPFREQ }), cs->option, ConfigVal::A_TRANSCODING_PROFILES_PROFLE_SAMPFREQ);
            setValue(item, entry->getSampleFreq());
            values.append(item);
        }
        {
            Json::Value item;
            createItem(item, cs->getItemPath(indexList, { ConfigVal::A_TRANSCODING_PROFILES_PROFLE, ConfigVal::A_TRANSCODING_PROFILES_PROFLE_NRCHAN }), cs->option, ConfigVal::A_TRANSCODING_PROFILES_PROFLE_NRCHAN);
            setValue(item, entry->getNumChannels());
            values.append(item);
        }
        {
            Json::Value item;
            createItem(item, cs->getItemPath(indexList, { ConfigVal::A_TRANSCODING_PROFILES_PROFLE, ConfigVal::A_TRANSCODING_PROFILES_PROFLE_HIDEORIG }), cs->option, ConfigVal::A_TRANSCODING_PROFILES_PROFLE_HIDEORIG);
            setValue(item, entry->hideOriginalResource());
            values.append(item);
        }
        {
            Json::Value item;
            createItem(item, cs->getItemPath({ pr }, { ConfigVal::A_TRANSCODING_PROFILES_PROFLE, ConfigVal::A_TRANSCODING_PROFILES_PROFLE_THUMB }), cs->option, ConfigVal::A_TRANSCODING_PROFILES_PROFLE_THUMB);
            setValue(item, entry->isThumbnail());
            values.append(item);
        }
        {
            Json::Value item;
            createItem(item, cs->getItemPath(indexList, { ConfigVal::A_TRANSCODING_PROFILES_PROFLE, ConfigVal::A_TRANSCODING_PROFILES_PROFLE_FIRST }), cs->option, ConfigVal::A_TRANSCODING_PROFILES_PROFLE_FIRST);
            setValue(item, entry->getFirstResource());
            values.append(item);
        }
        {
            Json::Value item;
            createItem(item, cs->getItemPath(indexList, { ConfigVal::A_TRANSCODING_PROFILES_PROFLE, ConfigVal::A_TRANSCODING_PROFILES_PROFLE_ACCOGG }), cs->option, ConfigVal::A_TRANSCODING_PROFILES_PROFLE_ACCOGG);
            setValue(item, entry->isTheora());
            values.append(item);
        }
        {
            Json::Value item;
            createItem(item, cs->getItemPath(indexList, { ConfigVal::A_TRANSCODING_PROFILES_PROFLE, ConfigVal::A_TRANSCODING_PROFILES_PROFLE_AGENT, ConfigVal::A_TRANSCODING_PROFILES_PROFLE_AGENT_COMMAND }), cs->option, ConfigVal::A_TRANSCODING_PROFILES_PROFLE_AGENT_COMMAND);
            setValue(item, entry->getCommand());
            values.append(item);
        }
        {
            Json::Value item;
            createItem(item, cs->getItemPath(indexList, { ConfigVal::A_TRANSCODING_PROFILES_PROFLE, ConfigVal::A_TRANSCODING_PROFILES_PROFLE_AGENT, ConfigVal::A_TRANSCODING_PROFILES_PROFLE_AGENT_ARGS }), cs->option, ConfigVal::A_TRANSCODING_PROFILES_PROFLE_AGENT_ARGS);
            setValue(item, entry->getArguments());
            values.append(item);
        }
        {
            Json::Value item;
            createItem(item, cs->getItemPath(indexList, { ConfigVal::A_TRANSCODING_PROFILES_PROFLE, ConfigVal::A_TRANSCODING_PROFILES_PROFLE_BUFFER, ConfigVal::A_TRANSCODING_PROFILES_PROFLE_BUFFER_SIZE }), cs->option, ConfigVal::A_TRANSCODING_PROFILES_PROFLE_BUFFER_SIZE);
            setValue(item, entry->getBufferSize());
            values.append(item);
        }
        {
            Json::Value item;
            createItem(item, cs->getItemPath(indexList, { ConfigVal::A_TRANSCODING_PROFILES_PROFLE, ConfigVal::A_TRANSCODING_PROFILES_PROFLE_BUFFER, ConfigVal::A_TRANSCODING_PROFILES_PROFLE_BUFFER_CHUNK }), cs->option, ConfigVal::A_TRANSCODING_PROFILES_PROFLE_BUFFER_CHUNK);
            setValue(item, entry->getBufferChunkSize());
            values.append(item);
        }
        {
            Json::Value item;
            createItem(item, cs->getItemPath(indexList, { ConfigVal::A_TRANSCODING_PROFILES_PROFLE, ConfigVal::A_TRANSCODING_PROFILES_PROFLE_BUFFER, ConfigVal::A_TRANSCODING_PROFILES_PROFLE_BUFFER_FILL }), cs->option, ConfigVal::A_TRANSCODING_PROFILES_PROFLE_BUFFER_FILL);
            setValue(item, entry->getBufferInitialFillSize());
            values.append(item);
        }
        auto fourCCMode = entry->getAVIFourCCListMode();
        if (fourCCMode != AviFourccListmode::None) {
            {
                Json::Value item;
                createItem(item, cs->getItemPath(indexList, { ConfigVal::A_TRANSCODING_PROFILES_PROFLE, ConfigVal::A_TRANSCODING_PROFILES_PROFLE_AVI4CC, ConfigVal::A_TRANSCODING_PROFILES_PROFLE_AVI4CC_MODE }), cs->option, ConfigVal::A_TRANSCODING_PROFILES_PROFLE_AVI4CC_MODE);
                auto fccEnumSetup = definition->findConfigSetup<ConfigEnumSetup<AviFourccListmode>>(ConfigVal::A_TRANSCODING_PROFILES_PROFLE_AVI4CC_MODE);
                setValue(item, fccEnumSetup->mapEnumValue(fourCCMode));
                values.append(item);
            }

            const auto& fourCCList = entry->getAVIFourCCList();
            if (!fourCCList.empty()) {
                Json::Value item;
                createItem(item, cs->getItemPath(indexList, { ConfigVal::A_TRANSCODING_PROFILES_PROFLE, ConfigVal::A_TRANSCODING_PROFILES_PROFLE_AVI4CC, ConfigVal::A_TRANSCODING_PROFILES_PROFLE_AVI4CC_4CC }), cs->option, ConfigVal::A_TRANSCODING_PROFILES_PROFLE_AVI4CC_4CC);
                setValue(item, std::accumulate(next(fourCCList.begin()), fourCCList.end(), fourCCList[0], [](auto&& a, auto&& b) { return fmt::format("{}, {}", a, b); }));
                values.append(item);
            }
        }
        pr++;
    }
}

/// \brief: write autoscan configuration
void Web::ConfigLoad::writeAutoscan(Json::Value& values)
{
    for (auto&& ascs : definition->getConfigSetupList<ConfigAutoscanSetup>()) {
        auto autoscan = ascs->getValue()->getAutoscanListOption();
        // create separate entries for timed and inotify
        for (std::size_t i = 0; i < autoscan.size(); i++) {
            auto&& entry = autoscan.at(i);
            std::vector<std::size_t> indexList = { i };
            auto&& adir = content->getAutoscanDirectory(entry->getLocation());

            {
                // path
                Json::Value item;
                createItem(item, ascs->getItemPath(indexList, { ConfigVal::A_AUTOSCAN_DIRECTORY_LOCATION }), ascs->option, ConfigVal::A_AUTOSCAN_DIRECTORY_LOCATION);
                setValue(item, adir->getLocation());
                values.append(item);
            }
            {
                // scan mode (timed|inotify)
                Json::Value item;
                createItem(item, ascs->getItemPath(indexList, { ConfigVal::A_AUTOSCAN_DIRECTORY_MODE }), ascs->option, ConfigVal::A_AUTOSCAN_DIRECTORY_MODE);
                setValue(item, fmt::to_string(AutoscanDirectory::mapScanmode(adir->getScanMode())));
                values.append(item);
            }
            {
                // interval for timed
                Json::Value item;
                createItem(item, ascs->getItemPath(indexList, { ConfigVal::A_AUTOSCAN_DIRECTORY_INTERVAL }), ascs->option, ConfigVal::A_AUTOSCAN_DIRECTORY_INTERVAL);
                setValue(item, adir->getInterval().count());
                values.append(item);
            }
            {
                // counter for retries
                Json::Value item;
                createItem(item, ascs->getItemPath(indexList, { ConfigVal::A_AUTOSCAN_DIRECTORY_RETRYCOUNT }), ascs->option, ConfigVal::A_AUTOSCAN_DIRECTORY_RETRYCOUNT);
                setValue(item, adir->getInterval().count());
                values.append(item);
            }
            {
                // recursively import files
                Json::Value item;
                createItem(item, ascs->getItemPath(indexList, { ConfigVal::A_AUTOSCAN_DIRECTORY_RECURSIVE }), ascs->option, ConfigVal::A_AUTOSCAN_DIRECTORY_RECURSIVE);
                setValue(item, adir->getRecursive());
                values.append(item);
            }
            {
                // directory types
                Json::Value item;
                createItem(item, ascs->getItemPath(indexList, { ConfigVal::A_AUTOSCAN_DIRECTORY_DIRTYPES }), ascs->option, ConfigVal::A_AUTOSCAN_DIRECTORY_DIRTYPES);
                setValue(item, adir->hasDirTypes());
                values.append(item);
            }
            {
                // media types imported from directory
                Json::Value item;
                createItem(item, ascs->getItemPath(indexList, { ConfigVal::A_AUTOSCAN_DIRECTORY_MEDIATYPE }), ascs->option, ConfigVal::A_AUTOSCAN_DIRECTORY_MEDIATYPE);
                setValue(item, AutoscanDirectory::mapMediaType(adir->getMediaType()));
                values.append(item);
            }
            {
                // import hidden files
                Json::Value item;
                createItem(item, ascs->getItemPath(indexList, { ConfigVal::A_AUTOSCAN_DIRECTORY_HIDDENFILES }), ascs->option, ConfigVal::A_AUTOSCAN_DIRECTORY_HIDDENFILES);
                setValue(item, adir->getHidden());
                values.append(item);
            }
            {
                // follow symbolic links
                Json::Value item;
                createItem(item, ascs->getItemPath(indexList, { ConfigVal::A_AUTOSCAN_DIRECTORY_FOLLOWSYMLINKS }), ascs->option, ConfigVal::A_AUTOSCAN_DIRECTORY_FOLLOWSYMLINKS);
                setValue(item, adir->getFollowSymlinks());
                values.append(item);
            }
            {
                // container type for audio
                Json::Value item;
                createItem(item, ascs->getItemPath(indexList, { ConfigVal::A_AUTOSCAN_CONTAINER_TYPE_AUDIO }), ascs->option, ConfigVal::A_AUTOSCAN_CONTAINER_TYPE_AUDIO);
                setValue(item, adir->getContainerTypes().at(AutoscanMediaMode::Audio));
                values.append(item);
            }
            {
                // container type for images
                Json::Value item;
                createItem(item, ascs->getItemPath(indexList, { ConfigVal::A_AUTOSCAN_CONTAINER_TYPE_IMAGE }), ascs->option, ConfigVal::A_AUTOSCAN_CONTAINER_TYPE_IMAGE);
                setValue(item, adir->getContainerTypes().at(AutoscanMediaMode::Image));
                values.append(item);
            }
            {
                // container type for videos
                Json::Value item;
                createItem(item, ascs->getItemPath(indexList, { ConfigVal::A_AUTOSCAN_CONTAINER_TYPE_VIDEO }), ascs->option, ConfigVal::A_AUTOSCAN_CONTAINER_TYPE_VIDEO);
                setValue(item, adir->getContainerTypes().at(AutoscanMediaMode::Video));
                values.append(item);
            }
            {
                // active scan count
                Json::Value item;
                createItem(item, ascs->getItemPath(indexList, { ConfigVal::A_AUTOSCAN_DIRECTORY_SCANCOUNT }), ascs->option, ConfigVal::A_AUTOSCAN_DIRECTORY_SCANCOUNT);
                setValue(item, adir->getActiveScanCount());
                values.append(item);
            }
            {
                // active task count
                Json::Value item;
                createItem(item, ascs->getItemPath(indexList, { ConfigVal::A_AUTOSCAN_DIRECTORY_TASKCOUNT }), ascs->option, ConfigVal::A_AUTOSCAN_DIRECTORY_TASKCOUNT);
                setValue(item, adir->getTaskCount());
                values.append(item);
            }
            {
                // Last modified
                Json::Value item;
                createItem(item, ascs->getItemPath(indexList, { ConfigVal::A_AUTOSCAN_DIRECTORY_LMT }), ascs->option, ConfigVal::A_AUTOSCAN_DIRECTORY_LMT);
                setValue(item, fmt::format("{:%Y-%m-%d %H:%M:%S}", fmt::localtime(adir->getPreviousLMT().count())));
                values.append(item);
            }
        }
        // Allow creation of entry in blank config
        {
            {
                // path
                Json::Value item;
                createItem(item, ascs->getItemPath(ITEM_PATH_NEW, { ConfigVal::A_AUTOSCAN_DIRECTORY_LOCATION }), ascs->option, ConfigVal::A_AUTOSCAN_DIRECTORY_LOCATION, definition->findConfigSetup(ConfigVal::A_AUTOSCAN_DIRECTORY_LOCATION));

                values.append(item);
            }
            {
                // scan mode (timed|inotify)
                Json::Value item;
                createItem(item, ascs->getItemPath(ITEM_PATH_NEW, { ConfigVal::A_AUTOSCAN_DIRECTORY_MODE }), ascs->option, ConfigVal::A_AUTOSCAN_DIRECTORY_MODE, definition->findConfigSetup(ConfigVal::A_AUTOSCAN_DIRECTORY_MODE));
                values.append(item);
            }
            {
                // interval for timed
                Json::Value item;
                createItem(item, ascs->getItemPath(ITEM_PATH_NEW, { ConfigVal::A_AUTOSCAN_DIRECTORY_INTERVAL }), ascs->option, ConfigVal::A_AUTOSCAN_DIRECTORY_INTERVAL, definition->findConfigSetup(ConfigVal::A_AUTOSCAN_DIRECTORY_INTERVAL));
                values.append(item);
            }
            {
                // counter for retries
                Json::Value item;
                createItem(item, ascs->getItemPath(ITEM_PATH_NEW, { ConfigVal::A_AUTOSCAN_DIRECTORY_RETRYCOUNT }), ascs->option, ConfigVal::A_AUTOSCAN_DIRECTORY_RETRYCOUNT);
                values.append(item);
            }
            {
                // recursively import files
                Json::Value item;
                createItem(item, ascs->getItemPath(ITEM_PATH_NEW, { ConfigVal::A_AUTOSCAN_DIRECTORY_RECURSIVE }), ascs->option, ConfigVal::A_AUTOSCAN_DIRECTORY_RECURSIVE, definition->findConfigSetup(ConfigVal::A_AUTOSCAN_DIRECTORY_RECURSIVE));
                values.append(item);
            }
            {
                // directory types
                Json::Value item;
                createItem(item, ascs->getItemPath(ITEM_PATH_NEW, { ConfigVal::A_AUTOSCAN_DIRECTORY_DIRTYPES }), ascs->option, ConfigVal::A_AUTOSCAN_DIRECTORY_DIRTYPES);
                values.append(item);
            }
            {
                // media types imported from directory
                Json::Value item;
                createItem(item, ascs->getItemPath(ITEM_PATH_NEW, { ConfigVal::A_AUTOSCAN_DIRECTORY_MEDIATYPE }), ascs->option, ConfigVal::A_AUTOSCAN_DIRECTORY_MEDIATYPE, definition->findConfigSetup(ConfigVal::A_AUTOSCAN_DIRECTORY_MEDIATYPE));
                values.append(item);
            }
            {
                // import hidden files
                Json::Value item;
                createItem(item, ascs->getItemPath(ITEM_PATH_NEW, { ConfigVal::A_AUTOSCAN_DIRECTORY_HIDDENFILES }), ascs->option, ConfigVal::A_AUTOSCAN_DIRECTORY_HIDDENFILES, definition->findConfigSetup(ConfigVal::A_AUTOSCAN_DIRECTORY_HIDDENFILES));
                values.append(item);
            }
            {
                // follow symbolic links
                Json::Value item;
                createItem(item, ascs->getItemPath(ITEM_PATH_NEW, { ConfigVal::A_AUTOSCAN_DIRECTORY_FOLLOWSYMLINKS }), ascs->option, ConfigVal::A_AUTOSCAN_DIRECTORY_FOLLOWSYMLINKS);
                values.append(item);
            }
            {
                // container type for audio
                Json::Value item;
                createItem(item, ascs->getItemPath(ITEM_PATH_NEW, { ConfigVal::A_AUTOSCAN_CONTAINER_TYPE_AUDIO }), ascs->option, ConfigVal::A_AUTOSCAN_CONTAINER_TYPE_AUDIO, definition->findConfigSetup(ConfigVal::A_AUTOSCAN_CONTAINER_TYPE_AUDIO));
                values.append(item);
            }
            {
                // container type for images
                Json::Value item;
                createItem(item, ascs->getItemPath(ITEM_PATH_NEW, { ConfigVal::A_AUTOSCAN_CONTAINER_TYPE_IMAGE }), ascs->option, ConfigVal::A_AUTOSCAN_CONTAINER_TYPE_IMAGE, definition->findConfigSetup(ConfigVal::A_AUTOSCAN_CONTAINER_TYPE_IMAGE));
                values.append(item);
            }
            {
                // container type for videos
                Json::Value item;
                createItem(item, ascs->getItemPath(ITEM_PATH_NEW, { ConfigVal::A_AUTOSCAN_CONTAINER_TYPE_VIDEO }), ascs->option, ConfigVal::A_AUTOSCAN_CONTAINER_TYPE_VIDEO, definition->findConfigSetup(ConfigVal::A_AUTOSCAN_CONTAINER_TYPE_VIDEO));
                values.append(item);
            }
            {
                // active scan count
                Json::Value item;
                createItem(item, ascs->getItemPath(ITEM_PATH_NEW, { ConfigVal::A_AUTOSCAN_DIRECTORY_SCANCOUNT }), ascs->option, ConfigVal::A_AUTOSCAN_DIRECTORY_SCANCOUNT, definition->findConfigSetup(ConfigVal::A_AUTOSCAN_DIRECTORY_SCANCOUNT));
                values.append(item);
            }
            {
                // active task count
                Json::Value item;
                createItem(item, ascs->getItemPath(ITEM_PATH_NEW, { ConfigVal::A_AUTOSCAN_DIRECTORY_TASKCOUNT }), ascs->option, ConfigVal::A_AUTOSCAN_DIRECTORY_TASKCOUNT, definition->findConfigSetup(ConfigVal::A_AUTOSCAN_DIRECTORY_TASKCOUNT));
                values.append(item);
            }
            {
                // Last modified
                Json::Value item;
                createItem(item, ascs->getItemPath(ITEM_PATH_NEW, { ConfigVal::A_AUTOSCAN_DIRECTORY_LMT }), ascs->option, ConfigVal::A_AUTOSCAN_DIRECTORY_LMT, definition->findConfigSetup(ConfigVal::A_AUTOSCAN_DIRECTORY_LMT));
                values.append(item);
            }
        }
    }
}

/// \brief: write content of all dictionaries
void Web::ConfigLoad::writeDictionaries(Json::Value& values)
{
    for (auto&& dcs : definition->getConfigSetupList<ConfigDictionarySetup>()) {
        std::size_t i = 0;
        auto dictionary = dcs->getValue()->getDictionaryOption(true);
        for (auto&& [key, val] : dictionary) {
            std::vector<std::size_t> indexList = { i };
            {
                Json::Value item;
                createItem(item, dcs->getItemPath(indexList, { dcs->keyOption }), dcs->option, dcs->keyOption, dcs);
                setValue(item, key.substr(5));
                values.append(item);
            }
            {
                Json::Value item;
                createItem(item, dcs->getItemPath(indexList, { dcs->valOption }), dcs->option, dcs->valOption, dcs);
                setValue(item, val);
            }
            i++;
        }
        {
            {
                Json::Value item;
                createItem(item, dcs->getItemPath(ITEM_PATH_NEW, { dcs->keyOption }), dcs->option, dcs->keyOption, dcs);
                values.append(item);
            }
            {
                Json::Value item;
                createItem(item, dcs->getItemPath(ITEM_PATH_NEW, { dcs->valOption }), dcs->option, dcs->valOption, dcs);
                values.append(item);
            }
        }
    }
}

/// \brief: write content of all vectors
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
                Json::Value item;
                std::size_t j = 0;
                auto pos = std::find(attrList.begin(), attrList.end(), key);
                if (pos != attrList.end())
                    j = std::distance(attrList.begin(), pos);
                createItem(item, vcs->getItemPath({ i }, {}, { key }), vcs->option, vcs->optionList[j], vcs);
                setValue(item, val);
                values.append(item);
            }
            i++;
        }
    }
}

/// \brief: write content of all arrays
void Web::ConfigLoad::writeArrays(Json::Value& values)
{
    for (auto&& acs : definition->getConfigSetupList<ConfigArraySetup>()) {
        auto array = acs->getValue()->getArrayOption(true);
        for (std::size_t i = 0; i < array.size(); i++) {
            auto&& entry = array[i];
            Json::Value item;
            createItem(item, acs->getItemPath({ i }, {}), acs->option, acs->attrOption != ConfigVal::MAX ? acs->attrOption : acs->nodeOption, acs);
            setValue(item, entry);
            values.append(item);
        }
        {
            Json::Value item;
            createItem(item, acs->getItemPath(ITEM_PATH_NEW, {}), acs->option, acs->attrOption != ConfigVal::MAX ? acs->attrOption : acs->nodeOption, acs);
            values.append(item);
        }
    }
}

/// \brief: update entries with datebase values
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
