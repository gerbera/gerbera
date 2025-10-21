/*MT*

    MediaTomb - http://www.mediatomb.cc/

    lastfm_scrobbler.cc - this file is part of MediaTomb.

    Copyright (C) 2005 Gena Batyan <bgeradz@mediatomb.cc>,
                       Sergey 'Jin' Bostandzhyan <jin@mediatomb.cc>

    Copyright (C) 2006-2010 Gena Batyan <bgeradz@mediatomb.cc>,
                            Sergey 'Jin' Bostandzhyan <jin@mediatomb.cc>,
                            Leonhard Wimmer <leo@mediatomb.cc>

    Copyright (C) 2016-2025 Gerbera Contributors

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

/// @file content/onlineservice/lastfm_scrobbler.cc

#ifdef HAVE_LASTFM
#define GRB_LOG_FAC GrbLogFacility::online
#include "lastfm_scrobbler.h" // API

#include "cds/cds_item.h"
#include "config/config.h"
#include "config/config_val.h"
#include "context.h"
#include "metadata/metadata_enums.h"
#include "util/grb_time.h"
#include "util/logger.h"
#include "util/tools.h"

#ifdef HAVE_LASTFMLIB
#include <lastfmlib/lastfmscrobbler.h>
#else
#include "lastfmlib.h"

#include <iostream>
#endif

int LastFm::setup()
{
#ifdef HAVE_LASTFMLIB
    log_error("Not supported with lastfmlib and old Last.fm API");
    return 1;
#else
    if (!enabled) {
        log_error("Enable Last.fm for setup.");
        return 1;
    }
    if (username.empty()) {
        log_error("Last.fm username not set");
        return 1;
    }
    if (password.empty()) {
        log_error("Last.fm password not set");
        return 1;
    }

    scrobbler = std::make_unique<LastFmScrobbler>(username, password, "");
    auto authUrl = config->getOption(ConfigVal::SERVER_EXTOPTS_LASTFM_AUTHURL);
    if (!authUrl.empty())
        scrobbler->setAuthUrl(authUrl);
    auto scrobbleUrl = config->getOption(ConfigVal::SERVER_EXTOPTS_LASTFM_SCROBBLEURL);
    if (!scrobbleUrl.empty())
        scrobbler->setScrobbleUrl(scrobbleUrl);

    // 1. Get a token from Last.fm
    std::string token = scrobbler->fetchAuthToken();
    if (token.empty()) {
        log_error("Could not fetch auth token from Last.fm.");
        return 1;
    }

    // 2. Direct the user to authorize your application in their browser
    std::cout << "Please authorize the application by visiting this URL:\n"
              << scrobbler->getAuthURL(token) << std::endl;

    // 3. After the user has authorized, fetch the session key
    std::cout << "Press ENTER after you have authorized the application...";
    std::cin.ignore();

    if (scrobbler->fetchSessionKey(token)) {
        log_info("Save the session key '{}' for future use and add it to config.xml", scrobbler->getSessionKey());
    } else {
        log_error("Could not fetch session key from Last.fm.");
        return 1;
    }

    return 0;
#endif
}

LastFm::LastFm(const std::shared_ptr<Context>& context)
    : config(context->getConfig())
    , enabled(config->getBoolOption(ConfigVal::SERVER_EXTOPTS_LASTFM_ENABLED))
{
    if (enabled) {
        username = config->getOption(ConfigVal::SERVER_EXTOPTS_LASTFM_USERNAME);
        password = config->getOption(ConfigVal::SERVER_EXTOPTS_LASTFM_PASSWORD);
    }
}

void LastFm::run()
{
    if (!enabled)
        return;

#ifdef HAVE_LASTFMLIB
    scrobbler = std::make_unique<LastFmScrobbler>(username, password, false, false);
    scrobbler->setCommitOnlyMode(true);
#else
    std::string sessionKey = config->getOption(ConfigVal::SERVER_EXTOPTS_LASTFM_SESSIONKEY);
    scrobbler = std::make_unique<LastFmScrobbler>(username, password, sessionKey);
#endif
    scrobbler->authenticate();
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

    std::string artist = item->getMetaData(MetadataFields::M_ARTIST);
    std::string title = item->getMetaData(MetadataFields::M_TITLE);

    log_debug("Artist:\t{}", artist);
    log_debug("Title:\t{}", title);

    if (artist.empty() || title.empty()) {
        scrobbler->finishedPlaying();
        currentTrackId = -1;
        return;
    }

    auto info = SubmissionInfo(artist, title);

    std::string album = item->getMetaData(MetadataFields::M_ALBUM);
    if (!album.empty())
        info.setAlbum(album);

    std::string trackNr = item->getMetaData(MetadataFields::M_TRACKNUMBER);
    if (!trackNr.empty())
        info.setTrackNr(std::stoi(trackNr));

    if (item->getResourceCount() > 0) {
        auto resource = item->getResource(ContentHandler::DEFAULT);
        std::string duration = resource->getAttribute(ResourceAttribute::DURATION);
        info.setTrackLength(HMSFToMilliseconds(duration) / 1000);
    }

    scrobbler->startedPlaying(info);
}

#endif // HAVE_LASTFM
