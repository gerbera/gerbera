/*MT*
    
    MediaTomb - http://www.mediatomb.cc/
    
    atrailers_service.cc - this file is part of MediaTomb.
    
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

/// \file atrailers_service.cc

#ifdef ATRAILERS
#include "atrailers_service.h" // API

#include <string>

#include "config/config_manager.h"
#include "config/config_options.h"
#include "content/content_manager.h"
#include "database/database.h"
#include "util/string_converter.h"
#include "util/url.h"

#define ATRAILERS_SERVICE_URL_640 "https://trailers.apple.com/trailers/home/xml/current.xml"
#define ATRAILERS_SERVICE_URL_720P "https://trailers.apple.com/trailers/home/xml/current_720p.xml"

ATrailersService::ATrailersService(std::shared_ptr<ContentManager> content)
    : CurlOnlineService(std::move(content), ATRAILERS_SERVICE)
{
    if (config->getOption(CFG_ONLINE_CONTENT_ATRAILERS_RESOLUTION) == "640")
        service_url = ATRAILERS_SERVICE_URL_640;
    else
        service_url = ATRAILERS_SERVICE_URL_720P;
}

std::unique_ptr<CurlContentHandler> ATrailersService::getContentHandler() const
{
    return std::make_unique<ATrailersContentHandler>(content->getContext());
}

service_type_t ATrailersService::getServiceType()
{
    return OS_ATrailers;
}
#endif //ATRAILERS
