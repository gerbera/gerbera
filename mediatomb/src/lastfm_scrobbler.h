/// \file lastfm_scrobbler.h
/// \brief Last.fm scrobbler.

#ifdef HAVE_LASTFMLIB

#ifndef __LASTFM_H__
#define __LASTFM_H__

#include <stdlib.h>
#include "zmm/zmm.h"
#include "zmm/ref.h"
#include "singleton.h"
#include "cds_objects.h"
#include <lastfmlib/lastfmscrobblerc.h>

class LastFm : public Singleton<LastFm>
{
public:
    LastFm();
    ~LastFm();

    /// \brief Initializes the LastFm client.
    ///
    /// This function reads information from the config and initializes
    /// various variables (like username and password).
    void init();

    /// \brief Destroys the LastFm client.
    ///
    /// This function destroys the LastFm client after submitting the
    /// last Track info
    void shutdown();

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

#endif//__LASTFM_H__

#endif//HAVE_LASTFMLIB
