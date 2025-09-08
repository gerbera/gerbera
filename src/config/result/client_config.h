/*GRB*

    Gerbera - https://gerbera.io/

    client_config.h - this file is part of Gerbera.

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

/// @file config/result/client_config.h
/// @brief Definitions of the ClientConfig classes.

#ifndef __CLIENTCONFIG_H__
#define __CLIENTCONFIG_H__

#include "edit_helper.h"
#include "upnp/clients.h"

#include <map>
#include <mutex>
#include <utility>
#include <vector>

// forward declaration
class ClientConfig;
class ClientGroupConfig;

using EditHelperClientConfig = EditHelper<ClientConfig>;
using EditHelperClientGroupConfig = EditHelper<ClientGroupConfig>;

/// @brief Store configuration of clients
class ClientConfigList : public EditHelperClientConfig, public EditHelperClientGroupConfig {
public:
    std::shared_ptr<ClientGroupConfig> getGroup(const std::string& name) const;
};

/// @brief Provides information about client group settings.
class ClientGroupConfig : public Editable {
public:
    ClientGroupConfig(std::string name = "")
        : groupName(std::move(name))
    {
    }

    bool equals(const std::shared_ptr<ClientGroupConfig>& other) { return this->groupName == other->groupName; }

    /// @brief get list of forbidden directories
    std::vector<std::string> getForbiddenDirectories(bool edit = false) const { return forbidden.getArrayOption(edit); }
    void setForbiddenDirectories(const std::vector<std::string>& forbidden) { this->forbidden = ArrayOption(forbidden); }
    void setForbiddenDirectory(std::size_t j, const std::string& value) { this->forbidden.setItem(j, value); }

    std::string getGroupName() { return groupName; }

private:
    std::string groupName { DEFAULT_CLIENT_GROUP };
    ArrayOption forbidden = ArrayOption({});
};

/// @brief Provides information about one manual client.
class ClientConfig : public Editable {
public:
    ClientConfig() = default;

    /// @brief Creates a new ClientConfig object.
    /// @param flags quirks flags
    /// @param group client group
    /// @param ip ip address
    /// @param userAgent user agent
    /// @param matchValues additional matches, last one wins
    /// @param captionInfoCount max count if \<captionInfo\> tags
    /// @param stringLimit maximum length of name strings
    /// @param multiValue client support multi value attributes
    /// @param isAllowed client is allowed to connect to server
    ClientConfig(int flags, std::string_view group, std::string_view ip, std::string_view userAgent,
        const std::map<ClientMatchType, std::string>& matchValues,
        int captionInfoCount, int stringLimit, bool multiValue, bool isAllowed);

    bool equals(const std::shared_ptr<ClientConfig>& other) const { return this->getIp() == other->getIp() && this->getUserAgent() == other->getUserAgent(); }
    const ClientProfile& getClientProfile() const { return clientProfile; }

    int getFlags() const { return this->clientProfile.flags; }
    void setFlags(int flags) { this->clientProfile.flags = flags; }

    /// @brief mimeMappings special mappings for client
    std::map<std::string, std::string> getMimeMappings(bool edit = false) const { return this->clientProfile.mimeMappings.getDictionaryOption(edit); }
    void setMimeMappings(const std::map<std::string, std::string>& mappings) { this->clientProfile.mimeMappings = DictionaryOption(mappings); }
    void setMimeMappingsFrom(std::size_t j, const std::string& from);
    void setMimeMappingsTo(std::size_t j, const std::string& to);

    /// @brief headers additional headers from client
    std::map<std::string, std::string> getHeaders(bool edit = false) const { return this->clientProfile.headers.getDictionaryOption(edit); }
    void setHeaders(const std::map<std::string, std::string>& headers) { this->clientProfile.headers = DictionaryOption(headers); }
    void setHeadersKey(std::size_t j, const std::string& key);
    void setHeadersValue(std::size_t j, const std::string& value);

    /// @brief dlnaMappings additional dlna profiles mappings from client
    std::vector<std::vector<std::pair<std::string, std::string>>> getDlnaMappings(bool edit = false) const { return this->clientProfile.dlnaMappings.getVectorOption(edit); }
    void setDlnaMappings(const std::vector<std::vector<std::pair<std::string, std::string>>>& dlnaMappings) { this->clientProfile.dlnaMappings = VectorOption(dlnaMappings); }
    void setDlnaMapping(std::size_t j, std::size_t k, const std::string& value);

    int getCaptionInfoCount() const { return this->clientProfile.captionInfoCount; }
    void setCaptionInfoCount(int captionInfoCount)
    {
        this->clientProfile.captionInfoCount = captionInfoCount;
    }

    int getStringLimit() const { return this->clientProfile.stringLimit; }
    void setStringLimit(int stringLimit)
    {
        this->clientProfile.stringLimit = stringLimit;
    }

    bool getMultiValue() const { return this->clientProfile.multiValue; }
    void setMultiValue(bool multiValue)
    {
        this->clientProfile.multiValue = multiValue;
    }

    bool getFullFilter() const { return this->clientProfile.fullFilter; }
    void setFullFilter(bool fullFilter)
    {
        this->clientProfile.fullFilter = fullFilter;
    }

    bool getAllowed() const { return this->clientProfile.isAllowed; }
    void setAllowed(bool isAllowed)
    {
        this->clientProfile.isAllowed = isAllowed;
    }

    std::string getIp() const { return (this->clientProfile.matchType == ClientMatchType::IP) ? this->clientProfile.match : ""; }
    void setIp(std::string_view ip)
    {
        this->clientProfile.matchType = ClientMatchType::IP;
        this->clientProfile.match = ip;
    }

    std::string getGroup() const { return this->clientProfile.groupConfig ? this->clientProfile.groupConfig->getGroupName() : this->clientProfile.group; }
    void setGroup(std::string_view group, const std::shared_ptr<ClientGroupConfig>& groupConfig = {})
    {
        this->clientProfile.groupConfig = groupConfig;
        if (groupConfig)
            this->clientProfile.group = groupConfig->getGroupName();
    }

    std::string getUserAgent() const { return (this->clientProfile.matchType == ClientMatchType::UserAgent) ? this->clientProfile.match : ""; }
    void setUserAgent(std::string_view userAgent)
    {
        this->clientProfile.matchType = ClientMatchType::UserAgent;
        this->clientProfile.match = userAgent;
    }

    /* helpers for clientType stuff */
    static std::string_view mapClientType(ClientType clientType);
    static std::string_view mapMatchType(ClientMatchType matchType);
    static ClientMatchType remapMatchType(const std::string& matchType);
    static int remapFlag(const std::string& flag);
    static int makeFlags(const std::string& optValue);

    static std::string mapFlags(QuirkFlags flags);

protected:
    ClientProfile clientProfile;
};

#endif
