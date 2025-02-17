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
#include "util/xml_to_json.h"

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

const std::string Web::ConfigLoad::PAGE = "config_load";

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

void Web::ConfigLoad::addTypeMeta(pugi::xml_node& meta, const std::shared_ptr<ConfigSetup>& cs)
{
    auto info = meta.append_child(CONFIG_LOAD_ITEM);
    info.append_attribute(CONFIG_LOAD_ITEM) = cs->getUniquePath().c_str();
    info.append_attribute(CONFIG_LOAD_ID) = fmt::format("{:03d}", cs->option).c_str();
    info.append_attribute(CONFIG_LOAD_TYPE) = cs->getTypeString().c_str();
    info.append_attribute(CONFIG_LOAD_VALUE) = cs->getDefaultValue().c_str();
    info.append_attribute(CONFIG_LOAD_HELP) = cs->getHelp();
}

void Web::ConfigLoad::createItem(pugi::xml_node& item, const std::string& name, ConfigVal id, ConfigVal aid, const std::shared_ptr<ConfigSetup>& cs)
{
    allItems[name] = fmt::format("{}[{}]", item.path(), allItems.size() + 1);
    item.append_attribute(CONFIG_LOAD_ITEM) = name.c_str();
    item.append_attribute(CONFIG_LOAD_ID) = fmt::format("{:03d}", id).c_str();
    item.append_attribute(CONFIG_LOAD_AID) = fmt::format("{:03d}", aid).c_str();
    if (std::any_of(dbEntries.begin(), dbEntries.end(), [=](auto&& s) { return s.item == name; })) {
        item.append_attribute(CONFIG_LOAD_STATUS) = CONFIG_LOAD_SOURCE_UNCHANGED;
        item.append_attribute(CONFIG_LOAD_SOURCE) = CONFIG_LOAD_SOURCE_DATABASE;
    } else {
        item.append_attribute(CONFIG_LOAD_STATUS) = !cs || !cs->isDefaultValueUsed() ? CONFIG_LOAD_SOURCE_UNCHANGED : CONFIG_LOAD_SOURCE_DEFAULT;
        item.append_attribute(CONFIG_LOAD_SOURCE) = !cs || !cs->isDefaultValueUsed() ? CONFIG_LOAD_SOURCE_CONFIG : CONFIG_LOAD_SOURCE_DEFAULT;
    }
    item.append_attribute(CONFIG_LOAD_ORIGVALUE) = config->getOrigValue(name).c_str();
    item.append_attribute(CONFIG_LOAD_DEFAULTVALUE) = cs ? cs->getDefaultValue().c_str() : "";
}

template <typename T>
void Web::ConfigLoad::setValue(pugi::xml_node& item, const T& value)
{
    static_assert(fmt::has_formatter<T, fmt::format_context>::value, "T must be formattable");
    item.append_attribute(CONFIG_LOAD_VALUE) = fmt::to_string(value).c_str();
}

template <>
void Web::ConfigLoad::setValue(pugi::xml_node& item, const std::string& value)
{
    item.append_attribute(CONFIG_LOAD_VALUE) = value.c_str();
}

template <>
void Web::ConfigLoad::setValue(pugi::xml_node& item, const std::string_view& value)
{
    item.append_attribute(CONFIG_LOAD_VALUE) = value.data();
}

template <>
void Web::ConfigLoad::setValue(pugi::xml_node& item, const fs::path& value)
{
    item.append_attribute(CONFIG_LOAD_VALUE) = value.c_str();
}

/// \brief: process config_load request
void Web::ConfigLoad::processPageAction(pugi::xml_node& element)
{
    auto values = element.append_child("values");
    std::string action = param("action");

    // set handling of json properties
    xml2Json->setArrayName(values, CONFIG_LOAD_ITEM);
    xml2Json->setFieldType(CONFIG_LOAD_ITEM, FieldType::STRING);
    xml2Json->setFieldType(CONFIG_LOAD_ID, FieldType::STRING);
    xml2Json->setFieldType(CONFIG_LOAD_AID, FieldType::STRING);
    xml2Json->setFieldType(CONFIG_LOAD_VALUE, FieldType::STRING);
    xml2Json->setFieldType(CONFIG_LOAD_DEFAULTVALUE, FieldType::STRING);
    xml2Json->setFieldType(CONFIG_LOAD_ORIGVALUE, FieldType::STRING);

    log_debug("Sending Config to web!");

    writeDatabaseStatus(values);

    if (action == "status")
        return;

    // generate meta info for ui
    auto meta = element.append_child("types");
    xml2Json->setArrayName(meta, CONFIG_LOAD_ITEM);
    for (auto&& cs : definition->getOptionList()) {
        addTypeMeta(meta, cs);
    }

    writeShortcuts(values);
    writeSimpleProperties(values);
    writeClientConfig(values);
    writeImportTweaks(values);
    writeDynamicContent(values);
    writeBoxLayout(values);
    writeTranscoding(values);
    writeAutoscan(values);
    writeDictionaries(values);
    writeVectors(values);
    writeArrays(values);
    updateEntriesFromDatabase(element, values);
}

/// \brief: write database status
void Web::ConfigLoad::writeDatabaseStatus(pugi::xml_node& values)
{
    {
        StatsParam stats(StatsParam::StatsMode::Count, "", "", false);
        auto item = values.append_child(CONFIG_LOAD_ITEM);
        createItem(item, "/status/attribute::totalCount", ConfigVal::MAX, ConfigVal::MAX);
        setValue(item, database->getFileStats(stats));
    }
    {
        StatsParam stats(StatsParam::StatsMode::Size, "", "", false);
        auto item = values.append_child(CONFIG_LOAD_ITEM);
        createItem(item, "/status/attribute::totalSize", ConfigVal::MAX, ConfigVal::MAX);
        setValue(item, CdsResource::formatSizeValue(database->getFileStats(stats)));
    }
    {
        StatsParam stats(StatsParam::StatsMode::Count, "", "", true);
        auto item = values.append_child(CONFIG_LOAD_ITEM);
        createItem(item, "/status/attribute::virtual", ConfigVal::MAX, ConfigVal::MAX);
        setValue(item, database->getFileStats(stats));
    }

    {
        StatsParam stats(StatsParam::StatsMode::Count, "audio", "", true);
        auto item = values.append_child(CONFIG_LOAD_ITEM);
        createItem(item, "/status/attribute::audioVirtual", ConfigVal::MAX, ConfigVal::MAX);
        setValue(item, database->getFileStats(stats));
    }
    {
        StatsParam stats(StatsParam::StatsMode::Count, "video", "", true);
        auto item = values.append_child(CONFIG_LOAD_ITEM);
        createItem(item, "/status/attribute::videoVirtual", ConfigVal::MAX, ConfigVal::MAX);
        setValue(item, database->getFileStats(stats));
    }
    {
        StatsParam stats(StatsParam::StatsMode::Count, "image", "", true);
        auto item = values.append_child(CONFIG_LOAD_ITEM);
        createItem(item, "/status/attribute::imageVirtual", ConfigVal::MAX, ConfigVal::MAX);
        setValue(item, database->getFileStats(stats));
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
            auto item = values.append_child(CONFIG_LOAD_ITEM);
            createItem(item, fmt::format("/status/attribute::{}Count", attr), ConfigVal::MAX, ConfigVal::MAX);
            setValue(item, cnt[cls]);
            auto item2 = values.append_child(CONFIG_LOAD_ITEM);
            createItem(item2, fmt::format("/status/attribute::{}Size", attr), ConfigVal::MAX, ConfigVal::MAX);
            setValue(item2, CdsResource::formatSizeValue(siz[cls]));
            auto item3 = values.append_child(CONFIG_LOAD_ITEM);
            createItem(item3, fmt::format("/status/attribute::{}Bytes", attr), ConfigVal::MAX, ConfigVal::MAX);
            setValue(item3, siz[cls]);
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
            auto item = values.append_child(CONFIG_LOAD_ITEM);
            createItem(item, fmt::format("/status/attribute::{}Count", attr), ConfigVal::MAX, ConfigVal::MAX);
            setValue(item, totalCnt);
            auto item2 = values.append_child(CONFIG_LOAD_ITEM);
            createItem(item2, fmt::format("/status/attribute::{}Size", attr), ConfigVal::MAX, ConfigVal::MAX);
            setValue(item2, CdsResource::formatSizeValue(totalSize));
            auto item3 = values.append_child(CONFIG_LOAD_ITEM);
            createItem(item3, fmt::format("/status/attribute::{}Bytes", attr), ConfigVal::MAX, ConfigVal::MAX);
            setValue(item3, totalSize);
        }
    }
}

/// \brief: write upnp shortcuts
void Web::ConfigLoad::writeShortcuts(pugi::xml_node& values)
{
    auto shortcuts = database->getShortcuts();
    std::size_t idx = 0;
    for (auto&& [shortcut, cont] : shortcuts) {
        if (!shortcut.empty()) {
            log_debug("shortcut {}={}", shortcut, cont->getID());
            auto item = values.append_child(CONFIG_LOAD_ITEM);
            createItem(item, fmt::format("/status/shortcut[{}]/attribute::name", idx), ConfigVal::MAX, ConfigVal::MAX);
            setValue(item, shortcut);
            item = values.append_child(CONFIG_LOAD_ITEM);
            createItem(item, fmt::format("/status/shortcut[{}]/attribute::id", idx), ConfigVal::MAX, ConfigVal::MAX);
            setValue(item, cont->getID());
            item = values.append_child(CONFIG_LOAD_ITEM);
            createItem(item, fmt::format("/status/shortcut[{}]/attribute::location", idx), ConfigVal::MAX, ConfigVal::MAX);
            setValue(item, cont->getLocation());
            idx++;
        }
    }
}

