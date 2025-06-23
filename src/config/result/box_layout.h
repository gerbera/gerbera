/*GRB*

    Gerbera - https://gerbera.io/

    box_layout.h - this file is part of Gerbera.

    Copyright (C) 2023-2025 Gerbera Contributors

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

/// @file box_layout.h
/// @brief Definition of the BoxLayout class.

#ifndef __BOXLAYOUT_H_
#define __BOXLAYOUT_H_

#include "common.h"
#include "config/config_options.h"
#include "edit_helper.h"
#include "upnp/upnp_common.h"

#include <map>
#include <memory>
#include <mutex>
#include <string>
#include <vector>

// forward declarations
class BoxLayout;
class BoxChain;
enum class AutoscanMediaMode;
using EditHelperBoxLayout = EditHelper<BoxLayout>;
using EditHelperBoxChain = EditHelper<BoxChain>;

/// @brief Predefined box keys
class BoxKeys {
public:
    static constexpr std::string_view root = "Root";
    static constexpr std::string_view pcDirectory = "PCDirectory";
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

/// @brief Manager class for BoxLayout and BoxChain
class BoxLayoutList : public EditHelperBoxLayout, public EditHelperBoxChain {
public:
    std::shared_ptr<BoxLayout> getKey(const std::string_view& key) const;
};

/// @brief Define properties of a tree entry in virtual layout
class BoxLayout : public Editable {
public:
    BoxLayout() = default;
    virtual ~BoxLayout() = default;
    explicit BoxLayout(
        const std::string_view& key,
        std::string title,
        std::string objClass = UPNP_CLASS_CONTAINER,
        std::string upnpShortcut = "",
        std::string sortKey = "",
        bool enabled = true,
        int size = 1)
        : key(key)
        , title(std::move(title))
        , objClass(std::move(objClass))
        , upnpShortcut(std::move(upnpShortcut))
        , sortKey(std::move(sortKey))
        , enabled(enabled)
        , size(size)
    {
    }

    bool equals(const std::shared_ptr<BoxLayout>& other) { return this->key == other->key; }

    void setKey(std::string key) { this->key = std::move(key); }
    std::string getKey() const { return key; }

    void setTitle(std::string title) { this->title = std::move(title); }
    std::string getTitle() const { return title; }

    void setUpnpShortcut(std::string upnpShortcut) { this->upnpShortcut = std::move(upnpShortcut); }
    std::string getUpnpShortcut() const { return upnpShortcut; }

    void setClass(std::string objClass) { this->objClass = std::move(objClass); }
    std::string getClass() const { return objClass; }

    void setEnabled(bool enabled) { this->enabled = enabled; }
    bool getEnabled() const { return enabled; }

    void setSize(int size) { this->size = size; }
    int getSize() const { return size; }

    void setId(int id) { this->id = id; }
    int getId() const { return id; }

    void setSortKey(std::string sortKey) { this->sortKey = std::move(sortKey); }
    std::string getSortKey() const { return sortKey; }

protected:
    /// \brief key for the box to be referenced in layouts
    std::string key;
    /// \brief title string for the box
    std::string title;
    /// \brief object class to be used for new containers
    std::string objClass;
    /// \brief shortcut name for upnp shortcuts list
    std::string upnpShortcut;
    /// \brief sorting key for directory listing
    std::string sortKey;
    /// \brief allow to disable boxes
    bool enabled { true };
    /// \brief size value for the box, esp. in structured layout
    int size { 1 };
    /// \brief objectId of the box when existing
    int id { INVALID_OBJECT_ID };
};

/// @brief Define a tree in virtual layout
class BoxChain : public Editable {
public:
    BoxChain() = default;
    virtual ~BoxChain() = default;
    explicit BoxChain(
        std::size_t index,
        AutoscanMediaMode type,
        const std::vector<std::vector<std::pair<std::string, std::string>>>& links)
        : index(index)
        , type(type)
        , links(VectorOption(links))
    {
    }

    std::size_t getIndex() const { return index; }
    AutoscanMediaMode getType() const { return type; }
    void setType(AutoscanMediaMode type) { this->type = type; }

    /// \brief comparison of chains
    bool equals(const std::shared_ptr<BoxChain>& other) { return this->type == other->type && this->index == other->index; }

    /// \brief links for chain
    std::vector<std::vector<std::pair<std::string, std::string>>> getLinks(bool edit = false) const { return this->links.getVectorOption(edit); }
    std::size_t getLinksSize(bool edit = false) const { return this->links.getVectorOption(edit).size(); }
    void setLinks(const std::vector<std::vector<std::pair<std::string, std::string>>>& links) { this->links = VectorOption(links); }
    void setLinkKey(std::size_t j, std::size_t k, const std::string& key) { return this->links.setKey(j, k, key); }
    void setLinkValue(std::size_t j, std::size_t k, const std::string& value) { return this->links.setValue(j, k, value); }

protected:
    /// @brief identification of chains
    std::size_t index = { 0 };
    /// \brief media type chain applies to
    AutoscanMediaMode type;
    /// \brief list of chain links making up the virtual tree
    VectorOption links;
};

#endif
