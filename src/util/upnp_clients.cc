/*GRB*

    Gerbera - https://gerbera.io/
    
    upnp_clients.cc - this file is part of Gerbera.
    
    Copyright (C) 2020 Gerbera Contributors
    
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

/// \file upnp_clients.cc
/// client info initially taken from https://sourceforge.net/p/minidlna/git/ci/master/tree/clients.cc

#include "upnp_clients.h" // API

#include "util/tools.h"

// table of supported clients (sequence of entries matters!)
static const struct ClientInfo clientInfo[] = {
    // Used for not explicitely listed clients, must be first entry
    {
        "Unknown",
        ClientType::Unknown,
        QUIRK_FLAG_NONE,
        ClientMatchType::None,
        NULL },
    // User-Agent: Linux/3.18.91-14843133-QB28034466 UPnP/1.0 BubbleUPnP/3.4.4
    {
        "BubbleUPnP",
        ClientType::BubbleUPnP,
        QUIRK_FLAG_NONE,
        ClientMatchType::UserAgent,
        "UPnP/1.0 BubbleUPnP" },
    // This is AllShare running on a PC. We don't want to respond with Samsung capabilities, or Windows (and AllShare) might get grumpy.
    // User-Agent: DLNADOC/1.50 SEC_HHP_[PC]LPC001/1.0  MS-DeviceCaps/1024
    {
        "AllShare",
        ClientType::SamsungAllShare,
        QUIRK_FLAG_NONE,
        ClientMatchType::UserAgent,
        "SEC_HHP_[PC]" },
    // Samsung Series [Q] TVs (2019)
    // User-Agent: DLNADOC/1.50 SEC_HHP_[TV] Samsung Q7 Series (49)/1.0
    {
        "Samsung Series [Q] TVs",
        ClientType::SamsungSeriesQ,
        QUIRK_FLAG_SAMSUNG,
        ClientMatchType::UserAgent,
        "SEC_HHP_[TV] Samsung Q" },
    // Samsung Blu-ray Player BD-D5100
    // User-Agent: DLNADOC/1.50 SEC_HHP_BD-D5100/1.0
    {
        "Samsung Blu-ray Player BD-D5100",
        ClientType::SamsungBDP,
        QUIRK_FLAG_SAMSUNG,
        ClientMatchType::UserAgent,
        "SEC_HHP_BD" },
    // Samsung other TVs
    // User-Agent: DLNADOC/1.50 SEC_HHP_[TV]UE40D7000/1.0
    // User-Agent: DLNADOC/1.50 SEC_HHP_ Family TV/1.0
    // User-Agent: DLNADOC/1.50 SEC_HHP_[TV] UE65JU7000/1.0 UPnP/1.0
    {
        "Samsung other TVs",
        ClientType::SamsungSeriesCDE,
        QUIRK_FLAG_SAMSUNG,
        ClientMatchType::UserAgent,
        "SEC_HHP_" },
    // Samsung Blu-ray Player J5500
    {
        "Samsung Blu-ray Player J5500",
        ClientType::SamsungBDJ5500,
        QUIRK_FLAG_SAMSUNG,
        ClientMatchType::UserAgent,
        "[BD]J5500" },
    // Gerbera, FRITZ!Box, etc...
    // User-Agent: Linux/5.4.0-4-amd64, UPnP/1.0, Portable SDK for UPnP devices/1.8.6
    // User-Agent: FRITZ!Box 5490 UPnP/1.0 AVM FRITZ!Box 5490 151.07.12
    {
        "Standard UPnP",
        ClientType::StandardUPnP,
        QUIRK_FLAG_NONE,
        ClientMatchType::UserAgent,
        "UPnP/1.0" }
};

void Clients::addClient(const struct sockaddr_storage* addr, const std::string& userAgent)
{
    // detect type
    const ClientInfo* info = nullptr;
    for (const auto& i : clientInfo) {
        if (!i.match)
            continue;
        if (i.matchType == ClientMatchType::UserAgent) {
            if (userAgent.find(i.match) != std::string::npos) {
                info = &i;
                break;
            }
        }
    }

    {
        AutoLock lock(mutex);

        // even if not detect type we add entry, to log 'Found client' not to often
        bool found = false;
        for (auto& entry : *cache) {
            if (sockAddrCmpAddr((struct sockaddr*)&entry.addr, (struct sockaddr*)addr) != 0)
                continue;
            entry.age = std::chrono::steady_clock::now();
            entry.pInfo = info;
            found = true;
            break;
        }

        if (!found) {
            assert(clientInfo[0].type == ClientType::Unknown);
            log_debug("client add: {} '{}' -> '{}'", sockAddrGetNameInfo((struct sockaddr*)addr), userAgent, info ? info->name : clientInfo[0].name);
            auto add = ClientCacheEntry();
            add.addr = *addr;
            add.age = std::chrono::steady_clock::now();
            add.pInfo = info;
            cache->push_back(add);
        }
    }
}

void Clients::getInfo(const struct sockaddr_storage* addr, ClientInfo* pInfo)
{
    AutoLock lock(mutex);

    // house cleaning, remove old entries
    auto now = std::chrono::steady_clock::now();
    cache->erase(
        std::remove_if(cache->begin(), cache->end(),
            [now](const auto& entry) { return entry.age + std::chrono::hours(1) < now; }),
        cache->end());

    const ClientInfo* info = &clientInfo[0]; // we always return something, 'Unknown' if we do not know better
    assert(info->type == ClientType::Unknown);
    for (const auto& entry : *cache) {
        if (sockAddrCmpAddr((struct sockaddr*)&entry.addr, (struct sockaddr*)addr) != 0)
            continue;
        if (entry.pInfo)
            info = entry.pInfo;
        break;
    }

    log_debug("client info: {} -> '{}'", sockAddrGetNameInfo((struct sockaddr*)addr), info->name);
    *pInfo = *info;
}

std::mutex Clients::mutex;
std::unique_ptr<std::vector<struct ClientCacheEntry>> Clients::cache = std::make_unique<std::vector<struct ClientCacheEntry>>();
