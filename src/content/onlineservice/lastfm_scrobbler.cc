/*MT*

    MediaTomb - http://www.mediatomb.cc/

    lastfm_scrobbler.cc - this file is part of MediaTomb.

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

/// \file lastfm_scrobbler.cc

#ifdef HAVE_LASTFMLIB
#define GRB_LOG_FAC GrbLogFacility::online
#include "lastfm_scrobbler.h" // API

#include "cds_item.h"
#include "config/config_manager.h"
#include "metadata/metadata_enums.h"
#include "util/tools.h"

LastFm::LastFm(const std::shared_ptr<Context>& context)
    : config(context->getConfig())
{
}

void LastFm::run()
{
    if (!config->getBoolOption(CFG_SERVER_EXTOPTS_LASTFM_ENABLED))
        return;

    std::string username = config->getOption(CFG_SERVER_EXTOPTS_LASTFM_USERNAME);
    std::string password = config->getOption(CFG_SERVER_EXTOPTS_LASTFM_PASSWORD);

    scrobbler = std::make_unique<LastFmScrobbler>(username, password, false, false);
    scrobbler->authenticate();
    scrobbler->setCommitOnlyMode(true);
}

void LastFm::shutdown()
{
    if (!scrobbler)
        return;

    scrobbler->finishedPlaying();
    scrobbler = nullptr;
}

void LastFm::startedPlaying(const std::shared_ptr<CdsItem>& item)
{
    if (currentTrackId == item->getID() || !scrobbler)
        return;

    currentTrackId = item->getID();

    std::string artist = item->getMetaData(MetadataField::M_ARTIST);
    std::string title = item->getMetaData(MetadataField::M_TITLE);

    log_debug("Artist:\t{}", artist);
    log_debug("Title:\t{}", title);

    if (artist.empty() || title.empty()) {
        scrobbler->finishedPlaying();
        currentTrackId = -1;
        return;
    }

    auto info = SubmissionInfo(artist, title);

    std::string album = item->getMetaData(MetadataField::M_ALBUM);
    if (!album.empty())
        info.setAlbum(album);

    std::string trackNr = item->getMetaData(MetadataField::M_TRACKNUMBER);
    if (!trackNr.empty())
        info.setTrackNr(std::stoi(trackNr));

    if (item->getResourceCount() > 0) {
        auto resource = item->getResource(ContentHandler::DEFAULT);
        std::string duration = resource->getAttribute(ResourceAttribute::DURATION);
        info.setTrackLength(HMSFToMilliseconds(duration) / 1000);
    }

    scrobbler->startedPlaying(info);
}

#endif // HAVE_LASTFMLIB
