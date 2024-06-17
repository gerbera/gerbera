/*GRB*

    Gerbera - https://gerbera.io/

    config_setup_transcoding.cc - this file is part of Gerbera.
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

/// \file config_setup_transcoding.cc
#define GRB_LOG_FAC GrbLogFacility::config

#include "config_setup_transcoding.h" // API

#include "config/config_definition.h"
#include "config/config_option_enum.h"
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
#include "util/logger.h"

#include <numeric>

/// \brief Creates an array of TranscodingProfile objects from an XML
/// nodeset.
/// \param element starting element of the nodeset.
bool ConfigTranscodingSetup::createOptionFromNode(const pugi::xml_node& element, std::shared_ptr<TranscodingProfileList>& result)
{
    if (!element)
        return true;

    const pugi::xml_node& root = element.root();

    // initialize mapping dictionary
    std::vector<std::shared_ptr<TranscodingFilter>> trFilters;
    auto fcs = ConfigDefinition::findConfigSetup<ConfigSetup>(ConfigVal::A_TRANSCODING_MIMETYPE_FILTER);
    const auto filterNodes = fcs->getXmlTree(element);
    if (filterNodes.empty())
        return true;

    auto pcs = ConfigDefinition::findConfigSetup<ConfigSetup>(ConfigVal::A_TRANSCODING_PROFILES_PROFLE);
    const auto profileNodes = pcs->getXmlTree(element);
    if (profileNodes.empty())
        return true;

    // go through filters
    for (auto&& it : filterNodes) {
        const pugi::xml_node child = it.node();

        auto filter = std::make_shared<TranscodingFilter>(
            ConfigDefinition::findConfigSetup<ConfigStringSetup>(ConfigVal::A_TRANSCODING_MIMETYPE_PROF_MAP_MIMETYPE)->getXmlContent(child),
            ConfigDefinition::findConfigSetup<ConfigStringSetup>(ConfigVal::A_TRANSCODING_MIMETYPE_PROF_MAP_USING)->getXmlContent(child));
        filter->setSourceProfile(ConfigDefinition::findConfigSetup<ConfigStringSetup>(ConfigVal::A_TRANSCODING_PROFILES_PROFLE_SRCDLNA)->getXmlContent(child));
        filter->setClientFlags(ConfigDefinition::findConfigSetup<ConfigUIntSetup>(ConfigVal::A_TRANSCODING_PROFILES_PROFLE_CLIENTFLAGS)->getXmlContent(child));
        auto noTranscoding = ConfigDefinition::findConfigSetup<ConfigStringSetup>(ConfigVal::A_TRANSCODING_PROFILES_PROFLE_NOTRANSCODING)->getXmlContent(child);
        std::vector<std::string> noTranscodingVector;
        for (auto&& mime : splitString(noTranscoding, ','))
            noTranscodingVector.push_back(trimString(mime));
        filter->setNoTranscodingMimeTypes(noTranscodingVector);
        trFilters.push_back(filter);
        result->add(filter);
    }

    bool allowUnusedProfiles = !ConfigDefinition::findConfigSetup<ConfigBoolSetup>(ConfigVal::TRANSCODING_PROFILES_PROFILE_ALLOW_UNUSED)->getXmlContent(root);
    if (!allowUnusedProfiles && trFilters.empty()) {
        log_error("error in configuration: transcoding "
                  "profiles exist, but no mimetype to profile mappings specified");
        return false;
    }

    // go through profiles
    for (auto&& it : profileNodes) {
        const pugi::xml_node child = it.node();

        auto prof = std::make_shared<TranscodingProfile>(
            ConfigDefinition::findConfigSetup<ConfigBoolSetup>(ConfigVal::A_TRANSCODING_PROFILES_PROFLE_ENABLED)->getXmlContent(child),
            ConfigDefinition::findConfigSetup<ConfigEnumSetup<TranscodingType>>(ConfigVal::A_TRANSCODING_PROFILES_PROFLE_TYPE)->getXmlContent(child),
            ConfigDefinition::findConfigSetup<ConfigStringSetup>(ConfigVal::A_TRANSCODING_PROFILES_PROFLE_NAME)->getXmlContent(child));
        prof->setTargetMimeType(ConfigDefinition::findConfigSetup<ConfigStringSetup>(ConfigVal::A_TRANSCODING_PROFILES_PROFLE_MIMETYPE)->getXmlContent(child));
        prof->setClientFlags(ConfigDefinition::findConfigSetup<ConfigUIntSetup>(ConfigVal::A_TRANSCODING_PROFILES_PROFLE_CLIENTFLAGS)->getXmlContent(child));

        pugi::xml_node sub;
        sub = ConfigDefinition::findConfigSetup<ConfigSetup>(ConfigVal::A_TRANSCODING_PROFILES_PROFLE_RES)->getXmlElement(child);
        if (sub) {
            std::string param = sub.text().as_string();
            if (!param.empty()) {
                try {
                    auto res = Resolution(param);
                    prof->setAttributeOverride(ResourceAttribute::RESOLUTION, res.string());
                } catch (const std::runtime_error& e) {
                    log_info("Config setup for resolution {} is invalid", param);
                }
            }
        }

        // read 4cc options
        {
            auto cs = ConfigDefinition::findConfigSetup<ConfigArraySetup>(ConfigVal::A_TRANSCODING_PROFILES_PROFLE_AVI4CC);
            if (cs->hasXmlElement(child)) {
                sub = cs->getXmlElement(child);
                AviFourccListmode fccMode = ConfigDefinition::findConfigSetup<ConfigEnumSetup<AviFourccListmode>>(ConfigVal::A_TRANSCODING_PROFILES_PROFLE_AVI4CC_MODE)->getXmlContent(sub);
                if (fccMode != AviFourccListmode::None) {
                    prof->setAVIFourCCList(cs->getXmlContent(sub), fccMode);
                }
            }
        }
        // read profile options
        {
            auto cs = ConfigDefinition::findConfigSetup<ConfigStringSetup>(ConfigVal::A_TRANSCODING_PROFILES_PROFLE_DLNAPROF);
            if (cs->hasXmlElement(child))
                prof->setDlnaProfile(cs->getXmlContent(child));
        }
        {
            auto cs = ConfigDefinition::findConfigSetup<ConfigBoolSetup>(ConfigVal::A_TRANSCODING_PROFILES_PROFLE_ACCURL);
            if (cs->hasXmlElement(child))
                prof->setAcceptURL(cs->getXmlContent(child));
        }
        {
            auto cs = ConfigDefinition::findConfigSetup<ConfigIntSetup>(ConfigVal::A_TRANSCODING_PROFILES_PROFLE_SAMPFREQ);
            if (cs->hasXmlElement(child))
                prof->setSampleFreq(cs->getXmlContent(child));
        }
        {
            auto cs = ConfigDefinition::findConfigSetup<ConfigIntSetup>(ConfigVal::A_TRANSCODING_PROFILES_PROFLE_NRCHAN);
            if (cs->hasXmlElement(child))
                prof->setNumChannels(cs->getXmlContent(child));
        }
        {
            auto cs = ConfigDefinition::findConfigSetup<ConfigBoolSetup>(ConfigVal::A_TRANSCODING_PROFILES_PROFLE_HIDEORIG);
            if (cs->hasXmlElement(child))
                prof->setHideOriginalResource(cs->getXmlContent(child));
        }
        {
            auto cs = ConfigDefinition::findConfigSetup<ConfigBoolSetup>(ConfigVal::A_TRANSCODING_PROFILES_PROFLE_THUMB);
            if (cs->hasXmlElement(child))
                prof->setThumbnail(cs->getXmlContent(child));
        }
        {
            auto cs = ConfigDefinition::findConfigSetup<ConfigBoolSetup>(ConfigVal::A_TRANSCODING_PROFILES_PROFLE_FIRST);
            if (cs->hasXmlElement(child))
                prof->setFirstResource(cs->getXmlContent(child));
        }
        {
            auto cs = ConfigDefinition::findConfigSetup<ConfigBoolSetup>(ConfigVal::A_TRANSCODING_PROFILES_PROFLE_ACCOGG);
            if (cs->hasXmlElement(child))
                prof->setTheora(cs->getXmlContent(child));
        }

        // read agent options
        sub = ConfigDefinition::findConfigSetup<ConfigSetup>(ConfigVal::A_TRANSCODING_PROFILES_PROFLE_AGENT)->getXmlElement(child);
        {
            auto cs = ConfigDefinition::findConfigSetup<ConfigPathSetup>(ConfigVal::A_TRANSCODING_PROFILES_PROFLE_AGENT_COMMAND);
            cs->setFlag(prof->isEnabled(), ConfigPathArguments::mustExist);
            prof->setCommand(cs->getXmlContent(sub));
        }
        prof->setArguments(ConfigDefinition::findConfigSetup<ConfigStringSetup>(ConfigVal::A_TRANSCODING_PROFILES_PROFLE_AGENT_ARGS)->getXmlContent(sub));
        {
            auto cs = ConfigDefinition::findConfigSetup<ConfigDictionarySetup>(ConfigVal::A_TRANSCODING_PROFILES_PROFLE_AGENT_ENVIRON);
            if (cs->hasXmlElement(child))
                prof->setEnviron(cs->getXmlContent(cs->getXmlElement(child)));
        }

        // set buffer options
        sub = ConfigDefinition::findConfigSetup<ConfigSetup>(ConfigVal::A_TRANSCODING_PROFILES_PROFLE_BUFFER)->getXmlElement(child);
        std::size_t buffer = ConfigDefinition::findConfigSetup<ConfigIntSetup>(ConfigVal::A_TRANSCODING_PROFILES_PROFLE_BUFFER_SIZE)->getXmlContent(sub);
        std::size_t chunk = ConfigDefinition::findConfigSetup<ConfigIntSetup>(ConfigVal::A_TRANSCODING_PROFILES_PROFLE_BUFFER_CHUNK)->getXmlContent(sub);
        std::size_t fill = ConfigDefinition::findConfigSetup<ConfigIntSetup>(ConfigVal::A_TRANSCODING_PROFILES_PROFLE_BUFFER_FILL)->getXmlContent(sub);

        if (chunk > buffer) {
            log_error("Error in configuration: transcoding profile \"{}\" chunk size can not be greater than buffer size", prof->getName());
            return false;
        }
        if (fill > buffer) {
            log_error("Error in configuration: transcoding profile \"{}\" fill size can not be greater than buffer size", prof->getName());
            return false;
        }

        prof->setBufferOptions(buffer, chunk, fill);

        bool set = false;
        for (auto&& filter : trFilters) {
            if (filter->getTranscoderName() == prof->getName()) {
                filter->setTranscodingProfile(prof);
                set = true;
            }
        }

        if (!set) {
            log_error("Error in configuration: you specified a mimetype to transcoding profile mapping, but no match for profile \"{}\" exists", prof->getName());
            if (!allowUnusedProfiles) {
                return false;
            }
        }
    }

    // validate profiles
    for (auto&& filter : trFilters) {
        if (!filter->getTranscodingProfile()) {
            log_error("Error in configuration: you specified a mimetype to transcoding profile mapping, but the profile \"{}\" for mimetype \"{}\" does not exists", filter->getTranscoderName(), filter->getMimeType());
            if (!ConfigDefinition::findConfigSetup<ConfigBoolSetup>(ConfigVal::TRANSCODING_MIMETYPE_PROF_MAP_ALLOW_UNUSED)->getXmlContent(root)) {
                return false;
            }
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

std::string ConfigTranscodingSetup::getItemPath(int index, const std::vector<ConfigVal>& propOptions) const
{
    if (index < 0) {
        return fmt::to_string(xpath);
    }

    auto opt2 = ConfigDefinition::ensureAttribute(propOptions.size() > 1 ? propOptions[1] : ConfigVal::MAX, propOptions.size() == 2);
    auto opt3 = ConfigDefinition::ensureAttribute(propOptions.size() > 2 ? propOptions[2] : ConfigVal::MAX, propOptions.size() == 3);
    auto opt4 = ConfigDefinition::ensureAttribute(propOptions.size() > 3 ? propOptions[3] : ConfigVal::MAX, propOptions.size() > 4);
    auto propOption = propOptions.size() > 0 ? propOptions[0] : ConfigVal::MAX;

    if (propOption == ConfigVal::A_TRANSCODING_PROFILES_PROFLE || propOption == ConfigVal::A_TRANSCODING_MIMETYPE_FILTER) {
        if (propOptions.size() < 3) {
            return fmt::format("{}[{}]/{}", ConfigDefinition::mapConfigOption(propOption), index, opt2);
        }
        return fmt::format("{}[{}]/{}/{}", ConfigDefinition::mapConfigOption(propOption), index, opt2, opt3);
    }
    if (propOption == ConfigVal::A_TRANSCODING_MIMETYPE_PROF_MAP) {
        if (propOptions.size() < 4) {
            return fmt::format("{}/{}[{}]/{}", ConfigDefinition::mapConfigOption(propOption), ConfigDefinition::mapConfigOption(propOptions[1]), index, opt3);
        }
        return fmt::format("{}/{}[{}]/{}/{}", ConfigDefinition::mapConfigOption(propOption), ConfigDefinition::mapConfigOption(propOptions[1]), index, opt3, opt4);
    }
    if (propOptions.size() > 1 && propOptions.size() < 4) {
        return fmt::format("{}/{}/{}[{}]/{}", xpath, ConfigDefinition::mapConfigOption(propOption), ConfigDefinition::mapConfigOption(propOptions[1]), index, opt3);
    }
    return fmt::format("{}/{}/{}[{}]/{}/{}", xpath, ConfigDefinition::mapConfigOption(propOption), ConfigDefinition::mapConfigOption(propOptions.size() > 1 ? propOptions[1] : ConfigVal::MAX), index, opt3, opt4);
}

bool ConfigTranscodingSetup::updateDetail(const std::string& optItem, std::string& optValue, const std::shared_ptr<Config>& config, const std::map<std::string, std::string>* arguments)
{
    if (startswith(optItem, xpath) && optionValue) {
        auto value = std::dynamic_pointer_cast<TranscodingProfileListOption>(optionValue);
        log_debug("Updating Transcoding Detail {} {} {}", xpath, optItem, optValue);
        std::map<std::string, std::shared_ptr<TranscodingProfile>> profiles;
        int i = 0;

        // update properties in profile part
        for (auto&& filter : value->getTranscodingProfileListOption()->getFilterList()) {
            auto index = getItemPath(i, { ConfigVal::A_TRANSCODING_MIMETYPE_FILTER, ConfigVal::A_TRANSCODING_MIMETYPE_PROF_MAP_MIMETYPE });
            if (optItem == index) {
                config->setOrigValue(index, filter->getMimeType());
                filter->setMimeType(optValue);
                log_debug("New Transcoding Detail {} {}", index, filter->getMimeType());
                return true;
            }
            index = getItemPath(i, { ConfigVal::A_TRANSCODING_MIMETYPE_FILTER, ConfigVal::A_TRANSCODING_MIMETYPE_PROF_MAP_USING });
            if (optItem == index) {
                log_error("Cannot change profile name in Transcoding Detail {} {}", index, filter->getTranscoderName());
                filter->setTranscoderName(optValue);
                log_debug("New Transcoding Detail {} {}", index, filter->getTranscoderName());
                return true;
            }
            index = getItemPath(i, { ConfigVal::A_TRANSCODING_MIMETYPE_FILTER, ConfigVal::A_TRANSCODING_PROFILES_PROFLE_CLIENTFLAGS });
            if (optItem == index) {
                log_error("Cannot change profile name in Transcoding Detail {} {}", index, filter->getClientFlags());
                filter->setClientFlags(ConfigDefinition::findConfigSetup<ConfigUIntSetup>(ConfigVal::A_TRANSCODING_PROFILES_PROFLE_CLIENTFLAGS)->checkIntValue(optValue));
                log_debug("New Transcoding Detail {} {}", index, filter->getClientFlags());
                return true;
            }
            index = getItemPath(i, { ConfigVal::A_TRANSCODING_MIMETYPE_FILTER, ConfigVal::A_TRANSCODING_PROFILES_PROFLE_SRCDLNA });
            if (optItem == index) {
                log_error("Cannot change profile name in Transcoding Detail {} {}", index, filter->getSourceProfile());
                filter->setSourceProfile(optValue);
                log_debug("New Transcoding Detail {} {}", index, filter->getSourceProfile());
                return true;
            }
            index = getItemPath(i, { ConfigVal::A_TRANSCODING_MIMETYPE_FILTER, ConfigVal::A_TRANSCODING_PROFILES_PROFLE_NOTRANSCODING });
            if (optItem == index) {
                log_error("Cannot change profile name in Transcoding Detail {} {}", index, fmt::join(filter->getNoTranscodingMimeTypes(), ","));
                std::vector<std::string> noTranscodingVector;
                for (auto&& mime : splitString(optValue, ','))
                    noTranscodingVector.push_back(trimString(mime));
                filter->setNoTranscodingMimeTypes(noTranscodingVector);
                log_debug("New Transcoding Detail {} {}", index, fmt::join(filter->getNoTranscodingMimeTypes(), ","));
                return true;
            }

            if (filter->getTranscodingProfile())
                profiles[filter->getTranscodingProfile()->getName()] = filter->getTranscodingProfile();
            i++;
        }

        i = 0;
        // update properties in transcoding part
        for (auto&& [name, entry] : profiles) {
            auto index = getItemPath(i, { ConfigVal::A_TRANSCODING_PROFILES_PROFLE, ConfigVal::A_TRANSCODING_PROFILES_PROFLE_NAME });
            if (optItem == index) {
                log_error("Cannot change profile name in Transcoding Detail {} {}", index, name);
                return false;
            }
            index = getItemPath(i, { ConfigVal::A_TRANSCODING_PROFILES_PROFLE, ConfigVal::A_TRANSCODING_PROFILES_PROFLE_ENABLED });
            if (optItem == index) {
                config->setOrigValue(index, entry->isEnabled());
                entry->setEnabled(ConfigDefinition::findConfigSetup<ConfigBoolSetup>(ConfigVal::A_TRANSCODING_PROFILES_PROFLE_ENABLED)->checkValue(optValue));
                log_debug("New Transcoding Detail {} {}", index, config->getTranscodingProfileListOption(option)->getByName(entry->getName(), true)->isEnabled());
                return true;
            }
            index = getItemPath(i, { ConfigVal::A_TRANSCODING_PROFILES_PROFLE, ConfigVal::A_TRANSCODING_PROFILES_PROFLE_CLIENTFLAGS });
            if (optItem == index) {
                config->setOrigValue(index, entry->getClientFlags());
                entry->setClientFlags(ConfigDefinition::findConfigSetup<ConfigUIntSetup>(ConfigVal::A_TRANSCODING_PROFILES_PROFLE_CLIENTFLAGS)->checkIntValue(optValue));
                log_debug("New Transcoding Detail {} {}", index, config->getTranscodingProfileListOption(option)->getByName(entry->getName(), true)->getClientFlags());
                return true;
            }
            index = getItemPath(i, { ConfigVal::A_TRANSCODING_PROFILES_PROFLE, ConfigVal::A_TRANSCODING_PROFILES_PROFLE_TYPE });
            if (optItem == index) {
                TranscodingType type;
                auto setup = ConfigDefinition::findConfigSetup<ConfigEnumSetup<TranscodingType>>(ConfigVal::A_TRANSCODING_PROFILES_PROFLE_TYPE);
                if (setup->checkEnumValue(optValue, type)) {
                    config->setOrigValue(index, setup->mapEnumValue(entry->getType()));
                    entry->setType(type);
                    log_debug("New Transcoding Detail {} {}", index, config->getTranscodingProfileListOption(option)->getByName(entry->getName(), true)->getType());
                    return true;
                }
            }
            index = getItemPath(i, { ConfigVal::A_TRANSCODING_PROFILES_PROFLE, ConfigVal::A_TRANSCODING_PROFILES_PROFLE_MIMETYPE });
            if (optItem == index) {
                if (ConfigDefinition::findConfigSetup<ConfigStringSetup>(ConfigVal::A_TRANSCODING_PROFILES_PROFLE_MIMETYPE)->checkValue(optValue)) {
                    config->setOrigValue(index, entry->getTargetMimeType());
                    entry->setTargetMimeType(optValue);
                    log_debug("New Transcoding Detail {} {}", index, config->getTranscodingProfileListOption(option)->getByName(entry->getName(), true)->getTargetMimeType());
                    return true;
                }
            }

            // update profile options
            index = getItemPath(i, { ConfigVal::A_TRANSCODING_PROFILES_PROFLE, ConfigVal::A_TRANSCODING_PROFILES_PROFLE_RES });
            if (optItem == index) {
                if (ConfigDefinition::findConfigSetup<ConfigStringSetup>(ConfigVal::A_TRANSCODING_PROFILES_PROFLE_RES)->checkValue(optValue)) {
                    config->setOrigValue(index, entry->getAttributeOverride(ResourceAttribute::RESOLUTION));
                    entry->setAttributeOverride(ResourceAttribute::RESOLUTION, optValue);
                    log_debug("New Transcoding Detail {} {}", index, config->getTranscodingProfileListOption(option)->getByName(entry->getName(), true)->getAttributeOverride(ResourceAttribute::RESOLUTION));
                    return true;
                }
            }
            index = getItemPath(i, { ConfigVal::A_TRANSCODING_PROFILES_PROFLE, ConfigVal::A_TRANSCODING_PROFILES_PROFLE_ACCURL });
            if (optItem == index) {
                config->setOrigValue(index, entry->getAcceptURL());
                entry->setAcceptURL(ConfigDefinition::findConfigSetup<ConfigBoolSetup>(ConfigVal::A_TRANSCODING_PROFILES_PROFLE_ACCURL)->checkValue(optValue));
                log_debug("New Transcoding Detail {} {}", index, config->getTranscodingProfileListOption(option)->getByName(entry->getName(), true)->getAcceptURL());
                return true;
            }
            index = getItemPath(i, { ConfigVal::A_TRANSCODING_PROFILES_PROFLE, ConfigVal::A_TRANSCODING_PROFILES_PROFLE_DLNAPROF });
            if (optItem == index) {
                if (ConfigDefinition::findConfigSetup<ConfigStringSetup>(ConfigVal::A_TRANSCODING_PROFILES_PROFLE_DLNAPROF)->checkValue(optValue)) {
                    config->setOrigValue(index, entry->getDlnaProfile());
                    entry->setDlnaProfile(optValue);
                    log_debug("New Transcoding Detail {} {}", index, config->getTranscodingProfileListOption(option)->getByName(entry->getName(), true)->getDlnaProfile());
                    return true;
                }
            }
            index = getItemPath(i, { ConfigVal::A_TRANSCODING_PROFILES_PROFLE, ConfigVal::A_TRANSCODING_PROFILES_PROFLE_SAMPFREQ });
            if (optItem == index) {
                config->setOrigValue(index, entry->getSampleFreq());
                entry->setSampleFreq(ConfigDefinition::findConfigSetup<ConfigIntSetup>(ConfigVal::A_TRANSCODING_PROFILES_PROFLE_SAMPFREQ)->checkIntValue(optValue));
                log_debug("New Transcoding Detail {} {}", index, config->getTranscodingProfileListOption(option)->getByName(entry->getName(), true)->getSampleFreq());
                return true;
            }
            index = getItemPath(i, { ConfigVal::A_TRANSCODING_PROFILES_PROFLE, ConfigVal::A_TRANSCODING_PROFILES_PROFLE_NRCHAN });
            if (optItem == index) {
                config->setOrigValue(index, entry->getNumChannels());
                entry->setNumChannels(ConfigDefinition::findConfigSetup<ConfigIntSetup>(ConfigVal::A_TRANSCODING_PROFILES_PROFLE_NRCHAN)->checkIntValue(optValue));
                log_debug("New Transcoding Detail {} {}", index, config->getTranscodingProfileListOption(option)->getByName(entry->getName(), true)->getNumChannels());
                return true;
            }
            index = getItemPath(i, { ConfigVal::A_TRANSCODING_PROFILES_PROFLE, ConfigVal::A_TRANSCODING_PROFILES_PROFLE_HIDEORIG });
            if (optItem == index) {
                config->setOrigValue(index, entry->hideOriginalResource());
                entry->setHideOriginalResource(ConfigDefinition::findConfigSetup<ConfigBoolSetup>(ConfigVal::A_TRANSCODING_PROFILES_PROFLE_HIDEORIG)->checkValue(optValue));
                log_debug("New Transcoding Detail {} {}", index, config->getTranscodingProfileListOption(option)->getByName(entry->getName(), true)->hideOriginalResource());
                return true;
            }
            index = getItemPath(i, { ConfigVal::A_TRANSCODING_PROFILES_PROFLE, ConfigVal::A_TRANSCODING_PROFILES_PROFLE_THUMB });
            if (optItem == index) {
                config->setOrigValue(index, entry->isThumbnail());
                entry->setThumbnail(ConfigDefinition::findConfigSetup<ConfigBoolSetup>(ConfigVal::A_TRANSCODING_PROFILES_PROFLE_THUMB)->checkValue(optValue));
                log_debug("New Transcoding Detail {} {}", index, config->getTranscodingProfileListOption(option)->getByName(entry->getName(), true)->isThumbnail());
                return true;
            }
            index = getItemPath(i, { ConfigVal::A_TRANSCODING_PROFILES_PROFLE, ConfigVal::A_TRANSCODING_PROFILES_PROFLE_FIRST });
            if (optItem == index) {
                config->setOrigValue(index, entry->getFirstResource());
                entry->setFirstResource(ConfigDefinition::findConfigSetup<ConfigBoolSetup>(ConfigVal::A_TRANSCODING_PROFILES_PROFLE_FIRST)->checkValue(optValue));
                log_debug("New Transcoding Detail {} {}", index, config->getTranscodingProfileListOption(option)->getByName(entry->getName(), true)->getFirstResource());
                return true;
            }
            index = getItemPath(i, { ConfigVal::A_TRANSCODING_PROFILES_PROFLE, ConfigVal::A_TRANSCODING_PROFILES_PROFLE_ACCOGG });
            if (optItem == index) {
                config->setOrigValue(index, entry->isTheora());
                entry->setTheora(ConfigDefinition::findConfigSetup<ConfigBoolSetup>(ConfigVal::A_TRANSCODING_PROFILES_PROFLE_ACCOGG)->checkValue(optValue));
                log_debug("New Transcoding Detail {} {}", index, config->getTranscodingProfileListOption(option)->getByName(entry->getName(), true)->isTheora());
                return true;
            }

            // update buffer options
            std::size_t buffer = entry->getBufferSize();
            std::size_t chunk = entry->getBufferChunkSize();
            std::size_t fill = entry->getBufferInitialFillSize();
            bool setBuffer = false;
            index = getItemPath(i, { ConfigVal::A_TRANSCODING_PROFILES_PROFLE, ConfigVal::A_TRANSCODING_PROFILES_PROFLE_BUFFER, ConfigVal::A_TRANSCODING_PROFILES_PROFLE_BUFFER_SIZE });
            if (optItem == index) {
                config->setOrigValue(index, static_cast<int>(buffer));
                buffer = ConfigDefinition::findConfigSetup<ConfigIntSetup>(ConfigVal::A_TRANSCODING_PROFILES_PROFLE_BUFFER_SIZE)->checkIntValue(optValue);
                setBuffer = true;
            }
            index = getItemPath(i, { ConfigVal::A_TRANSCODING_PROFILES_PROFLE, ConfigVal::A_TRANSCODING_PROFILES_PROFLE_BUFFER, ConfigVal::A_TRANSCODING_PROFILES_PROFLE_BUFFER_CHUNK });
            if (optItem == index) {
                config->setOrigValue(index, static_cast<int>(chunk));
                chunk = ConfigDefinition::findConfigSetup<ConfigIntSetup>(ConfigVal::A_TRANSCODING_PROFILES_PROFLE_BUFFER_CHUNK)->checkIntValue(optValue);
                setBuffer = true;
            }
            index = getItemPath(i, { ConfigVal::A_TRANSCODING_PROFILES_PROFLE, ConfigVal::A_TRANSCODING_PROFILES_PROFLE_BUFFER, ConfigVal::A_TRANSCODING_PROFILES_PROFLE_BUFFER_FILL });
            if (optItem == index) {
                config->setOrigValue(index, static_cast<int>(fill));
                fill = ConfigDefinition::findConfigSetup<ConfigIntSetup>(ConfigVal::A_TRANSCODING_PROFILES_PROFLE_BUFFER_FILL)->checkIntValue(optValue);
                setBuffer = true;
            }
            if (setBuffer && chunk > buffer) {
                log_error("error in configuration: transcoding profile \""
                    + entry->getName() + "\" chunk size can not be greater than buffer size");
                return false;
            }
            if (setBuffer && fill > buffer) {
                log_error("error in configuration: transcoding profile \""
                    + entry->getName() + "\" fill size can not be greater than buffer size");
                return false;
            }
            if (setBuffer) {
                entry->setBufferOptions(buffer, chunk, fill);
                return true;
            }

            // update agent options
            index = getItemPath(i, { ConfigVal::A_TRANSCODING_PROFILES_PROFLE, ConfigVal::A_TRANSCODING_PROFILES_PROFLE_AGENT, ConfigVal::A_TRANSCODING_PROFILES_PROFLE_AGENT_COMMAND });
            if (optItem == index) {
                if (ConfigDefinition::findConfigSetup<ConfigStringSetup>(ConfigVal::A_TRANSCODING_PROFILES_PROFLE_AGENT_COMMAND)->checkValue(optValue)) {
                    config->setOrigValue(index, entry->getCommand());
                    entry->setCommand(optValue);
                    log_debug("New Transcoding Detail {} {}", index, config->getTranscodingProfileListOption(option)->getByName(entry->getName(), true)->getCommand().string());
                    return true;
                }
            }
            index = getItemPath(i, { ConfigVal::A_TRANSCODING_PROFILES_PROFLE, ConfigVal::A_TRANSCODING_PROFILES_PROFLE_AGENT, ConfigVal::A_TRANSCODING_PROFILES_PROFLE_AGENT_ARGS });
            if (optItem == index) {
                if (ConfigDefinition::findConfigSetup<ConfigStringSetup>(ConfigVal::A_TRANSCODING_PROFILES_PROFLE_AGENT_ARGS)->checkValue(optValue)) {
                    config->setOrigValue(index, entry->getArguments());
                    entry->setArguments(optValue);
                    log_debug("New Transcoding Detail {} {}", index, config->getTranscodingProfileListOption(option)->getByName(entry->getName(), true)->getArguments());
                    return true;
                }
            }

            // update 4cc options
            AviFourccListmode fccMode = entry->getAVIFourCCListMode();
            auto fccList = entry->getAVIFourCCList();
            bool set4cc = false;
            index = getItemPath(i, { ConfigVal::A_TRANSCODING_PROFILES_PROFLE, ConfigVal::A_TRANSCODING_PROFILES_PROFLE_AVI4CC, ConfigVal::A_TRANSCODING_PROFILES_PROFLE_AVI4CC_MODE });
            if (optItem == index) {
                auto setup = ConfigDefinition::findConfigSetup<ConfigEnumSetup<AviFourccListmode>>(ConfigVal::A_TRANSCODING_PROFILES_PROFLE_AVI4CC_MODE);
                config->setOrigValue(index, setup->mapEnumValue(fccMode));
                if (setup->checkEnumValue(optValue, fccMode)) {
                    set4cc = true;
                }
            }
            index = getItemPath(i, { ConfigVal::A_TRANSCODING_PROFILES_PROFLE, ConfigVal::A_TRANSCODING_PROFILES_PROFLE_AVI4CC, ConfigVal::A_TRANSCODING_PROFILES_PROFLE_AVI4CC_4CC });
            if (optItem == index) {
                config->setOrigValue(index, std::accumulate(next(fccList.begin()), fccList.end(), fccList[0], [](auto&& a, auto&& b) { return fmt::format("{}, {}", a, b); }));
                fccList.clear();
                if (ConfigDefinition::findConfigSetup<ConfigArraySetup>(ConfigVal::A_TRANSCODING_PROFILES_PROFLE_AVI4CC)->checkArrayValue(optValue, fccList)) {
                    set4cc = true;
                }
            }
            if (set4cc) {
                entry->setAVIFourCCList(fccList, fccMode);
                return true;
            }
            i++;
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
