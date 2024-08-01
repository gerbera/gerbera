/*GRB*

    Gerbera - https://gerbera.io/

    upnp/client_manager.h - this file is part of Gerbera.

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

/// \file upnp/client_manager.h
/// \brief Definition of the ClientManager class.
/// inspired by https://sourceforge.net/p/minidlna/git/ci/master/tree/clients.h

#ifndef __UPNP_CLIENT_MANAGER_H__
#define __UPNP_CLIENT_MANAGER_H__

#include <chrono>
#include <memory>
#include <mutex>
#include <pugixml.hpp>
#include <vector>

// forward declarations
class Config;
class Database;
class GrbNet;
enum class ClientMatchType;
struct ClientProfile;
struct ClientObservation;

class ClientManager {
public:
    explicit ClientManager(const std::shared_ptr<Config>& config, std::shared_ptr<Database> database);
    void refresh(const std::shared_ptr<Config>& config);

    // always return something, 'Unknown' if we do not know better
    const ClientObservation* getInfo(const std::shared_ptr<GrbNet>& addr, const std::string& userAgent) const;

    void addClientByDiscovery(const std::shared_ptr<GrbNet>& addr, const std::string& userAgent, const std::string& descLocation);
    const std::vector<ClientObservation>& getClientList() const { return cache; }

private:
    const ClientProfile* getInfoByAddr(const std::shared_ptr<GrbNet>& addr) const;
    const ClientProfile* getInfoByType(const std::string& match, ClientMatchType type) const;

    const ClientObservation* getInfoByCache(const std::shared_ptr<GrbNet>& addr) const;
    const ClientObservation* updateCache(const std::shared_ptr<GrbNet>& addr, const std::string& userAgent, const ClientProfile* pInfo) const;

    static std::unique_ptr<pugi::xml_document> downloadDescription(const std::string& location);

    mutable std::mutex mutex;
    using AutoLock = std::scoped_lock<std::mutex>;
    mutable std::vector<ClientObservation> cache;

    std::vector<ClientProfile> clientProfile;
    std::shared_ptr<Database> database;
    std::chrono::hours cacheThreshold;
};

#endif // __UPNP_CLIENT_MANAGER_H__