/// \brief: write all values with simple type (string, int, bool)
void Web::ConfigLoad::writeSimpleProperties(pugi::xml_node& values)
{
    for (auto&& option : ConfigOptionIterator()) {
        try {
            auto scs = definition->findConfigSetup(option);
            auto item = values.append_child(CONFIG_LOAD_ITEM);
            createItem(item, scs->getItemPathRoot(), option, option, scs);

            log_debug("    Option {:03d} {} = {}", option, scs->getItemPath({ 0 }, {}), scs->getCurrentValue().c_str());
            setValue(item, scs->getCurrentValue());
        } catch (const std::runtime_error& e) {
            log_warning("Option {:03d} {}", option, e.what());
        }
    }
}

/// \brief: write client configuration
void Web::ConfigLoad::writeClientConfig(pugi::xml_node& values)
{
    auto cs = definition->findConfigSetup(ConfigVal::CLIENTS_LIST);
    auto clientConfig = cs->getValue()->getClientConfigListOption();
    for (std::size_t i = 0; i < EDIT_CAST(EditHelperClientConfig, clientConfig)->size(); i++) {
        auto client = EDIT_CAST(EditHelperClientConfig, clientConfig)->get(i);
        std::vector<std::size_t> indexList = { i };

        auto item = values.append_child(CONFIG_LOAD_ITEM);
        createItem(item, cs->getItemPath(indexList, { ConfigVal::A_CLIENTS_CLIENT, ConfigVal::A_CLIENTS_CLIENT_FLAGS }), cs->option, ConfigVal::A_CLIENTS_CLIENT_FLAGS, cs);
        setValue(item, ClientConfig::mapFlags(client->getFlags()));

        item = values.append_child(CONFIG_LOAD_ITEM);
        createItem(item, cs->getItemPath(indexList, { ConfigVal::A_CLIENTS_CLIENT, ConfigVal::A_CLIENTS_CLIENT_IP }), cs->option, ConfigVal::A_CLIENTS_CLIENT_IP, cs);
        setValue(item, client->getIp());

        item = values.append_child(CONFIG_LOAD_ITEM);
        createItem(item, cs->getItemPath(indexList, { ConfigVal::A_CLIENTS_CLIENT, ConfigVal::A_CLIENTS_CLIENT_USERAGENT }), cs->option, ConfigVal::A_CLIENTS_CLIENT_USERAGENT, cs);
        setValue(item, client->getUserAgent());

        item = values.append_child(CONFIG_LOAD_ITEM);
        createItem(item, cs->getItemPath(indexList, { ConfigVal::A_CLIENTS_CLIENT, ConfigVal::A_CLIENTS_CLIENT_GROUP }), cs->option, ConfigVal::A_CLIENTS_CLIENT_GROUP, cs);
        setValue(item, client->getGroup());

        item = values.append_child(CONFIG_LOAD_ITEM);
        createItem(item, cs->getItemPath(indexList, { ConfigVal::A_CLIENTS_CLIENT, ConfigVal::A_CLIENTS_CLIENT_ALLOWED }), cs->option, ConfigVal::A_CLIENTS_CLIENT_ALLOWED, cs);
        setValue(item, client->getAllowed());

        item = values.append_child(CONFIG_LOAD_ITEM);
        createItem(item, cs->getItemPath(indexList, { ConfigVal::A_CLIENTS_CLIENT, ConfigVal::A_CLIENTS_UPNP_CAPTION_COUNT }), cs->option, ConfigVal::A_CLIENTS_UPNP_CAPTION_COUNT, cs);
        setValue(item, client->getCaptionInfoCount());

        item = values.append_child(CONFIG_LOAD_ITEM);
        createItem(item, cs->getItemPath(indexList, { ConfigVal::A_CLIENTS_CLIENT, ConfigVal::A_CLIENTS_UPNP_STRING_LIMIT }), cs->option, ConfigVal::A_CLIENTS_UPNP_STRING_LIMIT, cs);
        setValue(item, client->getStringLimit());

        item = values.append_child(CONFIG_LOAD_ITEM);
        createItem(item, cs->getItemPath(indexList, { ConfigVal::A_CLIENTS_CLIENT, ConfigVal::A_CLIENTS_UPNP_MULTI_VALUE }), cs->option, ConfigVal::A_CLIENTS_UPNP_MULTI_VALUE, cs);
        setValue(item, client->getMultiValue());

        {
            // Sub Dictionary ConfigVal::A_CLIENTS_UPNP_MAP_MIMETYPE
            std::size_t j = 0;
            for (auto&& [from, to] : client->getMimeMappings(true)) {
                item = values.append_child(CONFIG_LOAD_ITEM);
                createItem(item, cs->getItemPath({ i, j }, { ConfigVal::A_CLIENTS_CLIENT, ConfigVal::A_IMPORT_MAPPINGS_MIMETYPE_MAP, ConfigVal::A_IMPORT_MAPPINGS_MIMETYPE_FROM }), cs->option, ConfigVal::A_IMPORT_MAPPINGS_MIMETYPE_FROM, cs);
                setValue(item, from);
                item = values.append_child(CONFIG_LOAD_ITEM);
                createItem(item, cs->getItemPath({ i, j }, { ConfigVal::A_CLIENTS_CLIENT, ConfigVal::A_IMPORT_MAPPINGS_MIMETYPE_MAP, ConfigVal::A_IMPORT_MAPPINGS_MIMETYPE_TO }), cs->option, ConfigVal::A_IMPORT_MAPPINGS_MIMETYPE_TO, cs);
                setValue(item, to);
            }
        }
        {
            // Sub Dictionary ConfigVal::A_CLIENTS_UPNP_HEADERS
            std::size_t j = 0;
            for (auto&& [key, value] : client->getHeaders(true)) {
                item = values.append_child(CONFIG_LOAD_ITEM);
                createItem(item, cs->getItemPath({ i, j }, { ConfigVal::A_CLIENTS_CLIENT, ConfigVal::A_CLIENTS_UPNP_HEADERS_HEADER, ConfigVal::A_CLIENTS_UPNP_HEADERS_KEY }), cs->option, ConfigVal::A_CLIENTS_UPNP_HEADERS_KEY, cs);
                setValue(item, key);
                item = values.append_child(CONFIG_LOAD_ITEM);
                createItem(item, cs->getItemPath({ i, j }, { ConfigVal::A_CLIENTS_CLIENT, ConfigVal::A_CLIENTS_UPNP_HEADERS_HEADER, ConfigVal::A_CLIENTS_UPNP_HEADERS_VALUE }), cs->option, ConfigVal::A_CLIENTS_UPNP_HEADERS_VALUE, cs);
                setValue(item, value);
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
                    item = values.append_child(CONFIG_LOAD_ITEM);
                    std::size_t k = 0;
                    auto pos = std::find(attrList.begin(), attrList.end(), key);
                    if (pos != attrList.end())
                        k = std::distance(attrList.begin(), pos);
                    createItem(item, cs->getItemPath({ i, j }, { ConfigVal::A_CLIENTS_CLIENT, ConfigVal::A_CLIENTS_UPNP_MAP_DLNAPROFILE_PROFILE }, key), cs->option, vcs->optionList[k], cs);
                    setValue(item, val);
                }
                j++;
            }
        }
    }
    // Allow creation of entry in blank config
    {
        auto item = values.append_child(CONFIG_LOAD_ITEM);
        createItem(item, cs->getItemPath(ITEM_PATH_NEW, { ConfigVal::A_CLIENTS_CLIENT, ConfigVal::A_CLIENTS_CLIENT_FLAGS }), cs->option, ConfigVal::A_CLIENTS_CLIENT_FLAGS, definition->findConfigSetup(ConfigVal::A_CLIENTS_CLIENT_FLAGS));

        item = values.append_child(CONFIG_LOAD_ITEM);
        createItem(item, cs->getItemPath(ITEM_PATH_NEW, { ConfigVal::A_CLIENTS_CLIENT, ConfigVal::A_CLIENTS_CLIENT_IP }), cs->option, ConfigVal::A_CLIENTS_CLIENT_IP, definition->findConfigSetup(ConfigVal::A_CLIENTS_CLIENT_IP));

        item = values.append_child(CONFIG_LOAD_ITEM);
        createItem(item, cs->getItemPath(ITEM_PATH_NEW, { ConfigVal::A_CLIENTS_CLIENT, ConfigVal::A_CLIENTS_CLIENT_USERAGENT }), cs->option, ConfigVal::A_CLIENTS_CLIENT_USERAGENT, definition->findConfigSetup(ConfigVal::A_CLIENTS_CLIENT_USERAGENT));

        item = values.append_child(CONFIG_LOAD_ITEM);
        createItem(item, cs->getItemPath(ITEM_PATH_NEW, { ConfigVal::A_CLIENTS_CLIENT, ConfigVal::A_CLIENTS_CLIENT_GROUP }), cs->option, ConfigVal::A_CLIENTS_CLIENT_GROUP, definition->findConfigSetup(ConfigVal::A_CLIENTS_CLIENT_GROUP));

        item = values.append_child(CONFIG_LOAD_ITEM);
        createItem(item, cs->getItemPath(ITEM_PATH_NEW, { ConfigVal::A_CLIENTS_CLIENT, ConfigVal::A_CLIENTS_UPNP_CAPTION_COUNT }), cs->option, ConfigVal::A_CLIENTS_UPNP_CAPTION_COUNT, definition->findConfigSetup(ConfigVal::A_CLIENTS_UPNP_CAPTION_COUNT));

        item = values.append_child(CONFIG_LOAD_ITEM);
        createItem(item, cs->getItemPath(ITEM_PATH_NEW, { ConfigVal::A_CLIENTS_CLIENT, ConfigVal::A_CLIENTS_UPNP_STRING_LIMIT }), cs->option, ConfigVal::A_CLIENTS_UPNP_STRING_LIMIT, definition->findConfigSetup(ConfigVal::A_CLIENTS_UPNP_STRING_LIMIT));

        item = values.append_child(CONFIG_LOAD_ITEM);
        createItem(item, cs->getItemPath(ITEM_PATH_NEW, { ConfigVal::A_CLIENTS_CLIENT, ConfigVal::A_CLIENTS_UPNP_MULTI_VALUE }), cs->option, ConfigVal::A_CLIENTS_UPNP_MULTI_VALUE, definition->findConfigSetup(ConfigVal::A_CLIENTS_UPNP_MULTI_VALUE));

        item = values.append_child(CONFIG_LOAD_ITEM);
        createItem(item, cs->getItemPath(ITEM_PATH_NEW, { ConfigVal::A_CLIENTS_CLIENT, ConfigVal::A_IMPORT_MAPPINGS_MIMETYPE_MAP, ConfigVal::A_IMPORT_MAPPINGS_MIMETYPE_FROM }), cs->option, ConfigVal::A_IMPORT_MAPPINGS_MIMETYPE_FROM, cs);

        item = values.append_child(CONFIG_LOAD_ITEM);
        createItem(item, cs->getItemPath(ITEM_PATH_NEW, { ConfigVal::A_CLIENTS_CLIENT, ConfigVal::A_IMPORT_MAPPINGS_MIMETYPE_MAP, ConfigVal::A_IMPORT_MAPPINGS_MIMETYPE_TO }), cs->option, ConfigVal::A_IMPORT_MAPPINGS_MIMETYPE_TO, cs);

        item = values.append_child(CONFIG_LOAD_ITEM);
        createItem(item, cs->getItemPath(ITEM_PATH_NEW, { ConfigVal::A_CLIENTS_CLIENT, ConfigVal::A_CLIENTS_UPNP_HEADERS_HEADER, ConfigVal::A_CLIENTS_UPNP_HEADERS_KEY }), cs->option, ConfigVal::A_CLIENTS_UPNP_HEADERS_KEY, cs);

        item = values.append_child(CONFIG_LOAD_ITEM);
        createItem(item, cs->getItemPath(ITEM_PATH_NEW, { ConfigVal::A_CLIENTS_CLIENT, ConfigVal::A_CLIENTS_UPNP_HEADERS_HEADER, ConfigVal::A_CLIENTS_UPNP_HEADERS_VALUE }), cs->option, ConfigVal::A_CLIENTS_UPNP_HEADERS_VALUE, cs);

        item = values.append_child(CONFIG_LOAD_ITEM);
        createItem(item, cs->getItemPath(ITEM_PATH_NEW, { ConfigVal::A_CLIENTS_CLIENT, ConfigVal::A_CLIENTS_UPNP_MAP_DLNAPROFILE_PROFILE, ConfigVal::A_IMPORT_MAPPINGS_MIMETYPE_FROM }), cs->option, ConfigVal::A_IMPORT_MAPPINGS_MIMETYPE_FROM, cs);

        item = values.append_child(CONFIG_LOAD_ITEM);
        createItem(item, cs->getItemPath(ITEM_PATH_NEW, { ConfigVal::A_CLIENTS_CLIENT, ConfigVal::A_CLIENTS_UPNP_MAP_DLNAPROFILE_PROFILE, ConfigVal::A_IMPORT_MAPPINGS_MIMETYPE_TO }), cs->option, ConfigVal::A_IMPORT_MAPPINGS_MIMETYPE_TO, cs);
        // special treatment for vector
        auto vcs = definition->findConfigSetup<ConfigVectorSetup>(ConfigVal::A_CLIENTS_UPNP_MAP_DLNAPROFILE);
        std::vector<std::string> attrList;
        attrList.reserve(vcs->optionList.size());
        for (const auto& opt : vcs->optionList) {
            attrList.push_back(definition->removeAttribute(opt));
        }
        for (auto&& key : { "audioCodec", "videoCodec" }) {
            item = values.append_child(CONFIG_LOAD_ITEM);
            std::size_t k = 0;
            auto pos = std::find(attrList.begin(), attrList.end(), key);
            if (pos != attrList.end())
                k = std::distance(attrList.begin(), pos);
            createItem(item, cs->getItemPath(ITEM_PATH_NEW, { ConfigVal::A_CLIENTS_CLIENT, ConfigVal::A_CLIENTS_UPNP_MAP_DLNAPROFILE_PROFILE }, key), cs->option, vcs->optionList[k], cs);
        }
    }
    for (std::size_t i = 0; i < EDIT_CAST(EditHelperClientGroupConfig, clientConfig)->size(); i++) {
        auto group = EDIT_CAST(EditHelperClientGroupConfig, clientConfig)->get(i);
        std::vector<std::size_t> indexList = { i };

        auto item = values.append_child(CONFIG_LOAD_ITEM);
        createItem(item, cs->getItemPath(indexList, { ConfigVal::A_CLIENTS_GROUP, ConfigVal::A_CLIENTS_GROUP_NAME }), cs->option, ConfigVal::A_CLIENTS_GROUP_NAME, cs);
        setValue(item, group->getGroupName());

        auto forbiddenDirs = group->getForbiddenDirectories();
        for (std::size_t j = 0; j < forbiddenDirs.size(); j++) {
            std::vector<std::size_t> subIndexList = { i, j };
            item = values.append_child(CONFIG_LOAD_ITEM);
            createItem(item, cs->getItemPath(subIndexList, { ConfigVal::A_CLIENTS_GROUP, ConfigVal::A_CLIENTS_GROUP_HIDE, ConfigVal::A_CLIENTS_GROUP_LOCATION }), cs->option, ConfigVal::A_CLIENTS_GROUP_LOCATION, cs);
            setValue(item, forbiddenDirs.at(j));
        }
    }
    // Allow creation of entry in blank config
    {
        auto item = values.append_child(CONFIG_LOAD_ITEM);
        createItem(item, cs->getItemPath(ITEM_PATH_NEW, { ConfigVal::A_CLIENTS_GROUP, ConfigVal::A_CLIENTS_GROUP_NAME }), cs->option, ConfigVal::A_CLIENTS_GROUP_NAME, definition->findConfigSetup(ConfigVal::A_CLIENTS_GROUP_NAME));

        item = values.append_child(CONFIG_LOAD_ITEM);
        createItem(item, cs->getItemPath(ITEM_PATH_NEW, { ConfigVal::A_CLIENTS_GROUP, ConfigVal::A_CLIENTS_GROUP_LOCATION }), cs->option, ConfigVal::A_CLIENTS_GROUP_LOCATION, definition->findConfigSetup(ConfigVal::A_CLIENTS_GROUP_LOCATION));
    }
}

