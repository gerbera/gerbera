/*GRB*

    Gerbera - https://gerbera.io/

    config_load.cc - this file is part of Gerbera.

    Copyright (C) 2020 Gerbera Contributors

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

#include <numeric>

#include "pages.h" // API

#include "autoscan.h"
#include "config/client_config.h"
#include "config/config_setup.h"
#include "metadata/metadata_handler.h"
#include "storage/storage.h"
#include "transcoding/transcoding.h"
#include "upnp_xml.h"
#include "util/upnp_clients.h"

web::configLoad::configLoad(std::shared_ptr<Config> config, std::shared_ptr<Storage> storage,
    std::shared_ptr<ContentManager> content, std::shared_ptr<SessionManager> sessionManager)
    : WebRequestHandler(std::move(config), std::move(storage), std::move(content), std::move(sessionManager))
{
    try {
        if (this->storage != nullptr) {
            dbEntries = this->storage->getConfigValues();
        } else {
            log_error("configLoad storage missing");
        }
    } catch (const std::runtime_error& e) {
        log_error("configLoad {}", e.what());
    }
}

void web::configLoad::createItem(pugi::xml_node& item, const std::string& name, config_option_t id)
{
    item.append_attribute("item") = name.c_str();
    item.append_attribute("id") = fmt::format("{:02d}", id).c_str();
    item.append_attribute("status") = "unchanaged";

    item.append_attribute("origValue") = config->getOrigValue(name).c_str();
    item.append_attribute("source") = std::find_if(dbEntries.begin(), dbEntries.end(), [&](const auto& s) { return s.item == name; }) != dbEntries.end() ? "database" : "config.xml";
}

void web::configLoad::process()
{
    check_request();
    auto root = xmlDoc->document_element();
    auto values = root.append_child("values");
    xml2JsonHints->setArrayName(values, "item");
    xml2JsonHints->setFieldType("item", "string");
    xml2JsonHints->setFieldType("id", "string");
    xml2JsonHints->setFieldType("value", "string");
    xml2JsonHints->setFieldType("origValue", "string");

    std::shared_ptr<ConfigSetup> cs;
    log_debug("Sending Config to web!");
    for (int i = 0; i < static_cast<int>(CFG_MAX); i++) {
        auto item = values.append_child("item");
        cs = ConfigManager::findConfigSetup(static_cast<config_option_t>(i));
        createItem(item, cs->getItemPath(-1), static_cast<config_option_t>(i));

        try {
            log_debug("    Option {:03d} {} = {}", i, cs->getItemPath(), cs->getCurrentValue().c_str());
            item.append_attribute("value") = cs->getCurrentValue().c_str();
        } catch (const std::runtime_error& e) {
        }
    }

    auto clientConfig = config->getClientConfigListOption(CFG_CLIENTS_LIST);
    cs = ConfigManager::findConfigSetup(CFG_CLIENTS_LIST);
    for (size_t i = 0; i < clientConfig->size(); i++) {
        auto client = clientConfig->get(i);

        auto item = values.append_child("item");
        createItem(item, cs->getItemPath(i, ATTR_CLIENTS_CLIENT_FLAGS), CFG_CLIENTS_LIST);
        item.append_attribute("value") = ClientConfig::mapFlags(client->getFlags()).c_str();

        item = values.append_child("item");
        createItem(item, cs->getItemPath(i, ATTR_CLIENTS_CLIENT_IP), CFG_CLIENTS_LIST);
        item.append_attribute("value") = client->getIp().c_str();

        item = values.append_child("item");
        createItem(item, cs->getItemPath(i, ATTR_CLIENTS_CLIENT_USERAGENT), CFG_CLIENTS_LIST);
        item.append_attribute("value") = client->getUserAgent().c_str();
    }

    auto transcoding = config->getTranscodingProfileListOption(CFG_TRANSCODING_PROFILE_LIST);
    cs = ConfigManager::findConfigSetup(CFG_TRANSCODING_PROFILE_LIST);
    int pr = 0;
    std::map<std::string, int> profiles;
    for (const auto& entry : transcoding->getList()) {
        for (auto it = entry.second->begin(); it != entry.second->end(); it++) {
            auto item = values.append_child("item");
            createItem(item, cs->getItemPath(pr, ATTR_TRANSCODING_MIMETYPE_PROF_MAP, ATTR_TRANSCODING_MIMETYPE_PROF_MAP_TRANSCODE, ATTR_TRANSCODING_MIMETYPE_PROF_MAP_MIMETYPE), CFG_TRANSCODING_PROFILE_LIST);
            item.append_attribute("value") = entry.first.c_str();

            item = values.append_child("item");
            createItem(item, cs->getItemPath(pr, ATTR_TRANSCODING_MIMETYPE_PROF_MAP, ATTR_TRANSCODING_MIMETYPE_PROF_MAP_TRANSCODE, ATTR_TRANSCODING_MIMETYPE_PROF_MAP_USING), CFG_TRANSCODING_PROFILE_LIST);
            item.append_attribute("value") = it->second->getName().c_str();
            profiles[it->second->getName()] = pr;

            pr++;
        }
    }
    pr = 0;
    for (const auto& prof : profiles) {
        auto entry = transcoding->getByName(prof.first, true);
        auto item = values.append_child("item");
        createItem(item, cs->getItemPath(pr, ATTR_TRANSCODING_PROFILES, ATTR_TRANSCODING_PROFILES_PROFLE, ATTR_TRANSCODING_PROFILES_PROFLE_NAME), CFG_TRANSCODING_PROFILE_LIST);
        item.append_attribute("value") = entry->getName().c_str();

        item = values.append_child("item");
        createItem(item, cs->getItemPath(pr, ATTR_TRANSCODING_PROFILES, ATTR_TRANSCODING_PROFILES_PROFLE, ATTR_TRANSCODING_PROFILES_PROFLE_ENABLED), CFG_TRANSCODING_PROFILE_LIST);
        item.append_attribute("value") = fmt::format("{}", entry->getEnabled()).c_str();

        item = values.append_child("item");
        createItem(item, cs->getItemPath(pr, ATTR_TRANSCODING_PROFILES, ATTR_TRANSCODING_PROFILES_PROFLE, ATTR_TRANSCODING_PROFILES_PROFLE_TYPE), CFG_TRANSCODING_PROFILE_LIST);
        item.append_attribute("value") = entry->getType() == TR_External ? "external" : "none";

        item = values.append_child("item");
        createItem(item, cs->getItemPath(pr, ATTR_TRANSCODING_PROFILES, ATTR_TRANSCODING_PROFILES_PROFLE, ATTR_TRANSCODING_PROFILES_PROFLE_MIMETYPE), CFG_TRANSCODING_PROFILE_LIST);
        item.append_attribute("value") = entry->getTargetMimeType().c_str();

        item = values.append_child("item");
        createItem(item, cs->getItemPath(pr, ATTR_TRANSCODING_PROFILES, ATTR_TRANSCODING_PROFILES_PROFLE, ATTR_TRANSCODING_PROFILES_PROFLE_RES), CFG_TRANSCODING_PROFILE_LIST);
        item.append_attribute("value") = entry->getAttributes()[MetadataHandler::getResAttrName(R_RESOLUTION)].c_str();

        item = values.append_child("item");
        createItem(item, cs->getItemPath(pr, ATTR_TRANSCODING_PROFILES, ATTR_TRANSCODING_PROFILES_PROFLE, ATTR_TRANSCODING_PROFILES_PROFLE_ACCURL), CFG_TRANSCODING_PROFILE_LIST);
        item.append_attribute("value") = fmt::format("{}", entry->acceptURL()).c_str();

        item = values.append_child("item");
        createItem(item, cs->getItemPath(pr, ATTR_TRANSCODING_PROFILES, ATTR_TRANSCODING_PROFILES_PROFLE, ATTR_TRANSCODING_PROFILES_PROFLE_SAMPFREQ), CFG_TRANSCODING_PROFILE_LIST);
        item.append_attribute("value") = fmt::format("{}", entry->getSampleFreq()).c_str();

        item = values.append_child("item");
        createItem(item, cs->getItemPath(pr, ATTR_TRANSCODING_PROFILES, ATTR_TRANSCODING_PROFILES_PROFLE, ATTR_TRANSCODING_PROFILES_PROFLE_NRCHAN), CFG_TRANSCODING_PROFILE_LIST);
        item.append_attribute("value") = fmt::format("{}", entry->getNumChannels()).c_str();

        item = values.append_child("item");
        createItem(item, cs->getItemPath(pr, ATTR_TRANSCODING_PROFILES, ATTR_TRANSCODING_PROFILES_PROFLE, ATTR_TRANSCODING_PROFILES_PROFLE_HIDEORIG), CFG_TRANSCODING_PROFILE_LIST);
        item.append_attribute("value") = fmt::format("{}", entry->hideOriginalResource()).c_str();

        item = values.append_child("item");
        createItem(item, cs->getItemPath(pr, ATTR_TRANSCODING_PROFILES, ATTR_TRANSCODING_PROFILES_PROFLE, ATTR_TRANSCODING_PROFILES_PROFLE_THUMB), CFG_TRANSCODING_PROFILE_LIST);
        item.append_attribute("value") = fmt::format("{}", entry->isThumbnail()).c_str();

        item = values.append_child("item");
        createItem(item, cs->getItemPath(pr, ATTR_TRANSCODING_PROFILES, ATTR_TRANSCODING_PROFILES_PROFLE, ATTR_TRANSCODING_PROFILES_PROFLE_FIRST), CFG_TRANSCODING_PROFILE_LIST);
        item.append_attribute("value") = fmt::format("{}", entry->firstResource()).c_str();

        item = values.append_child("item");
        createItem(item, cs->getItemPath(pr, ATTR_TRANSCODING_PROFILES, ATTR_TRANSCODING_PROFILES_PROFLE, ATTR_TRANSCODING_PROFILES_PROFLE_ACCOGG), CFG_TRANSCODING_PROFILE_LIST);
        item.append_attribute("value") = fmt::format("{}", entry->isTheora()).c_str();

        item = values.append_child("item");
        createItem(item, cs->getItemPath(pr, ATTR_TRANSCODING_PROFILES, ATTR_TRANSCODING_PROFILES_PROFLE, ATTR_TRANSCODING_PROFILES_PROFLE_USECHUNKEDENC), CFG_TRANSCODING_PROFILE_LIST);
        item.append_attribute("value") = fmt::format("{}", entry->getChunked()).c_str();

        item = values.append_child("item");
        createItem(item, cs->getItemPath(pr, ATTR_TRANSCODING_PROFILES, ATTR_TRANSCODING_PROFILES_PROFLE, ATTR_TRANSCODING_PROFILES_PROFLE_AGENT, ATTR_TRANSCODING_PROFILES_PROFLE_AGENT_COMMAND), CFG_TRANSCODING_PROFILE_LIST);
        item.append_attribute("value") = entry->getCommand().c_str();

        item = values.append_child("item");
        createItem(item, cs->getItemPath(pr, ATTR_TRANSCODING_PROFILES, ATTR_TRANSCODING_PROFILES_PROFLE, ATTR_TRANSCODING_PROFILES_PROFLE_AGENT, ATTR_TRANSCODING_PROFILES_PROFLE_AGENT_ARGS), CFG_TRANSCODING_PROFILE_LIST);
        item.append_attribute("value") = entry->getArguments().c_str();

        item = values.append_child("item");
        createItem(item, cs->getItemPath(pr, ATTR_TRANSCODING_PROFILES, ATTR_TRANSCODING_PROFILES_PROFLE, ATTR_TRANSCODING_PROFILES_PROFLE_BUFFER, ATTR_TRANSCODING_PROFILES_PROFLE_BUFFER_SIZE), CFG_TRANSCODING_PROFILE_LIST);
        item.append_attribute("value") = fmt::format("{}", entry->getBufferSize()).c_str();

        item = values.append_child("item");
        createItem(item, cs->getItemPath(pr, ATTR_TRANSCODING_PROFILES, ATTR_TRANSCODING_PROFILES_PROFLE, ATTR_TRANSCODING_PROFILES_PROFLE_BUFFER, ATTR_TRANSCODING_PROFILES_PROFLE_BUFFER_CHUNK), CFG_TRANSCODING_PROFILE_LIST);
        item.append_attribute("value") = fmt::format("{}", entry->getBufferChunkSize()).c_str();

        item = values.append_child("item");
        createItem(item, cs->getItemPath(pr, ATTR_TRANSCODING_PROFILES, ATTR_TRANSCODING_PROFILES_PROFLE, ATTR_TRANSCODING_PROFILES_PROFLE_BUFFER, ATTR_TRANSCODING_PROFILES_PROFLE_BUFFER_FILL), CFG_TRANSCODING_PROFILE_LIST);
        item.append_attribute("value") = fmt::format("{}", entry->getBufferInitialFillSize()).c_str();

        auto fourCCMode = entry->getAVIFourCCListMode();
        if (fourCCMode != FCC_None) {
            item = values.append_child("item");
            createItem(item, cs->getItemPath(pr, ATTR_TRANSCODING_PROFILES, ATTR_TRANSCODING_PROFILES_PROFLE, ATTR_TRANSCODING_PROFILES_PROFLE_AVI4CC, ATTR_TRANSCODING_PROFILES_PROFLE_AVI4CC_MODE), CFG_TRANSCODING_PROFILE_LIST);
            item.append_attribute("value") = TranscodingProfile::mapFourCcMode(fourCCMode).c_str();

            const auto fourCCList = entry->getAVIFourCCList();
            if (fourCCList.size() > 0) {
                item = values.append_child("item");
                createItem(item, cs->getItemPath(pr, ATTR_TRANSCODING_PROFILES, ATTR_TRANSCODING_PROFILES_PROFLE, ATTR_TRANSCODING_PROFILES_PROFLE_AVI4CC, ATTR_TRANSCODING_PROFILES_PROFLE_AVI4CC_4CC), CFG_TRANSCODING_PROFILE_LIST);
                item.append_attribute("value") = std::accumulate(std::next(fourCCList.begin()),
                    fourCCList.end(), fourCCList[0],
                    [](std::string a, std::string b) { return a + ", " + b; })
                                                     .c_str();
            }
        }
        pr++;
    }

    std::map<config_option_t, std::shared_ptr<AutoscanList>> autoscanList;
    auto autoscanTimed = config->getAutoscanListOption(CFG_IMPORT_AUTOSCAN_TIMED_LIST);
    autoscanList[CFG_IMPORT_AUTOSCAN_TIMED_LIST] = autoscanTimed;
#ifdef HAVE_INOTIFY
    auto autoscanInotify = config->getAutoscanListOption(CFG_IMPORT_AUTOSCAN_INOTIFY_LIST);
    autoscanList[CFG_IMPORT_AUTOSCAN_INOTIFY_LIST] = autoscanInotify;
#endif // HAVE_INOTIFY

    for (const auto& autoscan : autoscanList) {
        cs = ConfigManager::findConfigSetup(autoscan.first);
        for (size_t i = 0; i < autoscan.second->size(); i++) {
            const auto& entry = autoscan.second->get(i);
            auto item = values.append_child("item");
            createItem(item, cs->getItemPath(i, ATTR_AUTOSCAN_DIRECTORY_LOCATION), autoscan.first);
            item.append_attribute("value") = entry->getLocation().c_str();

            item = values.append_child("item");
            createItem(item, cs->getItemPath(i, ATTR_AUTOSCAN_DIRECTORY_MODE), autoscan.first);
            item.append_attribute("value") = AutoscanDirectory::mapScanmode(entry->getScanMode()).c_str();

            item = values.append_child("item");
            createItem(item, cs->getItemPath(i, ATTR_AUTOSCAN_DIRECTORY_INTERVAL), autoscan.first);
            item.append_attribute("value") = fmt::format("{}", entry->getInterval()).c_str();

            item = values.append_child("item");
            createItem(item, cs->getItemPath(i, ATTR_AUTOSCAN_DIRECTORY_RECURSIVE), autoscan.first);
            item.append_attribute("value") = fmt::format("{}", entry->getRecursive()).c_str();

            item = values.append_child("item");
            createItem(item, cs->getItemPath(i, ATTR_AUTOSCAN_DIRECTORY_HIDDENFILES), autoscan.first);
            item.append_attribute("value") = fmt::format("{}", entry->getHidden()).c_str();
        }
    }

    int i = 0;
    auto dictionary = config->getDictionaryOption(CFG_SERVER_UI_ACCOUNT_LIST);
    cs = ConfigManager::findConfigSetup(CFG_SERVER_UI_ACCOUNT_LIST);
    for (const auto& entry : dictionary) {
        auto item = values.append_child("item");
        createItem(item, cs->getItemPath(i, ATTR_SERVER_UI_ACCOUNT_LIST_USER), CFG_SERVER_UI_ACCOUNT_LIST);
        item.append_attribute("value") = entry.first.c_str();

        item = values.append_child("item");
        createItem(item, cs->getItemPath(i, ATTR_SERVER_UI_ACCOUNT_LIST_PASSWORD), CFG_SERVER_UI_ACCOUNT_LIST);
        item.append_attribute("value") = entry.second.c_str();
        i++;
    }

    i = 0;
    dictionary = config->getDictionaryOption(CFG_IMPORT_MAPPINGS_EXTENSION_TO_MIMETYPE_LIST);
    cs = ConfigManager::findConfigSetup(CFG_IMPORT_MAPPINGS_EXTENSION_TO_MIMETYPE_LIST);
    for (const auto& entry : dictionary) {
        auto item = values.append_child("item");
        createItem(item, cs->getItemPath(i, ATTR_IMPORT_MAPPINGS_MIMETYPE_FROM), CFG_IMPORT_MAPPINGS_EXTENSION_TO_MIMETYPE_LIST);
        item.append_attribute("value") = entry.first.c_str();

        item = values.append_child("item");
        createItem(item, cs->getItemPath(i, ATTR_IMPORT_MAPPINGS_MIMETYPE_TO), CFG_IMPORT_MAPPINGS_EXTENSION_TO_MIMETYPE_LIST);
        item.append_attribute("value") = entry.second.c_str();
        i++;
    }

    i = 0;
    dictionary = config->getDictionaryOption(CFG_IMPORT_MAPPINGS_MIMETYPE_TO_CONTENTTYPE_LIST);
    cs = ConfigManager::findConfigSetup(CFG_IMPORT_MAPPINGS_MIMETYPE_TO_CONTENTTYPE_LIST);
    for (const auto& entry : dictionary) {
        auto item = values.append_child("item");
        createItem(item, cs->getItemPath(i, ATTR_IMPORT_MAPPINGS_M2CTYPE_LIST_MIMETYPE), CFG_IMPORT_MAPPINGS_MIMETYPE_TO_CONTENTTYPE_LIST);
        item.append_attribute("value") = entry.first.c_str();

        item = values.append_child("item");
        createItem(item, cs->getItemPath(i, ATTR_IMPORT_MAPPINGS_M2CTYPE_LIST_AS), CFG_IMPORT_MAPPINGS_MIMETYPE_TO_CONTENTTYPE_LIST);
        item.append_attribute("value") = entry.second.c_str();
        i++;
    }

    i = 0;
    dictionary = config->getDictionaryOption(CFG_IMPORT_LAYOUT_MAPPING);
    cs = ConfigManager::findConfigSetup(CFG_IMPORT_LAYOUT_MAPPING);
    for (const auto& entry : dictionary) {
        auto item = values.append_child("item");
        createItem(item, cs->getItemPath(i, ATTR_IMPORT_LAYOUT_MAPPING_FROM), CFG_IMPORT_LAYOUT_MAPPING);
        item.append_attribute("value") = entry.first.c_str();

        item = values.append_child("item");
        createItem(item, cs->getItemPath(i, ATTR_IMPORT_LAYOUT_MAPPING_TO), CFG_IMPORT_LAYOUT_MAPPING);
        item.append_attribute("value") = entry.second.c_str();
        i++;
    }

    i = 0;
    dictionary = config->getDictionaryOption(CFG_IMPORT_MAPPINGS_MIMETYPE_TO_UPNP_CLASS_LIST);
    cs = ConfigManager::findConfigSetup(CFG_IMPORT_MAPPINGS_MIMETYPE_TO_UPNP_CLASS_LIST);
    for (const auto& entry : dictionary) {
        auto item = values.append_child("item");
        createItem(item, cs->getItemPath(i, ATTR_IMPORT_MAPPINGS_MIMETYPE_FROM), CFG_IMPORT_MAPPINGS_MIMETYPE_TO_UPNP_CLASS_LIST);
        item.append_attribute("value") = entry.first.c_str();

        item = values.append_child("item");
        createItem(item, cs->getItemPath(i, ATTR_IMPORT_MAPPINGS_MIMETYPE_TO), CFG_IMPORT_MAPPINGS_MIMETYPE_TO_UPNP_CLASS_LIST);
        item.append_attribute("value") = entry.second.c_str();
        i++;
    }

    auto array = config->getArrayOption(CFG_SERVER_UI_ITEMS_PER_PAGE_DROPDOWN);
    cs = ConfigManager::findConfigSetup(CFG_SERVER_UI_ITEMS_PER_PAGE_DROPDOWN);
    for (size_t i = 0; i < array.size(); i++) {
        auto entry = array[i];
        auto item = values.append_child("item");
        createItem(item, cs->getItemPath(i), CFG_SERVER_UI_ITEMS_PER_PAGE_DROPDOWN);
        item.append_attribute("value") = entry.c_str();
    }

#ifdef HAVE_LIBEXIF
    array = config->getArrayOption(CFG_IMPORT_LIBOPTS_EXIF_AUXDATA_TAGS_LIST);
    cs = ConfigManager::findConfigSetup(CFG_IMPORT_LIBOPTS_EXIF_AUXDATA_TAGS_LIST);
    for (size_t i = 0; i < array.size(); i++) {
        auto entry = array[i];
        auto item = values.append_child("item");
        createItem(item, cs->getItemPath(i), CFG_IMPORT_LIBOPTS_EXIF_AUXDATA_TAGS_LIST);
        item.append_attribute("value") = entry.c_str();
    }
#endif // HAVE_LIBEXIF

#ifdef HAVE_EXIV2
    array = config->getArrayOption(CFG_IMPORT_LIBOPTS_EXIV2_AUXDATA_TAGS_LIST);
    cs = ConfigManager::findConfigSetup(CFG_IMPORT_LIBOPTS_EXIV2_AUXDATA_TAGS_LIST);
    for (size_t i = 0; i < array.size(); i++) {
        auto entry = array[i];
        auto item = values.append_child("item");
        createItem(item, cs->getItemPath(i), CFG_IMPORT_LIBOPTS_EXIV2_AUXDATA_TAGS_LIST);
        item.append_attribute("value") = entry.c_str();
    }
#endif // HAVE_EXIV2

#ifdef HAVE_TAGLIB
    array = config->getArrayOption(CFG_IMPORT_LIBOPTS_ID3_AUXDATA_TAGS_LIST);
    cs = ConfigManager::findConfigSetup(CFG_IMPORT_LIBOPTS_ID3_AUXDATA_TAGS_LIST);
    for (size_t i = 0; i < array.size(); i++) {
        auto entry = array[i];
        auto item = values.append_child("item");
        createItem(item, cs->getItemPath(i), CFG_IMPORT_LIBOPTS_ID3_AUXDATA_TAGS_LIST);
        item.append_attribute("value") = entry.c_str();
    }
#endif // HAVE_TAGLIB

#ifdef HAVE_FFMPEG
    array = config->getArrayOption(CFG_IMPORT_LIBOPTS_FFMPEG_AUXDATA_TAGS_LIST);
    cs = ConfigManager::findConfigSetup(CFG_IMPORT_LIBOPTS_FFMPEG_AUXDATA_TAGS_LIST);
    for (size_t i = 0; i < array.size(); i++) {
        auto entry = array[i];
        auto item = values.append_child("item");
        createItem(item, cs->getItemPath(i), CFG_IMPORT_LIBOPTS_FFMPEG_AUXDATA_TAGS_LIST);
        item.append_attribute("value") = entry.c_str();
    }
#endif // HAVE_FFMPEG

    array = config->getArrayOption(CFG_IMPORT_RESOURCES_FANART_FILE_LIST);
    cs = ConfigManager::findConfigSetup(CFG_IMPORT_RESOURCES_FANART_FILE_LIST);
    for (size_t i = 0; i < array.size(); i++) {
        auto entry = array[i];
        auto item = values.append_child("item");
        createItem(item, cs->getItemPath(i), CFG_IMPORT_RESOURCES_FANART_FILE_LIST);
        item.append_attribute("value") = entry.c_str();
    }

    array = config->getArrayOption(CFG_IMPORT_RESOURCES_SUBTITLE_FILE_LIST);
    cs = ConfigManager::findConfigSetup(CFG_IMPORT_RESOURCES_SUBTITLE_FILE_LIST);
    for (size_t i = 0; i < array.size(); i++) {
        auto entry = array[i];
        auto item = values.append_child("item");
        createItem(item, cs->getItemPath(i), CFG_IMPORT_RESOURCES_SUBTITLE_FILE_LIST);
        item.append_attribute("value") = entry.c_str();
    }

    array = config->getArrayOption(CFG_IMPORT_RESOURCES_RESOURCE_FILE_LIST);
    cs = ConfigManager::findConfigSetup(CFG_IMPORT_RESOURCES_RESOURCE_FILE_LIST);
    for (size_t i = 0; i < array.size(); i++) {
        auto entry = array[i];
        auto item = values.append_child("item");
        createItem(item, cs->getItemPath(i), CFG_IMPORT_RESOURCES_RESOURCE_FILE_LIST);
        item.append_attribute("value") = entry.c_str();
    }

    array = config->getArrayOption(CFG_SERVER_EXTOPTS_MARK_PLAYED_ITEMS_CONTENT_LIST);
    cs = ConfigManager::findConfigSetup(CFG_SERVER_EXTOPTS_MARK_PLAYED_ITEMS_CONTENT_LIST);
    for (size_t i = 0; i < array.size(); i++) {
        auto entry = array[i];
        auto item = values.append_child("item");
        createItem(item, cs->getItemPath(i), CFG_SERVER_EXTOPTS_MARK_PLAYED_ITEMS_CONTENT_LIST);
        item.append_attribute("value") = entry.c_str();
    }
}
