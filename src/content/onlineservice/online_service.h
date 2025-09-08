/*MT*

    MediaTomb - http://www.mediatomb.cc/

    online_service.h - this file is part of MediaTomb.

    Copyright (C) 2005 Gena Batyan <bgeradz@mediatomb.cc>,
                       Sergey 'Jin' Bostandzhyan <jin@mediatomb.cc>

    Copyright (C) 2006-2010 Gena Batyan <bgeradz@mediatomb.cc>,
                            Sergey 'Jin' Bostandzhyan <jin@mediatomb.cc>,
                            Leonhard Wimmer <leo@mediatomb.cc>

    Copyright (C) 2016-2025 Gerbera Contributors

    MediaTomb is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License version 2
    as published by the Free Software Foundation.

    MediaTomb is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    version 2 along with MediaTomb; if not, write to the Free Software
    Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301, USA.

    $Id$
*/

/// @file content/onlineservice/online_service.h
/// @brief Definition of the OnlineService class.

#ifndef __ONLINE_SERVICE_H__
#define __ONLINE_SERVICE_H__

#ifdef ONLINE_SERVICES

#include "util/timer.h"

#include <map>
#include <memory>

// forward declaration
class Config;
class Database;
class Content;
class Layout;

#define ONLINE_SERVICE_AUX_ID "ols"
#define ONLINE_SERVICE_LAST_UPDATE "lu"

// make sure to add the database prefixes when adding new services
// keep old entries to avoid future collisions
enum class OnlineServiceType {
    None = 0,
#ifdef YOUTUBE // Until Jan 6, 2018
    YouTube = 1,
#endif
#ifdef SOPCAST // Until Oct 31, 2021
    SopCast = 2,
#endif
#ifdef WEBORAMA // Until Jan 1, 2017
    Weborama = 3,
#endif
#ifdef ATRAILERS // Until Aug 11, 2024
    ATrailers = 4,
#endif
    Max
};

/// @brief This is an interface for all online services, the function
/// handles adding/refreshing content in the database.
class OnlineService {
public:
    explicit OnlineService(const std::shared_ptr<Content>& content);
    virtual ~OnlineService() = default;

    /// @brief Retrieves user specified content from the service and adds
    /// the items to the database.
    ///
    /// Depending on the service, the content may be retrieved in chunks,
    /// thus allowing to split the retrieval into multiple tasks without
    /// completely blocking other tasks for a longer period of time.
    /// This function will return true if there are more chunks to get and
    /// false if all chunks have been processed and there is no more data
    /// to get. Another call to this function after it returned true will
    /// reset the internal counters and thus make it fetch the content from
    /// the beginning.
    virtual bool refreshServiceData(const std::shared_ptr<Layout>& layout) = 0;

    /// @brief Returns the service type
    virtual OnlineServiceType getServiceType() const = 0;

    /// @brief Returns the service name
    virtual std::string getServiceName() const = 0;

    /// @brief Get the database service prefix for a particular service
    char getDatabasePrefix() const;

    /// @brief Get the database prefix for a given service type
    static char getDatabasePrefix(OnlineServiceType service);

    /// @brief Increments the task count.
    ///
    /// When recursive autoscan is in progress, we only want to subcribe to
    /// a timer event when the scan is finished. However, recursive scans
    /// spawn tasks for each directory. When adding a rescan task for
    /// subdirectories, the taskCount will be incremented. When a task is
    /// done the count will be decremented. When timer gets to zero,
    /// we will resubscribe.
    void incTaskCount() { taskCount++; }

    void decTaskCount() { taskCount--; }

    int getTaskCount() const { return taskCount; }

    /// Parameter that can be used by timerNotify
    void setTimerParameter(std::shared_ptr<Timer::Parameter> param)
    {
        timer_parameter = std::move(param);
    }

    std::shared_ptr<Timer::Parameter> getTimerParameter() const { return timer_parameter; }

    /// @brief Sets the service refresh interval in seconds
    void setRefreshInterval(std::chrono::seconds interval) { refresh_interval = interval; }

    /// @brief Retrieves the service refresh interval in seconds
    std::chrono::seconds getRefreshInterval() const { return refresh_interval; }

    /// @brief Sets the "purge after" interval in seconds
    void setItemPurgeInterval(std::chrono::seconds interval) { purge_interval = interval; }

    /// @brief Retrieves the "purge after" interval in seconds
    std::chrono::seconds getItemPurgeInterval() const { return purge_interval; }

protected:
    std::shared_ptr<Config> config;
    std::shared_ptr<Database> database;
    std::shared_ptr<Content> content;

    int taskCount {};
    std::chrono::seconds refresh_interval = std::chrono::seconds::zero();
    std::chrono::seconds purge_interval = std::chrono::seconds::zero();
    std::shared_ptr<Timer::Parameter> timer_parameter;
};

class OnlineServiceList {
public:
    /// @brief Adds a service to the service list.
    void registerService(const std::shared_ptr<OnlineService>& service);

    /// @brief Retrieves a service given by the service ID from the list
    std::shared_ptr<OnlineService> getService(OnlineServiceType service) const;

protected:
    std::map<OnlineServiceType, std::shared_ptr<OnlineService>> service_list;
};

#endif // ONLINE_SERVICES

#endif //__ONLINE_SERVICE_H__