/// \brief: write import tweaks
void Web::ConfigLoad::writeImportTweaks(pugi::xml_node& values)
{
    auto cs = definition->findConfigSetup(ConfigVal::IMPORT_DIRECTORIES_LIST);
    auto directoryConfig = cs->getValue()->getDirectoryTweakOption();
    for (std::size_t i = 0; i < directoryConfig->size(); i++) {
        auto dir = directoryConfig->get(i);
        std::vector<std::size_t> indexList = { i };

        auto item = values.append_child(CONFIG_LOAD_ITEM);
        createItem(item, cs->getItemPath(indexList, { ConfigVal::A_DIRECTORIES_TWEAK_LOCATION }), cs->option, ConfigVal::A_DIRECTORIES_TWEAK_LOCATION);
        setValue(item, dir->getLocation());

        item = values.append_child(CONFIG_LOAD_ITEM);
        createItem(item, cs->getItemPath(indexList, { ConfigVal::A_DIRECTORIES_TWEAK_INHERIT }), cs->option, ConfigVal::A_DIRECTORIES_TWEAK_INHERIT);
        setValue(item, dir->getInherit());

        item = values.append_child(CONFIG_LOAD_ITEM);
        createItem(item, cs->getItemPath(indexList, { ConfigVal::A_DIRECTORIES_TWEAK_RECURSIVE }), cs->option, ConfigVal::A_DIRECTORIES_TWEAK_RECURSIVE);
        setValue(item, dir->getRecursive());

        item = values.append_child(CONFIG_LOAD_ITEM);
        createItem(item, cs->getItemPath(indexList, { ConfigVal::A_DIRECTORIES_TWEAK_HIDDEN }), cs->option, ConfigVal::A_DIRECTORIES_TWEAK_HIDDEN);
        setValue(item, dir->getHidden());

        item = values.append_child(CONFIG_LOAD_ITEM);
        createItem(item, cs->getItemPath(indexList, { ConfigVal::A_DIRECTORIES_TWEAK_CASE_SENSITIVE }), cs->option, ConfigVal::A_DIRECTORIES_TWEAK_CASE_SENSITIVE);
        setValue(item, dir->getCaseSensitive());

        item = values.append_child(CONFIG_LOAD_ITEM);
        createItem(item, cs->getItemPath(indexList, { ConfigVal::A_DIRECTORIES_TWEAK_FOLLOW_SYMLINKS }), cs->option, ConfigVal::A_DIRECTORIES_TWEAK_FOLLOW_SYMLINKS);
        setValue(item, dir->getFollowSymlinks());

        item = values.append_child(CONFIG_LOAD_ITEM);
        createItem(item, cs->getItemPath(indexList, { ConfigVal::A_DIRECTORIES_TWEAK_META_CHARSET }), cs->option, ConfigVal::A_DIRECTORIES_TWEAK_META_CHARSET);
        setValue(item, dir->hasMetaCharset() ? dir->getMetaCharset() : "");

        item = values.append_child(CONFIG_LOAD_ITEM);
        createItem(item, cs->getItemPath(indexList, { ConfigVal::A_DIRECTORIES_TWEAK_FANART_FILE }), cs->option, ConfigVal::A_DIRECTORIES_TWEAK_FANART_FILE);
        setValue(item, dir->hasFanArtFile() ? dir->getFanArtFile() : "");

        item = values.append_child(CONFIG_LOAD_ITEM);
        createItem(item, cs->getItemPath(indexList, { ConfigVal::A_DIRECTORIES_TWEAK_RESOURCE_FILE }), cs->option, ConfigVal::A_DIRECTORIES_TWEAK_RESOURCE_FILE);
        setValue(item, dir->hasResourceFile() ? dir->getResourceFile() : "");

        item = values.append_child(CONFIG_LOAD_ITEM);
        createItem(item, cs->getItemPath(indexList, { ConfigVal::A_DIRECTORIES_TWEAK_SUBTITLE_FILE }), cs->option, ConfigVal::A_DIRECTORIES_TWEAK_SUBTITLE_FILE);
        setValue(item, dir->hasSubTitleFile() ? dir->getSubTitleFile() : "");

        item = values.append_child(CONFIG_LOAD_ITEM);
        createItem(item, cs->getItemPath(indexList, { ConfigVal::A_DIRECTORIES_TWEAK_METAFILE_FILE }), cs->option, ConfigVal::A_DIRECTORIES_TWEAK_METAFILE_FILE);
        setValue(item, dir->hasMetafile() ? dir->getMetafile() : "");
    }
    // Allow creation of entry in blank config
    {
        auto item = values.append_child(CONFIG_LOAD_ITEM);
        createItem(item, cs->getItemPath(ITEM_PATH_NEW, { ConfigVal::A_DIRECTORIES_TWEAK_LOCATION }), cs->option, ConfigVal::A_DIRECTORIES_TWEAK_LOCATION);

        item = values.append_child(CONFIG_LOAD_ITEM);
        createItem(item, cs->getItemPath(ITEM_PATH_NEW, { ConfigVal::A_DIRECTORIES_TWEAK_INHERIT }), cs->option, ConfigVal::A_DIRECTORIES_TWEAK_INHERIT);

        item = values.append_child(CONFIG_LOAD_ITEM);
        createItem(item, cs->getItemPath(ITEM_PATH_NEW, { ConfigVal::A_DIRECTORIES_TWEAK_RECURSIVE }), cs->option, ConfigVal::A_DIRECTORIES_TWEAK_RECURSIVE);

        item = values.append_child(CONFIG_LOAD_ITEM);
        createItem(item, cs->getItemPath(ITEM_PATH_NEW, { ConfigVal::A_DIRECTORIES_TWEAK_HIDDEN }), cs->option, ConfigVal::A_DIRECTORIES_TWEAK_HIDDEN);

        item = values.append_child(CONFIG_LOAD_ITEM);
        createItem(item, cs->getItemPath(ITEM_PATH_NEW, { ConfigVal::A_DIRECTORIES_TWEAK_CASE_SENSITIVE }), cs->option, ConfigVal::A_DIRECTORIES_TWEAK_CASE_SENSITIVE);

        item = values.append_child(CONFIG_LOAD_ITEM);
        createItem(item, cs->getItemPath(ITEM_PATH_NEW, { ConfigVal::A_DIRECTORIES_TWEAK_FOLLOW_SYMLINKS }), cs->option, ConfigVal::A_DIRECTORIES_TWEAK_FOLLOW_SYMLINKS);

        item = values.append_child(CONFIG_LOAD_ITEM);
        createItem(item, cs->getItemPath(ITEM_PATH_NEW, { ConfigVal::A_DIRECTORIES_TWEAK_META_CHARSET }), cs->option, ConfigVal::A_DIRECTORIES_TWEAK_META_CHARSET);

        item = values.append_child(CONFIG_LOAD_ITEM);
        createItem(item, cs->getItemPath(ITEM_PATH_NEW, { ConfigVal::A_DIRECTORIES_TWEAK_FANART_FILE }), cs->option, ConfigVal::A_DIRECTORIES_TWEAK_FANART_FILE);

        item = values.append_child(CONFIG_LOAD_ITEM);
        createItem(item, cs->getItemPath(ITEM_PATH_NEW, { ConfigVal::A_DIRECTORIES_TWEAK_RESOURCE_FILE }), cs->option, ConfigVal::A_DIRECTORIES_TWEAK_RESOURCE_FILE);

        item = values.append_child(CONFIG_LOAD_ITEM);
        createItem(item, cs->getItemPath(ITEM_PATH_NEW, { ConfigVal::A_DIRECTORIES_TWEAK_SUBTITLE_FILE }), cs->option, ConfigVal::A_DIRECTORIES_TWEAK_SUBTITLE_FILE);

        item = values.append_child(CONFIG_LOAD_ITEM);
        createItem(item, cs->getItemPath(ITEM_PATH_NEW, { ConfigVal::A_DIRECTORIES_TWEAK_METAFILE_FILE }), cs->option, ConfigVal::A_DIRECTORIES_TWEAK_METAFILE_FILE);
    }
}

