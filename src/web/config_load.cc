/*GRB*

    Gerbera - https://gerbera.io/

    config_load.cc - this file is part of Gerbera.

    Copyright (C) 2020-2021 Gerbera Contributors

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

#include <fmt/chrono.h>
#include <numeric>

#include "pages.h" // API

#include "autoscan.h"
#include "config/client_config.h"
#include "config/config_setup.h"
#include "config/directory_tweak.h"
#include "content_manager.h"
#include "database/database.h"
#include "metadata/metadata_handler.h"
#include "transcoding/transcoding.h"
#include "upnp_xml.h"
#include "util/upnp_clients.h"

web::configLoad::configLoad(std::shared_ptr<ContentManager> content)
    : WebRequestHandler(std::move(content))
{
    try {
        if (this->database != nullptr) {
            dbEntries = this->database->getConfigValues();
        } else {
            log_error("configLoad database missing");
        }
    } catch (const std::runtime_error& e) {
        log_error("configLoad {}", e.what());
    }
}

void web::configLoad::addTypeMeta(pugi::xml_node& meta, const std::shared_ptr<ConfigSetup>& cs)
{
    auto info = meta.append_child("item");
    info.append_attribute("item") = cs->getUniquePath().c_str();
    info.append_attribute("id") = fmt::format("{}", cs->option).c_str();
    info.append_attribute("type") = cs->getTypeString().c_str();
    info.append_attribute("value") = cs->getDefaultValue().c_str();
    info.append_attribute("help") = cs->getHelp();
}

void web::configLoad::createItem(pugi::xml_node& item, const std::string& name, config_option_t id, config_option_t aid)
{
    allItems[name] = &item;
    item.append_attribute("item") = name.c_str();
    item.append_attribute("id") = fmt::format("{:02d}", id).c_str();
    item.append_attribute("aid") = fmt::format("{:02d}", aid).c_str();
    item.append_attribute("status") = "unchanged";

    item.append_attribute("origValue") = config->getOrigValue(name).c_str();
    item.append_attribute("source") = std::any_of(dbEntries.begin(), dbEntries.end(), [&](const auto& s) { return s.item == name; }) ? "database" : "config.xml";
}

template <typename T>
void web::configLoad::setValue(pugi::xml_node& item, const T& value)
{
    static_assert(fmt::has_formatter<T, fmt::format_context>::value, "T must be formattable");
    item.append_attribute("value") = fmt::format("{}", value).c_str();
}

template <>
void web::configLoad::setValue(pugi::xml_node& item, const std::string& value)
{
    item.append_attribute("value") = value.c_str();
}

template <>
void web::configLoad::setValue(pugi::xml_node& item, const fs::path& value)
{
    item.append_attribute("value") = value.c_str();
}

void web::configLoad::process()
{
    check_request();
    auto root = xmlDoc->document_element();
    auto values = root.append_child("values");

    xml2JsonHints->setArrayName(values, "item");
    xml2JsonHints->setFieldType("item", "string");
    xml2JsonHints->setFieldType("id", "string");
    xml2JsonHints->setFieldType("aid", "string");
    xml2JsonHints->setFieldType("value", "string");
    xml2JsonHints->setFieldType("origValue", "string");

    log_debug("Sending Config to web!");

    auto meta = root.append_child("types");
    xml2JsonHints->setArrayName(meta, "item");
    for (const auto& cs : ConfigManager::getOptionList()) {
        addTypeMeta(meta, cs);
    }

    {
        auto item = values.append_child("item");
        createItem(item, "/status/attribute::total", CFG_MAX, CFG_MAX);
        setValue(item, database->getTotalFiles());
        item = values.append_child("item");
        createItem(item, "/status/attribute::virtual", CFG_MAX, CFG_MAX);
        setValue(item, database->getTotalFiles(true));

        item = values.append_child("item");
        createItem(item, "/status/attribute::audio", CFG_MAX, CFG_MAX);
        setValue(item, database->getTotalFiles(false, "audio"));
        item = values.append_child("item");
        createItem(item, "/status/attribute::video", CFG_MAX, CFG_MAX);
        setValue(item, database->getTotalFiles(false, "video"));
        item = values.append_child("item");
        createItem(item, "/status/attribute::image", CFG_MAX, CFG_MAX);
        setValue(item, database->getTotalFiles(false, "image"));

        item = values.append_child("item");
        createItem(item, "/status/attribute::audioVirtual", CFG_MAX, CFG_MAX);
        setValue(item, database->getTotalFiles(true, "audio"));
        item = values.append_child("item");
        createItem(item, "/status/attribute::videoVirtual", CFG_MAX, CFG_MAX);
        setValue(item, database->getTotalFiles(true, "video"));
        item = values.append_child("item");
        createItem(item, "/status/attribute::imageVirtual", CFG_MAX, CFG_MAX);
        setValue(item, database->getTotalFiles(true, "image"));
    }

    for (int i = 0; i < static_cast<int>(CFG_MAX); i++) {
        auto scs = ConfigManager::findConfigSetup(static_cast<config_option_t>(i));
        auto item = values.append_child("item");
        createItem(item, scs->getItemPath(-1), static_cast<config_option_t>(i), static_cast<config_option_t>(i));

        try {
            log_debug("    Option {:03d} {} = {}", i, scs->getItemPath(), scs->getCurrentValue().c_str());
            setValue(item, scs->getCurrentValue());
        } catch (const std::runtime_error& e) {
        }
    }

    std::shared_ptr<ConfigSetup> cs;
    cs = ConfigManager::findConfigSetup(CFG_CLIENTS_LIST);
    auto clientConfig = cs->getValue()->getClientConfigListOption();
    for (size_t i = 0; i < clientConfig->size(); i++) {
        auto client = clientConfig->get(i);

        auto item = values.append_child("item");
        createItem(item, cs->getItemPath(i, ATTR_CLIENTS_CLIENT_FLAGS), cs->option, ATTR_CLIENTS_CLIENT_FLAGS);
        setValue(item, ClientConfig::mapFlags(client->getFlags()));

        item = values.append_child("item");
        createItem(item, cs->getItemPath(i, ATTR_CLIENTS_CLIENT_IP), cs->option, ATTR_CLIENTS_CLIENT_IP);
        setValue(item, client->getIp());

        item = values.append_child("item");
        createItem(item, cs->getItemPath(i, ATTR_CLIENTS_CLIENT_USERAGENT), cs->option, ATTR_CLIENTS_CLIENT_USERAGENT);
        setValue(item, client->getUserAgent());
    }

    cs = ConfigManager::findConfigSetup(CFG_IMPORT_DIRECTORIES_LIST);
    auto directoryConfig = cs->getValue()->getDirectoryTweakOption();
    for (size_t i = 0; i < directoryConfig->size(); i++) {
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
        createItem(item, cs->getItemPath(i, ATTR_DIRECTORIES_TWEAK_SUBTILTE_FILE), cs->option, ATTR_DIRECTORIES_TWEAK_SUBTILTE_FILE);
        setValue(item, dir->hasSubTitleFile() ? dir->getSubTitleFile() : "");
    }

    cs = ConfigManager::findConfigSetup(CFG_TRANSCODING_PROFILE_LIST);
    auto transcoding = cs->getValue()->getTranscodingProfileListOption();
    int pr = 0;
    std::map<std::string, int> profiles;
    for (const auto& [key, val] : transcoding->getList()) {
        for (const auto& [a, name] : *val) {
            auto item = values.append_child("item");
            createItem(item, cs->getItemPath(pr, ATTR_TRANSCODING_MIMETYPE_PROF_MAP, ATTR_TRANSCODING_MIMETYPE_PROF_MAP_TRANSCODE, ATTR_TRANSCODING_MIMETYPE_PROF_MAP_MIMETYPE), cs->option, ATTR_TRANSCODING_MIMETYPE_PROF_MAP_MIMETYPE);
            setValue(item, key);

            item = values.append_child("item");
            createItem(item, cs->getItemPath(pr, ATTR_TRANSCODING_MIMETYPE_PROF_MAP, ATTR_TRANSCODING_MIMETYPE_PROF_MAP_TRANSCODE, ATTR_TRANSCODING_MIMETYPE_PROF_MAP_USING), cs->option, ATTR_TRANSCODING_MIMETYPE_PROF_MAP_USING);
            setValue(item, name->getName());
            profiles[name->getName()] = pr;

            pr++;
        }
    }

    pr = 0;
    for (const auto& [key, val] : profiles) {
        auto entry = transcoding->getByName(key, true);
        auto item = values.append_child("item");
        createItem(item, cs->getItemPath(pr, ATTR_TRANSCODING_PROFILES, ATTR_TRANSCODING_PROFILES_PROFLE, ATTR_TRANSCODING_PROFILES_PROFLE_NAME), cs->option, ATTR_TRANSCODING_PROFILES_PROFLE_NAME);
        setValue(item, entry->getName());

        item = values.append_child("item");
        createItem(item, cs->getItemPath(pr, ATTR_TRANSCODING_PROFILES, ATTR_TRANSCODING_PROFILES_PROFLE, ATTR_TRANSCODING_PROFILES_PROFLE_ENABLED), cs->option, ATTR_TRANSCODING_PROFILES_PROFLE_ENABLED);
        setValue(item, entry->getEnabled());

        item = values.append_child("item");
        createItem(item, cs->getItemPath(pr, ATTR_TRANSCODING_PROFILES, ATTR_TRANSCODING_PROFILES_PROFLE, ATTR_TRANSCODING_PROFILES_PROFLE_TYPE), cs->option, ATTR_TRANSCODING_PROFILES_PROFLE_TYPE);
        setValue(item, std::string((entry->getType() == TR_External) ? "external" : "none"));

        item = values.append_child("item");
        createItem(item, cs->getItemPath(pr, ATTR_TRANSCODING_PROFILES, ATTR_TRANSCODING_PROFILES_PROFLE, ATTR_TRANSCODING_PROFILES_PROFLE_MIMETYPE), cs->option, ATTR_TRANSCODING_PROFILES_PROFLE_MIMETYPE);
        setValue(item, entry->getTargetMimeType());

        item = values.append_child("item");
        createItem(item, cs->getItemPath(pr, ATTR_TRANSCODING_PROFILES, ATTR_TRANSCODING_PROFILES_PROFLE, ATTR_TRANSCODING_PROFILES_PROFLE_RES), cs->option, ATTR_TRANSCODING_PROFILES_PROFLE_RES);
        setValue(item, entry->getAttributes()[MetadataHandler::getResAttrName(R_RESOLUTION)]);

        item = values.append_child("item");
        createItem(item, cs->getItemPath(pr, ATTR_TRANSCODING_PROFILES, ATTR_TRANSCODING_PROFILES_PROFLE, ATTR_TRANSCODING_PROFILES_PROFLE_ACCURL), cs->option, ATTR_TRANSCODING_PROFILES_PROFLE_ACCURL);
        setValue(item, entry->acceptURL());

        item = values.append_child("item");
        createItem(item, cs->getItemPath(pr, ATTR_TRANSCODING_PROFILES, ATTR_TRANSCODING_PROFILES_PROFLE, ATTR_TRANSCODING_PROFILES_PROFLE_SAMPFREQ), cs->option, ATTR_TRANSCODING_PROFILES_PROFLE_SAMPFREQ);
        setValue(item, entry->getSampleFreq());

        item = values.append_child("item");
        createItem(item, cs->getItemPath(pr, ATTR_TRANSCODING_PROFILES, ATTR_TRANSCODING_PROFILES_PROFLE, ATTR_TRANSCODING_PROFILES_PROFLE_NRCHAN), cs->option, ATTR_TRANSCODING_PROFILES_PROFLE_NRCHAN);
        setValue(item, entry->getNumChannels());

        item = values.append_child("item");
        createItem(item, cs->getItemPath(pr, ATTR_TRANSCODING_PROFILES, ATTR_TRANSCODING_PROFILES_PROFLE, ATTR_TRANSCODING_PROFILES_PROFLE_HIDEORIG), cs->option, ATTR_TRANSCODING_PROFILES_PROFLE_HIDEORIG);
        setValue(item, entry->hideOriginalResource());

        item = values.append_child("item");
        createItem(item, cs->getItemPath(pr, ATTR_TRANSCODING_PROFILES, ATTR_TRANSCODING_PROFILES_PROFLE, ATTR_TRANSCODING_PROFILES_PROFLE_THUMB), cs->option, ATTR_TRANSCODING_PROFILES_PROFLE_THUMB);
        setValue(item, entry->isThumbnail());

        item = values.append_child("item");
        createItem(item, cs->getItemPath(pr, ATTR_TRANSCODING_PROFILES, ATTR_TRANSCODING_PROFILES_PROFLE, ATTR_TRANSCODING_PROFILES_PROFLE_FIRST), cs->option, ATTR_TRANSCODING_PROFILES_PROFLE_FIRST);
        setValue(item, entry->firstResource());

        item = values.append_child("item");
        createItem(item, cs->getItemPath(pr, ATTR_TRANSCODING_PROFILES, ATTR_TRANSCODING_PROFILES_PROFLE, ATTR_TRANSCODING_PROFILES_PROFLE_ACCOGG), cs->option, ATTR_TRANSCODING_PROFILES_PROFLE_ACCOGG);
        setValue(item, entry->isTheora());

        item = values.append_child("item");
        createItem(item, cs->getItemPath(pr, ATTR_TRANSCODING_PROFILES, ATTR_TRANSCODING_PROFILES_PROFLE, ATTR_TRANSCODING_PROFILES_PROFLE_USECHUNKEDENC), cs->option, ATTR_TRANSCODING_PROFILES_PROFLE_USECHUNKEDENC);
        setValue(item, entry->getChunked());

        item = values.append_child("item");
        createItem(item, cs->getItemPath(pr, ATTR_TRANSCODING_PROFILES, ATTR_TRANSCODING_PROFILES_PROFLE, ATTR_TRANSCODING_PROFILES_PROFLE_AGENT, ATTR_TRANSCODING_PROFILES_PROFLE_AGENT_COMMAND), cs->option, ATTR_TRANSCODING_PROFILES_PROFLE_AGENT_COMMAND);
        setValue(item, entry->getCommand());

        item = values.append_child("item");
        createItem(item, cs->getItemPath(pr, ATTR_TRANSCODING_PROFILES, ATTR_TRANSCODING_PROFILES_PROFLE, ATTR_TRANSCODING_PROFILES_PROFLE_AGENT, ATTR_TRANSCODING_PROFILES_PROFLE_AGENT_ARGS), cs->option, ATTR_TRANSCODING_PROFILES_PROFLE_AGENT_ARGS);
        setValue(item, entry->getArguments());

        item = values.append_child("item");
        createItem(item, cs->getItemPath(pr, ATTR_TRANSCODING_PROFILES, ATTR_TRANSCODING_PROFILES_PROFLE, ATTR_TRANSCODING_PROFILES_PROFLE_BUFFER, ATTR_TRANSCODING_PROFILES_PROFLE_BUFFER_SIZE), cs->option, ATTR_TRANSCODING_PROFILES_PROFLE_BUFFER_SIZE);
        setValue(item, entry->getBufferSize());

        item = values.append_child("item");
        createItem(item, cs->getItemPath(pr, ATTR_TRANSCODING_PROFILES, ATTR_TRANSCODING_PROFILES_PROFLE, ATTR_TRANSCODING_PROFILES_PROFLE_BUFFER, ATTR_TRANSCODING_PROFILES_PROFLE_BUFFER_CHUNK), cs->option, ATTR_TRANSCODING_PROFILES_PROFLE_BUFFER_CHUNK);
        setValue(item, entry->getBufferChunkSize());

        item = values.append_child("item");
        createItem(item, cs->getItemPath(pr, ATTR_TRANSCODING_PROFILES, ATTR_TRANSCODING_PROFILES_PROFLE, ATTR_TRANSCODING_PROFILES_PROFLE_BUFFER, ATTR_TRANSCODING_PROFILES_PROFLE_BUFFER_FILL), cs->option, ATTR_TRANSCODING_PROFILES_PROFLE_BUFFER_FILL);
        setValue(item, entry->getBufferInitialFillSize());

        auto fourCCMode = entry->getAVIFourCCListMode();
        if (fourCCMode != FCC_None) {
            item = values.append_child("item");
            createItem(item, cs->getItemPath(pr, ATTR_TRANSCODING_PROFILES, ATTR_TRANSCODING_PROFILES_PROFLE, ATTR_TRANSCODING_PROFILES_PROFLE_AVI4CC, ATTR_TRANSCODING_PROFILES_PROFLE_AVI4CC_MODE), cs->option, ATTR_TRANSCODING_PROFILES_PROFLE_AVI4CC_MODE);
            setValue(item, TranscodingProfile::mapFourCcMode(fourCCMode));

            const auto fourCCList = entry->getAVIFourCCList();
            if (!fourCCList.empty()) {
                item = values.append_child("item");
                createItem(item, cs->getItemPath(pr, ATTR_TRANSCODING_PROFILES, ATTR_TRANSCODING_PROFILES_PROFLE, ATTR_TRANSCODING_PROFILES_PROFLE_AVI4CC, ATTR_TRANSCODING_PROFILES_PROFLE_AVI4CC_4CC), cs->option, ATTR_TRANSCODING_PROFILES_PROFLE_AVI4CC_4CC);
                setValue(item, std::accumulate(std::next(fourCCList.begin()), fourCCList.end(), fourCCList[0], [](const std::string& a, const std::string& b) { return a + ", " + b; }));
            }
        }
        pr++;
    }

#ifdef HAVE_INOTIFY
    constexpr auto autoscanList = std::array<config_option_t, 2> { CFG_IMPORT_AUTOSCAN_TIMED_LIST, CFG_IMPORT_AUTOSCAN_INOTIFY_LIST };
#else
    constexpr auto autoscanList = std::array<config_option_t, 1> { CFG_IMPORT_AUTOSCAN_TIMED_LIST };
#endif
    for (const auto& autoscanOption : autoscanList) {
        cs = ConfigManager::findConfigSetup(autoscanOption);
        auto autoscan = cs->getValue()->getAutoscanListOption();
        for (size_t i = 0; i < autoscan->size(); i++) {
            const auto& entry = autoscan->get(i);
            const auto& adir = content->getAutoscanDirectory(entry->getLocation());
            auto item = values.append_child("item");
            createItem(item, cs->getItemPath(i, ATTR_AUTOSCAN_DIRECTORY_LOCATION), cs->option, ATTR_AUTOSCAN_DIRECTORY_LOCATION);
            setValue(item, adir->getLocation());

            item = values.append_child("item");
            createItem(item, cs->getItemPath(i, ATTR_AUTOSCAN_DIRECTORY_MODE), cs->option, ATTR_AUTOSCAN_DIRECTORY_MODE);
            setValue(item, AutoscanDirectory::mapScanmode(adir->getScanMode()));

            item = values.append_child("item");
            createItem(item, cs->getItemPath(i, ATTR_AUTOSCAN_DIRECTORY_INTERVAL), cs->option, ATTR_AUTOSCAN_DIRECTORY_INTERVAL);
            setValue(item, adir->getInterval());

            item = values.append_child("item");
            createItem(item, cs->getItemPath(i, ATTR_AUTOSCAN_DIRECTORY_RECURSIVE), cs->option, ATTR_AUTOSCAN_DIRECTORY_RECURSIVE);
            setValue(item, adir->getRecursive());

            item = values.append_child("item");
            createItem(item, cs->getItemPath(i, ATTR_AUTOSCAN_DIRECTORY_HIDDENFILES), cs->option, ATTR_AUTOSCAN_DIRECTORY_HIDDENFILES);
            setValue(item, adir->getHidden());

            item = values.append_child("item");
            createItem(item, cs->getItemPath(i, ATTR_AUTOSCAN_DIRECTORY_SCANCOUNT), cs->option, ATTR_AUTOSCAN_DIRECTORY_SCANCOUNT);
            setValue(item, adir->getActiveScanCount());

            item = values.append_child("item");
            createItem(item, cs->getItemPath(i, ATTR_AUTOSCAN_DIRECTORY_LMT), cs->option, ATTR_AUTOSCAN_DIRECTORY_LMT);
            setValue(item, fmt::format("{:%Y-%m-%d %H:%M:%S}", fmt::localtime(adir->getPreviousLMT(""))));
        }
    }

    constexpr std::array<config_option_t, 5> dict_options {
        CFG_SERVER_UI_ACCOUNT_LIST,
        CFG_IMPORT_MAPPINGS_EXTENSION_TO_MIMETYPE_LIST,
        CFG_IMPORT_MAPPINGS_MIMETYPE_TO_CONTENTTYPE_LIST,
        CFG_IMPORT_MAPPINGS_MIMETYPE_TO_UPNP_CLASS_LIST,
        CFG_IMPORT_LAYOUT_MAPPING,
    };

    for (const auto& dict_option : dict_options) {
        int i = 0;
        auto dcs = ConfigSetup::findConfigSetup<ConfigDictionarySetup>(dict_option);
        auto dictionary = dcs->getValue()->getDictionaryOption(true);
        for (const auto& [key, val] : dictionary) {
            auto item = values.append_child("item");
            createItem(item, dcs->getItemPath(i, dcs->keyOption), dcs->option, dcs->keyOption);
            setValue(item, key.substr(5));

            item = values.append_child("item");
            createItem(item, dcs->getItemPath(i, dcs->valOption), dcs->option, dcs->valOption);
            setValue(item, val);
            i++;
        }
    }

    constexpr config_option_t array_options[] = {
        CFG_SERVER_UI_ITEMS_PER_PAGE_DROPDOWN,
        CFG_IMPORT_RESOURCES_FANART_FILE_LIST,
        CFG_IMPORT_RESOURCES_SUBTITLE_FILE_LIST,
        CFG_IMPORT_RESOURCES_RESOURCE_FILE_LIST,
        CFG_SERVER_EXTOPTS_MARK_PLAYED_ITEMS_CONTENT_LIST,
#ifdef HAVE_LIBEXIF
        CFG_IMPORT_LIBOPTS_EXIF_AUXDATA_TAGS_LIST,
#endif
#ifdef HAVE_EXIV2
        CFG_IMPORT_LIBOPTS_EXIV2_AUXDATA_TAGS_LIST,
#endif
#ifdef HAVE_TAGLIB
        CFG_IMPORT_LIBOPTS_ID3_AUXDATA_TAGS_LIST,
#endif
#ifdef HAVE_FFMPEG
        CFG_IMPORT_LIBOPTS_FFMPEG_AUXDATA_TAGS_LIST,
#endif
    };

    for (auto array_option : array_options) {
        auto acs = ConfigSetup::findConfigSetup<ConfigArraySetup>(array_option);
        auto array = acs->getValue()->getArrayOption(true);
        for (size_t i = 0; i < array.size(); i++) {
            auto entry = array[i];
            auto item = values.append_child("item");
            createItem(item, acs->getItemPath(i), acs->option, acs->attrOption != CFG_MAX ? acs->attrOption : acs->nodeOption);
            setValue(item, entry);
        }
    }

    for (const auto& entry : dbEntries) {
        auto exItem = allItems.find(entry.item);
        if (exItem != allItems.end()) {
            auto item = (*exItem).second;
            item->attribute("source") = "database";
            item->attribute("status") = entry.status.c_str();
        } else {
            auto cs = ConfigManager::findConfigSetupByPath(entry.item, true);
            auto acs = ConfigManager::findConfigSetupByPath(entry.item, true, cs);
            if (cs != nullptr) {
                auto item = values.append_child("item");
                createItem(item, entry.item, cs->option, acs != nullptr ? acs->option : CFG_MAX);
                setValue(item, entry.value);
                item.attribute("status") = entry.status.c_str();
                item.attribute("origValue") = config->getOrigValue(entry.item).c_str();
                item.attribute("source") = "database";
            }
        }
    }
}
