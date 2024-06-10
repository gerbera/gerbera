/*GRB*

    Gerbera - https://gerbera.io/

    config_load.cc - this file is part of Gerbera.

    Copyright (C) 2020-2024 Gerbera Contributors

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

/// \file config_load.cc
#define GRB_LOG_FAC GrbLogFacility::web

#include "pages.h" // API

#include <fmt/chrono.h>
#include <numeric>

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
#include "content/content_manager.h"
#include "database/database.h"
#include "util/xml_to_json.h"

Web::ConfigLoad::ConfigLoad(const std::shared_ptr<ContentManager>& content)
    : WebRequestHandler(content)
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
    auto info = meta.append_child("item");
    info.append_attribute("item") = cs->getUniquePath().c_str();
    info.append_attribute("id") = fmt::format("{:03d}", cs->option).c_str();
    info.append_attribute("type") = cs->getTypeString().c_str();
    info.append_attribute("value") = cs->getDefaultValue().c_str();
    info.append_attribute("help") = cs->getHelp();
}

void Web::ConfigLoad::createItem(pugi::xml_node& item, const std::string& name, config_option_t id, config_option_t aid, const std::shared_ptr<ConfigSetup>& cs)
{
    allItems[name] = fmt::format("{}[{}]", item.path(), allItems.size() + 1);
    item.append_attribute("item") = name.c_str();
    item.append_attribute("id") = fmt::format("{:03d}", id).c_str();
    item.append_attribute("aid") = fmt::format("{:03d}", aid).c_str();
    if (std::any_of(dbEntries.begin(), dbEntries.end(), [=](auto&& s) { return s.item == name; })) {
        item.append_attribute("status") = "unchanged";
        item.append_attribute("source") = "database";
    } else {
        item.append_attribute("status") = !cs || !cs->isDefaultValueUsed() ? "unchanged" : "default";
        item.append_attribute("source") = !cs || !cs->isDefaultValueUsed() ? "config.xml" : "default";
    }
    item.append_attribute("origValue") = config->getOrigValue(name).c_str();
    item.append_attribute("defaultValue") = cs ? cs->getDefaultValue().c_str() : "";
}

template <typename T>
void Web::ConfigLoad::setValue(pugi::xml_node& item, const T& value)
{
    static_assert(fmt::has_formatter<T, fmt::format_context>::value, "T must be formattable");
    item.append_attribute("value") = fmt::to_string(value).c_str();
}

template <>
void Web::ConfigLoad::setValue(pugi::xml_node& item, const std::string& value)
{
    item.append_attribute("value") = value.c_str();
}

template <>
void Web::ConfigLoad::setValue(pugi::xml_node& item, const std::string_view& value)
{
    item.append_attribute("value") = value.data();
}

template <>
void Web::ConfigLoad::setValue(pugi::xml_node& item, const fs::path& value)
{
    item.append_attribute("value") = value.c_str();
}

/// \brief: process config_load request
void Web::ConfigLoad::process()
{
    checkRequest();
    auto root = xmlDoc->document_element();
    auto values = root.append_child("values");
    std::string action = param("action");

    // set handling of json properties
    xml2Json->setArrayName(values, "item");
    xml2Json->setFieldType("item", "string");
    xml2Json->setFieldType("id", "string");
    xml2Json->setFieldType("aid", "string");
    xml2Json->setFieldType("value", "string");
    xml2Json->setFieldType("defaultValue", "string");
    xml2Json->setFieldType("origValue", "string");

    log_debug("Sending Config to web!");

    writeDatabaseStatus(values);

    if (action == "status")
        return;

    // generate meta info for ui
    auto meta = root.append_child("types");
    xml2Json->setArrayName(meta, "item");
    for (auto&& cs : ConfigDefinition::getOptionList()) {
        addTypeMeta(meta, cs);
    }

    writeSimpleProperties(values);
    writeClientConfig(values);
    writeImportTweaks(values);
    writeDynamicContent(values);
    writeBoxLayout(values);
    writeTransconding(values);
    writeAutoscan(values);
    writeDictionaries(values);
    writeVectors(values);
    writeArrays(values);
    updateEntriesFromDatabase(root, values);
}

// \brief: write database status
void Web::ConfigLoad::writeDatabaseStatus(pugi::xml_node& values)
{
    {
        StatsParam stats(StatsParam::StatsMode::Count, "", "", false);
        auto item = values.append_child("item");
        createItem(item, "/status/attribute::totalCount", CFG_MAX, CFG_MAX);
        setValue(item, database->getFileStats(stats));
    }
    {
        StatsParam stats(StatsParam::StatsMode::Size, "", "", false);
        auto item = values.append_child("item");
        createItem(item, "/status/attribute::totalSize", CFG_MAX, CFG_MAX);
        setValue(item, CdsResource::formatSizeValue(database->getFileStats(stats)));
    }
    {
        StatsParam stats(StatsParam::StatsMode::Count, "", "", true);
        auto item = values.append_child("item");
        createItem(item, "/status/attribute::virtual", CFG_MAX, CFG_MAX);
        setValue(item, database->getFileStats(stats));
    }

    {
        StatsParam stats(StatsParam::StatsMode::Count, "audio", "", true);
        auto item = values.append_child("item");
        createItem(item, "/status/attribute::audioVirtual", CFG_MAX, CFG_MAX);
        setValue(item, database->getFileStats(stats));
    }
    {
        StatsParam stats(StatsParam::StatsMode::Count, "video", "", true);
        auto item = values.append_child("item");
        createItem(item, "/status/attribute::videoVirtual", CFG_MAX, CFG_MAX);
        setValue(item, database->getFileStats(stats));
    }
    {
        StatsParam stats(StatsParam::StatsMode::Count, "image", "", true);
        auto item = values.append_child("item");
        createItem(item, "/status/attribute::imageVirtual", CFG_MAX, CFG_MAX);
        setValue(item, database->getFileStats(stats));
    }

    StatsParam statc(StatsParam::StatsMode::Count, "", "", false);
    auto cnt = database->getGroupStats(statc);
    StatsParam stats(StatsParam::StatsMode::Size, "", "", false);
    auto siz = database->getGroupStats(stats);

    static std::map<std::string, std::string> statMapBase {
        { UPNP_CLASS_ITEM, "item" },
    };
    for (auto&& [cls, attr] : statMapBase) {
        if (cnt[cls] > 0) {
            auto item = values.append_child("item");
            createItem(item, fmt::format("/status/attribute::{}Count", attr), CFG_MAX, CFG_MAX);
            setValue(item, cnt[cls]);
            auto item2 = values.append_child("item");
            createItem(item2, fmt::format("/status/attribute::{}Size", attr), CFG_MAX, CFG_MAX);
            setValue(item2, CdsResource::formatSizeValue(siz[cls]));
            auto item3 = values.append_child("item");
            createItem(item3, fmt::format("/status/attribute::{}Bytes", attr), CFG_MAX, CFG_MAX);
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
            auto item = values.append_child("item");
            createItem(item, fmt::format("/status/attribute::{}Count", attr), CFG_MAX, CFG_MAX);
            setValue(item, totalCnt);
            auto item2 = values.append_child("item");
            createItem(item2, fmt::format("/status/attribute::{}Size", attr), CFG_MAX, CFG_MAX);
            setValue(item2, CdsResource::formatSizeValue(totalSize));
            auto item3 = values.append_child("item");
            createItem(item3, fmt::format("/status/attribute::{}Bytes", attr), CFG_MAX, CFG_MAX);
            setValue(item3, totalSize);
        }
    }
}

// \brief: write all values with simple type (string, int, bool)
void Web::ConfigLoad::writeSimpleProperties(pugi::xml_node& values)
{
    for (auto&& option : ConfigOptionIterator()) {
        try {
            auto scs = ConfigDefinition::findConfigSetup(option);
            auto item = values.append_child("item");
            createItem(item, scs->getItemPath(ITEM_PATH_ROOT), option, option, scs);

            log_debug("    Option {:03d} {} = {}", option, scs->getItemPath(), scs->getCurrentValue().c_str());
            setValue(item, scs->getCurrentValue());
        } catch (const std::runtime_error& e) {
            log_warning("Option {:03d} {}", option, e.what());
        }
    }
}

/// \brief: write client configuration
void Web::ConfigLoad::writeClientConfig(pugi::xml_node& values)
{
    auto cs = ConfigDefinition::findConfigSetup(CFG_CLIENTS_LIST);
    auto clientConfig = cs->getValue()->getClientConfigListOption();
    for (std::size_t i = 0; i < clientConfig->size(); i++) {
        auto client = clientConfig->get(i);

        auto item = values.append_child("item");
        createItem(item, cs->getItemPath(i, ATTR_CLIENTS_CLIENT_FLAGS), cs->option, ATTR_CLIENTS_CLIENT_FLAGS, cs);
        setValue(item, ClientConfig::mapFlags(client->getFlags()));

        item = values.append_child("item");
        createItem(item, cs->getItemPath(i, ATTR_CLIENTS_CLIENT_IP), cs->option, ATTR_CLIENTS_CLIENT_IP, cs);
        setValue(item, client->getIp());

        item = values.append_child("item");
        createItem(item, cs->getItemPath(i, ATTR_CLIENTS_CLIENT_USERAGENT), cs->option, ATTR_CLIENTS_CLIENT_USERAGENT, cs);
        setValue(item, client->getUserAgent());

        item = values.append_child("item");
        createItem(item, cs->getItemPath(i, ATTR_CLIENTS_CLIENT_GROUP), cs->option, ATTR_CLIENTS_CLIENT_GROUP, cs);
        setValue(item, client->getGroup());

        item = values.append_child("item");
        createItem(item, cs->getItemPath(i, ATTR_CLIENTS_UPNP_CAPTION_COUNT), cs->option, ATTR_CLIENTS_UPNP_CAPTION_COUNT, cs);
        setValue(item, client->getCaptionInfoCount());

        item = values.append_child("item");
        createItem(item, cs->getItemPath(i, ATTR_CLIENTS_UPNP_STRING_LIMIT), cs->option, ATTR_CLIENTS_UPNP_STRING_LIMIT, cs);
        setValue(item, client->getStringLimit());

        item = values.append_child("item");
        createItem(item, cs->getItemPath(i, ATTR_CLIENTS_UPNP_MULTI_VALUE), cs->option, ATTR_CLIENTS_UPNP_MULTI_VALUE, cs);
        setValue(item, client->getMultiValue());

        // ToDo: Sub Dictionary ATTR_CLIENTS_UPNP_MAP_MIMETYPE
        // ToDo: Sub Dictionary ATTR_CLIENTS_UPNP_HEADERS
    }
    if (clientConfig->size() == 0) {
        auto item = values.append_child("item");
        createItem(item, cs->getItemPath(ITEM_PATH_NEW, ATTR_CLIENTS_CLIENT_FLAGS), cs->option, ATTR_CLIENTS_CLIENT_FLAGS, ConfigDefinition::findConfigSetup(ATTR_CLIENTS_CLIENT_FLAGS));

        item = values.append_child("item");
        createItem(item, cs->getItemPath(ITEM_PATH_NEW, ATTR_CLIENTS_CLIENT_IP), cs->option, ATTR_CLIENTS_CLIENT_IP, ConfigDefinition::findConfigSetup(ATTR_CLIENTS_CLIENT_IP));

        item = values.append_child("item");
        createItem(item, cs->getItemPath(ITEM_PATH_NEW, ATTR_CLIENTS_CLIENT_USERAGENT), cs->option, ATTR_CLIENTS_CLIENT_USERAGENT, ConfigDefinition::findConfigSetup(ATTR_CLIENTS_CLIENT_USERAGENT));

        item = values.append_child("item");
        createItem(item, cs->getItemPath(ITEM_PATH_NEW, ATTR_CLIENTS_CLIENT_GROUP), cs->option, ATTR_CLIENTS_CLIENT_GROUP, ConfigDefinition::findConfigSetup(ATTR_CLIENTS_CLIENT_GROUP));

        item = values.append_child("item");
        createItem(item, cs->getItemPath(ITEM_PATH_NEW, ATTR_CLIENTS_UPNP_CAPTION_COUNT), cs->option, ATTR_CLIENTS_UPNP_CAPTION_COUNT, ConfigDefinition::findConfigSetup(ATTR_CLIENTS_UPNP_CAPTION_COUNT));

        item = values.append_child("item");
        createItem(item, cs->getItemPath(ITEM_PATH_NEW, ATTR_CLIENTS_UPNP_STRING_LIMIT), cs->option, ATTR_CLIENTS_UPNP_STRING_LIMIT, ConfigDefinition::findConfigSetup(ATTR_CLIENTS_UPNP_STRING_LIMIT));

        item = values.append_child("item");
        createItem(item, cs->getItemPath(ITEM_PATH_NEW, ATTR_CLIENTS_UPNP_MULTI_VALUE), cs->option, ATTR_CLIENTS_UPNP_MULTI_VALUE, ConfigDefinition::findConfigSetup(ATTR_CLIENTS_UPNP_MULTI_VALUE));
    }
}

/// \brief: write import tweaks
void Web::ConfigLoad::writeImportTweaks(pugi::xml_node& values)
{
    auto cs = ConfigDefinition::findConfigSetup(CFG_IMPORT_DIRECTORIES_LIST);
    auto directoryConfig = cs->getValue()->getDirectoryTweakOption();
    for (std::size_t i = 0; i < directoryConfig->size(); i++) {
        auto dir = directoryConfig->get(i);

        auto item = values.append_child("item");
        createItem(item, cs->getItemPath(i, ATTR_DIRECTORIES_TWEAK_LOCATION), cs->option, ATTR_DIRECTORIES_TWEAK_LOCATION);
        setValue(item, dir->getLocation());

        item = values.append_child("item");
        createItem(item, cs->getItemPath(i, ATTR_DIRECTORIES_TWEAK_INHERIT), cs->option, ATTR_DIRECTORIES_TWEAK_INHERIT);
        setValue(item, dir->getInherit());

        item = values.append_child("item");
        createItem(item, cs->getItemPath(i, ATTR_DIRECTORIES_TWEAK_RECURSIVE), cs->option, ATTR_DIRECTORIES_TWEAK_RECURSIVE);
        setValue(item, dir->getRecursive());

        item = values.append_child("item");
        createItem(item, cs->getItemPath(i, ATTR_DIRECTORIES_TWEAK_HIDDEN), cs->option, ATTR_DIRECTORIES_TWEAK_HIDDEN);
        setValue(item, dir->getHidden());

        item = values.append_child("item");
        createItem(item, cs->getItemPath(i, ATTR_DIRECTORIES_TWEAK_CASE_SENSITIVE), cs->option, ATTR_DIRECTORIES_TWEAK_CASE_SENSITIVE);
        setValue(item, dir->getCaseSensitive());

        item = values.append_child("item");
        createItem(item, cs->getItemPath(i, ATTR_DIRECTORIES_TWEAK_FOLLOW_SYMLINKS), cs->option, ATTR_DIRECTORIES_TWEAK_FOLLOW_SYMLINKS);
        setValue(item, dir->getFollowSymlinks());

        item = values.append_child("item");
        createItem(item, cs->getItemPath(i, ATTR_DIRECTORIES_TWEAK_META_CHARSET), cs->option, ATTR_DIRECTORIES_TWEAK_META_CHARSET);
        setValue(item, dir->hasMetaCharset() ? dir->getMetaCharset() : "");

        item = values.append_child("item");
        createItem(item, cs->getItemPath(i, ATTR_DIRECTORIES_TWEAK_FANART_FILE), cs->option, ATTR_DIRECTORIES_TWEAK_FANART_FILE);
        setValue(item, dir->hasFanArtFile() ? dir->getFanArtFile() : "");

        item = values.append_child("item");
        createItem(item, cs->getItemPath(i, ATTR_DIRECTORIES_TWEAK_RESOURCE_FILE), cs->option, ATTR_DIRECTORIES_TWEAK_RESOURCE_FILE);
        setValue(item, dir->hasResourceFile() ? dir->getResourceFile() : "");

        item = values.append_child("item");
        createItem(item, cs->getItemPath(i, ATTR_DIRECTORIES_TWEAK_SUBTITLE_FILE), cs->option, ATTR_DIRECTORIES_TWEAK_SUBTITLE_FILE);
        setValue(item, dir->hasSubTitleFile() ? dir->getSubTitleFile() : "");

        item = values.append_child("item");
        createItem(item, cs->getItemPath(i, ATTR_DIRECTORIES_TWEAK_METAFILE_FILE), cs->option, ATTR_DIRECTORIES_TWEAK_METAFILE_FILE);
        setValue(item, dir->hasMetafile() ? dir->getMetafile() : "");
    }
}

/// \brief: write dynamic content
void Web::ConfigLoad::writeDynamicContent(pugi::xml_node& values)
{
    auto cs = ConfigDefinition::findConfigSetup(CFG_SERVER_DYNAMIC_CONTENT_LIST);
    auto dynContent = cs->getValue()->getDynamicContentListOption();
    for (std::size_t i = 0; i < dynContent->size(); i++) {
        auto cont = dynContent->get(i);

        auto item = values.append_child("item");
        createItem(item, cs->getItemPath(i, ATTR_DYNAMIC_CONTAINER_LOCATION), cs->option, ATTR_DYNAMIC_CONTAINER_LOCATION);
        setValue(item, cont->getLocation());

        item = values.append_child("item");
        createItem(item, cs->getItemPath(i, ATTR_DYNAMIC_CONTAINER_IMAGE), cs->option, ATTR_DYNAMIC_CONTAINER_IMAGE);
        setValue(item, cont->getImage());

        item = values.append_child("item");
        createItem(item, cs->getItemPath(i, ATTR_DYNAMIC_CONTAINER_TITLE), cs->option, ATTR_DYNAMIC_CONTAINER_TITLE);
        setValue(item, cont->getTitle());

        item = values.append_child("item");
        createItem(item, cs->getItemPath(i, ATTR_DYNAMIC_CONTAINER_FILTER), cs->option, ATTR_DYNAMIC_CONTAINER_FILTER);
        setValue(item, cont->getFilter());

        item = values.append_child("item");
        createItem(item, cs->getItemPath(i, ATTR_DYNAMIC_CONTAINER_SORT), cs->option, ATTR_DYNAMIC_CONTAINER_SORT);
        setValue(item, cont->getSort());

        item = values.append_child("item");
        createItem(item, cs->getItemPath(i, ATTR_DYNAMIC_CONTAINER_MAXCOUNT), cs->option, ATTR_DYNAMIC_CONTAINER_MAXCOUNT);
        setValue(item, cont->getMaxCount());
    }
    if (dynContent->size() == 0) {
        auto item = values.append_child("item");
        createItem(item, cs->getItemPath(ITEM_PATH_NEW, ATTR_DYNAMIC_CONTAINER_LOCATION), cs->option, ATTR_DYNAMIC_CONTAINER_LOCATION, ConfigDefinition::findConfigSetup(ATTR_DYNAMIC_CONTAINER_LOCATION));

        item = values.append_child("item");
        createItem(item, cs->getItemPath(ITEM_PATH_NEW, ATTR_DYNAMIC_CONTAINER_IMAGE), cs->option, ATTR_DYNAMIC_CONTAINER_IMAGE, ConfigDefinition::findConfigSetup(ATTR_DYNAMIC_CONTAINER_IMAGE));

        item = values.append_child("item");
        createItem(item, cs->getItemPath(ITEM_PATH_NEW, ATTR_DYNAMIC_CONTAINER_TITLE), cs->option, ATTR_DYNAMIC_CONTAINER_TITLE, ConfigDefinition::findConfigSetup(ATTR_DYNAMIC_CONTAINER_TITLE));

        item = values.append_child("item");
        createItem(item, cs->getItemPath(ITEM_PATH_NEW, ATTR_DYNAMIC_CONTAINER_FILTER), cs->option, ATTR_DYNAMIC_CONTAINER_FILTER, ConfigDefinition::findConfigSetup(ATTR_DYNAMIC_CONTAINER_FILTER));

        item = values.append_child("item");
        createItem(item, cs->getItemPath(ITEM_PATH_NEW, ATTR_DYNAMIC_CONTAINER_SORT), cs->option, ATTR_DYNAMIC_CONTAINER_SORT, ConfigDefinition::findConfigSetup(ATTR_DYNAMIC_CONTAINER_SORT));

        item = values.append_child("item");
        createItem(item, cs->getItemPath(ITEM_PATH_NEW, ATTR_DYNAMIC_CONTAINER_MAXCOUNT), cs->option, ATTR_DYNAMIC_CONTAINER_MAXCOUNT, ConfigDefinition::findConfigSetup(ATTR_DYNAMIC_CONTAINER_MAXCOUNT));
    }
}

/// \brief: write box layout
void Web::ConfigLoad::writeBoxLayout(pugi::xml_node& values)
{
    auto cs = ConfigDefinition::findConfigSetup(CFG_BOXLAYOUT_BOX);
    auto boxlayoutContent = cs->getValue()->getBoxLayoutListOption();
    for (std::size_t i = 0; i < boxlayoutContent->size(); i++) {
        auto cont = boxlayoutContent->get(i);

        auto item = values.append_child("item");
        createItem(item, cs->getItemPath(i, ATTR_BOXLAYOUT_BOX_KEY), cs->option, ATTR_BOXLAYOUT_BOX_KEY);
        setValue(item, cont->getKey());

        item = values.append_child("item");
        createItem(item, cs->getItemPath(i, ATTR_BOXLAYOUT_BOX_TITLE), cs->option, ATTR_BOXLAYOUT_BOX_TITLE);
        setValue(item, cont->getTitle());

        item = values.append_child("item");
        createItem(item, cs->getItemPath(i, ATTR_BOXLAYOUT_BOX_CLASS), cs->option, ATTR_BOXLAYOUT_BOX_CLASS);
        setValue(item, cont->getClass());

        item = values.append_child("item");
        createItem(item, cs->getItemPath(i, ATTR_BOXLAYOUT_BOX_SIZE), cs->option, ATTR_BOXLAYOUT_BOX_SIZE);
        setValue(item, cont->getSize());

        item = values.append_child("item");
        createItem(item, cs->getItemPath(i, ATTR_BOXLAYOUT_BOX_ENABLED), cs->option, ATTR_BOXLAYOUT_BOX_ENABLED);
        setValue(item, cont->getEnabled());
    }
    if (boxlayoutContent->size() == 0) {
        auto item = values.append_child("item");
        createItem(item, cs->getItemPath(ITEM_PATH_NEW, ATTR_BOXLAYOUT_BOX_KEY), cs->option, ATTR_BOXLAYOUT_BOX_KEY, ConfigDefinition::findConfigSetup(ATTR_BOXLAYOUT_BOX_KEY));

        item = values.append_child("item");
        createItem(item, cs->getItemPath(ITEM_PATH_NEW, ATTR_BOXLAYOUT_BOX_TITLE), cs->option, ATTR_BOXLAYOUT_BOX_TITLE, ConfigDefinition::findConfigSetup(ATTR_BOXLAYOUT_BOX_TITLE));

        item = values.append_child("item");
        createItem(item, cs->getItemPath(ITEM_PATH_NEW, ATTR_BOXLAYOUT_BOX_CLASS), cs->option, ATTR_BOXLAYOUT_BOX_CLASS, ConfigDefinition::findConfigSetup(ATTR_BOXLAYOUT_BOX_CLASS));

        item = values.append_child("item");
        createItem(item, cs->getItemPath(ITEM_PATH_NEW, ATTR_BOXLAYOUT_BOX_SIZE), cs->option, ATTR_BOXLAYOUT_BOX_SIZE, ConfigDefinition::findConfigSetup(ATTR_BOXLAYOUT_BOX_SIZE));

        item = values.append_child("item");
        createItem(item, cs->getItemPath(ITEM_PATH_NEW, ATTR_BOXLAYOUT_BOX_ENABLED), cs->option, ATTR_BOXLAYOUT_BOX_ENABLED, ConfigDefinition::findConfigSetup(ATTR_BOXLAYOUT_BOX_ENABLED));
    }
}

/// \brief: write transconding configuration
void Web::ConfigLoad::writeTransconding(pugi::xml_node& values)
{
    auto cs = ConfigDefinition::findConfigSetup(CFG_TRANSCODING_PROFILE_LIST);
    auto transcoding = cs->getValue()->getTranscodingProfileListOption();
    int pr = 0;
    std::map<std::string, std::shared_ptr<TranscodingProfile>> profiles;
    // write filter list and collect required profiles
    for (auto&& filter : transcoding->getFilterList()) {
        auto item = values.append_child("item");
        createItem(item, cs->getItemPath(pr, ATTR_TRANSCODING_MIMETYPE_FILTER, ATTR_TRANSCODING_MIMETYPE_PROF_MAP_MIMETYPE), cs->option, ATTR_TRANSCODING_MIMETYPE_PROF_MAP_MIMETYPE, cs);
        setValue(item, filter->getMimeType());

        item = values.append_child("item");
        createItem(item, cs->getItemPath(pr, ATTR_TRANSCODING_MIMETYPE_FILTER, ATTR_TRANSCODING_MIMETYPE_PROF_MAP_USING), cs->option, ATTR_TRANSCODING_MIMETYPE_PROF_MAP_USING, cs);
        setValue(item, filter->getTranscoderName());

        item = values.append_child("item");
        createItem(item, cs->getItemPath(pr, ATTR_TRANSCODING_MIMETYPE_FILTER, ATTR_TRANSCODING_PROFILES_PROFLE_SRCDLNA), cs->option, ATTR_TRANSCODING_PROFILES_PROFLE_SRCDLNA, cs);
        setValue(item, filter->getSourceProfile());

        item = values.append_child("item");
        createItem(item, cs->getItemPath(pr, ATTR_TRANSCODING_MIMETYPE_FILTER, ATTR_TRANSCODING_PROFILES_PROFLE_NOTRANSCODING), cs->option, ATTR_TRANSCODING_PROFILES_PROFLE_NOTRANSCODING, cs);
        setValue(item, fmt::join(filter->getNoTranscodingMimeTypes(), ","));

        item = values.append_child("item");
        createItem(item, cs->getItemPath(pr, ATTR_TRANSCODING_MIMETYPE_FILTER, ATTR_TRANSCODING_PROFILES_PROFLE_CLIENTFLAGS), cs->option, ATTR_TRANSCODING_PROFILES_PROFLE_CLIENTFLAGS, cs);
        setValue(item, ClientConfig::mapFlags(filter->getClientFlags()));

        if (filter->getTranscodingProfile())
            profiles[filter->getTranscodingProfile()->getName()] = filter->getTranscodingProfile();

        pr++;
    }

    pr = 0;
    // write profile list
    // profiles can only be exported when linked to at least one filter
    for (auto&& [name, entry] : profiles) {
        auto item = values.append_child("item");
        createItem(item, cs->getItemPath(pr, ATTR_TRANSCODING_PROFILES_PROFLE, ATTR_TRANSCODING_PROFILES_PROFLE_NAME), cs->option, ATTR_TRANSCODING_PROFILES_PROFLE_NAME);
        setValue(item, name);

        item = values.append_child("item");
        createItem(item, cs->getItemPath(pr, ATTR_TRANSCODING_PROFILES_PROFLE, ATTR_TRANSCODING_PROFILES_PROFLE_CLIENTFLAGS), cs->option, ATTR_TRANSCODING_PROFILES_PROFLE_CLIENTFLAGS);
        setValue(item, ClientConfig::mapFlags(entry->getClientFlags()));

        item = values.append_child("item");
        createItem(item, cs->getItemPath(pr, ATTR_TRANSCODING_PROFILES_PROFLE, ATTR_TRANSCODING_PROFILES_PROFLE_ENABLED), cs->option, ATTR_TRANSCODING_PROFILES_PROFLE_ENABLED);
        setValue(item, entry->isEnabled());

        item = values.append_child("item");
        auto ttEnumSetup = ConfigDefinition::findConfigSetup<ConfigEnumSetup<TranscodingType>>(ATTR_TRANSCODING_PROFILES_PROFLE_TYPE);
        createItem(item, cs->getItemPath(pr, ATTR_TRANSCODING_PROFILES_PROFLE, ATTR_TRANSCODING_PROFILES_PROFLE_TYPE), cs->option, ATTR_TRANSCODING_PROFILES_PROFLE_TYPE);
        setValue(item, ttEnumSetup->mapEnumValue(entry->getType()));

        item = values.append_child("item");
        createItem(item, cs->getItemPath(pr, ATTR_TRANSCODING_PROFILES_PROFLE, ATTR_TRANSCODING_PROFILES_PROFLE_DLNAPROF), cs->option, ATTR_TRANSCODING_PROFILES_PROFLE_DLNAPROF);
        setValue(item, entry->getDlnaProfile());

        item = values.append_child("item");
        createItem(item, cs->getItemPath(pr, ATTR_TRANSCODING_PROFILES_PROFLE, ATTR_TRANSCODING_PROFILES_PROFLE_MIMETYPE), cs->option, ATTR_TRANSCODING_PROFILES_PROFLE_MIMETYPE);
        setValue(item, entry->getTargetMimeType());

        item = values.append_child("item");
        createItem(item, cs->getItemPath(pr, ATTR_TRANSCODING_PROFILES_PROFLE, ATTR_TRANSCODING_PROFILES_PROFLE_RES), cs->option, ATTR_TRANSCODING_PROFILES_PROFLE_RES);
        setValue(item, entry->getAttributeOverride(ResourceAttribute::RESOLUTION));

        item = values.append_child("item");
        createItem(item, cs->getItemPath(pr, ATTR_TRANSCODING_PROFILES_PROFLE, ATTR_TRANSCODING_PROFILES_PROFLE_ACCURL), cs->option, ATTR_TRANSCODING_PROFILES_PROFLE_ACCURL);
        setValue(item, entry->getAcceptURL());

        item = values.append_child("item");
        createItem(item, cs->getItemPath(pr, ATTR_TRANSCODING_PROFILES_PROFLE, ATTR_TRANSCODING_PROFILES_PROFLE_SAMPFREQ), cs->option, ATTR_TRANSCODING_PROFILES_PROFLE_SAMPFREQ);
        setValue(item, entry->getSampleFreq());

        item = values.append_child("item");
        createItem(item, cs->getItemPath(pr, ATTR_TRANSCODING_PROFILES_PROFLE, ATTR_TRANSCODING_PROFILES_PROFLE_NRCHAN), cs->option, ATTR_TRANSCODING_PROFILES_PROFLE_NRCHAN);
        setValue(item, entry->getNumChannels());

        item = values.append_child("item");
        createItem(item, cs->getItemPath(pr, ATTR_TRANSCODING_PROFILES_PROFLE, ATTR_TRANSCODING_PROFILES_PROFLE_HIDEORIG), cs->option, ATTR_TRANSCODING_PROFILES_PROFLE_HIDEORIG);
        setValue(item, entry->hideOriginalResource());

        item = values.append_child("item");
        createItem(item, cs->getItemPath(pr, ATTR_TRANSCODING_PROFILES_PROFLE, ATTR_TRANSCODING_PROFILES_PROFLE_THUMB), cs->option, ATTR_TRANSCODING_PROFILES_PROFLE_THUMB);
        setValue(item, entry->isThumbnail());

        item = values.append_child("item");
        createItem(item, cs->getItemPath(pr, ATTR_TRANSCODING_PROFILES_PROFLE, ATTR_TRANSCODING_PROFILES_PROFLE_FIRST), cs->option, ATTR_TRANSCODING_PROFILES_PROFLE_FIRST);
        setValue(item, entry->getFirstResource());

        item = values.append_child("item");
        createItem(item, cs->getItemPath(pr, ATTR_TRANSCODING_PROFILES_PROFLE, ATTR_TRANSCODING_PROFILES_PROFLE_ACCOGG), cs->option, ATTR_TRANSCODING_PROFILES_PROFLE_ACCOGG);
        setValue(item, entry->isTheora());

        item = values.append_child("item");
        createItem(item, cs->getItemPath(pr, ATTR_TRANSCODING_PROFILES_PROFLE, ATTR_TRANSCODING_PROFILES_PROFLE_AGENT, ATTR_TRANSCODING_PROFILES_PROFLE_AGENT_COMMAND), cs->option, ATTR_TRANSCODING_PROFILES_PROFLE_AGENT_COMMAND);
        setValue(item, entry->getCommand());

        item = values.append_child("item");
        createItem(item, cs->getItemPath(pr, ATTR_TRANSCODING_PROFILES_PROFLE, ATTR_TRANSCODING_PROFILES_PROFLE_AGENT, ATTR_TRANSCODING_PROFILES_PROFLE_AGENT_ARGS), cs->option, ATTR_TRANSCODING_PROFILES_PROFLE_AGENT_ARGS);
        setValue(item, entry->getArguments());

        item = values.append_child("item");
        createItem(item, cs->getItemPath(pr, ATTR_TRANSCODING_PROFILES_PROFLE, ATTR_TRANSCODING_PROFILES_PROFLE_BUFFER, ATTR_TRANSCODING_PROFILES_PROFLE_BUFFER_SIZE), cs->option, ATTR_TRANSCODING_PROFILES_PROFLE_BUFFER_SIZE);
        setValue(item, entry->getBufferSize());

        item = values.append_child("item");
        createItem(item, cs->getItemPath(pr, ATTR_TRANSCODING_PROFILES_PROFLE, ATTR_TRANSCODING_PROFILES_PROFLE_BUFFER, ATTR_TRANSCODING_PROFILES_PROFLE_BUFFER_CHUNK), cs->option, ATTR_TRANSCODING_PROFILES_PROFLE_BUFFER_CHUNK);
        setValue(item, entry->getBufferChunkSize());

        item = values.append_child("item");
        createItem(item, cs->getItemPath(pr, ATTR_TRANSCODING_PROFILES_PROFLE, ATTR_TRANSCODING_PROFILES_PROFLE_BUFFER, ATTR_TRANSCODING_PROFILES_PROFLE_BUFFER_FILL), cs->option, ATTR_TRANSCODING_PROFILES_PROFLE_BUFFER_FILL);
        setValue(item, entry->getBufferInitialFillSize());

        auto fourCCMode = entry->getAVIFourCCListMode();
        if (fourCCMode != AviFourccListmode::None) {
            item = values.append_child("item");
            createItem(item, cs->getItemPath(pr, ATTR_TRANSCODING_PROFILES_PROFLE, ATTR_TRANSCODING_PROFILES_PROFLE_AVI4CC, ATTR_TRANSCODING_PROFILES_PROFLE_AVI4CC_MODE), cs->option, ATTR_TRANSCODING_PROFILES_PROFLE_AVI4CC_MODE);
            auto fccEnumSetup = ConfigDefinition::findConfigSetup<ConfigEnumSetup<AviFourccListmode>>(ATTR_TRANSCODING_PROFILES_PROFLE_AVI4CC_MODE);
            setValue(item, fccEnumSetup->mapEnumValue(fourCCMode));

            const auto& fourCCList = entry->getAVIFourCCList();
            if (!fourCCList.empty()) {
                item = values.append_child("item");
                createItem(item, cs->getItemPath(pr, ATTR_TRANSCODING_PROFILES_PROFLE, ATTR_TRANSCODING_PROFILES_PROFLE_AVI4CC, ATTR_TRANSCODING_PROFILES_PROFLE_AVI4CC_4CC), cs->option, ATTR_TRANSCODING_PROFILES_PROFLE_AVI4CC_4CC);
                setValue(item, std::accumulate(next(fourCCList.begin()), fourCCList.end(), fourCCList[0], [](auto&& a, auto&& b) { return fmt::format("{}, {}", a, b); }));
            }
        }
        pr++;
    }
}

/// \brief: write autoscan configuration
void Web::ConfigLoad::writeAutoscan(pugi::xml_node& values)
{
    for (auto&& ascs : ConfigDefinition::getConfigSetupList<ConfigAutoscanSetup>()) {
        auto autoscan = ascs->getValue()->getAutoscanListOption();
        for (std::size_t i = 0; i < autoscan.size(); i++) {
            auto&& entry = autoscan.at(i);
            auto&& adir = content->getAutoscanDirectory(entry->getLocation());
            auto item = values.append_child("item");
            createItem(item, ascs->getItemPath(i, ATTR_AUTOSCAN_DIRECTORY_LOCATION), ascs->option, ATTR_AUTOSCAN_DIRECTORY_LOCATION);
            setValue(item, adir->getLocation());

            item = values.append_child("item");
            createItem(item, ascs->getItemPath(i, ATTR_AUTOSCAN_DIRECTORY_MODE), ascs->option, ATTR_AUTOSCAN_DIRECTORY_MODE);
            setValue(item, AutoscanDirectory::mapScanmode(adir->getScanMode()));

            item = values.append_child("item");
            createItem(item, ascs->getItemPath(i, ATTR_AUTOSCAN_DIRECTORY_INTERVAL), ascs->option, ATTR_AUTOSCAN_DIRECTORY_INTERVAL);
            setValue(item, adir->getInterval().count());

            item = values.append_child("item");
            createItem(item, ascs->getItemPath(i, ATTR_AUTOSCAN_DIRECTORY_RECURSIVE), ascs->option, ATTR_AUTOSCAN_DIRECTORY_RECURSIVE);
            setValue(item, adir->getRecursive());

            item = values.append_child("item");
            createItem(item, ascs->getItemPath(i, ATTR_AUTOSCAN_DIRECTORY_MEDIATYPE), ascs->option, ATTR_AUTOSCAN_DIRECTORY_MEDIATYPE);
            setValue(item, AutoscanDirectory::mapMediaType(adir->getMediaType()));

            item = values.append_child("item");
            createItem(item, ascs->getItemPath(i, ATTR_AUTOSCAN_DIRECTORY_HIDDENFILES), ascs->option, ATTR_AUTOSCAN_DIRECTORY_HIDDENFILES);
            setValue(item, adir->getHidden());

            item = values.append_child("item");
            createItem(item, ascs->getItemPath(i, ATTR_AUTOSCAN_DIRECTORY_FOLLOWSYMLINKS), ascs->option, ATTR_AUTOSCAN_DIRECTORY_FOLLOWSYMLINKS);
            setValue(item, adir->getFollowSymlinks());

            item = values.append_child("item");
            createItem(item, ascs->getItemPath(i, ATTR_AUTOSCAN_CONTAINER_TYPE_AUDIO), ascs->option, ATTR_AUTOSCAN_CONTAINER_TYPE_AUDIO);
            setValue(item, adir->getContainerTypes().at(AutoscanMediaMode::Audio));

            item = values.append_child("item");
            createItem(item, ascs->getItemPath(i, ATTR_AUTOSCAN_CONTAINER_TYPE_IMAGE), ascs->option, ATTR_AUTOSCAN_CONTAINER_TYPE_IMAGE);
            setValue(item, adir->getContainerTypes().at(AutoscanMediaMode::Image));

            item = values.append_child("item");
            createItem(item, ascs->getItemPath(i, ATTR_AUTOSCAN_CONTAINER_TYPE_VIDEO), ascs->option, ATTR_AUTOSCAN_CONTAINER_TYPE_VIDEO);
            setValue(item, adir->getContainerTypes().at(AutoscanMediaMode::Video));

            item = values.append_child("item");
            createItem(item, ascs->getItemPath(i, ATTR_AUTOSCAN_DIRECTORY_SCANCOUNT), ascs->option, ATTR_AUTOSCAN_DIRECTORY_SCANCOUNT);
            setValue(item, adir->getActiveScanCount());

            item = values.append_child("item");
            createItem(item, ascs->getItemPath(i, ATTR_AUTOSCAN_DIRECTORY_TASKCOUNT), ascs->option, ATTR_AUTOSCAN_DIRECTORY_TASKCOUNT);
            setValue(item, adir->getTaskCount());

            item = values.append_child("item");
            createItem(item, ascs->getItemPath(i, ATTR_AUTOSCAN_DIRECTORY_LMT), ascs->option, ATTR_AUTOSCAN_DIRECTORY_LMT);
            setValue(item, fmt::format("{:%Y-%m-%d %H:%M:%S}", fmt::localtime(adir->getPreviousLMT().count())));
        }
        if (autoscan.empty()) {
            auto item = values.append_child("item");
            createItem(item, ascs->getItemPath(ITEM_PATH_NEW, ATTR_AUTOSCAN_DIRECTORY_LOCATION), ascs->option, ATTR_AUTOSCAN_DIRECTORY_LOCATION, ConfigDefinition::findConfigSetup(ATTR_AUTOSCAN_DIRECTORY_LOCATION));

            item = values.append_child("item");
            createItem(item, ascs->getItemPath(ITEM_PATH_NEW, ATTR_AUTOSCAN_DIRECTORY_MODE), ascs->option, ATTR_AUTOSCAN_DIRECTORY_MODE, ConfigDefinition::findConfigSetup(ATTR_AUTOSCAN_DIRECTORY_MODE));

            item = values.append_child("item");
            createItem(item, ascs->getItemPath(ITEM_PATH_NEW, ATTR_AUTOSCAN_DIRECTORY_INTERVAL), ascs->option, ATTR_AUTOSCAN_DIRECTORY_INTERVAL, ConfigDefinition::findConfigSetup(ATTR_AUTOSCAN_DIRECTORY_INTERVAL));

            item = values.append_child("item");
            createItem(item, ascs->getItemPath(ITEM_PATH_NEW, ATTR_AUTOSCAN_DIRECTORY_RECURSIVE), ascs->option, ATTR_AUTOSCAN_DIRECTORY_RECURSIVE, ConfigDefinition::findConfigSetup(ATTR_AUTOSCAN_DIRECTORY_RECURSIVE));

            item = values.append_child("item");
            createItem(item, ascs->getItemPath(ITEM_PATH_NEW, ATTR_AUTOSCAN_DIRECTORY_MEDIATYPE), ascs->option, ATTR_AUTOSCAN_DIRECTORY_MEDIATYPE, ConfigDefinition::findConfigSetup(ATTR_AUTOSCAN_DIRECTORY_MEDIATYPE));

            item = values.append_child("item");
            createItem(item, ascs->getItemPath(ITEM_PATH_NEW, ATTR_AUTOSCAN_DIRECTORY_HIDDENFILES), ascs->option, ATTR_AUTOSCAN_DIRECTORY_HIDDENFILES, ConfigDefinition::findConfigSetup(ATTR_AUTOSCAN_DIRECTORY_HIDDENFILES));

            item = values.append_child("item");
            createItem(item, ascs->getItemPath(ITEM_PATH_NEW, ATTR_AUTOSCAN_CONTAINER_TYPE_AUDIO), ascs->option, ATTR_AUTOSCAN_CONTAINER_TYPE_AUDIO, ConfigDefinition::findConfigSetup(ATTR_AUTOSCAN_CONTAINER_TYPE_AUDIO));

            item = values.append_child("item");
            createItem(item, ascs->getItemPath(ITEM_PATH_NEW, ATTR_AUTOSCAN_CONTAINER_TYPE_IMAGE), ascs->option, ATTR_AUTOSCAN_CONTAINER_TYPE_IMAGE, ConfigDefinition::findConfigSetup(ATTR_AUTOSCAN_CONTAINER_TYPE_IMAGE));

            item = values.append_child("item");
            createItem(item, ascs->getItemPath(ITEM_PATH_NEW, ATTR_AUTOSCAN_CONTAINER_TYPE_VIDEO), ascs->option, ATTR_AUTOSCAN_CONTAINER_TYPE_VIDEO, ConfigDefinition::findConfigSetup(ATTR_AUTOSCAN_CONTAINER_TYPE_VIDEO));

            item = values.append_child("item");
            createItem(item, ascs->getItemPath(ITEM_PATH_NEW, ATTR_AUTOSCAN_DIRECTORY_SCANCOUNT), ascs->option, ATTR_AUTOSCAN_DIRECTORY_SCANCOUNT, ConfigDefinition::findConfigSetup(ATTR_AUTOSCAN_DIRECTORY_SCANCOUNT));

            item = values.append_child("item");
            createItem(item, ascs->getItemPath(ITEM_PATH_NEW, ATTR_AUTOSCAN_DIRECTORY_TASKCOUNT), ascs->option, ATTR_AUTOSCAN_DIRECTORY_TASKCOUNT, ConfigDefinition::findConfigSetup(ATTR_AUTOSCAN_DIRECTORY_TASKCOUNT));

            item = values.append_child("item");
            createItem(item, ascs->getItemPath(ITEM_PATH_NEW, ATTR_AUTOSCAN_DIRECTORY_LMT), ascs->option, ATTR_AUTOSCAN_DIRECTORY_LMT, ConfigDefinition::findConfigSetup(ATTR_AUTOSCAN_DIRECTORY_LMT));
        }
    }
}

/// \brief: write content of all dictionaries
void Web::ConfigLoad::writeDictionaries(pugi::xml_node& values)
{
    for (auto&& dcs : ConfigDefinition::getConfigSetupList<ConfigDictionarySetup>()) {
        int i = 0;
        auto dictionary = dcs->getValue()->getDictionaryOption(true);
        for (auto&& [key, val] : dictionary) {
            auto item = values.append_child("item");
            createItem(item, dcs->getItemPath(i, dcs->keyOption), dcs->option, dcs->keyOption, dcs);
            setValue(item, key.substr(5));

            item = values.append_child("item");
            createItem(item, dcs->getItemPath(i, dcs->valOption), dcs->option, dcs->valOption, dcs);
            setValue(item, val);
            i++;
        }
    }
}

/// \brief: write content of all vectors
void Web::ConfigLoad::writeVectors(pugi::xml_node& values)
{
    for (auto&& vcs : ConfigDefinition::getConfigSetupList<ConfigVectorSetup>()) {
        int i = 0;
        auto vectorOption = vcs->getValue()->getVectorOption(true);
        std::vector<std::string> attrList;
        attrList.reserve(vcs->optionList.size());
        for (const auto& opt : vcs->optionList) {
            attrList.push_back(ConfigDefinition::removeAttribute(opt));
        }
        for (auto&& vector : vectorOption) {
            for (auto&& [key, val] : vector) {
                auto item = values.append_child("item");
                int j = 0;
                auto pos = std::find(attrList.begin(), attrList.end(), key);
                if (pos != attrList.end())
                    j = std::distance(attrList.begin(), pos);
                createItem(item, vcs->getItemPath(i, key), vcs->option, vcs->optionList[j], vcs);
                setValue(item, val);
            }
            i++;
        }
    }
}

/// \brief: write content of all arrays
void Web::ConfigLoad::writeArrays(pugi::xml_node& values)
{
    for (auto&& acs : ConfigDefinition::getConfigSetupList<ConfigArraySetup>()) {
        auto array = acs->getValue()->getArrayOption(true);
        for (std::size_t i = 0; i < array.size(); i++) {
            auto&& entry = array[i];
            auto item = values.append_child("item");
            createItem(item, acs->getItemPath(i), acs->option, acs->attrOption != CFG_MAX ? acs->attrOption : acs->nodeOption, acs);
            setValue(item, entry);
        }
    }
}

/// \brief: update entries with datebase values
void Web::ConfigLoad::updateEntriesFromDatabase(pugi::xml_node& root, pugi::xml_node& values)
{
    for (auto&& entry : dbEntries) {
        auto exItem = allItems.find(entry.item);
        if (exItem != allItems.end()) {
            auto item = root.select_node(exItem->second.c_str()).node();
            item.attribute("source") = "database";
            item.attribute("status") = entry.status.c_str();
        } else {
            auto cs = ConfigDefinition::findConfigSetupByPath(entry.item, true);
            auto acs = ConfigDefinition::findConfigSetupByPath(entry.item, true, cs);
            if (cs) {
                auto item = values.append_child("item");
                createItem(item, entry.item, cs->option, acs ? acs->option : CFG_MAX);
                setValue(item, entry.value);
                item.attribute("status") = entry.status.c_str();
                item.attribute("origValue") = config->getOrigValue(entry.item).c_str();
                item.attribute("source") = "database";
            }
        }
    }
}
