/*GRB*
    Gerbera - https://gerbera.io/

    lastfmlib.h - this file is part of Gerbera.

    Copyright (C) 2025 Gerbera Contributors

    Gerbera is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License version 2
    as published by the Free Software Foundation.

    Gerbera is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with Gerbera.  If not, see <http://www.gnu.org/licenses/>.
*/

/// @file content/onlineservice/lastfmlib.h
/// @brief Definitions of the LastFMScrobbler class.

#ifndef __GRB_LASTFMLIB_H__
#define __GRB_LASTFMLIB_H__

#ifdef HAVE_CURL

#include <ctime>
#include <string>
#include <vector>

class SubmissionInfo {
public:
    /// @brief Constructor that sets artist and track
    SubmissionInfo(const std::string& artist, const std::string& title)
        : artist(artist)
        , track(title)
        , trackLengthInSecs(-1)
        , trackNr(-1)
    {
    }

    /// @brief sets the artist of the track
    void setArtist(const std::string& artist) { this->artist = artist; }
    /// @brief returns the artist
    const std::string& getArtist() const { return this->artist; }

    /// @brief sets the title of the track
    void setTrack(const std::string& track) { this->track = track; }
    /// @brief returns the track title
    const std::string& getTrack() const { return this->track; }

    /// @brief sets the album of the track
    void setAlbum(const std::string& album) { this->album = album; }
    /// @brief returns the album
    const std::string& getAlbum() const { return this->album; }

    /// @brief sets the track length (in seconds)
    void setTrackLength(int lengthInSecs) { this->trackLengthInSecs = lengthInSecs; }
    /// @brief returns the track length (in seconds)
    int getTrackLength() const { return this->trackLengthInSecs; }

    /// @brief sets the track number
    void setTrackNr(int trackNr) { this->trackNr = trackNr; }
    /// @brief returns the track number
    int getTrackNr() const { return this->trackNr; }

protected:
    /// @brief the artist
    std::string artist;
    /// @brief the track title
    std::string track;
    /// @brief the album
    std::string album;
    /// @brief the track length (in seconds)
    int trackLengthInSecs;
    /// @brief the track number
    int trackNr;
};

/// @brief Class to wrap last.fm web API.
class LastFmScrobbler {
public:
    LastFmScrobbler(const std::string& apiKey, const std::string& apiSecret);
    LastFmScrobbler(const std::string& apiKey, const std::string& apiSecret, const std::string& sessionKey);

    /// @brief ensure full credentials are available
    void authenticate();
    void startedPlaying(const SubmissionInfo& info);
    void finishedPlaying();

    /// @brief Get a token for authentication
    std::string fetchAuthToken();

    /// @brief Authentication URL for the user to authorize with Last.fm
    std::string getAuthURL(const std::string& token) const;

    /// @brief Fetch session key after user authorizes the token in browser
    bool fetchSessionKey(const std::string& token);

    /// @brief Set session key (load from persistent storage)
    void setSessionKey(const std::string& sessionKey) { this->sessionKey = sessionKey; }

    /// @brief Get session key (save to persistent storage)
    std::string getSessionKey() const { return this->sessionKey; }

    void setAuthUrl(std::string url) { authUrl = std::move(url); }
    void setScrobbleUrl(std::string url) { scrobbleUrl = std::move(url); }

    /// @brief Send Now Playing info
    bool updateNowPlaying(
        const std::string& artist,
        const std::string& track,
        const std::string& album = "",
        int duration = 0);

    /// @brief Scrobble a track
    bool scrobbleTrack(
        const std::string& artist,
        const std::string& track,
        std::time_t timestamp,
        const std::string& album = "",
        int duration = 0);

private:
    std::string apiKey;
    std::string apiSecret;
    std::string sessionKey;

    std::string authUrl { "https://www.last.fm/api/auth/" };
    std::string scrobbleUrl { "https://ws.audioscrobbler.com/2.0/" };

    std::string buildApiSig(const std::vector<std::pair<std::string, std::string>>& params) const;
    std::string postRequest(const std::vector<std::pair<std::string, std::string>>& params) const;
};

#endif // HAVE_CURL
#endif // __GRB_LASTFMLIB_H__