/// \brief: write dynamic content
void Web::ConfigLoad::writeDynamicContent(pugi::xml_node& values)
{
    auto cs = definition->findConfigSetup(ConfigVal::SERVER_DYNAMIC_CONTENT_LIST);
    auto dynContent = cs->getValue()->getDynamicContentListOption();
    for (std::size_t i = 0; i < dynContent->size(); i++) {
        auto cont = dynContent->get(i);
        std::vector<std::size_t> indexList = { i };

        auto item = values.append_child(CONFIG_LOAD_ITEM);
        createItem(item, cs->getItemPath(indexList, { ConfigVal::A_DYNAMIC_CONTAINER_LOCATION }), cs->option, ConfigVal::A_DYNAMIC_CONTAINER_LOCATION);
        setValue(item, cont->getLocation());

        item = values.append_child(CONFIG_LOAD_ITEM);
        createItem(item, cs->getItemPath(indexList, { ConfigVal::A_DYNAMIC_CONTAINER_IMAGE }), cs->option, ConfigVal::A_DYNAMIC_CONTAINER_IMAGE);
        setValue(item, cont->getImage());

        item = values.append_child(CONFIG_LOAD_ITEM);
        createItem(item, cs->getItemPath(indexList, { ConfigVal::A_DYNAMIC_CONTAINER_TITLE }), cs->option, ConfigVal::A_DYNAMIC_CONTAINER_TITLE);
        setValue(item, cont->getTitle());

        item = values.append_child(CONFIG_LOAD_ITEM);
        createItem(item, cs->getItemPath(indexList, { ConfigVal::A_DYNAMIC_CONTAINER_FILTER }), cs->option, ConfigVal::A_DYNAMIC_CONTAINER_FILTER);
        setValue(item, cont->getFilter());

        item = values.append_child(CONFIG_LOAD_ITEM);
        createItem(item, cs->getItemPath(indexList, { ConfigVal::A_DYNAMIC_CONTAINER_SORT }), cs->option, ConfigVal::A_DYNAMIC_CONTAINER_SORT);
        setValue(item, cont->getSort());

        item = values.append_child(CONFIG_LOAD_ITEM);
        createItem(item, cs->getItemPath(indexList, { ConfigVal::A_DYNAMIC_CONTAINER_MAXCOUNT }), cs->option, ConfigVal::A_DYNAMIC_CONTAINER_MAXCOUNT);
        setValue(item, cont->getMaxCount());

        item = values.append_child(CONFIG_LOAD_ITEM);
        createItem(item, cs->getItemPath(indexList, { ConfigVal::A_DYNAMIC_CONTAINER_UPNP_SHORTCUT }), cs->option, ConfigVal::A_DYNAMIC_CONTAINER_UPNP_SHORTCUT);
        setValue(item, cont->getUpnpShortcut());
    }
    // Allow creation of entry in blank config
    {
        auto item = values.append_child(CONFIG_LOAD_ITEM);
        createItem(item, cs->getItemPath(ITEM_PATH_NEW, { ConfigVal::A_DYNAMIC_CONTAINER_LOCATION }), cs->option, ConfigVal::A_DYNAMIC_CONTAINER_LOCATION, definition->findConfigSetup(ConfigVal::A_DYNAMIC_CONTAINER_LOCATION));

        item = values.append_child(CONFIG_LOAD_ITEM);
        createItem(item, cs->getItemPath(ITEM_PATH_NEW, { ConfigVal::A_DYNAMIC_CONTAINER_IMAGE }), cs->option, ConfigVal::A_DYNAMIC_CONTAINER_IMAGE, definition->findConfigSetup(ConfigVal::A_DYNAMIC_CONTAINER_IMAGE));

        item = values.append_child(CONFIG_LOAD_ITEM);
        createItem(item, cs->getItemPath(ITEM_PATH_NEW, { ConfigVal::A_DYNAMIC_CONTAINER_TITLE }), cs->option, ConfigVal::A_DYNAMIC_CONTAINER_TITLE, definition->findConfigSetup(ConfigVal::A_DYNAMIC_CONTAINER_TITLE));

        item = values.append_child(CONFIG_LOAD_ITEM);
        createItem(item, cs->getItemPath(ITEM_PATH_NEW, { ConfigVal::A_DYNAMIC_CONTAINER_FILTER }), cs->option, ConfigVal::A_DYNAMIC_CONTAINER_FILTER, definition->findConfigSetup(ConfigVal::A_DYNAMIC_CONTAINER_FILTER));

        item = values.append_child(CONFIG_LOAD_ITEM);
        createItem(item, cs->getItemPath(ITEM_PATH_NEW, { ConfigVal::A_DYNAMIC_CONTAINER_SORT }), cs->option, ConfigVal::A_DYNAMIC_CONTAINER_SORT, definition->findConfigSetup(ConfigVal::A_DYNAMIC_CONTAINER_SORT));

        item = values.append_child(CONFIG_LOAD_ITEM);
        createItem(item, cs->getItemPath(ITEM_PATH_NEW, { ConfigVal::A_DYNAMIC_CONTAINER_MAXCOUNT }), cs->option, ConfigVal::A_DYNAMIC_CONTAINER_MAXCOUNT, definition->findConfigSetup(ConfigVal::A_DYNAMIC_CONTAINER_MAXCOUNT));

        item = values.append_child(CONFIG_LOAD_ITEM);
        createItem(item, cs->getItemPath(ITEM_PATH_NEW, { ConfigVal::A_DYNAMIC_CONTAINER_UPNP_SHORTCUT }), cs->option, ConfigVal::A_DYNAMIC_CONTAINER_UPNP_SHORTCUT, definition->findConfigSetup(ConfigVal::A_DYNAMIC_CONTAINER_UPNP_SHORTCUT));
    }
}

