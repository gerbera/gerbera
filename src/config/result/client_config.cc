/*GRB*

    Gerbera - https://gerbera.io/

    client_config.cc - this file is part of Gerbera.

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

/// @file config/result/client_config.cc
#define GRB_LOG_FAC GrbLogFacility::clients

#include "client_config.h" // API

#include "cds/cds_enums.h"
#include "exceptions.h"
#include "util/logger.h"
#include "util/tools.h"

#include <algorithm>
#include <array>
#include <numeric>

static constexpr QuirkFlags QUIRKBASE = 1;

std::shared_ptr<ClientGroupConfig> ClientConfigList::getGroup(const std::string& name) const
{
    EditHelperClientGroupConfig::AutoLock lock(EditHelperClientGroupConfig::mutex);
    auto entry = std::find_if(EditHelperClientGroupConfig::list.begin(), EditHelperClientGroupConfig::list.end(), [=](auto&& c) { return c->getGroupName() == name; });
    if (entry != EditHelperClientGroupConfig::list.end() && *entry) {
        return *entry;
    }
    return {};
}

ClientConfig::ClientConfig(
    QuirkFlags flags,
    std::string_view group,
    std::string_view ip,
    std::string_view userAgent,
    const std::map<ClientMatchType, std::string>& matchValues,
    int captionInfoCount,
    int stringLimit,
    bool multiValue,
    bool isAllowed)
{
    clientProfile.type = ClientType::Custom;
    if (!ip.empty()) {
        clientProfile.matchType = ClientMatchType::IP;
        clientProfile.match = ip;
    } else if (!userAgent.empty()) {
        clientProfile.matchType = ClientMatchType::UserAgent;
        clientProfile.match = userAgent;
    } else {
        clientProfile.matchType = ClientMatchType::None;
        for (auto&& [mType, mLabel] : matchValues) {
            clientProfile.matchType = mType;
            clientProfile.match = mLabel;
        }
    }
    clientProfile.mimeMappings = DictionaryOption();
    clientProfile.headers = DictionaryOption();
    clientProfile.group = group;
    clientProfile.groupConfig = {};
    clientProfile.flags = flags;
    clientProfile.captionInfoCount = captionInfoCount;
    clientProfile.stringLimit = stringLimit;
    clientProfile.multiValue = multiValue;
    clientProfile.isAllowed = isAllowed;
    if (hasFlag(flags, Quirk::HideResourceThumbnail)) {
        auto res = std::find(clientProfile.supportedResources.begin(), clientProfile.supportedResources.end(), ResourcePurpose::Thumbnail);
        clientProfile.supportedResources.erase(res);
    }
    if (hasFlag(flags, Quirk::HideResourceSubtitle)) {
        auto res = std::find(clientProfile.supportedResources.begin(), clientProfile.supportedResources.end(), ResourcePurpose::Subtitle);
        clientProfile.supportedResources.erase(res);
    }
    if (hasFlag(flags, Quirk::HideResourceTranscode)) {
        auto res = std::find(clientProfile.supportedResources.begin(), clientProfile.supportedResources.end(), ResourcePurpose::Transcode);
        clientProfile.supportedResources.erase(res);
    }
    clientProfile.name = fmt::format("{} Setup for {} {}", mapClientType(clientProfile.type), mapMatchType(clientProfile.matchType), clientProfile.match);
}

QuirkFlags ClientConfig::getFlag(Quirk quirkFlag)
{
    return (quirkFlag == Quirk::None) ? QUIRK_FLAG_NONE : (QUIRKBASE << to_underlying(quirkFlag));
}

QuirkFlags ClientConfig::getFlags(const std::vector<Quirk> quirkFlags)
{
    QuirkFlags result = QUIRK_FLAG_NONE;
    for (auto&& flag : quirkFlags)
        if (flag != Quirk::None)
            result |= QUIRKBASE << to_underlying(flag);
    return result;
}

bool ClientConfig::hasFlag(QuirkFlags flag, Quirk quirkFlag)
{
    return (quirkFlag != Quirk::None) && (flag & (QUIRKBASE << to_underlying(quirkFlag)));
}

void ClientConfig::setMimeMappingsFrom(std::size_t j, const std::string& from)
{
    this->clientProfile.mimeMappings.setKey(j, from);
}

void ClientConfig::setMimeMappingsTo(std::size_t j, const std::string& to)
{
    this->clientProfile.mimeMappings.setValue(j, to);
}

void ClientConfig::setHeadersKey(std::size_t j, const std::string& key)
{
    this->clientProfile.headers.setKey(j, key);
}

void ClientConfig::setHeadersValue(std::size_t j, const std::string& value)
{
    this->clientProfile.headers.setValue(j, value);
}

void ClientConfig::setDlnaMapping(std::size_t j, std::size_t k, const std::string& value)
{
    this->clientProfile.dlnaMappings.setValue(j, k, value);
}

static constexpr std::array clientTypes {
    std::pair("None", ClientType::Unknown),
    std::pair("Custom", ClientType::Custom),
    std::pair("BubbleUPnP", ClientType::BubbleUPnP),
    std::pair("SamsungAllShare", ClientType::SamsungAllShare),
    std::pair("SamsungSeriesQ", ClientType::SamsungSeriesQ),
    std::pair("SamsungBDP", ClientType::SamsungBDP),
    std::pair("SamsungSeriesCDE", ClientType::SamsungSeriesCDE),
    std::pair("SamsungBDJ5500", ClientType::SamsungBDJ5500),
    std::pair("EC-IRadio", ClientType::IRadio),
    std::pair("FSL", ClientType::FSL),
    std::pair("PanasonicTV", ClientType::PanasonicTV),
    std::pair("BoseSoundtouch", ClientType::BoseSoundtouch),
    std::pair("YamahaRX", ClientType::YamahaRX),
    std::pair("FreeboxPlayer", ClientType::Freebox),
    std::pair("StandardUPnP", ClientType::StandardUPnP),
};

std::string ClientConfig::mapClientType(ClientType clientType)
{
    for (auto [cLabel, cType] : clientTypes) {
        if (clientType == cType) {
            return cLabel;
        }
    }
    throw_std_runtime_error("illegal clientType {} given to mapClientType()", to_underlying(clientType));
}

static constexpr std::array matchTypes {
    std::pair(ClientMatchType::None, "None"),
    std::pair(ClientMatchType::UserAgent, "UserAgent"),
    std::pair(ClientMatchType::IP, "IP"),
    std::pair(ClientMatchType::FriendlyName, "FriendlyName"),
    std::pair(ClientMatchType::ModelName, "ModelName"),
    std::pair(ClientMatchType::Manufacturer, "Manufacturer"),
};

std::string ClientConfig::mapMatchType(ClientMatchType matchType)
{
    for (auto [mType, mLabel] : matchTypes) {
        if (matchType == mType) {
            return mLabel;
        }
    }
    throw_std_runtime_error("illegal matchType {} given to mapMatchType()", to_underlying(matchType));
}

ClientMatchType ClientConfig::remapMatchType(const std::string& matchType)
{
    for (auto [mType, mLabel] : matchTypes) {
        if (toLower(matchType) == toLower(mLabel)) {
            return mType;
        }
    }
    return ClientMatchType::None;
}

std::vector<std::pair<std::string, Quirk>> ClientConfig::quirkFlags {
    { "None", Quirk::None },
    { "Samsung", Quirk::Samsung },
    { "SamsungBookmarkSeconds", Quirk::SamsungBookmarkSeconds },
    { "SAMSUNG_BOOKMARK_SEC", Quirk::SamsungBookmarkSeconds },
    { "SamsungBookmarkMilliSeconds", Quirk::SamsungBookmarkMilliSeconds },
    { "SAMSUNG_BOOKMARK_MSEC", Quirk::SamsungBookmarkMilliSeconds },
    { "SamsungFeatures", Quirk::SamsungFeatures },
    { "SAMSUNG_FEATURES", Quirk::SamsungFeatures },
    { "SamsungHideDynamic", Quirk::SamsungHideDynamic },
    { "SAMSUNG_HIDE_DYNAMIC", Quirk::SamsungHideDynamic },
    { "IRadio", Quirk::IRadio },
    { "PvSubtitles", Quirk::PvSubtitles },
    { "PV_SUBTITLES", Quirk::PvSubtitles },
    { "Panasonic", Quirk::Panasonic },
    { "StrictXML", Quirk::StrictXML },
    { "HideThumbnailResource", Quirk::HideResourceThumbnail },
    { "HideResourceThumbnail", Quirk::HideResourceThumbnail },
    { "HIDE_THUMBNAIL_RESOURCE", Quirk::HideResourceThumbnail },
    { "HideSubtitleResource", Quirk::HideResourceSubtitle },
    { "HideResourceSubtitle", Quirk::HideResourceSubtitle },
    { "HIDE_SUBTITLE_RESOURCE", Quirk::HideResourceSubtitle },
    { "HideTranscodeResource", Quirk::HideResourceTranscode },
    { "HideResourceTranscode", Quirk::HideResourceTranscode },
    { "HIDE_TRANSCODE_RESOURCE", Quirk::HideResourceTranscode },
    { "SimpleDate", Quirk::SimpleDate },
    { "SIMPLE_DATE", Quirk::SimpleDate },
    { "DCM10", Quirk::DCM10 },
    { "HideContainerShortcuts", Quirk::HideContainerShortcuts },
    { "HIDE_CONTAINER_SHORTCUTS", Quirk::HideContainerShortcuts },
    { "AsciiXML", Quirk::AsciiXML },
    { "ForceNoConversion", Quirk::ForceNoConversion },
    { "NoConversion", Quirk::ForceNoConversion },
    { "NO_CONVERSION", Quirk::ForceNoConversion },
    { "ShowInternalSubtitles", Quirk::ShowInternalSubtitles },
    { "SHOW_INTERNAL_SUBTITLES", Quirk::ShowInternalSubtitles },
    { "ForceSortCriteriaTitle", Quirk::ForceSortCriteriaTitle },
    { "FORCE_SORT_CRITERIA_TITLE", Quirk::ForceSortCriteriaTitle },
    { "CaptionProtocol", Quirk::CaptionProtocol },
    { "CAPTION_PROTOCOL", Quirk::CaptionProtocol },
    { "Transcoding1", Quirk::Transcoding1 },
    { "Transcoding2", Quirk::Transcoding2 },
    { "Transcoding3", Quirk::Transcoding3 },
};

QuirkFlags ClientConfig::remapFlag(const std::string& flag)
{
    for (auto [qLabel, qFlag] : quirkFlags) {
        if (toLower(flag) == toLower(qLabel)) {
            return getFlag(qFlag);
        }
    }
    return stoiString(flag, 0, 0);
}

QuirkFlags ClientConfig::makeFlags(const std::string& optValue)
{
    auto val = trimString(optValue);
    auto negate = startswith(val, "~");
    if (negate)
        val = val.substr(1);
    std::vector<std::string> flagsVector = splitString(val, '|');
    auto flags = std::accumulate(flagsVector.begin(), flagsVector.end(), 0, [](auto flg, auto&& i) { return flg | ClientConfig::remapFlag(trimString(i)); });
    return negate ? ~flags : flags;
}

std::string ClientConfig::mapFlags(QuirkFlags flags)
{
    if (!flags)
        return "None";

    std::vector<std::string> myFlags;

    for (auto [qLabel, qFlag] : quirkFlags) {
        if (flags & getFlag(qFlag)) {
            myFlags.emplace_back(qLabel);
            flags &= ~getFlag(qFlag);
        }
    }

    if (flags) {
        myFlags.push_back(fmt::format("{:#04x}", flags));
    }

    return fmt::format("{}", fmt::join(myFlags, " | "));
}
