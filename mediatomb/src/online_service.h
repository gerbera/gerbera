/*MT*
    
    MediaTomb - http://www.mediatomb.cc/
    
    online_service.h - this file is part of MediaTomb.
    
    Copyright (C) 2005 Gena Batyan <bgeradz@mediatomb.cc>,
                       Sergey 'Jin' Bostandzhyan <jin@mediatomb.cc>
    
    Copyright (C) 2006-2010 Gena Batyan <bgeradz@mediatomb.cc>,
                            Sergey 'Jin' Bostandzhyan <jin@mediatomb.cc>,
                            Leonhard Wimmer <leo@mediatomb.cc>
    
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

/// \file online_service.h
/// \brief Definition of the OnlineService class.

#ifdef ONLINE_SERVICES

#ifndef __ONLINE_SERVICE_H__
#define __ONLINE_SERVICE_H__

#include "zmm/zmm.h"
#include "zmmf/zmmf.h"
#include "mxml/mxml.h"
#include "layout/layout.h"

#define ONLINE_SERVICE_AUX_ID "ols"
#define ONLINE_SERVICE_LAST_UPDATE "lu"

// make sure to add the storage prefixes when adding new services
typedef enum 
{
    OS_None         = 0,
    OS_YouTube      = 1,
    OS_SopCast      = 2,
    OS_Weborama     = 3,
    OS_ATrailers    = 4,
    OS_Max
} service_type_t;

/// \brief This is an interface for all online services, the function
/// handles adding/refreshing content in the database.
class OnlineService : public zmm::Object
{
public:
    OnlineService();

    /// \brief Retrieves user specified content from the service and adds
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
    virtual bool refreshServiceData(zmm::Ref<Layout> layout) = 0;

    /// \brief Returns the service type
    virtual service_type_t getServiceType() = 0;

    /// \brief Returns the service name
    virtual zmm::String getServiceName() = 0;

    /// \brief Get the storage service prefix for a particular service
    char getStoragePrefix();
    
    /// \brief Get the storage prefix for a given service type
    static char getStoragePrefix(service_type_t service);
    
    /// \brief Parses the service related line from config.xml and creates
    /// a task object, which can be anything that helps the service to
    /// identify what data it has to fetch.
    virtual zmm::Ref<zmm::Object> defineServiceTask(zmm::Ref<mxml::Element> xmlopt, zmm::Ref<zmm::Object> params) = 0;

    /// \brief Increments the task count. 
    ///
    /// When recursive autoscan is in progress, we only want to subcribe to
    /// a timer event when the scan is finished. However, recursive scans
    /// spawn tasks for each directory. When adding a rescan task for 
    /// subdirectories, the taskCount will be incremented. When a task is
    /// done the count will be decremented. When timer gets to zero, 
    /// we will resubscribe.
    void incTaskCount() { taskCount++; }

    void decTaskCount() { taskCount--; }

    int getTaskCount() { return taskCount; }

    void setTaskCount(int taskCount) { this->taskCount = taskCount; }

    /// Parameter that can be used by timerNotify
    void setTimerParameter(zmm::Ref<zmm::Object> param) 
                           { timer_parameter = param; }

    zmm::Ref<Object> getTimerParameter() { return timer_parameter; }

    /// \brief Sets the service refresh interval in seconds
    void setRefreshInterval(int interval) {refresh_interval = interval; }

    /// \brief Retrieves the service refresh interval in seconds
    int getRefreshInterval() { return refresh_interval; }

    /// \brief Sets the "purge after" interval in seconds
    void setItemPurgeInterval(int interval) { purge_interval = interval; }

    /// \brieg Retrieves the "purte after" interval in seconds
    int getItemPurgeInterval() { return purge_interval; }

protected:
    int taskCount;
    int refresh_interval;
    int purge_interval;
    zmm::Ref<zmm::Object> timer_parameter;

    /// \brief retrieves a required attribute given by the name
    zmm::String getCheckAttr(zmm::Ref<mxml::Element> xml, zmm::String attrname);
    /// \brief retrieves a required positive integer attribute given by the name
    int getCheckPosIntAttr(zmm::Ref<mxml::Element> xml, zmm::String attrname);
};

class OnlineServiceList : public zmm::Object
{
public:
    OnlineServiceList();

    /// \brief Adds a service to the service list.
    void registerService(zmm::Ref<OnlineService> service);

    /// \brief Retrieves a service given by the service ID from the list
    zmm::Ref<OnlineService> getService(service_type_t service);

protected:
    zmm::Ref<zmm::Array<OnlineService> > service_list;
};

#endif//__ONLINE_SERVICE_H__

#endif//ONLINE_SERVICE
