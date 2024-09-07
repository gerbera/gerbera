/*GRB*

    Gerbera - https://gerbera.io/

    client_config.h - this file is part of Gerbera.

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

/// \file client_config.h
/// \brief Definitions of the ClientConfig classes.

#ifndef __CLIENTCONFIG_H__
#define __CLIENTCONFIG_H__

#include "upnp/clients.h"

#include <map>
#include <mutex>
#include <vector>

// forward declaration
class ClientConfig;

class ClientConfigList {
public:
    /// \brief Adds a new ClientConfig to the list.
    void add(const std::shared_ptr<ClientConfig>& client, std::size_t index = std::numeric_limits<std::size_t>::max());

    std::shared_ptr<ClientConfig> get(std::size_t id, bool edit = false) const;

    std::size_t getEditSize() const;

    std::size_t size() const { return list.size(); }

    /// \brief removes the ClientConfig given by its scan ID
    void remove(std::size_t id, bool edit = false);

    /// \brief returns a copy of the client config list in the form of an array
    std::vector<std::shared_ptr<ClientConfig>> getArrayCopy() const;

protected:
    std::size_t origSize {};
    std::map<std::size_t, std::shared_ptr<ClientConfig>> indexMap;

    mutable std::recursive_mutex mutex;
    using AutoLock = std::scoped_lock<std::recursive_mutex>;

    std::vector<std::shared_ptr<ClientConfig>> list;
    void _add(const std::shared_ptr<ClientConfig>& client, std::size_t index);
};

/// \brief Provides information about one manual client.
class ClientConfig {
public:
    ClientConfig() = default;

    /// \brief Creates a new ClientConfig object.
    /// \param flags quirks flags
    /// \param group client group
    /// \param ip ip address
    /// \param userAgent user agent
    /// \param matchValues additional matches, last one wins
    /// \param captionInfoCount max count if \<captionInfo\> tags
    /// \param stringLimit maximum length of name strings
    /// \param multiValue client support multi value attributes
    /// \param isAllowed client is allowed to connect to server
    ClientConfig(int flags, std::string_view group, std::string_view ip, std::string_view userAgent,
        const std::map<ClientMatchType, std::string>& matchValues,
        int captionInfoCount, int stringLimit, bool multiValue, bool isAllowed);

    const ClientProfile& getClientProfile() const { return clientProfile; }

    int getFlags() const { return this->clientProfile.flags; }
    void setFlags(int flags) { this->clientProfile.flags = flags; }

    /// \brief mimeMappings special mappings for client
    std::map<std::string, std::string> getMimeMappings(bool edit = false) const { return this->clientProfile.mimeMappings.getDictionaryOption(edit); }
    void setMimeMappings(const std::map<std::string, std::string>& mappings) { this->clientProfile.mimeMappings = DictionaryOption(mappings); }
    void setMimeMappingsFrom(std::size_t j, const std::string& from);
    void setMimeMappingsTo(std::size_t j, const std::string& to);

    /// \brief headers additional headers from client
    std::map<std::string, std::string> getHeaders(bool edit = false) const { return this->clientProfile.headers.getDictionaryOption(edit); }
    void setHeaders(const std::map<std::string, std::string>& headers) { this->clientProfile.headers = DictionaryOption(headers); }
    void setHeadersKey(std::size_t j, const std::string& key);
    void setHeadersValue(std::size_t j, const std::string& value);

    /// \brief dlnaMappings additional dlna profiles mappings from client
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

    std::string getGroup() const { return this->clientProfile.group; }
    void setGroup(std::string_view group)
    {
        this->clientProfile.group = group;
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

    void setOrig(bool orig) { this->isOrig = orig; }

    bool getOrig() const { return isOrig; }

protected:
    bool isOrig {};
    ClientProfile clientProfile;
};

#endif
