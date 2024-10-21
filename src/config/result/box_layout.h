/*GRB*

    Gerbera - https://gerbera.io/

    box_layout.h - this file is part of Gerbera.

    Copyright (C) 2023-2024 Gerbera Contributors

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

/// \file box_layout.h
///\brief Definition of the BoxLayout class.

#ifndef __BOXLAYOUT_H_
#define __BOXLAYOUT_H_

#include "common.h"
#include "edit_helper.h"

#include <map>
#include <memory>
#include <mutex>
#include <string>
#include <vector>

// forward declaration
class BoxLayout;
using EditHelperBoxLayout = EditHelper<BoxLayout>;

class BoxKeys {
public:
    static constexpr std::string_view audioAllAlbums = "Audio/allAlbums";
    static constexpr std::string_view audioAllArtists = "Audio/allArtists";
    static constexpr std::string_view audioAll = "Audio/allAudio";
    static constexpr std::string_view audioAllComposers = "Audio/allComposers";
    static constexpr std::string_view audioAllDirectories = "Audio/allDirectories";
    static constexpr std::string_view audioAllGenres = "Audio/allGenres";
    static constexpr std::string_view audioAllSongs = "Audio/allSongs";
    static constexpr std::string_view audioAllTracks = "Audio/allTracks";
    static constexpr std::string_view audioAllYears = "Audio/allYears";
    static constexpr std::string_view audioArtistChronology = "Audio/artistChronology";
    static constexpr std::string_view audioRoot = "Audio/audioRoot";

    static constexpr std::string_view audioInitialAbc = "AudioInitial/abc";
    static constexpr std::string_view audioInitialAllArtistTracks = "AudioInitial/allArtistTracks";
    static constexpr std::string_view audioInitialAllBooks = "AudioInitial/allBooks";
    static constexpr std::string_view audioInitialAudioBookRoot = "AudioInitial/audioBookRoot";

    static constexpr std::string_view audioStructuredAllAlbums = "AudioStructured/allAlbums";
    static constexpr std::string_view audioStructuredAllArtistTracks = "AudioStructured/allArtistTracks";
    static constexpr std::string_view audioStructuredAllArtists = "AudioStructured/allArtists";
    static constexpr std::string_view audioStructuredAllGenres = "AudioStructured/allGenres";
    static constexpr std::string_view audioStructuredAllTracks = "AudioStructured/allTracks";
    static constexpr std::string_view audioStructuredAllYears = "AudioStructured/allYears";

    static constexpr std::string_view videoAllDates = "Video/allDates";
    static constexpr std::string_view videoAllDirectories = "Video/allDirectories";
    static constexpr std::string_view videoAll = "Video/allVideo";
    static constexpr std::string_view videoAllYears = "Video/allYears";
    static constexpr std::string_view videoRoot = "Video/videoRoot";
    static constexpr std::string_view videoUnknown = "Video/unknown";

    static constexpr std::string_view imageAllDates = "Image/allDates";
    static constexpr std::string_view imageAllDirectories = "Image/allDirectories";
    static constexpr std::string_view imageAll = "Image/allImages";
    static constexpr std::string_view imageAllYears = "Image/allYears";
    static constexpr std::string_view imageRoot = "Image/imageRoot";
    static constexpr std::string_view imageUnknown = "Image/unknown";

#ifdef ONLINE_SERVICES
    static constexpr std::string_view trailerAllGenres = "Trailer/allGenres";
    static constexpr std::string_view trailerAll = "Trailer/allTrailers";
    static constexpr std::string_view trailerPostDate = "Trailer/postDate";
    static constexpr std::string_view trailerRelDate = "Trailer/relDate";
    static constexpr std::string_view trailerRoot = "Trailer/trailerRoot";
    static constexpr std::string_view trailerUnknown = "Trailer/unknown";
#endif

    static constexpr std::string_view playlistAll = "Playlist/allPlaylists";
    static constexpr std::string_view playlistAllDirectories = "Playlist/allDirectories";
    static constexpr std::string_view playlistRoot = "Playlist/playlistRoot";
};

class BoxLayoutList : public EditHelperBoxLayout {
public:
    std::shared_ptr<BoxLayout> getKey(const std::string_view& key) const;
};

/// \brief Define properties of a tree entry in virtual layout
class BoxLayout : public Editable {
public:
    BoxLayout() = default;
    explicit BoxLayout(const std::string_view& key, std::string title, std::string objClass, bool enabled = true, int size = 1)
        : key(key)
        , title(std::move(title))
        , objClass(std::move(objClass))
        , enabled(enabled)
        , size(size)
    {
    }

    bool equals(const std::shared_ptr<BoxLayout>& other) { return this->key == other->key; }

    void setKey(std::string key) { this->key = std::move(key); }
    std::string getKey() const { return key; }

    void setTitle(std::string title) { this->title = std::move(title); }
    std::string getTitle() const { return title; }

    void setClass(std::string objClass) { this->objClass = std::move(objClass); }
    std::string getClass() const { return objClass; }

    void setEnabled(bool enabled) { this->enabled = enabled; }
    bool getEnabled() const { return enabled; }

    void setSize(int size) { this->size = size; }
    int getSize() const { return size; }

    void setId(int id) { this->id = id; }
    int getId() const { return id; }

protected:
    std::string key;
    std::string title;
    std::string objClass;
    bool enabled { true };
    int size { 1 };
    int id { INVALID_OBJECT_ID };
};

#endif