/// \brief: write box layout
void Web::ConfigLoad::writeBoxLayout(pugi::xml_node& values)
{
    auto cs = definition->findConfigSetup(ConfigVal::BOXLAYOUT_BOX);
    auto boxlayoutContent = cs->getValue()->getBoxLayoutListOption();
    for (std::size_t i = 0; i < boxlayoutContent->size(); i++) {
        auto cont = boxlayoutContent->get(i);
        std::vector<std::size_t> indexList = { i };

        auto item = values.append_child(CONFIG_LOAD_ITEM);
        createItem(item, cs->getItemPath(indexList, { ConfigVal::A_BOXLAYOUT_BOX_KEY }), cs->option, ConfigVal::A_BOXLAYOUT_BOX_KEY);
        setValue(item, cont->getKey());

        item = values.append_child(CONFIG_LOAD_ITEM);
        createItem(item, cs->getItemPath(indexList, { ConfigVal::A_BOXLAYOUT_BOX_TITLE }), cs->option, ConfigVal::A_BOXLAYOUT_BOX_TITLE);
        setValue(item, cont->getTitle());

        item = values.append_child(CONFIG_LOAD_ITEM);
        createItem(item, cs->getItemPath(indexList, { ConfigVal::A_BOXLAYOUT_BOX_CLASS }), cs->option, ConfigVal::A_BOXLAYOUT_BOX_CLASS);
        setValue(item, cont->getClass());

        item = values.append_child(CONFIG_LOAD_ITEM);
        createItem(item, cs->getItemPath(indexList, { ConfigVal::A_BOXLAYOUT_BOX_SIZE }), cs->option, ConfigVal::A_BOXLAYOUT_BOX_SIZE);
        setValue(item, cont->getSize());

        item = values.append_child(CONFIG_LOAD_ITEM);
        createItem(item, cs->getItemPath(indexList, { ConfigVal::A_BOXLAYOUT_BOX_ENABLED }), cs->option, ConfigVal::A_BOXLAYOUT_BOX_ENABLED);
        setValue(item, cont->getEnabled());

        item = values.append_child(CONFIG_LOAD_ITEM);
        createItem(item, cs->getItemPath(indexList, { ConfigVal::A_BOXLAYOUT_BOX_UPNP_SHORTCUT }), cs->option, ConfigVal::A_BOXLAYOUT_BOX_UPNP_SHORTCUT);
        setValue(item, cont->getUpnpShortcut());
    }
    // Allow creation of entry in blank config
    {
        auto item = values.append_child(CONFIG_LOAD_ITEM);
        createItem(item, cs->getItemPath(ITEM_PATH_NEW, { ConfigVal::A_BOXLAYOUT_BOX_KEY }), cs->option, ConfigVal::A_BOXLAYOUT_BOX_KEY, definition->findConfigSetup(ConfigVal::A_BOXLAYOUT_BOX_KEY));

        item = values.append_child(CONFIG_LOAD_ITEM);
        createItem(item, cs->getItemPath(ITEM_PATH_NEW, { ConfigVal::A_BOXLAYOUT_BOX_TITLE }), cs->option, ConfigVal::A_BOXLAYOUT_BOX_TITLE, definition->findConfigSetup(ConfigVal::A_BOXLAYOUT_BOX_TITLE));

        item = values.append_child(CONFIG_LOAD_ITEM);
        createItem(item, cs->getItemPath(ITEM_PATH_NEW, { ConfigVal::A_BOXLAYOUT_BOX_CLASS }), cs->option, ConfigVal::A_BOXLAYOUT_BOX_CLASS, definition->findConfigSetup(ConfigVal::A_BOXLAYOUT_BOX_CLASS));

        item = values.append_child(CONFIG_LOAD_ITEM);
        createItem(item, cs->getItemPath(ITEM_PATH_NEW, { ConfigVal::A_BOXLAYOUT_BOX_SIZE }), cs->option, ConfigVal::A_BOXLAYOUT_BOX_SIZE, definition->findConfigSetup(ConfigVal::A_BOXLAYOUT_BOX_SIZE));

        item = values.append_child(CONFIG_LOAD_ITEM);
        createItem(item, cs->getItemPath(ITEM_PATH_NEW, { ConfigVal::A_BOXLAYOUT_BOX_ENABLED }), cs->option, ConfigVal::A_BOXLAYOUT_BOX_ENABLED, definition->findConfigSetup(ConfigVal::A_BOXLAYOUT_BOX_ENABLED));

        item = values.append_child(CONFIG_LOAD_ITEM);
        createItem(item, cs->getItemPath(ITEM_PATH_NEW, { ConfigVal::A_BOXLAYOUT_BOX_UPNP_SHORTCUT }), cs->option, ConfigVal::A_BOXLAYOUT_BOX_UPNP_SHORTCUT, definition->findConfigSetup(ConfigVal::A_BOXLAYOUT_BOX_UPNP_SHORTCUT));
    }
}

