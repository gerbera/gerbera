/*MT*
    
    MediaTomb - http://www.mediatomb.org/
    
    upnp_mrreg.cc - this file is part of MediaTomb.
    
    Copyright (C) 2005 Gena Batyan <bgeradz@mediatomb.org>,
                       Sergey Bostandzhyan <jin@mediatomb.org>
    Copyright (C) 2006 Gena Batyan <bgeradz@mediatomb.org>,
                       Sergey Bostandzhyan <jin@mediatomb.org>,
                       Leonhard Wimmer <leo@mediatomb.org>
    
    MediaTomb is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License version 2
    as published by the Free Software Foundation.
    
    MediaTomb is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.
    
    You should have received a copy of the GNU General Public License
    version 2 along with MediaTomb; if not, write to the Free Software
    Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
    
    $Id$
*/

/// \file upnp_mrreg.cc

#ifdef HAVE_CONFIG_H
    #include "autoconfig.h"
#endif

#include "upnp_mrreg.h"

using namespace zmm;
using namespace mxml;

SINGLETON_MUTEX(MRRegistrarService, false);

String MRRegistrarService::serviceType = nil;
String MRRegistrarService::serviceID = nil;

MRRegistrarService::MRRegistrarService() : Singleton<MRRegistrarService>()
{
    if (serviceType == nil || serviceID == nil)
        throw _Exception(_("serviceType or serviceID not set!"));
}

MRRegistrarService::~MRRegistrarService()
{
    serviceType = nil;
    serviceID = nil;
}

void MRRegistrarService::setStaticArgs(String _serviceType, String _serviceID)
{
    serviceType = _serviceType;
    serviceID = _serviceID;
}

