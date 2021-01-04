/*GRB*

    Gerbera - https://gerbera.io/

    directory_tweak.h - this file is part of Gerbera.

    Copyright (C) 2020-2021 Gerbera Contributors

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
class AutoscanDirectory;
class Config;
class DirectoryTweak;

class AutoScanSetting {
public:
    bool followSymlinks = false;
    bool recursive = true;
    bool hidden = false;
    bool rescanResource = true;
    std::shared_ptr<AutoscanDirectory> adir;
    std::vector<std::string> resourcePatterns;

    void mergeOptions(const std::shared_ptr<Config>& config, const fs::path& location);
};

class DirectoryConfigList {
public:
    explicit DirectoryConfigList() = default;

    /// \brief Adds a new DirectoryTweak to the list.
    ///
    /// \param dir DirectoryTweak to add to the list.
    /// \return scanID of the newly added DirectoryTweak
    void add(const std::shared_ptr<DirectoryTweak>& dir, size_t index = SIZE_MAX);

    std::shared_ptr<DirectoryTweak> get(size_t id, bool edit = false);

    std::shared_ptr<DirectoryTweak> get(const fs::path& location);

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
    void _add(const std::shared_ptr<DirectoryTweak>& dir, size_t index);
};

/// \brief Provides information about one directory.
class DirectoryTweak {
public:
    explicit DirectoryTweak()
        : location("")

    {
    }

    explicit DirectoryTweak(fs::path location, bool inherit)
        : location(std::move(location))
        , inherit(inherit)
    {
    }

    void setLocation(const fs::path& location) { this->location = location; };
    fs::path getLocation() const { return location; }

    void setInherit(bool inherit) { this->inherit = inherit; }
    bool getInherit() const { return inherit; }

    void setOrig(bool orig) { this->isOrig = orig; }
    bool getOrig() const { return isOrig; }

    void setRecursive(bool recursive) { this->flags["Recursive"] = recursive; }
    bool hasRecursive() const { return flags.find("Recursive") != flags.end(); }
    bool getRecursive() const { return flags.at("Recursive"); }

    void setHidden(bool hidden) { this->flags["Hidden"] = hidden; }
    bool hasHidden() const { return flags.find("Hidden") != flags.end(); }
    bool getHidden() const { return flags.at("Hidden"); }

    void setCaseSensitive(bool caseSensitive) { this->flags["CaseSens"] = caseSensitive; }
    bool hasCaseSensitive() const { return flags.find("CaseSens") != flags.end(); }
    bool getCaseSensitive() const { return flags.at("CaseSens"); }

    void setFollowSymlinks(bool followSymlinks) { this->flags["FollowSymlinks"] = followSymlinks; }
    bool hasFollowSymlinks() const { return flags.find("FollowSymlinks") != flags.end(); }
    bool getFollowSymlinks() const { return flags.at("FollowSymlinks"); }

    void setMetaCharset(const std::string& metaCharset) { this->resourceFiles["MetaCharset"] = metaCharset; }
    bool hasMetaCharset() const { return resourceFiles.find("MetaCharset") != resourceFiles.end(); }
    std::string getMetaCharset() const { return resourceFiles.at("MetaCharset"); }

    void setFanArtFile(const std::string& fanArtFile) { this->resourceFiles["FanArt"] = fanArtFile; }
    bool hasFanArtFile() const { return resourceFiles.find("FanArt") != resourceFiles.end(); }
    std::string getFanArtFile() const { return resourceFiles.at("FanArt"); }

    void setSubTitleFile(const std::string& subTitleFile) { this->resourceFiles["SubTitle"] = subTitleFile; }
    bool hasSubTitleFile() const { return resourceFiles.find("SubTitle") != resourceFiles.end(); }
    std::string getSubTitleFile() const { return resourceFiles.at("SubTitle"); }

    void setResourceFile(const std::string& resourceFile) { this->resourceFiles["Resource"] = resourceFile; }
    bool hasResourceFile() const { return resourceFiles.find("Resource") != resourceFiles.end(); }
    std::string getResourceFile() const { return resourceFiles.at("Resource"); }

protected:
    fs::path location;
    bool isOrig { false };
    bool inherit { true };
    std::map<std::string, std::string> resourceFiles;
    std::map<std::string, bool> flags;
};

#endif