/// \brief: write transconding configuration
void Web::ConfigLoad::writeTranscoding(pugi::xml_node& values)
{
    auto cs = definition->findConfigSetup(ConfigVal::TRANSCODING_PROFILE_LIST);
    auto transcoding = cs->getValue()->getTranscodingProfileListOption();
    std::size_t pr = 0;
    std::map<std::string, std::shared_ptr<TranscodingProfile>> profiles;
    // write filter list and collect required profiles
    for (auto&& filter : transcoding->getFilterList()) {
        std::vector<std::size_t> indexList = { pr };
        auto item = values.append_child(CONFIG_LOAD_ITEM);
        createItem(item, cs->getItemPath(indexList, { ConfigVal::A_TRANSCODING_MIMETYPE_FILTER, ConfigVal::A_TRANSCODING_MIMETYPE_PROF_MAP_MIMETYPE }), cs->option, ConfigVal::A_TRANSCODING_MIMETYPE_PROF_MAP_MIMETYPE, cs);
        setValue(item, filter->getMimeType());

        item = values.append_child(CONFIG_LOAD_ITEM);
        createItem(item, cs->getItemPath(indexList, { ConfigVal::A_TRANSCODING_MIMETYPE_FILTER, ConfigVal::A_TRANSCODING_MIMETYPE_PROF_MAP_USING }), cs->option, ConfigVal::A_TRANSCODING_MIMETYPE_PROF_MAP_USING, cs);
        setValue(item, filter->getTranscoderName());

        item = values.append_child(CONFIG_LOAD_ITEM);
        createItem(item, cs->getItemPath(indexList, { ConfigVal::A_TRANSCODING_MIMETYPE_FILTER, ConfigVal::A_TRANSCODING_PROFILES_PROFLE_SRCDLNA }), cs->option, ConfigVal::A_TRANSCODING_PROFILES_PROFLE_SRCDLNA, cs);
        setValue(item, filter->getSourceProfile());

        item = values.append_child(CONFIG_LOAD_ITEM);
        createItem(item, cs->getItemPath(indexList, { ConfigVal::A_TRANSCODING_MIMETYPE_FILTER, ConfigVal::A_TRANSCODING_PROFILES_PROFLE_NOTRANSCODING }), cs->option, ConfigVal::A_TRANSCODING_PROFILES_PROFLE_NOTRANSCODING, cs);
        setValue(item, fmt::to_string(fmt::join(filter->getNoTranscodingMimeTypes(), ",")));

        item = values.append_child(CONFIG_LOAD_ITEM);
        createItem(item, cs->getItemPath(indexList, { ConfigVal::A_TRANSCODING_MIMETYPE_FILTER, ConfigVal::A_TRANSCODING_PROFILES_PROFLE_CLIENTFLAGS }), cs->option, ConfigVal::A_TRANSCODING_PROFILES_PROFLE_CLIENTFLAGS, cs);
        setValue(item, ClientConfig::mapFlags(filter->getClientFlags()));

        if (filter->getTranscodingProfile())
            profiles[filter->getTranscodingProfile()->getName()] = filter->getTranscodingProfile();

        pr++;
    }

    pr = 0;
    // write profile list
    // profiles can only be exported when linked to at least one filter
    for (auto&& [name, entry] : profiles) {
        std::vector<std::size_t> indexList = { pr };
        auto item = values.append_child(CONFIG_LOAD_ITEM);
        createItem(item, cs->getItemPath(indexList, { ConfigVal::A_TRANSCODING_PROFILES_PROFLE, ConfigVal::A_TRANSCODING_PROFILES_PROFLE_NAME }), cs->option, ConfigVal::A_TRANSCODING_PROFILES_PROFLE_NAME);
        setValue(item, name);

        item = values.append_child(CONFIG_LOAD_ITEM);
        createItem(item, cs->getItemPath(indexList, { ConfigVal::A_TRANSCODING_PROFILES_PROFLE, ConfigVal::A_TRANSCODING_PROFILES_PROFLE_CLIENTFLAGS }), cs->option, ConfigVal::A_TRANSCODING_PROFILES_PROFLE_CLIENTFLAGS);
        setValue(item, ClientConfig::mapFlags(entry->getClientFlags()));

        item = values.append_child(CONFIG_LOAD_ITEM);
        createItem(item, cs->getItemPath(indexList, { ConfigVal::A_TRANSCODING_PROFILES_PROFLE, ConfigVal::A_TRANSCODING_PROFILES_PROFLE_ENABLED }), cs->option, ConfigVal::A_TRANSCODING_PROFILES_PROFLE_ENABLED);
        setValue(item, entry->isEnabled());

        item = values.append_child(CONFIG_LOAD_ITEM);
        auto ttEnumSetup = definition->findConfigSetup<ConfigEnumSetup<TranscodingType>>(ConfigVal::A_TRANSCODING_PROFILES_PROFLE_TYPE);
        createItem(item, cs->getItemPath(indexList, { ConfigVal::A_TRANSCODING_PROFILES_PROFLE, ConfigVal::A_TRANSCODING_PROFILES_PROFLE_TYPE }), cs->option, ConfigVal::A_TRANSCODING_PROFILES_PROFLE_TYPE);
        setValue(item, ttEnumSetup->mapEnumValue(entry->getType()));

        item = values.append_child(CONFIG_LOAD_ITEM);
        createItem(item, cs->getItemPath(indexList, { ConfigVal::A_TRANSCODING_PROFILES_PROFLE, ConfigVal::A_TRANSCODING_PROFILES_PROFLE_DLNAPROF }), cs->option, ConfigVal::A_TRANSCODING_PROFILES_PROFLE_DLNAPROF);
        setValue(item, entry->getDlnaProfile());

        item = values.append_child(CONFIG_LOAD_ITEM);
        createItem(item, cs->getItemPath(indexList, { ConfigVal::A_TRANSCODING_PROFILES_PROFLE, ConfigVal::A_TRANSCODING_PROFILES_PROFLE_MIMETYPE }), cs->option, ConfigVal::A_TRANSCODING_PROFILES_PROFLE_MIMETYPE);
        setValue(item, entry->getTargetMimeType());

        item = values.append_child(CONFIG_LOAD_ITEM);
        createItem(item, cs->getItemPath(indexList, { ConfigVal::A_TRANSCODING_PROFILES_PROFLE, ConfigVal::A_TRANSCODING_PROFILES_PROFLE_RESOLUTION }), cs->option, ConfigVal::A_TRANSCODING_PROFILES_PROFLE_RESOLUTION);
        setValue(item, entry->getAttributeOverride(ResourceAttribute::RESOLUTION));

        item = values.append_child(CONFIG_LOAD_ITEM);
        createItem(item, cs->getItemPath(indexList, { ConfigVal::A_TRANSCODING_PROFILES_PROFLE, ConfigVal::A_TRANSCODING_PROFILES_PROFLE_ACCURL }), cs->option, ConfigVal::A_TRANSCODING_PROFILES_PROFLE_ACCURL);
        setValue(item, entry->getAcceptURL());

        item = values.append_child(CONFIG_LOAD_ITEM);
        createItem(item, cs->getItemPath(indexList, { ConfigVal::A_TRANSCODING_PROFILES_PROFLE, ConfigVal::A_TRANSCODING_PROFILES_PROFLE_SAMPFREQ }), cs->option, ConfigVal::A_TRANSCODING_PROFILES_PROFLE_SAMPFREQ);
        setValue(item, entry->getSampleFreq());

        item = values.append_child(CONFIG_LOAD_ITEM);
        createItem(item, cs->getItemPath(indexList, { ConfigVal::A_TRANSCODING_PROFILES_PROFLE, ConfigVal::A_TRANSCODING_PROFILES_PROFLE_NRCHAN }), cs->option, ConfigVal::A_TRANSCODING_PROFILES_PROFLE_NRCHAN);
        setValue(item, entry->getNumChannels());

        item = values.append_child(CONFIG_LOAD_ITEM);
        createItem(item, cs->getItemPath(indexList, { ConfigVal::A_TRANSCODING_PROFILES_PROFLE, ConfigVal::A_TRANSCODING_PROFILES_PROFLE_HIDEORIG }), cs->option, ConfigVal::A_TRANSCODING_PROFILES_PROFLE_HIDEORIG);
        setValue(item, entry->hideOriginalResource());

        item = values.append_child(CONFIG_LOAD_ITEM);
        createItem(item, cs->getItemPath({ pr }, { ConfigVal::A_TRANSCODING_PROFILES_PROFLE, ConfigVal::A_TRANSCODING_PROFILES_PROFLE_THUMB }), cs->option, ConfigVal::A_TRANSCODING_PROFILES_PROFLE_THUMB);
        setValue(item, entry->isThumbnail());

        item = values.append_child(CONFIG_LOAD_ITEM);
        createItem(item, cs->getItemPath(indexList, { ConfigVal::A_TRANSCODING_PROFILES_PROFLE, ConfigVal::A_TRANSCODING_PROFILES_PROFLE_FIRST }), cs->option, ConfigVal::A_TRANSCODING_PROFILES_PROFLE_FIRST);
        setValue(item, entry->getFirstResource());

        item = values.append_child(CONFIG_LOAD_ITEM);
        createItem(item, cs->getItemPath(indexList, { ConfigVal::A_TRANSCODING_PROFILES_PROFLE, ConfigVal::A_TRANSCODING_PROFILES_PROFLE_ACCOGG }), cs->option, ConfigVal::A_TRANSCODING_PROFILES_PROFLE_ACCOGG);
        setValue(item, entry->isTheora());

        item = values.append_child(CONFIG_LOAD_ITEM);
        createItem(item, cs->getItemPath(indexList, { ConfigVal::A_TRANSCODING_PROFILES_PROFLE, ConfigVal::A_TRANSCODING_PROFILES_PROFLE_AGENT, ConfigVal::A_TRANSCODING_PROFILES_PROFLE_AGENT_COMMAND }), cs->option, ConfigVal::A_TRANSCODING_PROFILES_PROFLE_AGENT_COMMAND);
        setValue(item, entry->getCommand());

        item = values.append_child(CONFIG_LOAD_ITEM);
        createItem(item, cs->getItemPath(indexList, { ConfigVal::A_TRANSCODING_PROFILES_PROFLE, ConfigVal::A_TRANSCODING_PROFILES_PROFLE_AGENT, ConfigVal::A_TRANSCODING_PROFILES_PROFLE_AGENT_ARGS }), cs->option, ConfigVal::A_TRANSCODING_PROFILES_PROFLE_AGENT_ARGS);
        setValue(item, entry->getArguments());

        item = values.append_child(CONFIG_LOAD_ITEM);
        createItem(item, cs->getItemPath(indexList, { ConfigVal::A_TRANSCODING_PROFILES_PROFLE, ConfigVal::A_TRANSCODING_PROFILES_PROFLE_BUFFER, ConfigVal::A_TRANSCODING_PROFILES_PROFLE_BUFFER_SIZE }), cs->option, ConfigVal::A_TRANSCODING_PROFILES_PROFLE_BUFFER_SIZE);
        setValue(item, entry->getBufferSize());

        item = values.append_child(CONFIG_LOAD_ITEM);
        createItem(item, cs->getItemPath(indexList, { ConfigVal::A_TRANSCODING_PROFILES_PROFLE, ConfigVal::A_TRANSCODING_PROFILES_PROFLE_BUFFER, ConfigVal::A_TRANSCODING_PROFILES_PROFLE_BUFFER_CHUNK }), cs->option, ConfigVal::A_TRANSCODING_PROFILES_PROFLE_BUFFER_CHUNK);
        setValue(item, entry->getBufferChunkSize());

        item = values.append_child(CONFIG_LOAD_ITEM);
        createItem(item, cs->getItemPath(indexList, { ConfigVal::A_TRANSCODING_PROFILES_PROFLE, ConfigVal::A_TRANSCODING_PROFILES_PROFLE_BUFFER, ConfigVal::A_TRANSCODING_PROFILES_PROFLE_BUFFER_FILL }), cs->option, ConfigVal::A_TRANSCODING_PROFILES_PROFLE_BUFFER_FILL);
        setValue(item, entry->getBufferInitialFillSize());

        auto fourCCMode = entry->getAVIFourCCListMode();
        if (fourCCMode != AviFourccListmode::None) {
            item = values.append_child(CONFIG_LOAD_ITEM);
            createItem(item, cs->getItemPath(indexList, { ConfigVal::A_TRANSCODING_PROFILES_PROFLE, ConfigVal::A_TRANSCODING_PROFILES_PROFLE_AVI4CC, ConfigVal::A_TRANSCODING_PROFILES_PROFLE_AVI4CC_MODE }), cs->option, ConfigVal::A_TRANSCODING_PROFILES_PROFLE_AVI4CC_MODE);
            auto fccEnumSetup = definition->findConfigSetup<ConfigEnumSetup<AviFourccListmode>>(ConfigVal::A_TRANSCODING_PROFILES_PROFLE_AVI4CC_MODE);
            setValue(item, fccEnumSetup->mapEnumValue(fourCCMode));

            const auto& fourCCList = entry->getAVIFourCCList();
            if (!fourCCList.empty()) {
                item = values.append_child(CONFIG_LOAD_ITEM);
                createItem(item, cs->getItemPath(indexList, { ConfigVal::A_TRANSCODING_PROFILES_PROFLE, ConfigVal::A_TRANSCODING_PROFILES_PROFLE_AVI4CC, ConfigVal::A_TRANSCODING_PROFILES_PROFLE_AVI4CC_4CC }), cs->option, ConfigVal::A_TRANSCODING_PROFILES_PROFLE_AVI4CC_4CC);
                setValue(item, std::accumulate(next(fourCCList.begin()), fourCCList.end(), fourCCList[0], [](auto&& a, auto&& b) { return fmt::format("{}, {}", a, b); }));
            }
        }
        pr++;
    }
}

