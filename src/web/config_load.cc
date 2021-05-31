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

#include "pages.h" // API

#include <fmt/chrono.h>
#include <numeric>

#include "config/client_config.h"
#include "config/config_definition.h"
#include "config/config_setup.h"
#include "config/directory_tweak.h"
#include "content/autoscan.h"
#include "content/content_manager.h"
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

void web::configLoad::createItem(pugi::xml_node& item, const std::string& name, config_option_t id, config_option_t aid, const std::shared_ptr<ConfigSetup>& cs)
{
    allItems[name] = &item;
    item.append_attribute("item") = name.c_str();
    item.append_attribute("id") = fmt::format("{:02d}", id).c_str();
    item.append_attribute("aid") = fmt::format("{:02d}", aid).c_str();
    if (std::any_of(dbEntries.begin(), dbEntries.end(), [&](auto&& s) { return s.item == name; })) {
        item.append_attribute("status") = "unchanged";
        item.append_attribute("source") = "database";
    } else {
        item.append_attribute("status") = cs == nullptr || !cs->isDefaultValueUsed() ? "unchanged" : "default";
        item.append_attribute("source") = cs == nullptr || !cs->isDefaultValueUsed() ? "config.xml" : "default";
    }
    item.append_attribute("origValue") = config->getOrigValue(name).c_str();
    item.append_attribute("defaultValue") = cs != nullptr ? cs->getDefaultValue().c_str() : "";
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

/// \brief: process config_load request
void web::configLoad::process()
{
    check_request();
    auto root = xmlDoc->document_element();
    auto values = root.append_child("values");

    // set handling of json properties
    xml2JsonHints->setArrayName(values, "item");
    xml2JsonHints->setFieldType("item", "string");
    xml2JsonHints->setFieldType("id", "string");
    xml2JsonHints->setFieldType("aid", "string");
    xml2JsonHints->setFieldType("value", "string");
    xml2JsonHints->setFieldType("origValue", "string");

    log_debug("Sending Config to web!");

    // generate meta info for ui
    auto meta = root.append_child("types");
    xml2JsonHints->setArrayName(meta, "item");
    for (auto&& cs : ConfigDefinition::getOptionList()) {
        addTypeMeta(meta, cs);
    }

    // write database status
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

    // write all values with simple type (string, int, bool)
    for (int i = 0; i < int(CFG_MAX); i++) {
        auto scs = ConfigDefinition::findConfigSetup(config_option_t(i));
        auto item = values.append_child("item");
        createItem(item, scs->getItemPath(-1), config_option_t(i), config_option_t(i), scs);

        try {
            log_debug("    Option {:03d} {} = {}", i, scs->getItemPath(), scs->getCurrentValue().c_str());
            setValue(item, scs->getCurrentValue());
        } catch (const std::runtime_error& e) {
        }
    }

    // write client configuration
    std::shared_ptr<ConfigSetup> cs;
    cs = ConfigDefinition::findConfigSetup(CFG_CLIENTS_LIST);
    auto clientConfig = cs->getValue()->getClientConfigListOption();
    for (size_t i = 0; i < clientConfig->size(); i++) {
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
    }

    // write import tweaks
    cs = ConfigDefinition::findConfigSetup(CFG_IMPORT_DIRECTORIES_LIST);
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

    // write transconding configuration
    cs = ConfigDefinition::findConfigSetup(CFG_TRANSCODING_PROFILE_LIST);
    auto transcoding = cs->getValue()->getTranscodingProfileListOption();
    int pr = 0;
    std::map<std::string, int> profiles;
    for (auto&& [key, val] : transcoding->getList()) {
        for (auto&& [a, name] : *val) {
            auto item = values.append_child("item");
            createItem(item, cs->getItemPath(pr, ATTR_TRANSCODING_MIMETYPE_PROF_MAP, ATTR_TRANSCODING_MIMETYPE_PROF_MAP_TRANSCODE, ATTR_TRANSCODING_MIMETYPE_PROF_MAP_MIMETYPE), cs->option, ATTR_TRANSCODING_MIMETYPE_PROF_MAP_MIMETYPE, cs);
            setValue(item, key);

            item = values.append_child("item");
            createItem(item, cs->getItemPath(pr, ATTR_TRANSCODING_MIMETYPE_PROF_MAP, ATTR_TRANSCODING_MIMETYPE_PROF_MAP_TRANSCODE, ATTR_TRANSCODING_MIMETYPE_PROF_MAP_USING), cs->option, ATTR_TRANSCODING_MIMETYPE_PROF_MAP_USING, cs);
            setValue(item, name->getName());
            profiles[name->getName()] = pr;

            pr++;
        }
    }

    pr = 0;
    for (auto&& [key, val] : profiles) {
        auto entry = transcoding->getByName(key, true);
        auto item = values.append_child("item");
        createItem(item, cs->getItemPath(pr, ATTR_TRANSCODING_PROFILES_PROFLE, ATTR_TRANSCODING_PROFILES_PROFLE_NAME), cs->option, ATTR_TRANSCODING_PROFILES_PROFLE_NAME);
        setValue(item, entry->getName());

        item = values.append_child("item");
        createItem(item, cs->getItemPath(pr, ATTR_TRANSCODING_PROFILES_PROFLE, ATTR_TRANSCODING_PROFILES_PROFLE_ENABLED), cs->option, ATTR_TRANSCODING_PROFILES_PROFLE_ENABLED);
        setValue(item, entry->getEnabled());

        item = values.append_child("item");
        createItem(item, cs->getItemPath(pr, ATTR_TRANSCODING_PROFILES_PROFLE, ATTR_TRANSCODING_PROFILES_PROFLE_TYPE), cs->option, ATTR_TRANSCODING_PROFILES_PROFLE_TYPE);
        setValue(item, std::string((entry->getType() == TR_External) ? "external" : "none"));

        item = values.append_child("item");
        createItem(item, cs->getItemPath(pr, ATTR_TRANSCODING_PROFILES_PROFLE, ATTR_TRANSCODING_PROFILES_PROFLE_MIMETYPE), cs->option, ATTR_TRANSCODING_PROFILES_PROFLE_MIMETYPE);
        setValue(item, entry->getTargetMimeType());

        item = values.append_child("item");
        createItem(item, cs->getItemPath(pr, ATTR_TRANSCODING_PROFILES_PROFLE, ATTR_TRANSCODING_PROFILES_PROFLE_RES), cs->option, ATTR_TRANSCODING_PROFILES_PROFLE_RES);
        setValue(item, entry->getAttributes()[MetadataHandler::getResAttrName(R_RESOLUTION)]);

        item = values.append_child("item");
        createItem(item, cs->getItemPath(pr, ATTR_TRANSCODING_PROFILES_PROFLE, ATTR_TRANSCODING_PROFILES_PROFLE_ACCURL), cs->option, ATTR_TRANSCODING_PROFILES_PROFLE_ACCURL);
        setValue(item, entry->acceptURL());

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
        setValue(item, entry->firstResource());

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
        if (fourCCMode != FCC_None) {
            item = values.append_child("item");
            createItem(item, cs->getItemPath(pr, ATTR_TRANSCODING_PROFILES_PROFLE, ATTR_TRANSCODING_PROFILES_PROFLE_AVI4CC, ATTR_TRANSCODING_PROFILES_PROFLE_AVI4CC_MODE), cs->option, ATTR_TRANSCODING_PROFILES_PROFLE_AVI4CC_MODE);
            setValue(item, TranscodingProfile::mapFourCcMode(fourCCMode));

            const auto fourCCList = entry->getAVIFourCCList();
            if (!fourCCList.empty()) {
                item = values.append_child("item");
                createItem(item, cs->getItemPath(pr, ATTR_TRANSCODING_PROFILES_PROFLE, ATTR_TRANSCODING_PROFILES_PROFLE_AVI4CC, ATTR_TRANSCODING_PROFILES_PROFLE_AVI4CC_4CC), cs->option, ATTR_TRANSCODING_PROFILES_PROFLE_AVI4CC_4CC);
                setValue(item, std::accumulate(std::next(fourCCList.begin()), fourCCList.end(), fourCCList[0], [](auto&& a, auto&& b) { return fmt::format("{}, {}", a.c_str(), b.c_str()); }));
            }
        }
        pr++;
    }

    // write autoscan configuration
    for (auto&& ascs : ConfigDefinition::getConfigSetupList<ConfigAutoscanSetup>()) {
        auto autoscan = ascs->getValue()->getAutoscanListOption();
        for (size_t i = 0; i < autoscan->size(); i++) {
            auto&& entry = autoscan->get(i);
            auto&& adir = content->getAutoscanDirectory(entry->getLocation());
            auto item = values.append_child("item");
            createItem(item, ascs->getItemPath(i, ATTR_AUTOSCAN_DIRECTORY_LOCATION), ascs->option, ATTR_AUTOSCAN_DIRECTORY_LOCATION);
            setValue(item, adir->getLocation());

            item = values.append_child("item");
            createItem(item, ascs->getItemPath(i, ATTR_AUTOSCAN_DIRECTORY_MODE), ascs->option, ATTR_AUTOSCAN_DIRECTORY_MODE);
            setValue(item, AutoscanDirectory::mapScanmode(adir->getScanMode()));

            item = values.append_child("item");
            createItem(item, ascs->getItemPath(i, ATTR_AUTOSCAN_DIRECTORY_INTERVAL), ascs->option, ATTR_AUTOSCAN_DIRECTORY_INTERVAL);
            setValue(item, adir->getInterval());

            item = values.append_child("item");
            createItem(item, ascs->getItemPath(i, ATTR_AUTOSCAN_DIRECTORY_RECURSIVE), ascs->option, ATTR_AUTOSCAN_DIRECTORY_RECURSIVE);
            setValue(item, adir->getRecursive());

            item = values.append_child("item");
            createItem(item, ascs->getItemPath(i, ATTR_AUTOSCAN_DIRECTORY_HIDDENFILES), ascs->option, ATTR_AUTOSCAN_DIRECTORY_HIDDENFILES);
            setValue(item, adir->getHidden());

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
    }

    // write content of all dictionaries
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

    // write content of all arrays
    for (auto&& acs : ConfigDefinition::getConfigSetupList<ConfigArraySetup>()) {
        auto array = acs->getValue()->getArrayOption(true);
        for (size_t i = 0; i < array.size(); i++) {
            auto entry = array[i];
            auto item = values.append_child("item");
            createItem(item, acs->getItemPath(i), acs->option, acs->attrOption != CFG_MAX ? acs->attrOption : acs->nodeOption, acs);
            setValue(item, entry);
        }
    }

    // update entries with datebase values
    for (auto&& entry : dbEntries) {
        auto exItem = allItems.find(entry.item);
        if (exItem != allItems.end()) {
            auto item = exItem->second;
            item->attribute("source") = "database";
            item->attribute("status") = entry.status.c_str();
        } else {
            auto cs = ConfigDefinition::findConfigSetupByPath(entry.item, true);
            auto acs = ConfigDefinition::findConfigSetupByPath(entry.item, true, cs);
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
