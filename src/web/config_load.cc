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
    log_info("Loading {} configuration items from storage", dbEntries.size());
        } else {
            log_error("configLoad storage missing");
        }
    } catch (const std::runtime_error& e) {
        log_error("configLoad {}", e.what());
    }
}

void web::configLoad::createItem(pugi::xml_node& item, const std::string& name, config_option_t id)
{
    allItems[name] = &item;
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

    log_debug("Sending Config to web!");
    for (int i = 0; i < static_cast<int>(CFG_MAX); i++) {
        auto item = values.append_child("item");
        auto scs = ConfigManager::findConfigSetup(static_cast<config_option_t>(i));
        createItem(item, scs->getItemPath(-1), static_cast<config_option_t>(i));

        try {
            log_debug("    Option {:03d} {} = {}", i, scs->getItemPath(), scs->getCurrentValue().c_str());
            item.append_attribute("value") = scs->getCurrentValue().c_str();
        } catch (const std::runtime_error& e) {
        }
    }

    std::shared_ptr<ConfigSetup> cs;
    cs = ConfigManager::findConfigSetup(CFG_CLIENTS_LIST);
    auto clientConfig = cs->getValue()->getClientConfigListOption();
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

    cs = ConfigManager::findConfigSetup(CFG_TRANSCODING_PROFILE_LIST);
    auto transcoding = cs->getValue()->getTranscodingProfileListOption();
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

    std::vector<config_option_t> autoscanList = {
        CFG_IMPORT_AUTOSCAN_TIMED_LIST,
#ifdef HAVE_INOTIFY
        CFG_IMPORT_AUTOSCAN_INOTIFY_LIST
#endif
    };
    for (const auto& autoscanOption : autoscanList) {
        cs = ConfigManager::findConfigSetup(autoscanOption);
        auto autoscan = cs->getValue()->getAutoscanListOption();
        for (size_t i = 0; i < autoscan->size(); i++) {
            const auto& entry = autoscan->get(i);
            auto item = values.append_child("item");
            createItem(item, cs->getItemPath(i, ATTR_AUTOSCAN_DIRECTORY_LOCATION), cs->option);
            item.append_attribute("value") = entry->getLocation().c_str();

            item = values.append_child("item");
            createItem(item, cs->getItemPath(i, ATTR_AUTOSCAN_DIRECTORY_MODE), cs->option);
            item.append_attribute("value") = AutoscanDirectory::mapScanmode(entry->getScanMode()).c_str();

            item = values.append_child("item");
            createItem(item, cs->getItemPath(i, ATTR_AUTOSCAN_DIRECTORY_INTERVAL), cs->option);
            item.append_attribute("value") = fmt::format("{}", entry->getInterval()).c_str();

            item = values.append_child("item");
            createItem(item, cs->getItemPath(i, ATTR_AUTOSCAN_DIRECTORY_RECURSIVE), cs->option);
            item.append_attribute("value") = fmt::format("{}", entry->getRecursive()).c_str();

            item = values.append_child("item");
            createItem(item, cs->getItemPath(i, ATTR_AUTOSCAN_DIRECTORY_HIDDENFILES), cs->option);
            item.append_attribute("value") = fmt::format("{}", entry->getHidden()).c_str();
        }
    }

    std::vector<config_option_t> dict_options = {CFG_SERVER_UI_ACCOUNT_LIST, CFG_IMPORT_MAPPINGS_EXTENSION_TO_MIMETYPE_LIST, CFG_IMPORT_MAPPINGS_MIMETYPE_TO_CONTENTTYPE_LIST, CFG_IMPORT_MAPPINGS_MIMETYPE_TO_UPNP_CLASS_LIST, CFG_IMPORT_LAYOUT_MAPPING};

    for (auto dict_option : dict_options) {
        int i = 0;
        auto dcs = ConfigSetup::findConfigSetup<ConfigDictionarySetup>(dict_option);
        auto dictionary = dcs->getValue()->getDictionaryOption(true);
        for (const auto& entry : dictionary) {
            auto item = values.append_child("item");
            createItem(item, dcs->getItemPath(i, dcs->keyOption), dcs->option);
            item.append_attribute("value") = entry.first.substr(5).c_str();

            item = values.append_child("item");
            createItem(item, dcs->getItemPath(i, dcs->valOption), dcs->option);
            item.append_attribute("value") = entry.second.c_str();
            i++;
        }
    }

    std::vector<config_option_t> array_options = {
        CFG_SERVER_UI_ITEMS_PER_PAGE_DROPDOWN, CFG_IMPORT_RESOURCES_FANART_FILE_LIST, CFG_IMPORT_RESOURCES_SUBTITLE_FILE_LIST, CFG_IMPORT_RESOURCES_RESOURCE_FILE_LIST, CFG_SERVER_EXTOPTS_MARK_PLAYED_ITEMS_CONTENT_LIST,
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
        auto acs = ConfigManager::findConfigSetup(array_option);
        auto array = acs->getValue()->getArrayOption(true);
        for (size_t i = 0; i < array.size(); i++) {
            auto entry = array[i];
            auto item = values.append_child("item");
            createItem(item, acs->getItemPath(i), acs->option);
            item.append_attribute("value") = entry.c_str();
        }
    }

    for (auto entry : dbEntries) {
        auto exItem = allItems.find(entry.item);
        if (exItem != allItems.end()) {
            auto item = (*exItem).second;
            item->attribute("source") = "database";
            item->attribute("status") = entry.status.c_str();
        } else {
            auto cs = ConfigManager::findConfigSetupByPath(entry.item, true);
            if (cs != nullptr) {
                auto item = values.append_child("item");
                createItem(item, entry.item, cs->option);
                item.append_attribute("value") = entry.value.c_str();
                item.attribute("status") = entry.status.c_str();
                item.attribute("origValue") = config->getOrigValue(entry.item).c_str();
                item.attribute("source") = "database";
            }
        }
    }
}