/// \brief: write autoscan configuration
void Web::ConfigLoad::writeAutoscan(pugi::xml_node& values)
{
    for (auto&& ascs : definition->getConfigSetupList<ConfigAutoscanSetup>()) {
        auto autoscan = ascs->getValue()->getAutoscanListOption();
        // create separate entries for timed and inotify
        for (std::size_t i = 0; i < autoscan.size(); i++) {
            auto&& entry = autoscan.at(i);
            std::vector<std::size_t> indexList = { i };
            auto&& adir = content->getAutoscanDirectory(entry->getLocation());

            // path
            auto item = values.append_child(CONFIG_LOAD_ITEM);
            createItem(item, ascs->getItemPath(indexList, { ConfigVal::A_AUTOSCAN_DIRECTORY_LOCATION }), ascs->option, ConfigVal::A_AUTOSCAN_DIRECTORY_LOCATION);
            setValue(item, adir->getLocation());

            // scan mode (timed|inotify)
            item = values.append_child(CONFIG_LOAD_ITEM);
            createItem(item, ascs->getItemPath(indexList, { ConfigVal::A_AUTOSCAN_DIRECTORY_MODE }), ascs->option, ConfigVal::A_AUTOSCAN_DIRECTORY_MODE);
            setValue(item, fmt::to_string(AutoscanDirectory::mapScanmode(adir->getScanMode())));

            // interval for timed
            item = values.append_child(CONFIG_LOAD_ITEM);
            createItem(item, ascs->getItemPath(indexList, { ConfigVal::A_AUTOSCAN_DIRECTORY_INTERVAL }), ascs->option, ConfigVal::A_AUTOSCAN_DIRECTORY_INTERVAL);
            setValue(item, adir->getInterval().count());

            // counter for retries
            item = values.append_child(CONFIG_LOAD_ITEM);
            createItem(item, ascs->getItemPath(indexList, { ConfigVal::A_AUTOSCAN_DIRECTORY_RETRYCOUNT }), ascs->option, ConfigVal::A_AUTOSCAN_DIRECTORY_RETRYCOUNT);
            setValue(item, adir->getInterval().count());

            // recursively import files
            item = values.append_child(CONFIG_LOAD_ITEM);
            createItem(item, ascs->getItemPath(indexList, { ConfigVal::A_AUTOSCAN_DIRECTORY_RECURSIVE }), ascs->option, ConfigVal::A_AUTOSCAN_DIRECTORY_RECURSIVE);
            setValue(item, adir->getRecursive());

            // directory types
            item = values.append_child(CONFIG_LOAD_ITEM);
            createItem(item, ascs->getItemPath(indexList, { ConfigVal::A_AUTOSCAN_DIRECTORY_DIRTYPES }), ascs->option, ConfigVal::A_AUTOSCAN_DIRECTORY_DIRTYPES);
            setValue(item, adir->hasDirTypes());

            // media types imported from directory
            item = values.append_child(CONFIG_LOAD_ITEM);
            createItem(item, ascs->getItemPath(indexList, { ConfigVal::A_AUTOSCAN_DIRECTORY_MEDIATYPE }), ascs->option, ConfigVal::A_AUTOSCAN_DIRECTORY_MEDIATYPE);
            setValue(item, AutoscanDirectory::mapMediaType(adir->getMediaType()));

            // import hidden files
            item = values.append_child(CONFIG_LOAD_ITEM);
            createItem(item, ascs->getItemPath(indexList, { ConfigVal::A_AUTOSCAN_DIRECTORY_HIDDENFILES }), ascs->option, ConfigVal::A_AUTOSCAN_DIRECTORY_HIDDENFILES);
            setValue(item, adir->getHidden());

            // follow symbolic links
            item = values.append_child(CONFIG_LOAD_ITEM);
            createItem(item, ascs->getItemPath(indexList, { ConfigVal::A_AUTOSCAN_DIRECTORY_FOLLOWSYMLINKS }), ascs->option, ConfigVal::A_AUTOSCAN_DIRECTORY_FOLLOWSYMLINKS);
            setValue(item, adir->getFollowSymlinks());

            // container type for audio
            item = values.append_child(CONFIG_LOAD_ITEM);
            createItem(item, ascs->getItemPath(indexList, { ConfigVal::A_AUTOSCAN_CONTAINER_TYPE_AUDIO }), ascs->option, ConfigVal::A_AUTOSCAN_CONTAINER_TYPE_AUDIO);
            setValue(item, adir->getContainerTypes().at(AutoscanMediaMode::Audio));

            // container type for images
            item = values.append_child(CONFIG_LOAD_ITEM);
            createItem(item, ascs->getItemPath(indexList, { ConfigVal::A_AUTOSCAN_CONTAINER_TYPE_IMAGE }), ascs->option, ConfigVal::A_AUTOSCAN_CONTAINER_TYPE_IMAGE);
            setValue(item, adir->getContainerTypes().at(AutoscanMediaMode::Image));

            // container type for videos
            item = values.append_child(CONFIG_LOAD_ITEM);
            createItem(item, ascs->getItemPath(indexList, { ConfigVal::A_AUTOSCAN_CONTAINER_TYPE_VIDEO }), ascs->option, ConfigVal::A_AUTOSCAN_CONTAINER_TYPE_VIDEO);
            setValue(item, adir->getContainerTypes().at(AutoscanMediaMode::Video));

            // active scan count
            item = values.append_child(CONFIG_LOAD_ITEM);
            createItem(item, ascs->getItemPath(indexList, { ConfigVal::A_AUTOSCAN_DIRECTORY_SCANCOUNT }), ascs->option, ConfigVal::A_AUTOSCAN_DIRECTORY_SCANCOUNT);
            setValue(item, adir->getActiveScanCount());

            // active task count
            item = values.append_child(CONFIG_LOAD_ITEM);
            createItem(item, ascs->getItemPath(indexList, { ConfigVal::A_AUTOSCAN_DIRECTORY_TASKCOUNT }), ascs->option, ConfigVal::A_AUTOSCAN_DIRECTORY_TASKCOUNT);
            setValue(item, adir->getTaskCount());

            // Last modified
            item = values.append_child(CONFIG_LOAD_ITEM);
            createItem(item, ascs->getItemPath(indexList, { ConfigVal::A_AUTOSCAN_DIRECTORY_LMT }), ascs->option, ConfigVal::A_AUTOSCAN_DIRECTORY_LMT);
            setValue(item, fmt::format("{:%Y-%m-%d %H:%M:%S}", fmt::localtime(adir->getPreviousLMT().count())));
        }
        // Allow creation of entry in blank config
        {
            // path
            auto item = values.append_child(CONFIG_LOAD_ITEM);
            createItem(item, ascs->getItemPath(ITEM_PATH_NEW, { ConfigVal::A_AUTOSCAN_DIRECTORY_LOCATION }), ascs->option, ConfigVal::A_AUTOSCAN_DIRECTORY_LOCATION, definition->findConfigSetup(ConfigVal::A_AUTOSCAN_DIRECTORY_LOCATION));

            // scan mode (timed|inotify)
            item = values.append_child(CONFIG_LOAD_ITEM);
            createItem(item, ascs->getItemPath(ITEM_PATH_NEW, { ConfigVal::A_AUTOSCAN_DIRECTORY_MODE }), ascs->option, ConfigVal::A_AUTOSCAN_DIRECTORY_MODE, definition->findConfigSetup(ConfigVal::A_AUTOSCAN_DIRECTORY_MODE));

            // interval for timed
            item = values.append_child(CONFIG_LOAD_ITEM);
            createItem(item, ascs->getItemPath(ITEM_PATH_NEW, { ConfigVal::A_AUTOSCAN_DIRECTORY_INTERVAL }), ascs->option, ConfigVal::A_AUTOSCAN_DIRECTORY_INTERVAL, definition->findConfigSetup(ConfigVal::A_AUTOSCAN_DIRECTORY_INTERVAL));

            // counter for retries
            item = values.append_child(CONFIG_LOAD_ITEM);
            createItem(item, ascs->getItemPath(ITEM_PATH_NEW, { ConfigVal::A_AUTOSCAN_DIRECTORY_RETRYCOUNT }), ascs->option, ConfigVal::A_AUTOSCAN_DIRECTORY_RETRYCOUNT);

            // recursively import files
            item = values.append_child(CONFIG_LOAD_ITEM);
            createItem(item, ascs->getItemPath(ITEM_PATH_NEW, { ConfigVal::A_AUTOSCAN_DIRECTORY_RECURSIVE }), ascs->option, ConfigVal::A_AUTOSCAN_DIRECTORY_RECURSIVE, definition->findConfigSetup(ConfigVal::A_AUTOSCAN_DIRECTORY_RECURSIVE));

            // directory types
            item = values.append_child(CONFIG_LOAD_ITEM);
            createItem(item, ascs->getItemPath(ITEM_PATH_NEW, { ConfigVal::A_AUTOSCAN_DIRECTORY_DIRTYPES }), ascs->option, ConfigVal::A_AUTOSCAN_DIRECTORY_DIRTYPES);

            // media types imported from directory
            item = values.append_child(CONFIG_LOAD_ITEM);
            createItem(item, ascs->getItemPath(ITEM_PATH_NEW, { ConfigVal::A_AUTOSCAN_DIRECTORY_MEDIATYPE }), ascs->option, ConfigVal::A_AUTOSCAN_DIRECTORY_MEDIATYPE, definition->findConfigSetup(ConfigVal::A_AUTOSCAN_DIRECTORY_MEDIATYPE));

            // import hidden files
            item = values.append_child(CONFIG_LOAD_ITEM);
            createItem(item, ascs->getItemPath(ITEM_PATH_NEW, { ConfigVal::A_AUTOSCAN_DIRECTORY_HIDDENFILES }), ascs->option, ConfigVal::A_AUTOSCAN_DIRECTORY_HIDDENFILES, definition->findConfigSetup(ConfigVal::A_AUTOSCAN_DIRECTORY_HIDDENFILES));

            // follow symbolic links
            item = values.append_child(CONFIG_LOAD_ITEM);
            createItem(item, ascs->getItemPath(ITEM_PATH_NEW, { ConfigVal::A_AUTOSCAN_DIRECTORY_FOLLOWSYMLINKS }), ascs->option, ConfigVal::A_AUTOSCAN_DIRECTORY_FOLLOWSYMLINKS);

            // container type for audio
            item = values.append_child(CONFIG_LOAD_ITEM);
            createItem(item, ascs->getItemPath(ITEM_PATH_NEW, { ConfigVal::A_AUTOSCAN_CONTAINER_TYPE_AUDIO }), ascs->option, ConfigVal::A_AUTOSCAN_CONTAINER_TYPE_AUDIO, definition->findConfigSetup(ConfigVal::A_AUTOSCAN_CONTAINER_TYPE_AUDIO));

            // container type for images
            item = values.append_child(CONFIG_LOAD_ITEM);
            createItem(item, ascs->getItemPath(ITEM_PATH_NEW, { ConfigVal::A_AUTOSCAN_CONTAINER_TYPE_IMAGE }), ascs->option, ConfigVal::A_AUTOSCAN_CONTAINER_TYPE_IMAGE, definition->findConfigSetup(ConfigVal::A_AUTOSCAN_CONTAINER_TYPE_IMAGE));

            // container type for videos
            item = values.append_child(CONFIG_LOAD_ITEM);
            createItem(item, ascs->getItemPath(ITEM_PATH_NEW, { ConfigVal::A_AUTOSCAN_CONTAINER_TYPE_VIDEO }), ascs->option, ConfigVal::A_AUTOSCAN_CONTAINER_TYPE_VIDEO, definition->findConfigSetup(ConfigVal::A_AUTOSCAN_CONTAINER_TYPE_VIDEO));

            // active scan count
            item = values.append_child(CONFIG_LOAD_ITEM);
            createItem(item, ascs->getItemPath(ITEM_PATH_NEW, { ConfigVal::A_AUTOSCAN_DIRECTORY_SCANCOUNT }), ascs->option, ConfigVal::A_AUTOSCAN_DIRECTORY_SCANCOUNT, definition->findConfigSetup(ConfigVal::A_AUTOSCAN_DIRECTORY_SCANCOUNT));

            // active task count
            item = values.append_child(CONFIG_LOAD_ITEM);
            createItem(item, ascs->getItemPath(ITEM_PATH_NEW, { ConfigVal::A_AUTOSCAN_DIRECTORY_TASKCOUNT }), ascs->option, ConfigVal::A_AUTOSCAN_DIRECTORY_TASKCOUNT, definition->findConfigSetup(ConfigVal::A_AUTOSCAN_DIRECTORY_TASKCOUNT));

            // Last modified
            item = values.append_child(CONFIG_LOAD_ITEM);
            createItem(item, ascs->getItemPath(ITEM_PATH_NEW, { ConfigVal::A_AUTOSCAN_DIRECTORY_LMT }), ascs->option, ConfigVal::A_AUTOSCAN_DIRECTORY_LMT, definition->findConfigSetup(ConfigVal::A_AUTOSCAN_DIRECTORY_LMT));
        }
    }
}

