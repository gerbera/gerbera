/*GRB*

    Gerbera - https://gerbera.io/

    config_load.cc - this file is part of Gerbera.

    Copyright (C) 2020-2022 Gerbera Contributors

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
#include "config/dynamic_content.h"
#include "content/autoscan.h"
#include "content/content_manager.h"
#include "database/database.h"
#include "metadata/metadata_handler.h"
#include "transcoding/transcoding.h"
#include "util/upnp_clients.h"

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
    xml2JsonHints->setArrayName(values, "item");
    xml2JsonHints->setFieldType("item", "string");
    xml2JsonHints->setFieldType("id", "string");
    xml2JsonHints->setFieldType("aid", "string");
    xml2JsonHints->setFieldType("value", "string");
    xml2JsonHints->setFieldType("origValue", "string");

    log_debug("Sending Config to web!");

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

    if (action == "status")
        return;

    // generate meta info for ui
    auto meta = root.append_child("types");
    xml2JsonHints->setArrayName(meta, "item");
    for (auto&& cs : ConfigDefinition::getOptionList()) {
        addTypeMeta(meta, cs);
    }

    // write all values with simple type (string, int, bool)
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

    // write client configuration
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
    }

    // write import tweaks
    cs = ConfigDefinition::findConfigSetup(CFG_IMPORT_DIRECTORIES_LIST);
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
        createItem(item, cs->getItemPath(i, ATTR_DIRECTORIES_TWEAK_SUBTILTE_FILE), cs->option, ATTR_DIRECTORIES_TWEAK_SUBTILTE_FILE);
        setValue(item, dir->hasSubTitleFile() ? dir->getSubTitleFile() : "");
    }

    // write dynamic content
    cs = ConfigDefinition::findConfigSetup(CFG_SERVER_DYNAMIC_CONTENT_LIST);
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
    }

    // write transconding configuration
    cs = ConfigDefinition::findConfigSetup(CFG_TRANSCODING_PROFILE_LIST);
    auto transcoding = cs->getValue()->getTranscodingProfileListOption();
    int pr = 0;
    std::map<std::string, std::shared_ptr<TranscodingProfile>> profiles;
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
        createItem(item, cs->getItemPath(pr, ATTR_TRANSCODING_MIMETYPE_FILTER, ATTR_TRANSCODING_PROFILES_PROFLE_CLIENTFLAGS), cs->option, ATTR_TRANSCODING_PROFILES_PROFLE_CLIENTFLAGS, cs);
        setValue(item, ClientConfig::mapFlags(filter->getClientFlags()));

        if (filter->getTranscodingProfile())
            profiles[filter->getTranscodingProfile()->getName()] = filter->getTranscodingProfile();

        pr++;
    }

    pr = 0;
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
        createItem(item, cs->getItemPath(pr, ATTR_TRANSCODING_PROFILES_PROFLE, ATTR_TRANSCODING_PROFILES_PROFLE_TYPE), cs->option, ATTR_TRANSCODING_PROFILES_PROFLE_TYPE);
        setValue(item, (entry->getType() == TR_External ? "external" : "none"));

        item = values.append_child("item");
        createItem(item, cs->getItemPath(pr, ATTR_TRANSCODING_PROFILES_PROFLE, ATTR_TRANSCODING_PROFILES_PROFLE_DLNAPROF), cs->option, ATTR_TRANSCODING_PROFILES_PROFLE_DLNAPROF);
        setValue(item, entry->getDlnaProfile());

        item = values.append_child("item");
        createItem(item, cs->getItemPath(pr, ATTR_TRANSCODING_PROFILES_PROFLE, ATTR_TRANSCODING_PROFILES_PROFLE_MIMETYPE), cs->option, ATTR_TRANSCODING_PROFILES_PROFLE_MIMETYPE);
        setValue(item, entry->getTargetMimeType());

        item = values.append_child("item");
        createItem(item, cs->getItemPath(pr, ATTR_TRANSCODING_PROFILES_PROFLE, ATTR_TRANSCODING_PROFILES_PROFLE_RES), cs->option, ATTR_TRANSCODING_PROFILES_PROFLE_RES);
        setValue(item, entry->getAttributeOverride(CdsResource::Attribute::RESOLUTION));

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

            const auto& fourCCList = entry->getAVIFourCCList();
            if (!fourCCList.empty()) {
                item = values.append_child("item");
                createItem(item, cs->getItemPath(pr, ATTR_TRANSCODING_PROFILES_PROFLE, ATTR_TRANSCODING_PROFILES_PROFLE_AVI4CC, ATTR_TRANSCODING_PROFILES_PROFLE_AVI4CC_4CC), cs->option, ATTR_TRANSCODING_PROFILES_PROFLE_AVI4CC_4CC);
                setValue(item, std::accumulate(next(fourCCList.begin()), fourCCList.end(), fourCCList[0], [](auto&& a, auto&& b) { return fmt::format("{}, {}", a, b); }));
            }
        }
        pr++;
    }

    // write autoscan configuration
    for (auto&& ascs : ConfigDefinition::getConfigSetupList<ConfigAutoscanSetup>()) {
        auto autoscan = ascs->getValue()->getAutoscanListOption();
        for (std::size_t i = 0; i < autoscan.size(); i++) {
            auto&& entry = autoscan.at(i);
            auto&& adir = content->getAutoscanDirectory(entry.getLocation());
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
            createItem(item, ascs->getItemPath(i, ATTR_AUTOSCAN_DIRECTORY_MEDIATYPE), ascs->option, ATTR_AUTOSCAN_DIRECTORY_MEDIATYPE);
            setValue(item, adir->getMediaType());

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
            createItem(item, ascs->getItemPath(ITEM_PATH_NEW, ATTR_AUTOSCAN_DIRECTORY_SCANCOUNT), ascs->option, ATTR_AUTOSCAN_DIRECTORY_SCANCOUNT, ConfigDefinition::findConfigSetup(ATTR_AUTOSCAN_DIRECTORY_SCANCOUNT));

            item = values.append_child("item");
            createItem(item, ascs->getItemPath(ITEM_PATH_NEW, ATTR_AUTOSCAN_DIRECTORY_TASKCOUNT), ascs->option, ATTR_AUTOSCAN_DIRECTORY_TASKCOUNT, ConfigDefinition::findConfigSetup(ATTR_AUTOSCAN_DIRECTORY_TASKCOUNT));

            item = values.append_child("item");
            createItem(item, ascs->getItemPath(ITEM_PATH_NEW, ATTR_AUTOSCAN_DIRECTORY_LMT), ascs->option, ATTR_AUTOSCAN_DIRECTORY_LMT, ConfigDefinition::findConfigSetup(ATTR_AUTOSCAN_DIRECTORY_LMT));
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
    // write content of all vectors
    for (auto&& vcs : ConfigDefinition::getConfigSetupList<ConfigVectorSetup>()) {
        int i = 0;
        auto vector = vcs->getValue()->getVectorOption(true);
        for (auto&& value : vector) {
            for (auto&& [key, val] : value) {
                auto item = values.append_child("item");
                createItem(item, vcs->getItemPath(i, vcs->option) + key, vcs->option, vcs->optionList[0], vcs); // FIxME
                setValue(item, val);
                i++;
            }
        }
    }

    // write content of all arrays
    for (auto&& acs : ConfigDefinition::getConfigSetupList<ConfigArraySetup>()) {
        auto array = acs->getValue()->getArrayOption(true);
        for (std::size_t i = 0; i < array.size(); i++) {
            auto&& entry = array[i];
            auto item = values.append_child("item");
            createItem(item, acs->getItemPath(i), acs->option, acs->attrOption != CFG_MAX ? acs->attrOption : acs->nodeOption, acs);
            setValue(item, entry);
        }
    }

    // update entries with datebase values
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
