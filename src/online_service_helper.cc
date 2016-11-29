/*MT*
    
    MediaTomb - http://www.mediatomb.cc/
    
    online_service_helper.cc - this file is part of MediaTomb.
    
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

/// \file online_service_helper.cc
/// \brief Definition of the OnlineServiceHelper class.

#ifdef ONLINE_SERVICES

#include "zmm/zmm.h"
#include "zmm/zmmf.h"
#include "mxml/mxml.h"
#include "online_service.h"
#include "online_service_helper.h"
#include "config_manager.h"

#ifdef YOUTUBE
    #include "youtube_video_url.h"
    #include "youtube_content_handler.h"
    #include "cached_url.h"
    #include "content_manager.h"
#endif

using namespace zmm;

OnlineServiceHelper::OnlineServiceHelper()
{
}

String OnlineServiceHelper::resolveURL(Ref<CdsItemExternalURL> item)
{
    if (!item->getFlag(OBJECT_FLAG_ONLINE_SERVICE))
        throw _Exception(_("The given item does not belong to an online service"));

    service_type_t service = (service_type_t)(item->getAuxData(_(ONLINE_SERVICE_AUX_ID)).toInt());
    if (service > OS_Max)
        throw _Exception(_("Invalid service id!"));

    String url;
    
    switch (service)
    {
#ifdef YOUTUBE
        case OS_YouTube:
            {
                url = ContentManager::getInstance()->getCachedURL(item->getID());
                if (string_ok(url))
                    break;

                Ref<YouTubeVideoURL> yt_url;
                yt_url = Ref<YouTubeVideoURL> (new YouTubeVideoURL());
                //            log_debug("------> REQUESTING YT ID : %s\n", item->getServiceID().substring(1).c_str());
                //url = yt_url->getVideoURL(item->getURL());
                url = yt_url->getVideoURL(item->getServiceID().substring(1),
                        ConfigManager::getInstance()->getBoolOption(CFG_ONLINE_CONTENT_YOUTUBE_FORMAT_MP4), ConfigManager::getInstance()->getBoolOption(CFG_ONLINE_CONTENT_YOUTUBE_PREFER_HD));
                Ref<CachedURL> cached(new CachedURL(item->getID(), url));
                ContentManager::getInstance()->cacheURL(cached);
            }
            break;
#endif
#ifdef SOPCAST
        case OS_SopCast:
            url = item->getLocation();
            break;
#endif
#ifdef WEBORAMA
        case OS_Weborama:
            url = item->getLocation();
            break;
#endif
#ifdef ATRAILERS
        case OS_ATrailers:
            url = item->getLocation();
            break;
#endif
        case OS_Max:
        default:
            throw _Exception(_("No handler for this service!"));
    }

    return url; 
}

#endif//ONLINE_SERVICES