/// \brief: write content of all dictionaries
void Web::ConfigLoad::writeDictionaries(pugi::xml_node& values)
{
    for (auto&& dcs : definition->getConfigSetupList<ConfigDictionarySetup>()) {
        std::size_t i = 0;
        auto dictionary = dcs->getValue()->getDictionaryOption(true);
        for (auto&& [key, val] : dictionary) {
            std::vector<std::size_t> indexList = { i };
            auto item = values.append_child(CONFIG_LOAD_ITEM);
            createItem(item, dcs->getItemPath(indexList, { dcs->keyOption }), dcs->option, dcs->keyOption, dcs);
            setValue(item, key.substr(5));

            item = values.append_child(CONFIG_LOAD_ITEM);
            createItem(item, dcs->getItemPath(indexList, { dcs->valOption }), dcs->option, dcs->valOption, dcs);
            setValue(item, val);
            i++;
        }
        {
            auto item = values.append_child(CONFIG_LOAD_ITEM);
            createItem(item, dcs->getItemPath(ITEM_PATH_NEW, { dcs->keyOption }), dcs->option, dcs->keyOption, dcs);

            item = values.append_child(CONFIG_LOAD_ITEM);
            createItem(item, dcs->getItemPath(ITEM_PATH_NEW, { dcs->valOption }), dcs->option, dcs->valOption, dcs);
        }
    }
}

/// \brief: write content of all vectors
void Web::ConfigLoad::writeVectors(pugi::xml_node& values)
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
                auto item = values.append_child(CONFIG_LOAD_ITEM);
                std::size_t j = 0;
                auto pos = std::find(attrList.begin(), attrList.end(), key);
                if (pos != attrList.end())
                    j = std::distance(attrList.begin(), pos);
                createItem(item, vcs->getItemPath({ i }, {}, { key }), vcs->option, vcs->optionList[j], vcs);
                setValue(item, val);
            }
            i++;
        }
    }
}

/// \brief: write content of all arrays
void Web::ConfigLoad::writeArrays(pugi::xml_node& values)
{
    for (auto&& acs : definition->getConfigSetupList<ConfigArraySetup>()) {
        auto array = acs->getValue()->getArrayOption(true);
        for (std::size_t i = 0; i < array.size(); i++) {
            auto&& entry = array[i];
            auto item = values.append_child(CONFIG_LOAD_ITEM);
            createItem(item, acs->getItemPath({ i }, {}), acs->option, acs->attrOption != ConfigVal::MAX ? acs->attrOption : acs->nodeOption, acs);
            setValue(item, entry);
        }
        {
            auto item = values.append_child(CONFIG_LOAD_ITEM);
            createItem(item, acs->getItemPath(ITEM_PATH_NEW, {}), acs->option, acs->attrOption != ConfigVal::MAX ? acs->attrOption : acs->nodeOption, acs);
        }
    }
}

/// \brief: update entries with datebase values
void Web::ConfigLoad::updateEntriesFromDatabase(pugi::xml_node& element, pugi::xml_node& values)
{
    for (auto&& entry : dbEntries) {
        auto exItem = allItems.find(entry.item);
        if (exItem != allItems.end()) {
            auto item = element.select_node(exItem->second.c_str()).node();
            item.attribute(CONFIG_LOAD_SOURCE) = CONFIG_LOAD_SOURCE_DATABASE;
            item.attribute(CONFIG_LOAD_STATUS) = entry.status.c_str();
        } else {
            auto cs = definition->findConfigSetupByPath(entry.item, true);
            auto acs = definition->findConfigSetupByPath(entry.item, true, cs);
            if (cs) {
                auto item = values.append_child(CONFIG_LOAD_ITEM);
                createItem(item, entry.item, cs->option, acs ? acs->option : ConfigVal::MAX);
                setValue(item, entry.value);
                item.attribute(CONFIG_LOAD_STATUS) = entry.status.c_str();
                item.attribute(CONFIG_LOAD_ORIGVALUE) = config->getOrigValue(entry.item).c_str();
                item.attribute(CONFIG_LOAD_SOURCE) = CONFIG_LOAD_SOURCE_DATABASE;
            }
        }
    }
}
