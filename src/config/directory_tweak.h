/*GRB*

    Gerbera - https://gerbera.io/

    directory_tweak.h - this file is part of Gerbera.

    Copyright (C) 2020 Gerbera Contributors

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

/// \file directory_tweak.h
///\brief Definitions of the DirectoryTweak class.

#ifndef __DIRECTORYTWEAK_H__
#define __DIRECTORYTWEAK_H__

#include <filesystem>
#include <map>
#include <mutex>
#include <vector>
namespace fs = std::filesystem;

// forward declaration
class DirectoryTweak;

class DirectoryConfigList {
public:
    explicit DirectoryConfigList() = default;

    /// \brief Adds a new DirectoryTweak to the list.
    ///
    /// \param dir DirectoryTweak to add to the list.
    /// \return scanID of the newly added DirectoryTweak
    void add(const std::shared_ptr<DirectoryTweak>& client, size_t index = SIZE_MAX);

    std::shared_ptr<DirectoryTweak> get(size_t id, bool edit = false);

    size_t getEditSize() const;

    size_t size() const { return list.size(); }

    /// \brief removes the DirectoryTweak given by its ID
    void remove(size_t id, bool edit = false);

    /// \brief returns a copy of the directory config list in the form of an array
    std::vector<std::shared_ptr<DirectoryTweak>> getArrayCopy();

protected:
    size_t origSize;
    std::map<size_t, std::shared_ptr<DirectoryTweak>> indexMap;

    std::recursive_mutex mutex;
    using AutoLock = std::lock_guard<std::recursive_mutex>;

    std::vector<std::shared_ptr<DirectoryTweak>> list;
    void _add(const std::shared_ptr<DirectoryTweak>& client, size_t index);
};

/// \brief Provides information about one directory.
class DirectoryTweak {
public:
    explicit DirectoryTweak()
    : location ("")
    , isOrig(false)
    , recursive(false)
    , hidden(false)
    , caseSensitive(true)
    , followSymlinks(true)
    , fanArtFile("")
    , subTitleFile("")
    , resourceFile("")
    {
    }

    explicit DirectoryTweak(const fs::path& location, bool recursive, bool hidden, bool caseSens, bool symlinks, const std::string& fanart, const std::string& subtitle, const std::string& resource)
    : location (location)
    , isOrig(false)
    , recursive(recursive)
    , hidden(hidden)
    , caseSensitive(caseSens)
    , followSymlinks(symlinks)
    , fanArtFile(fanart)
    , subTitleFile(subtitle)
    , resourceFile(resource)
    {
    }

    void setLocation(const fs::path& location) { this->location = location; };
    fs::path getLocation() const { return location; }

    void setRecursive(bool recursive) { this->recursive = recursive; }
    bool getRecursive() const { return recursive; }

    void setOrig(bool orig) { this->isOrig = orig; }
    bool getOrig() const { return isOrig; }

    void setHidden(bool hidden) { this->hidden = hidden; }
    bool getHidden() const { return hidden; }

    void setCaseSensitive(bool caseSensitive) { this->caseSensitive = caseSensitive; }
    bool getCaseSensitive() const { return caseSensitive; }

    void setFollowSymlinks(bool followSymlinks) { this->followSymlinks = followSymlinks; }
    bool getFollowSymlinks() const { return followSymlinks; }

    void setFanArtFile(const std::string& fanArtFile) { this->fanArtFile = fanArtFile; }
    std::string getFanArtFile() const { return fanArtFile; }

    void setSubTitleFile(const std::string& subTitleFile) { this->subTitleFile = subTitleFile; }
    std::string getSubTitleFile() const { return subTitleFile; }

    void setResourceFile(const std::string& resourceFile) { this->resourceFile = resourceFile; }
    std::string getResourceFile() const { return resourceFile; }

protected:
    fs::path location;
    bool isOrig;
    bool recursive;
    bool hidden;
    bool caseSensitive;
    bool followSymlinks;
    std::string fanArtFile;
    std::string subTitleFile;
    std::string resourceFile;
};

#endif
