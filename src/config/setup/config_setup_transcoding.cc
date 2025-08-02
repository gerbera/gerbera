/*GRB*

    Gerbera - https://gerbera.io/

    config_setup_transcoding.cc - this file is part of Gerbera.

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

/// \file config_setup_transcoding.cc
#define GRB_LOG_FAC GrbLogFacility::config

#include "config_setup_transcoding.h" // API

#include "config/config_definition.h"
#include "config/config_options.h"
#include "config/config_val.h"
#include "config/result/transcoding.h"
#include "config_setup_array.h"
#include "config_setup_bool.h"
#include "config_setup_dictionary.h"
#include "config_setup_enum.h"
#include "config_setup_int.h"
#include "config_setup_path.h"
#include "config_setup_string.h"
#include "metadata/resolution.h"
#include "setup_util.h"
#include "util/logger.h"

/// \brief Creates an array of TranscodingProfile objects from an XML
/// nodeset.
/// \param element starting element of the nodeset.
bool ConfigTranscodingSetup::createOptionFromNode(const pugi::xml_node& element, std::shared_ptr<TranscodingProfileList>& result) const
{
    if (!element)
        return true;

    const pugi::xml_node& root = element.root();

    // initialize mapping dictionary
    std::vector<std::shared_ptr<TranscodingFilter>> trFilters;
    auto fcs = definition->findConfigSetup<ConfigSetup>(ConfigVal::A_TRANSCODING_MIMETYPE_FILTER);
    const auto filterNodes = fcs->getXmlTree(element);
    if (filterNodes.empty())
        return true;

    auto pcs = definition->findConfigSetup<ConfigSetup>(ConfigVal::A_TRANSCODING_PROFILES_PROFLE);
    const auto profileNodes = pcs->getXmlTree(element);
    if (profileNodes.empty())
        return true;

    // go through filters
    for (auto&& it : filterNodes) {
        const pugi::xml_node child = it.node();

        auto filter = std::make_shared<TranscodingFilter>(
            definition->findConfigSetup<ConfigStringSetup>(ConfigVal::A_TRANSCODING_MIMETYPE_PROF_MAP_MIMETYPE)->getXmlContent(child),
            definition->findConfigSetup<ConfigStringSetup>(ConfigVal::A_TRANSCODING_MIMETYPE_PROF_MAP_USING)->getXmlContent(child));
        filter->setSourceProfile(definition->findConfigSetup<ConfigStringSetup>(ConfigVal::A_TRANSCODING_PROFILES_PROFLE_SRCDLNA)->getXmlContent(child));
        filter->setClientFlags(definition->findConfigSetup<ConfigUIntSetup>(ConfigVal::A_TRANSCODING_PROFILES_PROFLE_CLIENTFLAGS)->getXmlContent(child));
        auto noTranscoding = definition->findConfigSetup<ConfigStringSetup>(ConfigVal::A_TRANSCODING_PROFILES_PROFLE_NOTRANSCODING)->getXmlContent(child);
        std::vector<std::string> noTranscodingVector;
        for (auto&& mime : splitString(noTranscoding, ','))
            noTranscodingVector.push_back(trimString(mime));
        filter->setNoTranscodingMimeTypes(noTranscodingVector);
        trFilters.push_back(filter);
        result->add(filter);
    }

    bool allowUnusedProfiles = definition->findConfigSetup<ConfigBoolSetup>(ConfigVal::TRANSCODING_PROFILES_PROFILE_ALLOW_UNUSED)->getXmlContent(root);
    if (!allowUnusedProfiles && trFilters.empty()) {
        log_error("Error in configuration: transcoding "
                  "profiles exist, but no mimetype to profile mappings specified");
        return false;
    }

    // go through profiles
    for (auto&& it : profileNodes) {
        const pugi::xml_node child = it.node();

        auto prof = std::make_shared<TranscodingProfile>(
            definition->findConfigSetup<ConfigBoolSetup>(ConfigVal::A_TRANSCODING_PROFILES_PROFLE_ENABLED)->getXmlContent(child),
            definition->findConfigSetup<ConfigEnumSetup<TranscodingType>>(ConfigVal::A_TRANSCODING_PROFILES_PROFLE_TYPE)->getXmlContent(child),
            definition->findConfigSetup<ConfigStringSetup>(ConfigVal::A_TRANSCODING_PROFILES_PROFLE_NAME)->getXmlContent(child));
        prof->setClientFlags(definition->findConfigSetup<ConfigUIntSetup>(ConfigVal::A_TRANSCODING_PROFILES_PROFLE_CLIENTFLAGS)->getXmlContent(child));

        // read resolution
        {
            pugi::xml_node sub = definition->findConfigSetup<ConfigSetup>(ConfigVal::A_TRANSCODING_PROFILES_PROFLE_RESOLUTION)->getXmlElement(child);
            if (sub) {
                std::string param = sub.text().as_string();
                if (!param.empty()) {
                    try {
                        auto res = Resolution(param);
                        prof->setAttributeOverride(ResourceAttribute::RESOLUTION, res.string());
                    } catch (const std::runtime_error& e) {
                        log_warning("Config setup for resolution {} is invalid", param);
                    }
                }
            }
        }
        // read mimetype
        {
            pugi::xml_node sub = definition->findConfigSetup<ConfigSetup>(ConfigVal::A_TRANSCODING_PROFILES_PROFLE_MIMETYPE)->getXmlElement(child);
            std::string mimetype;
            if (sub) {
                mimetype = definition->findConfigSetup<ConfigStringSetup>(ConfigVal::A_TRANSCODING_PROFILES_PROFLE_MIMETYPE_VALUE)->getXmlContent(sub);
                if (!mimetype.empty()) {
                    // handle properties
                    for (auto prop : sub) {
                        auto key = definition->findConfigSetup<ConfigStringSetup>(ConfigVal::A_TRANSCODING_PROFILES_PROFLE_MIMETYPE_PROPERTIES_KEY)->getXmlContent(prop);
                        auto resource = definition->findConfigSetup<ConfigEnumSetup<ResourceAttribute>>(ConfigVal::A_TRANSCODING_PROFILES_PROFLE_MIMETYPE_PROPERTIES_RESOURCE)->getXmlContent(prop);
                        auto metadata = definition->findConfigSetup<ConfigEnumSetup<MetadataFields>>(ConfigVal::A_TRANSCODING_PROFILES_PROFLE_MIMETYPE_PROPERTIES_METADATA)->getXmlContent(prop);
                        prof->addTargetMimeProperty(TranscodingMimeProperty(key, resource, metadata));
                        log_debug("MimeProperty {}, key={}, resource={}, metadata={}", mimetype, key, EnumMapper::getAttributeName(resource), MetaEnumMapper::getMetaFieldName(metadata));
                    }
                } else {
                    mimetype = definition->findConfigSetup<ConfigStringSetup>(ConfigVal::A_TRANSCODING_PROFILES_PROFLE_MIMETYPE)->getXmlContent(child);
                }
                prof->setTargetMimeType(mimetype);
            }
        }
        // read 4cc options
        {
            auto cs = definition->findConfigSetup<ConfigArraySetup>(ConfigVal::A_TRANSCODING_PROFILES_PROFLE_AVI4CC);
            if (cs->hasXmlElement(child)) {
                pugi::xml_node sub = cs->getXmlElement(child);
                AviFourccListmode fccMode = definition->findConfigSetup<ConfigEnumSetup<AviFourccListmode>>(ConfigVal::A_TRANSCODING_PROFILES_PROFLE_AVI4CC_MODE)->getXmlContent(sub);
                if (fccMode != AviFourccListmode::None) {
                    prof->setAVIFourCCList(cs->getXmlContent(sub), fccMode);
                }
            }
        }
        // read profile options
        {
            auto cs = definition->findConfigSetup<ConfigStringSetup>(ConfigVal::A_TRANSCODING_PROFILES_PROFLE_DLNAPROF);
            if (cs->hasXmlElement(child))
                prof->setDlnaProfile(cs->getXmlContent(child));
        }
        {
            auto cs = definition->findConfigSetup<ConfigBoolSetup>(ConfigVal::A_TRANSCODING_PROFILES_PROFLE_ACCURL);
            if (cs->hasXmlElement(child))
                prof->setAcceptURL(cs->getXmlContent(child));
        }
        {
            auto cs = definition->findConfigSetup<ConfigIntSetup>(ConfigVal::A_TRANSCODING_PROFILES_PROFLE_SAMPFREQ);
            if (cs->hasXmlElement(child))
                prof->setSampleFreq(cs->getXmlContent(child));
        }
        {
            auto cs = definition->findConfigSetup<ConfigIntSetup>(ConfigVal::A_TRANSCODING_PROFILES_PROFLE_NRCHAN);
            if (cs->hasXmlElement(child))
                prof->setNumChannels(cs->getXmlContent(child));
        }
        {
            auto cs = definition->findConfigSetup<ConfigBoolSetup>(ConfigVal::A_TRANSCODING_PROFILES_PROFLE_HIDEORIG);
            if (cs->hasXmlElement(child))
                prof->setHideOriginalResource(cs->getXmlContent(child));
        }
        {
            auto cs = definition->findConfigSetup<ConfigBoolSetup>(ConfigVal::A_TRANSCODING_PROFILES_PROFLE_THUMB);
            if (cs->hasXmlElement(child))
                prof->setThumbnail(cs->getXmlContent(child));
        }
        {
            auto cs = definition->findConfigSetup<ConfigBoolSetup>(ConfigVal::A_TRANSCODING_PROFILES_PROFLE_FIRST);
            if (cs->hasXmlElement(child))
                prof->setFirstResource(cs->getXmlContent(child));
        }
        {
            auto cs = definition->findConfigSetup<ConfigBoolSetup>(ConfigVal::A_TRANSCODING_PROFILES_PROFLE_ACCOGG);
            if (cs->hasXmlElement(child))
                prof->setTheora(cs->getXmlContent(child));
        }

        // read agent options
        {
            pugi::xml_node sub = definition->findConfigSetup<ConfigSetup>(ConfigVal::A_TRANSCODING_PROFILES_PROFLE_AGENT)->getXmlElement(child);
            auto cs = definition->findConfigSetup<ConfigPathSetup>(ConfigVal::A_TRANSCODING_PROFILES_PROFLE_AGENT_COMMAND);
            cs->setFlag(prof->isEnabled(), ConfigPathArguments::mustExist);
            prof->setCommand(cs->getXmlContent(sub));
            prof->setArguments(definition->findConfigSetup<ConfigStringSetup>(ConfigVal::A_TRANSCODING_PROFILES_PROFLE_AGENT_ARGS)->getXmlContent(sub));
        }
        {
            auto cs = definition->findConfigSetup<ConfigDictionarySetup>(ConfigVal::A_TRANSCODING_PROFILES_PROFLE_AGENT_ENVIRON);
            if (cs->hasXmlElement(child))
                prof->setEnviron(cs->getXmlContent(cs->getXmlElement(child)));
        }

        // set buffer options
        {
            pugi::xml_node sub = definition->findConfigSetup<ConfigSetup>(ConfigVal::A_TRANSCODING_PROFILES_PROFLE_BUFFER)->getXmlElement(child);
            std::size_t buffer = definition->findConfigSetup<ConfigUIntSetup>(ConfigVal::A_TRANSCODING_PROFILES_PROFLE_BUFFER_SIZE)->getXmlContent(sub);
            std::size_t chunk = definition->findConfigSetup<ConfigUIntSetup>(ConfigVal::A_TRANSCODING_PROFILES_PROFLE_BUFFER_CHUNK)->getXmlContent(sub);
            std::size_t fill = definition->findConfigSetup<ConfigUIntSetup>(ConfigVal::A_TRANSCODING_PROFILES_PROFLE_BUFFER_FILL)->getXmlContent(sub);

            if (chunk > buffer) {
                log_error("Error in configuration: transcoding profile \"{}\" chunk size can not be greater than buffer size", prof->getName());
                return false;
            }
            if (fill > buffer) {
                log_error("Error in configuration: transcoding profile \"{}\" fill size can not be greater than buffer size", prof->getName());
                return false;
            }

            prof->setBufferOptions(buffer, chunk, fill);
        }

        bool set = false;
        for (auto&& filter : trFilters) {
            if (filter->getTranscoderName() == prof->getName()) {
                filter->setTranscodingProfile(prof);
                set = true;
            }
        }

        if (!set) {
            if (!allowUnusedProfiles) {
                log_error("Error in configuration: you specified a mimetype to transcoding profile mapping, but no match for profile \"{}\" exists", prof->getName());
                return false;
            }
            log_warning("You specified a mimetype to transcoding profile mapping, but no match for profile \"{}\" exists", prof->getName());
        }
    }

    // validate profiles
    bool allowUnusedFilter = definition->findConfigSetup<ConfigBoolSetup>(ConfigVal::TRANSCODING_MIMETYPE_PROF_MAP_ALLOW_UNUSED)->getXmlContent(root);
    for (auto&& filter : trFilters) {
        if (!filter->getTranscodingProfile()) {
            if (!allowUnusedFilter) {
                log_error("Error in configuration: you specified a mimetype to transcoding profile mapping, but the profile \"{}\" for mimetype \"{}\" does not exists", filter->getTranscoderName(), filter->getMimeType());
                return false;
            }
            log_warning("You specified a mimetype to transcoding profile mapping, but the profile \"{}\" for mimetype \"{}\" does not exists", filter->getTranscoderName(), filter->getMimeType());
        }
    }
    return true;
}

void ConfigTranscodingSetup::makeOption(const pugi::xml_node& root, const std::shared_ptr<Config>& config, const std::map<std::string, std::string>* arguments)
{
    if (arguments && arguments->find("isEnabled") != arguments->end()) {
        isEnabled = arguments->at("isEnabled") == "true";
    }
    newOption(getXmlElement(root));
    setOption(config);
}

std::string ConfigTranscodingSetup::getItemPath(const std::vector<std::size_t>& indexList, const std::vector<ConfigVal>& propOptions, const std::string& propText) const
{
    auto opt2 = definition->ensureAttribute(propOptions.size() > 1 ? propOptions[1] : ConfigVal::MAX, propOptions.size() == 2);
    auto opt3 = definition->ensureAttribute(propOptions.size() > 2 ? propOptions[2] : ConfigVal::MAX, propOptions.size() == 3);
    auto opt4 = definition->ensureAttribute(propOptions.size() > 3 ? propOptions[3] : ConfigVal::MAX, propOptions.size() > 4);
    auto propOption = !propOptions.empty() ? propOptions[0] : ConfigVal::MAX;

    auto index = !indexList.empty() ? fmt::format("{}", indexList[0]) : "_";

    if (propOption == ConfigVal::A_TRANSCODING_PROFILES_PROFLE || propOption == ConfigVal::A_TRANSCODING_MIMETYPE_FILTER) {
        if (propOptions.size() < 3) {
            return fmt::format("{}[{}]/{}", definition->mapConfigOption(propOption), index, opt2);
        }
        return fmt::format("{}[{}]/{}/{}", definition->mapConfigOption(propOption), index, opt2, opt3);
    }
    if (propOption == ConfigVal::A_TRANSCODING_MIMETYPE_PROF_MAP) {
        if (propOptions.size() < 4) {
            return fmt::format("{}/{}[{}]/{}", definition->mapConfigOption(propOption), definition->mapConfigOption(propOptions[1]), index, opt3);
        }
        return fmt::format("{}/{}[{}]/{}/{}", definition->mapConfigOption(propOption), definition->mapConfigOption(propOptions[1]), index, opt3, opt4);
    }
    if (propOptions.size() > 1 && propOptions.size() < 4) {
        return fmt::format("{}/{}/{}[{}]/{}", xpath, definition->mapConfigOption(propOption), definition->mapConfigOption(propOptions[1]), index, opt3);
    }
    return fmt::format("{}/{}/{}[{}]/{}/{}", xpath, definition->mapConfigOption(propOption), definition->mapConfigOption(propOptions.size() > 1 ? propOptions[1] : ConfigVal::MAX), index, opt3, opt4);
}

std::string ConfigTranscodingSetup::getItemPathRoot(bool prefix) const
{
    return xpath;
}

bool ConfigTranscodingSetup::updateDetail(const std::string& optItem,
    std::string& optValue,
    const std::shared_ptr<Config>& config,
    const std::map<std::string, std::string>* arguments)
{
    if (startswith(optItem, xpath) && optionValue) {
        auto value = std::dynamic_pointer_cast<TranscodingProfileListOption>(optionValue);
        log_debug("Updating Transcoding Detail {} {} {}", xpath, optItem, optValue);
        std::map<std::string, std::shared_ptr<TranscodingProfile>> profiles;
        std::size_t iFilter = 0;

        static auto resultFilterProperties = std::vector<ConfigResultProperty<TranscodingFilter>> {
            // Mimetype
            {
                { ConfigVal::A_TRANSCODING_MIMETYPE_FILTER, ConfigVal::A_TRANSCODING_MIMETYPE_PROF_MAP_MIMETYPE },
                "Mimetype",
                [&](const std::shared_ptr<TranscodingFilter>& entry) { return entry->getMimeType(); },
                [&](const std::shared_ptr<TranscodingFilter>& entry, const std::shared_ptr<ConfigDefinition>& definition, ConfigVal cfg, std::string& optValue) {
                    entry->setMimeType(optValue);
                    return true;
                },
            },
            // Transcoder
            {
                { ConfigVal::A_TRANSCODING_MIMETYPE_FILTER, ConfigVal::A_TRANSCODING_MIMETYPE_PROF_MAP_USING },
                "Transcoder",
                [&](const std::shared_ptr<TranscodingFilter>& entry) { return entry->getTranscoderName(); },
                [&](const std::shared_ptr<TranscodingFilter>& entry, const std::shared_ptr<ConfigDefinition>& definition, ConfigVal cfg, std::string& optValue) {
                    entry->setTranscoderName(optValue);
                    return true;
                },
            },
            // Client Flags
            {
                { ConfigVal::A_TRANSCODING_MIMETYPE_FILTER, ConfigVal::A_TRANSCODING_PROFILES_PROFLE_CLIENTFLAGS },
                "Client Flags",
                [&](const std::shared_ptr<TranscodingFilter>& entry) { return fmt::to_string(entry->getClientFlags()); },
                [&](const std::shared_ptr<TranscodingFilter>& entry, const std::shared_ptr<ConfigDefinition>& definition, ConfigVal cfg, std::string& optValue) {
                    entry->setClientFlags(definition->findConfigSetup<ConfigUIntSetup>(cfg)->checkIntValue(optValue));
                    return true;
                },
            },
            // Source DLNA Profile
            {
                { ConfigVal::A_TRANSCODING_MIMETYPE_FILTER, ConfigVal::A_TRANSCODING_PROFILES_PROFLE_SRCDLNA },
                "Source DLNA Profile",
                [&](const std::shared_ptr<TranscodingFilter>& entry) { return entry->getSourceProfile(); },
                [&](const std::shared_ptr<TranscodingFilter>& entry, const std::shared_ptr<ConfigDefinition>& definition, ConfigVal cfg, std::string& optValue) {
                    entry->setSourceProfile(optValue);
                    return true;
                },
            },
            // No Transcoding Mimetypes
            {
                { ConfigVal::A_TRANSCODING_MIMETYPE_FILTER, ConfigVal::A_TRANSCODING_PROFILES_PROFLE_NOTRANSCODING },
                "No Transcoding Mimetypes",
                [&](const std::shared_ptr<TranscodingFilter>& entry) { return fmt::format("{}", fmt::join(entry->getNoTranscodingMimeTypes(), ",")); },
                [&](const std::shared_ptr<TranscodingFilter>& entry, const std::shared_ptr<ConfigDefinition>& definition, ConfigVal cfg, std::string& optValue) {
                    std::vector<std::string> noTranscodingVector;
                    for (auto&& mime : splitString(optValue, ','))
                        noTranscodingVector.push_back(trimString(mime));
                    entry->setNoTranscodingMimeTypes(noTranscodingVector);
                    return true;
                },
            },
        };

        // update properties in profile part
        for (auto&& filter : value->getTranscodingProfileListOption()->getFilterList()) {
            std::vector<std::size_t> indexList = { iFilter };
            for (auto&& [cfg, label, getProperty, setProperty] : resultFilterProperties) {
                auto index = getItemPath(indexList, cfg);
                if (optItem == index) {
                    config->setOrigValue(index, getProperty(filter));
                    if (setProperty(filter, definition, cfg.at(0), optValue)) {
                        log_debug("New value for Transcoding Filter {} {} = {}", label.data(), index, getProperty(filter));
                        return true;
                    }
                }
            }

            if (filter->getTranscodingProfile())
                profiles[filter->getTranscodingProfile()->getName()] = filter->getTranscodingProfile();
            iFilter++;
        }

        static auto resultProfileProperties = std::vector<ConfigResultProperty<TranscodingProfile>> {
            // Enabled
            {
                { ConfigVal::A_TRANSCODING_PROFILES_PROFLE, ConfigVal::A_TRANSCODING_PROFILES_PROFLE_ENABLED },
                "Enabled",
                [&](const std::shared_ptr<TranscodingProfile>& entry) { return fmt::to_string(entry->isEnabled()); },
                [&](const std::shared_ptr<TranscodingProfile>& entry, const std::shared_ptr<ConfigDefinition>& definition, ConfigVal cfg, std::string& optValue) {
                    entry->setEnabled(definition->findConfigSetup<ConfigBoolSetup>(cfg)->checkValue(optValue));
                    return true;
                },
            },
            // Client Flags
            {
                { ConfigVal::A_TRANSCODING_PROFILES_PROFLE, ConfigVal::A_TRANSCODING_PROFILES_PROFLE_CLIENTFLAGS },
                "Client Flags",
                [&](const std::shared_ptr<TranscodingProfile>& entry) { return fmt::to_string(entry->getClientFlags()); },
                [&](const std::shared_ptr<TranscodingProfile>& entry, const std::shared_ptr<ConfigDefinition>& definition, ConfigVal cfg, std::string& optValue) {
                    entry->setClientFlags(definition->findConfigSetup<ConfigUIntSetup>(cfg)->checkIntValue(optValue));
                    return true;
                },
            },
            // Profile Type
            {
                { ConfigVal::A_TRANSCODING_PROFILES_PROFLE, ConfigVal::A_TRANSCODING_PROFILES_PROFLE_TYPE },
                "Profile Type",
                [&](const std::shared_ptr<TranscodingProfile>& entry) { return fmt::to_string(entry->getType()); },
                [&](const std::shared_ptr<TranscodingProfile>& entry, const std::shared_ptr<ConfigDefinition>& definition, ConfigVal cfg, std::string& optValue) {
                    auto setup = definition->findConfigSetup<ConfigEnumSetup<TranscodingType>>(cfg);
                    TranscodingType type;
                    if (setup->checkEnumValue(optValue, type)) {
                        entry->setType(type);
                        return true;
                    }
                    return false;
                },
            },
            // Target Mimetype
            {
                { ConfigVal::A_TRANSCODING_PROFILES_PROFLE, ConfigVal::A_TRANSCODING_PROFILES_PROFLE_MIMETYPE },
                "Target Mimetype",
                [&](const std::shared_ptr<TranscodingProfile>& entry) { return entry->getTargetMimeType(); },
                [&](const std::shared_ptr<TranscodingProfile>& entry, const std::shared_ptr<ConfigDefinition>& definition, ConfigVal cfg, std::string& optValue) {
                    if (definition->findConfigSetup<ConfigStringSetup>(cfg)->checkValue(optValue)) {
                        entry->setTargetMimeType(optValue);
                        return true;
                    }
                    return false;
                },
            },
            // Resolution
            {
                { ConfigVal::A_TRANSCODING_PROFILES_PROFLE, ConfigVal::A_TRANSCODING_PROFILES_PROFLE_RESOLUTION },
                "Resolution",
                [&](const std::shared_ptr<TranscodingProfile>& entry) { return entry->getAttributeOverride(ResourceAttribute::RESOLUTION); },
                [&](const std::shared_ptr<TranscodingProfile>& entry, const std::shared_ptr<ConfigDefinition>& definition, ConfigVal cfg, std::string& optValue) {
                    entry->setAttributeOverride(ResourceAttribute::RESOLUTION, optValue);
                    return true;
                },
            },
            // AcceptUrl
            {
                { ConfigVal::A_TRANSCODING_PROFILES_PROFLE, ConfigVal::A_TRANSCODING_PROFILES_PROFLE_ACCURL },
                "AcceptUrl",
                [&](const std::shared_ptr<TranscodingProfile>& entry) { return fmt::to_string(entry->getAcceptURL()); },
                [&](const std::shared_ptr<TranscodingProfile>& entry, const std::shared_ptr<ConfigDefinition>& definition, ConfigVal cfg, std::string& optValue) {
                    entry->setAcceptURL(definition->findConfigSetup<ConfigBoolSetup>(cfg)->checkValue(optValue));
                    return true;
                },
            },
            // DLNA Profile
            {
                { ConfigVal::A_TRANSCODING_PROFILES_PROFLE, ConfigVal::A_TRANSCODING_PROFILES_PROFLE_DLNAPROF },
                "DLNA Profile",
                [&](const std::shared_ptr<TranscodingProfile>& entry) { return entry->getDlnaProfile(); },
                [&](const std::shared_ptr<TranscodingProfile>& entry, const std::shared_ptr<ConfigDefinition>& definition, ConfigVal cfg, std::string& optValue) {
                    if (definition->findConfigSetup<ConfigStringSetup>(cfg)->checkValue(optValue)) {
                        entry->setDlnaProfile(optValue);
                        return true;
                    }
                    return false;
                },
            },
            // SampleFrequency
            {
                { ConfigVal::A_TRANSCODING_PROFILES_PROFLE, ConfigVal::A_TRANSCODING_PROFILES_PROFLE_SAMPFREQ },
                "SampleFrequency",
                [&](const std::shared_ptr<TranscodingProfile>& entry) { return fmt::to_string(entry->getSampleFreq()); },
                [&](const std::shared_ptr<TranscodingProfile>& entry, const std::shared_ptr<ConfigDefinition>& definition, ConfigVal cfg, std::string& optValue) {
                    entry->setSampleFreq(definition->findConfigSetup<ConfigIntSetup>(cfg)->checkIntValue(optValue));
                    return true;
                },
            },
            // NumChannels
            {
                { ConfigVal::A_TRANSCODING_PROFILES_PROFLE, ConfigVal::A_TRANSCODING_PROFILES_PROFLE_NRCHAN },
                "NumChannels",
                [&](const std::shared_ptr<TranscodingProfile>& entry) { return fmt::to_string(entry->getNumChannels()); },
                [&](const std::shared_ptr<TranscodingProfile>& entry, const std::shared_ptr<ConfigDefinition>& definition, ConfigVal cfg, std::string& optValue) {
                    entry->setNumChannels(definition->findConfigSetup<ConfigIntSetup>(cfg)->checkIntValue(optValue));
                    return true;
                },
            },
            // hideOriginalResource
            {
                { ConfigVal::A_TRANSCODING_PROFILES_PROFLE, ConfigVal::A_TRANSCODING_PROFILES_PROFLE_HIDEORIG },
                "Hide Original Resource",
                [&](const std::shared_ptr<TranscodingProfile>& entry) { return fmt::to_string(entry->hideOriginalResource()); },
                [&](const std::shared_ptr<TranscodingProfile>& entry, const std::shared_ptr<ConfigDefinition>& definition, ConfigVal cfg, std::string& optValue) {
                    entry->setHideOriginalResource(definition->findConfigSetup<ConfigBoolSetup>(cfg)->checkValue(optValue));
                    return true;
                },
            },
            // Thumbnail
            {
                { ConfigVal::A_TRANSCODING_PROFILES_PROFLE, ConfigVal::A_TRANSCODING_PROFILES_PROFLE_THUMB },
                "Thumbnail",
                [&](const std::shared_ptr<TranscodingProfile>& entry) { return fmt::to_string(entry->isThumbnail()); },
                [&](const std::shared_ptr<TranscodingProfile>& entry, const std::shared_ptr<ConfigDefinition>& definition, ConfigVal cfg, std::string& optValue) {
                    entry->setThumbnail(definition->findConfigSetup<ConfigBoolSetup>(cfg)->checkValue(optValue));
                    return true;
                },
            },
            // FirstResource
            {
                { ConfigVal::A_TRANSCODING_PROFILES_PROFLE, ConfigVal::A_TRANSCODING_PROFILES_PROFLE_FIRST },
                "First Resource",
                [&](const std::shared_ptr<TranscodingProfile>& entry) { return fmt::to_string(entry->getFirstResource()); },
                [&](const std::shared_ptr<TranscodingProfile>& entry, const std::shared_ptr<ConfigDefinition>& definition, ConfigVal cfg, std::string& optValue) {
                    entry->setFirstResource(definition->findConfigSetup<ConfigBoolSetup>(cfg)->checkValue(optValue));
                    return true;
                },
            },
            // Accept OggTheora
            {
                { ConfigVal::A_TRANSCODING_PROFILES_PROFLE, ConfigVal::A_TRANSCODING_PROFILES_PROFLE_ACCOGG },
                "Accept OggTheora",
                [&](const std::shared_ptr<TranscodingProfile>& entry) { return fmt::to_string(entry->isTheora()); },
                [&](const std::shared_ptr<TranscodingProfile>& entry, const std::shared_ptr<ConfigDefinition>& definition, ConfigVal cfg, std::string& optValue) {
                    entry->setTheora(definition->findConfigSetup<ConfigBoolSetup>(cfg)->checkValue(optValue));
                    return true;
                },
            },

            // Buffer
            // Buffersize
            {
                { ConfigVal::A_TRANSCODING_PROFILES_PROFLE, ConfigVal::A_TRANSCODING_PROFILES_PROFLE_BUFFER, ConfigVal::A_TRANSCODING_PROFILES_PROFLE_BUFFER_SIZE },
                "Buffersize",
                [&](const std::shared_ptr<TranscodingProfile>& entry) { return fmt::to_string(entry->getBufferSize()); },
                [&](const std::shared_ptr<TranscodingProfile>& entry, const std::shared_ptr<ConfigDefinition>& definition, ConfigVal cfg, std::string& optValue) {
                    std::size_t buffer = definition->findConfigSetup<ConfigUIntSetup>(cfg)->checkIntValue(optValue);
                    std::size_t chunk = entry->getBufferChunkSize();
                    std::size_t fill = entry->getBufferInitialFillSize();
                    entry->setBufferOptions(buffer, chunk, fill);
                    return true;
                },
            },
            // BufferChunkSize
            {
                { ConfigVal::A_TRANSCODING_PROFILES_PROFLE, ConfigVal::A_TRANSCODING_PROFILES_PROFLE_BUFFER, ConfigVal::A_TRANSCODING_PROFILES_PROFLE_BUFFER_CHUNK },
                "BufferChunkSize",
                [&](const std::shared_ptr<TranscodingProfile>& entry) { return fmt::to_string(entry->getBufferChunkSize()); },
                [&](const std::shared_ptr<TranscodingProfile>& entry, const std::shared_ptr<ConfigDefinition>& definition, ConfigVal cfg, std::string& optValue) {
                    std::size_t buffer = entry->getBufferSize();
                    std::size_t chunk = definition->findConfigSetup<ConfigUIntSetup>(cfg)->checkIntValue(optValue);
                    std::size_t fill = entry->getBufferInitialFillSize();
                    entry->setBufferOptions(buffer, chunk, fill);
                    return true;
                },
            },
            // BufferChunkFill
            {
                { ConfigVal::A_TRANSCODING_PROFILES_PROFLE, ConfigVal::A_TRANSCODING_PROFILES_PROFLE_BUFFER, ConfigVal::A_TRANSCODING_PROFILES_PROFLE_BUFFER_FILL },
                "BufferChunkFill",
                [&](const std::shared_ptr<TranscodingProfile>& entry) { return fmt::to_string(entry->getBufferInitialFillSize()); },
                [&](const std::shared_ptr<TranscodingProfile>& entry, const std::shared_ptr<ConfigDefinition>& definition, ConfigVal cfg, std::string& optValue) {
                    std::size_t buffer = entry->getBufferSize();
                    std::size_t chunk = entry->getBufferChunkSize();
                    std::size_t fill = definition->findConfigSetup<ConfigUIntSetup>(cfg)->checkIntValue(optValue);
                    entry->setBufferOptions(buffer, chunk, fill);
                    return true;
                },
            },

            // Agent
            // Agent Command
            {
                { ConfigVal::A_TRANSCODING_PROFILES_PROFLE, ConfigVal::A_TRANSCODING_PROFILES_PROFLE_AGENT, ConfigVal::A_TRANSCODING_PROFILES_PROFLE_AGENT_COMMAND },
                "Agent Command",
                [&](const std::shared_ptr<TranscodingProfile>& entry) { return entry->getCommand(); },
                [&](const std::shared_ptr<TranscodingProfile>& entry, const std::shared_ptr<ConfigDefinition>& definition, ConfigVal cfg, std::string& optValue) {
                    if (definition->findConfigSetup<ConfigStringSetup>(cfg)->checkValue(optValue)) {
                        entry->setCommand(optValue);
                        return true;
                    }
                    return false;
                },
            },
            // Agent Arguments
            {
                { ConfigVal::A_TRANSCODING_PROFILES_PROFLE, ConfigVal::A_TRANSCODING_PROFILES_PROFLE_AGENT, ConfigVal::A_TRANSCODING_PROFILES_PROFLE_AGENT_ARGS },
                "Agent Arguments",
                [&](const std::shared_ptr<TranscodingProfile>& entry) { return entry->getArguments(); },
                [&](const std::shared_ptr<TranscodingProfile>& entry, const std::shared_ptr<ConfigDefinition>& definition, ConfigVal cfg, std::string& optValue) {
                    if (definition->findConfigSetup<ConfigStringSetup>(cfg)->checkValue(optValue)) {
                        entry->setArguments(optValue);
                        return true;
                    }
                    return false;
                },
            },

            // 4CC
            // 4CC Mode
            {
                { ConfigVal::A_TRANSCODING_PROFILES_PROFLE, ConfigVal::A_TRANSCODING_PROFILES_PROFLE_AVI4CC, ConfigVal::A_TRANSCODING_PROFILES_PROFLE_AVI4CC_MODE },
                "4CC Mode",
                [&](const std::shared_ptr<TranscodingProfile>& entry) { return fmt::to_string(entry->getAVIFourCCListMode()); },
                [&](const std::shared_ptr<TranscodingProfile>& entry, const std::shared_ptr<ConfigDefinition>& definition, ConfigVal cfg, std::string& optValue) {
                    AviFourccListmode fccMode;
                    auto fccList = entry->getAVIFourCCList();
                    auto setup = definition->findConfigSetup<ConfigEnumSetup<AviFourccListmode>>(ConfigVal::A_TRANSCODING_PROFILES_PROFLE_AVI4CC_MODE);
                    if (setup->checkEnumValue(optValue, fccMode)) {
                        entry->setAVIFourCCList(fccList, fccMode);
                        return true;
                    }
                    return false;
                },
            },
            // 4CC List
            {
                { ConfigVal::A_TRANSCODING_PROFILES_PROFLE, ConfigVal::A_TRANSCODING_PROFILES_PROFLE_AVI4CC, ConfigVal::A_TRANSCODING_PROFILES_PROFLE_AVI4CC_4CC },
                "4CC List",
                [&](const std::shared_ptr<TranscodingProfile>& entry) { return fmt::format("{}", fmt::join(entry->getAVIFourCCList(), ", ")); },
                [&](const std::shared_ptr<TranscodingProfile>& entry, const std::shared_ptr<ConfigDefinition>& definition, ConfigVal cfg, std::string& optValue) {
                    AviFourccListmode fccMode = entry->getAVIFourCCListMode();
                    std::vector<std::string> fccList;
                    if (definition->findConfigSetup<ConfigArraySetup>(ConfigVal::A_TRANSCODING_PROFILES_PROFLE_AVI4CC)->checkArrayValue(optValue, fccList)) {
                        entry->setAVIFourCCList(fccList, fccMode);
                        return true;
                    }
                    return false;
                },
            },
        };

        std::size_t iProfile = 0;
        // update properties in transcoding part
        for (auto&& [name, entry] : profiles) {
            std::vector<std::size_t> indexList = { iProfile };

            {
                auto index = getItemPath(indexList, { ConfigVal::A_TRANSCODING_PROFILES_PROFLE, ConfigVal::A_TRANSCODING_PROFILES_PROFLE_NAME });
                if (optItem == index) {
                    log_error("Cannot change profile name in Transcoding Detail {} {}", index, name);
                    return false;
                }
            }

            // cache buffer options
            bool setBuffer = false;
            std::size_t bufferOrig = entry->getBufferSize();
            std::size_t chunkOrig = entry->getBufferChunkSize();
            std::size_t fillOrig = entry->getBufferInitialFillSize();

            for (auto&& [cfg, label, getProperty, setProperty] : resultProfileProperties) {
                auto index = getItemPath(indexList, cfg);
                if (optItem == index) {
                    auto origValue = getProperty(entry);
                    if (setProperty(entry, definition, cfg.at(0), optValue)) {
                        config->setOrigValue(index, origValue);
                        log_debug("New value for Transcoding Profile {} {} = {}", label.data(), index, getProperty(config->getTranscodingProfileListOption(option)->getByName(entry->getName(), true)));
                        if (cfg.at(1) == ConfigVal::A_TRANSCODING_PROFILES_PROFLE_BUFFER) {
                            setBuffer = true;
                            break; // do check and return outside loop
                        }
                        return true;
                    }
                }
            }

            if (setBuffer) {
                std::size_t buffer = entry->getBufferSize();
                std::size_t chunk = entry->getBufferChunkSize();
                std::size_t fill = entry->getBufferInitialFillSize();
                if (chunk > buffer) {
                    log_error("Error in configuration: transcoding profile \"{}\" chunk size {} can not be greater than buffer size {}",
                        entry->getName(), chunk, buffer);
                    entry->setBufferOptions(bufferOrig, chunkOrig, fillOrig);
                    return false;
                }
                if (fill > buffer) {
                    log_error("Error in configuration: transcoding profile \"{}\" fill size {} can not be greater than buffer size {}",
                        entry->getName(), fill, buffer);
                    entry->setBufferOptions(bufferOrig, chunkOrig, fillOrig);
                    return false;
                }
                return true;
            }

            iProfile++;
        }
    }
    return false;
}

std::shared_ptr<ConfigOption> ConfigTranscodingSetup::newOption(const pugi::xml_node& optValue)
{
    auto result = std::make_shared<TranscodingProfileList>();

    if (!createOptionFromNode(isEnabled ? optValue : pugi::xml_node(nullptr), result)) {
        throw_std_runtime_error("Init {} transcoding failed '{}'", xpath, optValue.name());
    }
    optionValue = std::make_shared<TranscodingProfileListOption>(result);
    return optionValue;
}
