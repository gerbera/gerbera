/*MT*
    
    MediaTomb - http://www.mediatomb.cc/
    
    online_service.h - this file is part of MediaTomb.
    
    Copyright (C) 2005 Gena Batyan <bgeradz@mediatomb.cc>,
                       Sergey 'Jin' Bostandzhyan <jin@mediatomb.cc>
    
    Copyright (C) 2006-2007 Gena Batyan <bgeradz@mediatomb.cc>,
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
#include "layout/layout.h"

typedef enum service_type_t
{
#ifdef YOUTUBE
    OS_YouTube = 0,
#endif
    OS_Max
};

/// \brief This is an interface for all online services, the function
/// handles adding/refreshing content in the database.
class OnlineService : public zmm::Object
{
public:
    OnlineService() { taskCount = 0; }

    /// \brief Retrieves user specified content from the service and adds
    /// the items to the database.
    virtual void refreshServiceData(zmm::Ref<Layout> layout) = 0;

    /// \brief returns the service type
    virtual service_type_t getServiceType() = 0;

    /// \brief returns the service name
    virtual zmm::String getServiceName() = 0;

    /// \brief retrieves the service refresh interval in seconds
    virtual int getRefreshInterval() = 0;

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
    
protected:
    int taskCount;

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
