/*GRB*

    Gerbera - https://gerbera.io/

    box_layout.cc - this file is part of Gerbera.

    Copyright (C) 2023-2026 Gerbera Contributors

    Gerbera is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License version 2
    as published by the Free Software Foundation.

    Gerbera is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with Gerbera.  If not, see <http://www.gnu.org/licenses/>.

    $Id$
*/

/// @file config/result/box_layout.cc
#define GRB_LOG_FAC GrbLogFacility::content

#include "box_layout.h" // API

#include "config/config.h"

#include <algorithm>

std::shared_ptr<BoxLayout> BoxLayoutList::getKey(BoxKeys bkey) const
{
    auto&& key = BoxLayout::getBoxKey(bkey);
    EditHelperBoxLayout::AutoLock lock(EditHelperBoxLayout::mutex);
    auto entry = std::find_if(EditHelperBoxLayout::list.begin(), EditHelperBoxLayout::list.end(), [&](auto&& d) { return d->getKey() == key; });
    if (entry != EditHelperBoxLayout::list.end() && *entry) {
        return *entry;
    }
    return nullptr;
}

/// @brief Predefined box keys
const std::map<BoxKeys, std::string_view> BoxLayout::boxKeys = {
    { BoxKeys::root, "Root" },
    { BoxKeys::pcDirectory, "PCDirectory" },
    { BoxKeys::audioAllAlbums, "Audio/allAlbums" },
    { BoxKeys::audioAllArtists, "Audio/allArtists" },
    { BoxKeys::audioAll, "Audio/allAudio" },
    { BoxKeys::audioAllComposers, "Audio/allComposers" },
    { BoxKeys::audioAllDirectories, "Audio/allDirectories" },
    { BoxKeys::audioAllGenres, "Audio/allGenres" },
    { BoxKeys::audioAllSongs, "Audio/allSongs" },
    { BoxKeys::audioAllTracks, "Audio/allTracks" },
    { BoxKeys::audioAllYears, "Audio/allYears" },
    { BoxKeys::audioArtistChronology, "Audio/artistChronology" },
    { BoxKeys::audioRoot, "Audio/audioRoot" },

    { BoxKeys::audioInitialAbc, "AudioInitial/abc" },
    { BoxKeys::audioInitialAllArtistTracks, "AudioInitial/allArtistTracks" },
    { BoxKeys::audioInitialAllBooks, "AudioInitial/allBooks" },
    { BoxKeys::audioInitialAudioBookRoot, "AudioInitial/audioBookRoot" },

    { BoxKeys::audioStructuredAllAlbums, "AudioStructured/allAlbums" },
    { BoxKeys::audioStructuredAllArtistTracks, "AudioStructured/allArtistTracks" },
    { BoxKeys::audioStructuredAllArtists, "AudioStructured/allArtists" },
    { BoxKeys::audioStructuredAllGenres, "AudioStructured/allGenres" },
    { BoxKeys::audioStructuredAllTracks, "AudioStructured/allTracks" },
    { BoxKeys::audioStructuredAllYears, "AudioStructured/allYears" },

    { BoxKeys::videoAllDates, "Video/allDates" },
    { BoxKeys::videoAllDirectories, "Video/allDirectories" },
    { BoxKeys::videoAll, "Video/allVideo" },
    { BoxKeys::videoAllYears, "Video/allYears" },
    { BoxKeys::videoRoot, "Video/videoRoot" },
    { BoxKeys::videoUnknown, "Video/unknown" },

    { BoxKeys::imageAllDates, "Image/allDates" },
    { BoxKeys::imageAllDirectories, "Image/allDirectories" },
    { BoxKeys::imageAll, "Image/allImages" },
    { BoxKeys::imageAllYears, "Image/allYears" },
    { BoxKeys::imageRoot, "Image/imageRoot" },
    { BoxKeys::imageUnknown, "Image/unknown" },
    { BoxKeys::imageAllModels, "ImageDetail/allModels" },
    { BoxKeys::imageYearMonth, "ImageDetail/yearMonth" },
    { BoxKeys::imageYearDate, "ImageDetail/yearDate" },

    { BoxKeys::topicRoot, "Topic/topicRoot" },
    { BoxKeys::topic, "Topic/topic" },
    { BoxKeys::topicExtra, "Topic/topicExtra" },

#ifdef ONLINE_SERVICES
    { BoxKeys::trailerAllGenres, "Trailer/allGenres" },
    { BoxKeys::trailerAll, "Trailer/allTrailers" },
    { BoxKeys::trailerPostDate, "Trailer/postDate" },
    { BoxKeys::trailerRelDate, "Trailer/relDate" },
    { BoxKeys::trailerRoot, "Trailer/trailerRoot" },
    { BoxKeys::trailerUnknown, "Trailer/unknown" },
#endif

    { BoxKeys::playlistAll, "Playlist/allPlaylists" },
    { BoxKeys::playlistAllDirectories, "Playlist/allDirectories" },
    { BoxKeys::playlistRoot, "Playlist/playlistRoot" },

    { BoxKeys::Custom, "./." },
};

std::string BoxLayout::getBoxKey(BoxKeys bkey)
{
    return std::string(boxKeys.at(bkey));
}

BoxKeys BoxLayout::getBoxKey(const std::string& key)
{
    auto entry = std::find_if(boxKeys.begin(), boxKeys.end(), [&](auto&& bk) { return bk.second == key; });
    if (entry != boxKeys.end())
        return entry->first;
    return BoxKeys::Custom;
}
