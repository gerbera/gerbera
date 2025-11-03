/*GRB*

    Gerbera - https://gerbera.io/

    upnp/client_manager.h - this file is part of Gerbera.

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

/// @file upnp/client_manager.h
/// @brief Definition of the ClientManager class.
/// inspired by https://sourceforge.net/p/minidlna/git/ci/master/tree/clients.h

#ifndef __UPNP_CLIENT_MANAGER_H__
#define __UPNP_CLIENT_MANAGER_H__

#include <chrono>
#include <memory>
#include <mutex>
#include <vector>

// forward declarations
class Config;
class Database;
class GrbNet;
class Headers;
class Server;
enum class ClientMatchType;
struct ClientProfile;
struct ClientObservation;

namespace pugi {
class xml_document;
} // namespace pugi

/// @brief class to manage all known clients and profile information
class ClientManager {
public:
    explicit ClientManager(
        std::shared_ptr<Config> config,
        std::shared_ptr<Database> database,
        std::shared_ptr<Server> server);

    /// @brief reload predefined profiles and configuration values
    void refresh();

    /// @brief get stored client information for client with addr and userAgent
    /// @return always return something, 'Unknown' if we do not know better
    const ClientObservation* getInfo(
        const std::shared_ptr<GrbNet>& addr,
        const std::string& userAgent,
        const std::shared_ptr<Headers>& headers) const;

    /// @brief store client found by UPnP discovery messages
    void addClientByDiscovery(
        const std::shared_ptr<GrbNet>& addr,
        const std::string& userAgent,
        const std::string& descLocation);

    /// @brief get current cache content
    const std::vector<ClientObservation>& getClientList() const { return cache; }

    /// @brief Remove single client from cache and database
    void removeClient(const std::string& clientIp);

private:
    const ClientProfile* getInfoByAddr(const std::shared_ptr<GrbNet>& addr) const;
    const ClientProfile* getInfoByType(const std::string& match, ClientMatchType type) const;

    const ClientObservation* getInfoByCache(const std::shared_ptr<GrbNet>& addr) const;
    const ClientObservation* updateCache(
        const std::shared_ptr<GrbNet>& addr,
        const std::string& userAgent,
        const std::shared_ptr<Headers>& headers,
        const ClientProfile* pInfo) const;

    static std::unique_ptr<pugi::xml_document> downloadDescription(const std::string& location);

    mutable std::mutex mutex;
    using AutoLock = std::scoped_lock<std::mutex>;
    mutable std::vector<ClientObservation> cache;

    std::vector<ClientProfile> clientProfile;
    std::shared_ptr<Database> database;
    std::shared_ptr<Config> config;
    std::shared_ptr<Server> server;
    std::chrono::hours cacheThreshold;
};

#endif // __UPNP_CLIENT_MANAGER_H__
