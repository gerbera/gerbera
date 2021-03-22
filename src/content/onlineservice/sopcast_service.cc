/*MT*
    
    MediaTomb - http://www.mediatomb.cc/
    
    sopcast_service.cc - this file is part of MediaTomb.
    
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

/// \file sopcast_service.cc

#ifdef SOPCAST
#include "sopcast_service.h" // API

#include "config/config_manager.h"
#include "content/content_manager.h"
#include "database/database.h"
#include "sopcast_content_handler.h"
#include "util/string_converter.h"
#include "util/url.h"

#define SOPCAST_CHANNEL_URL "http://www.sopcast.com/gchlxml"

SopCastService::SopCastService(std::shared_ptr<ContentManager> content)
    : CurlOnlineService(std::move(content), SOPCAST_SERVICE)
{
    service_url = SOPCAST_CHANNEL_URL;
}

std::unique_ptr<CurlContentHandler> SopCastService::getContentHandler() const
{
    return std::make_unique<SopCastContentHandler>(content->getContext());
}

service_type_t SopCastService::getServiceType()
{
    return OS_SopCast;
}

#endif //SOPCAST
