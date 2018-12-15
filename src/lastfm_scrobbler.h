/*MT*
    
    MediaTomb - http://www.mediatomb.cc/
    
    lastfm_scrobbler.h - this file is part of MediaTomb.
    
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

/// \file lastfm_scrobbler.h
/// \brief Last.fm scrobbler.

#ifdef HAVE_LASTFMLIB

#ifndef __LASTFM_H__
#define __LASTFM_H__

#include "cds_objects.h"
#include "singleton.h"
#include "zmm/ref.h"
#include "zmm/zmm.h"
#include <lastfmlib/lastfmscrobblerc.h>
#include <stdlib.h>

class LastFm : public Singleton<LastFm, std::mutex> {
public:
    LastFm();
    zmm::String getName() override { return _("LastFM Scrobbler"); }
    ~LastFm();

    /// \brief Initializes the LastFm client.
    ///
    /// This function reads information from the config and initializes
    /// various variables (like username and password).
    void init() override;

    /// \brief Destroys the LastFm client.
    ///
    /// This function destroys the LastFm client after submitting the
    /// last Track info
    void shutdown() override;

    /// \brief indicates that a new file has started playing.
    ///
    /// This function uses notifies Last.fm that the user started listening
    /// to a file
    ///
    /// \param item the audio item that is being played
    void startedPlaying(zmm::Ref<CdsItem> item);

private:
    lastfm_scrobbler* scrobbler;
    int currentTrackId;
};

#endif //__LASTFM_H__

#endif //HAVE_LASTFMLIB
