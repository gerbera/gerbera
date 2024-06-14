/*MT*

    MediaTomb - http://www.mediatomb.cc/

    atrailers_service.cc - this file is part of MediaTomb.

    Copyright (C) 2005 Gena Batyan <bgeradz@mediatomb.cc>,
                       Sergey 'Jin' Bostandzhyan <jin@mediatomb.cc>

    Copyright (C) 2006-2010 Gena Batyan <bgeradz@mediatomb.cc>,
                            Sergey 'Jin' Bostandzhyan <jin@mediatomb.cc>,
                            Leonhard Wimmer <leo@mediatomb.cc>

    Copyright (C) 2016-2024 Gerbera Contributors

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
#define GRB_LOG_FAC GrbLogFacility::online
#include "atrailers_service.h" // API

#include "atrailers_content_handler.h"
#include "config/config_option_enum.h"
#include "content/content.h"
#include "util/tools.h"

#define ATRAILERS_SERVICE_URL_640 "https://trailers.apple.com/trailers/home/xml/current.xml"
#define ATRAILERS_SERVICE_URL_720P "https://trailers.apple.com/trailers/home/xml/current_720p.xml"

ATrailersService::ATrailersService(const std::shared_ptr<Content>& content)
    : CurlOnlineService(content, ATRAILERS_SERVICE)
{
    if (EnumOption<AtrailerResolution>::getEnumOption(config, CFG_ONLINE_CONTENT_ATRAILERS_RESOLUTION) == AtrailerResolution::Low)
        service_url = ATRAILERS_SERVICE_URL_640;
    else
        service_url = ATRAILERS_SERVICE_URL_720P;
}

std::unique_ptr<CurlContentHandler> ATrailersService::getContentHandler() const
{
    return std::make_unique<ATrailersContentHandler>(content->getContext());
}

OnlineServiceType ATrailersService::getServiceType() const
{
    return OnlineServiceType::OS_ATrailers;
}
#endif // ATRAILERS
