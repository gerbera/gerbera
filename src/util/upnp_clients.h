/*GRB*

    Gerbera - https://gerbera.io/

    upnp_clients.h - this file is part of Gerbera.

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

/// \file upnp_clients.h
/// \brief Definition of the Clients class.
/// inspired by https://sourceforge.net/p/minidlna/git/ci/master/tree/clients.h

#ifndef __UPNP_CLIENTS_H__
#define __UPNP_CLIENTS_H__

#include <chrono>
#include <memory>
#include <mutex>
#include <pugixml.hpp>
#include <sys/socket.h>
#include <vector>

#include "common.h"
#include "util/upnp_quirks.h"

// forward declaration
class Config;
class Database;
class GrbNet;

// specific customer products
enum class ClientType {
    Unknown = 0, // if not listed otherwise
    BubbleUPnP,
    SamsungAllShare,
    SamsungSeriesQ,
    SamsungBDP,
    SamsungSeriesCDE,
    SamsungBDJ5500,
    IRadio,
    PanasonicTV,
    StandardUPnP
};

// specify what must match
enum class ClientMatchType {
    None,
    UserAgent, // received via UpnpActionRequest, UpnpFileInfo and UpnpDiscovery (all might be slitely different)
    // FriendlyName,
    // ModelName,
    IP, // use client's network address
};

struct ClientInfo {
    std::string name { "Unknown" }; // used for logging/debugging proposes only
    std::string group { DEFAULT_CLIENT_GROUP };
    ClientType type { ClientType::Unknown };
    QuirkFlags flags { QUIRK_FLAG_NONE };

    // to match the client
    ClientMatchType matchType { ClientMatchType::None };
    std::string match;
    std::map<std::string, std::string> mimeMappings {};
    int captionInfoCount { 1 };
    int stringLimit { -1 };
    bool multiValue { true };
};

struct ClientCacheEntry {
    ClientCacheEntry(std::shared_ptr<GrbNet> addr, std::string userAgent, std::chrono::seconds last, std::chrono::seconds age, const struct ClientInfo* pInfo)
        : addr(std::move(addr))
        , userAgent(std::move(userAgent))
        , last(last)
        , age(age)
        , pInfo(pInfo)
    {
    }

    std::shared_ptr<GrbNet> addr {};
    std::string userAgent;
    std::chrono::seconds last;
    std::chrono::seconds age;
    const struct ClientInfo* pInfo;
};

class ClientStatusDetail {
public:
    ClientStatusDetail(std::string group, int itemId, int playCount, int lastPlayed, int lastPlayedPosition, int bookMarkPos)
        : group(std::move(group))
        , itemId(itemId)
        , playCount(playCount)
        , lastPlayed(lastPlayed)
        , lastPlayedPosition(lastPlayedPosition)
        , bookMarkPos(bookMarkPos)
    {
        if (lastPlayed == 0)
            setLastPlayed();
    }

    std::string getGroup() const { return group; }
    int getItemId() const { return itemId; }

    int getPlayCount() const { return playCount; }
    int increasePlayCount() { return ++playCount; }

    std::chrono::seconds getLastPlayed() const { return lastPlayed; }
    void setLastPlayed() { this->lastPlayed = currentTime(); }

    std::chrono::milliseconds getLastPlayedPosition() const { return lastPlayedPosition; }
    void setLastPlayedPosition(int lastPlayedPosition) { this->lastPlayedPosition = std::chrono::milliseconds(lastPlayedPosition); }

    std::chrono::milliseconds getBookMarkPosition() const { return bookMarkPos; }
    void setBookMarkPosition(int bookMarkPos) { this->bookMarkPos = std::chrono::milliseconds(bookMarkPos); }

    std::shared_ptr<ClientStatusDetail> clone() const;

private:
    std::string group; // default for any, otherwise group name from config
    int itemId { INVALID_OBJECT_ID };
    int playCount { 0 };
    std::chrono::seconds lastPlayed;
    std::chrono::milliseconds lastPlayedPosition {};
    std::chrono::milliseconds bookMarkPos {};
};

class ClientManager {
public:
    explicit ClientManager(const std::shared_ptr<Config>& config, std::shared_ptr<Database> database);
    void refresh(const std::shared_ptr<Config>& config);

    // always return something, 'Unknown' if we do not know better
    const ClientInfo* getInfo(const std::shared_ptr<GrbNet>& addr, const std::string& userAgent) const;

    void addClientByDiscovery(const std::shared_ptr<GrbNet>& addr, const std::string& userAgent, const std::string& descLocation);
    const std::vector<ClientCacheEntry>& getClientList() const { return cache; }

private:
    const ClientInfo* getInfoByAddr(const std::shared_ptr<GrbNet>& addr) const;
    const ClientInfo* getInfoByType(const std::string& match, ClientMatchType type) const;

    const ClientInfo* getInfoByCache(const std::shared_ptr<GrbNet>& addr) const;
    void updateCache(const std::shared_ptr<GrbNet>& addr, const std::string& userAgent, const ClientInfo* pInfo) const;

    static std::unique_ptr<pugi::xml_document> downloadDescription(const std::string& location);

    mutable std::mutex mutex;
    using AutoLock = std::scoped_lock<std::mutex>;
    mutable std::vector<ClientCacheEntry> cache;

    std::vector<ClientInfo> clientInfo;
    std::shared_ptr<Database> database;
    std::chrono::hours cacheThreshold;
};

#endif // __UPNP_CLIENTS_H__
