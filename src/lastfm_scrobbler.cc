/*MT*
    
    MediaTomb - http://www.mediatomb.cc/
    
    lastfm_scrobbler.cc - this file is part of MediaTomb.
    
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

/// \file lastfm_scrobbler.cc

#ifdef HAVE_LASTFMLIB

#include "lastfm_scrobbler.h"
#include "config_manager.h"
#include "metadata_handler.h"
#include "tools.h"
using namespace zmm;

LastFm::LastFm()
    : Singleton<LastFm, std::mutex>()
    , scrobbler(NULL)
    , currentTrackId(-1)
{
}

LastFm::~LastFm()
{
    if (currentTrackId != -1 && scrobbler)
        finished_playing(scrobbler);
}

void LastFm::init()
{
    Ref<ConfigManager> config = ConfigManager::getInstance();

    if (!config->getBoolOption(CFG_SERVER_EXTOPTS_LASTFM_ENABLED))
        return;

    String username = config->getOption(CFG_SERVER_EXTOPTS_LASTFM_USERNAME);
    String password = config->getOption(CFG_SERVER_EXTOPTS_LASTFM_PASSWORD);

    scrobbler = create_scrobbler(username.c_str(), password.c_str(), 0, 0);
    authenticate_scrobbler(scrobbler);
    set_commit_only_mode(scrobbler, 1);
}

void LastFm::shutdown()
{
    if (!scrobbler)
        return;

    finished_playing(scrobbler);
    destroy_scrobbler(scrobbler);
    scrobbler = NULL;
}

void LastFm::startedPlaying(Ref<CdsItem> item)
{
    if (currentTrackId == item->getID() || scrobbler == NULL)
        return;

    currentTrackId = item->getID();

    log_debug("Artist:\t%s\n",
        item->getMetadata(MetadataHandler::getMetaFieldName(M_ARTIST)).c_str());
    log_debug("Title:\t%s\n",
        item->getMetadata(MetadataHandler::getMetaFieldName(M_TITLE)).c_str());

    String artist = item->getMetadata(MetadataHandler::getMetaFieldName(M_ARTIST));
    String title = item->getMetadata(MetadataHandler::getMetaFieldName(M_TITLE));

    if (!string_ok(artist) || !string_ok(title)) {
        finished_playing(scrobbler);
        currentTrackId = -1;
        return;
    }

    submission_info* info = create_submission_info();
    info->artist = const_cast<char*>(artist.c_str());
    info->track = const_cast<char*>(title.c_str());

    String album = item->getMetadata(MetadataHandler::getMetaFieldName(M_ALBUM));
    if (string_ok(album))
        info->album = const_cast<char*>(album.c_str());

    String trackNr = item->getMetadata(MetadataHandler::getMetaFieldName(M_TRACKNUMBER));
    if (string_ok(trackNr))
        info->track_nr = atoi(trackNr.c_str());

    if (item->getResourceCount() > 0) {
        Ref<CdsResource> resource = item->getResource(0);
        String duration = resource->getAttribute(MetadataHandler::getResAttrName(R_DURATION));
        info->track_length_in_secs = HMSToSeconds(duration.c_str());
    }

    started_playing(scrobbler, info);

    destroy_submission_info(info);
}

#endif //HAVE_LASTFMLIB
