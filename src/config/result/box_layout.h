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

/// @file config/result/box_layout.h
/// @brief Definition of the BoxLayout class.

#ifndef __BOXLAYOUT_H_
#define __BOXLAYOUT_H_

#include "common.h"
#include "config/config_options.h"
#include "edit_helper.h"
#include "upnp/upnp_common.h"

#include <map>
#include <memory>
#include <string>
#include <vector>

// forward declarations
class BoxLayout;
class BoxChain;
enum class AutoscanMediaMode;
using EditHelperBoxLayout = EditHelper<BoxLayout>;
using EditHelperBoxChain = EditHelper<BoxChain>;

enum class BoxKeys {
    root,
    pcDirectory,
    audioAllAlbums,
    audioAllArtists,
    audioAll,
    audioAllComposers,
    audioAllDirectories,
    audioAllGenres,
    audioAllSongs,
    audioAllTracks,
    audioAllYears,
    audioArtistChronology,
    audioRoot,

    audioInitialAbc,
    audioInitialAllArtistTracks,
    audioInitialAllBooks,
    audioInitialAudioBookRoot,

    audioStructuredAllAlbums,
    audioStructuredAllArtistTracks,
    audioStructuredAllArtists,
    audioStructuredAllGenres,
    audioStructuredAllTracks,
    audioStructuredAllYears,

    videoAllDates,
    videoAllDirectories,
    videoAll,
    videoAllYears,
    videoRoot,
    videoUnknown,

    imageAllDates,
    imageAllDirectories,
    imageAll,
    imageAllYears,
    imageRoot,
    imageUnknown,

#ifdef ONLINE_SERVICES
    trailerAllGenres,
    trailerAll,
    trailerPostDate,
    trailerRelDate,
    trailerRoot,
    trailerUnknown,
#endif

    playlistAll,
    playlistAllDirectories,
    playlistRoot,

    Custom,
};

/// @brief Manager class for BoxLayout and BoxChain
class BoxLayoutList : public EditHelperBoxLayout, public EditHelperBoxChain {
public:
    std::shared_ptr<BoxLayout> getKey(BoxKeys key) const;
};

/// @brief Define properties of a tree entry in virtual layout
class BoxLayout : public Editable {
    static const std::map<BoxKeys, std::string_view> boxKeys;

public:
    static std::string getBoxKey(BoxKeys bkey);
    static BoxKeys getBoxKey(const std::string& key);

    BoxLayout() = default;
    ~BoxLayout() override = default;
    explicit BoxLayout(
        BoxKeys bkey,
        std::string title,
        std::string objClass = UPNP_CLASS_CONTAINER,
        std::string upnpShortcut = "",
        std::string sortKey = "",
        bool enabled = true,
        bool searchable = true,
        int size = 1)
        : key(BoxLayout::getBoxKey(bkey))
        , title(std::move(title))
        , objClass(std::move(objClass))
        , upnpShortcut(std::move(upnpShortcut))
        , sortKey(std::move(sortKey))
        , enabled(enabled)
        , searchable(searchable)
        , size(size)
    {
    }
    explicit BoxLayout(
        std::string key,
        std::string title,
        std::string objClass = UPNP_CLASS_CONTAINER,
        std::string upnpShortcut = "",
        std::string sortKey = "",
        bool enabled = true,
        bool searchable = true,
        int size = 1)
        : key(std::move(key))
        , title(std::move(title))
        , objClass(std::move(objClass))
        , upnpShortcut(std::move(upnpShortcut))
        , sortKey(std::move(sortKey))
        , enabled(enabled)
        , searchable(searchable)
        , size(size)
    {
    }

    bool equals(const std::shared_ptr<BoxLayout>& other) { return this->key == other->key; }

    /// @brief key for the box to be referenced in layouts
    void setKey(std::string key) { this->key = std::move(key); }
    std::string getKey() const { return key; }

    /// @brief title string for the box
    void setTitle(std::string title) { this->title = std::move(title); }
    std::string getTitle() const { return title; }

    /// @brief shortcut name for upnp shortcuts list
    void setUpnpShortcut(std::string upnpShortcut) { this->upnpShortcut = std::move(upnpShortcut); }
    std::string getUpnpShortcut() const { return upnpShortcut; }

    /// @brief object class to be used for new containers
    void setClass(std::string objClass) { this->objClass = std::move(objClass); }
    std::string getClass() const { return objClass; }

    /// @brief allow to disable boxes
    void setEnabled(bool enabled) { this->enabled = enabled; }
    bool getEnabled() const { return enabled; }

    /// @brief allow to search for boxes
    void setSearchable(bool searchable) { this->searchable = searchable; }
    bool getSearchable() const { return searchable; }

    /// @brief size value for the box, esp. in structured layout
    void setSize(int size) { this->size = size; }
    int getSize() const { return size; }

    /// @brief objectId of the box when existing
    void setId(int id) { this->id = id; }
    int getId() const { return id; }

    /// @brief sorting key for directory listing
    void setSortKey(std::string sortKey) { this->sortKey = std::move(sortKey); }
    std::string getSortKey() const { return sortKey; }

protected:
    /// @brief key for the box to be referenced in layouts
    std::string key;
    /// @brief title string for the box
    std::string title;
    /// @brief object class to be used for new containers
    std::string objClass;
    /// @brief shortcut name for upnp shortcuts list
    std::string upnpShortcut;
    /// @brief sorting key for directory listing
    std::string sortKey;
    /// @brief allow to disable boxes
    bool enabled { true };
    /// @brief allow to search for boxes
    bool searchable { true };
    /// @brief size value for the box, esp. in structured layout
    int size { 1 };
    /// @brief objectId of the box when existing
    int id { INVALID_OBJECT_ID };
};

/// @brief Define a tree in virtual layout
class BoxChain : public Editable {
public:
    BoxChain() = default;
    ~BoxChain() override = default;
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

    /// @brief comparison of chains
    bool equals(const std::shared_ptr<BoxChain>& other) { return this->type == other->type && this->index == other->index; }

    /// @brief links for chain
    std::vector<std::vector<std::pair<std::string, std::string>>> getLinks(bool edit = false) const { return this->links.getVectorOption(edit); }
    std::size_t getLinksSize(bool edit = false) const { return this->links.getVectorOption(edit).size(); }
    void setLinks(const std::vector<std::vector<std::pair<std::string, std::string>>>& links) { this->links = VectorOption(links); }
    void setLinkKey(std::size_t j, std::size_t k, const std::string& key) { return this->links.setKey(j, k, key); }
    void setLinkValue(std::size_t j, std::size_t k, const std::string& value) { return this->links.setValue(j, k, value); }

protected:
    /// @brief identification of chains
    std::size_t index = { 0 };
    /// @brief media type chain applies to
    AutoscanMediaMode type;
    /// @brief list of chain links making up the virtual tree
    VectorOption links;
};

#endif
