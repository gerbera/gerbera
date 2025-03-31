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

/// \file client_config.cc
#define GRB_LOG_FAC GrbLogFacility::clients

#include "client_config.h" // API

#include "cds/cds_enums.h"
#include "exceptions.h"
#include "util/logger.h"
#include "util/tools.h"

#include <algorithm>
#include <array>
#include <iterator>
#include <numeric>

std::shared_ptr<ClientGroupConfig> ClientConfigList::getGroup(const std::string& name) const
{
    EditHelperClientGroupConfig::AutoLock lock(EditHelperClientGroupConfig::mutex);
    auto entry = std::find_if(EditHelperClientGroupConfig::list.begin(), EditHelperClientGroupConfig::list.end(), [=](auto&& c) { return c->getGroupName() == name; });
    if (entry != EditHelperClientGroupConfig::list.end() && *entry) {
        return *entry;
    }
    return {};
}

ClientConfig::ClientConfig(int flags, std::string_view group, std::string_view ip, std::string_view userAgent,
    const std::map<ClientMatchType, std::string>& matchValues,
    int captionInfoCount, int stringLimit, bool multiValue, bool isAllowed)
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
    clientProfile.mimeMappings = DictionaryOption({});
    clientProfile.headers = DictionaryOption({});
    clientProfile.group = group;
    clientProfile.groupConfig = {};
    clientProfile.flags = flags;
    clientProfile.captionInfoCount = captionInfoCount;
    clientProfile.stringLimit = stringLimit;
    clientProfile.multiValue = multiValue;
    clientProfile.isAllowed = isAllowed;
    if (flags & QUIRK_FLAG_HIDE_RES_THUMBNAIL) {
        auto res = std::find(clientProfile.supportedResources.begin(), clientProfile.supportedResources.end(), ResourcePurpose::Thumbnail);
        clientProfile.supportedResources.erase(res);
    }
    if (flags & QUIRK_FLAG_HIDE_RES_SUBTITLE) {
        auto res = std::find(clientProfile.supportedResources.begin(), clientProfile.supportedResources.end(), ResourcePurpose::Subtitle);
        clientProfile.supportedResources.erase(res);
    }
    if (flags & QUIRK_FLAG_HIDE_RES_TRANSCODE) {
        auto res = std::find(clientProfile.supportedResources.begin(), clientProfile.supportedResources.end(), ResourcePurpose::Transcode);
        clientProfile.supportedResources.erase(res);
    }
    clientProfile.name = fmt::format("{} Setup for {} {}", mapClientType(clientProfile.type), mapMatchType(clientProfile.matchType), clientProfile.match);
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
    std::pair("StandardUPnP", ClientType::StandardUPnP),
};

std::string_view ClientConfig::mapClientType(ClientType clientType)
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

std::string_view ClientConfig::mapMatchType(ClientMatchType matchType)
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

static constexpr std::array quirkFlags {
    std::pair("SAMSUNG", QUIRK_FLAG_SAMSUNG),
    std::pair("SAMSUNG_BOOKMARK_SEC", QUIRK_FLAG_SAMSUNG_BOOKMARK_SEC),
    std::pair("SAMSUNG_BOOKMARK_MSEC", QUIRK_FLAG_SAMSUNG_BOOKMARK_MSEC),
    std::pair("SAMSUNG_FEATURES", QUIRK_FLAG_SAMSUNG_FEATURES),
    std::pair("SAMSUNG_HIDE_DYNAMIC", QUIRK_FLAG_SAMSUNG_HIDE_DYNAMIC),
    std::pair("IRADIO", QUIRK_FLAG_IRADIO),
    std::pair("PV_SUBTITLES", QUIRK_FLAG_PV_SUBTITLES),
    std::pair("PANASONIC", QUIRK_FLAG_PANASONIC),
    std::pair("STRICTXML", QUIRK_FLAG_STRICTXML),
    std::pair("HIDE_THUMBNAIL_RESOURCE", QUIRK_FLAG_HIDE_RES_THUMBNAIL),
    std::pair("HIDE_SUBTITLE_RESOURCE", QUIRK_FLAG_HIDE_RES_SUBTITLE),
    std::pair("HIDE_TRANSCODE_RESOURCE", QUIRK_FLAG_HIDE_RES_TRANSCODE),
    std::pair("SIMPLE_DATE", QUIRK_FLAG_SIMPLE_DATE),
    std::pair("DCM10", QUIRK_FLAG_DCM10),
    std::pair("HIDE_CONTAINER_SHORTCUTS", QUIRK_FLAG_HIDE_CONTAINER_SHORTCUTS),
    std::pair("ASCIIXML", QUIRK_FLAG_ASCIIXML),
    std::pair("TRANSCODING1", QUIRK_FLAG_TRANSCODING1),
    std::pair("TRANSCODING2", QUIRK_FLAG_TRANSCODING2),
    std::pair("TRANSCODING3", QUIRK_FLAG_TRANSCODING3),
};

int ClientConfig::remapFlag(const std::string& flag)
{
    for (auto [qLabel, qFlag] : quirkFlags) {
        if (flag == qLabel) {
            return qFlag;
        }
    }
    return stoiString(flag, 0, 0);
}

int ClientConfig::makeFlags(const std::string& optValue)
{
    std::vector<std::string> flagsVector = splitString(optValue, '|', false);
    return std::accumulate(flagsVector.begin(), flagsVector.end(), 0, [](auto flg, auto&& i) { return flg | ClientConfig::remapFlag(trimString(i)); });
}

std::string ClientConfig::mapFlags(QuirkFlags flags)
{
    if (!flags)
        return "None";

    std::vector<std::string> myFlags;

    for (auto [qLabel, qFlag] : quirkFlags) {
        if (flags & qFlag) {
            myFlags.emplace_back(qLabel);
            flags &= ~qFlag;
        }
    }

    if (flags) {
        myFlags.push_back(fmt::format("{:#04x}", flags));
    }

    return fmt::format("{}", fmt::join(myFlags, " | "));
}
