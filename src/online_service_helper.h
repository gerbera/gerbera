/*MT*
    
    MediaTomb - http://www.mediatomb.cc/
    
    online_service_helper.h - this file is part of MediaTomb.
    
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

/// \file online_service_helper.h
/// \brief Definition of the OnlineServiceHelper class.

#ifdef ONLINE_SERVICES

#ifndef __ONLINE_SERVICE_HELPER_H__
#define __ONLINE_SERVICE_HELPER_H__

#include "cds_objects.h"
#include "zmm/zmm.h"
#include "zmm/zmmf.h"

/// \brief This class will handle things that are specific to various services
class OnlineServiceHelper : public zmm::Object {
public:
    OnlineServiceHelper();

    /// \brief this function will determine the final URL for services that
    /// need extra steps to do that
    zmm::String resolveURL(zmm::Ref<CdsItemExternalURL> item);
};

#endif //__ONLINE_SERVICE_HELPER_H__

#endif //ONLINE_SERVICES
