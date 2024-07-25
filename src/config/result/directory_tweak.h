/*GRB*

    Gerbera - https://gerbera.io/

    directory_tweak.h - this file is part of Gerbera.

    Copyright (C) 2020-2024 Gerbera Contributors

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
/// \brief Definitions of the DirectoryTweak class.

#ifndef __DIRECTORYTWEAK_H__
#define __DIRECTORYTWEAK_H__

#include <map>
#include <mutex>
#include <vector>

#include "util/grb_fs.h"

// forward declaration
class AutoscanDirectory;
class Config;
class DirectoryTweak;

class DirectoryConfigList {
public:
    /// \brief Adds a new DirectoryTweak to the list.
    ///
    /// \param dir DirectoryTweak to add to the list.
    /// \param index position of new entry
    /// \return scanID of the newly added DirectoryTweak
    void add(const std::shared_ptr<DirectoryTweak>& dir, std::size_t index = std::numeric_limits<std::size_t>::max());

    std::shared_ptr<DirectoryTweak> get(std::size_t id, bool edit = false) const;

    std::shared_ptr<DirectoryTweak> get(const fs::path& location) const;

    std::size_t getEditSize() const;

    std::size_t size() const { return list.size(); }

    /// \brief removes the DirectoryTweak given by its ID
    void remove(std::size_t id, bool edit = false);

    /// \brief returns a copy of the directory config list in the form of an array
    std::vector<std::shared_ptr<DirectoryTweak>> getArrayCopy() const;

protected:
    std::size_t origSize {};
    std::map<std::size_t, std::shared_ptr<DirectoryTweak>> indexMap;

    mutable std::recursive_mutex mutex;
    using AutoLock = std::scoped_lock<std::recursive_mutex>;

    std::vector<std::shared_ptr<DirectoryTweak>> list;
    void _add(const std::shared_ptr<DirectoryTweak>& dir, std::size_t index);
};

#define SETTING_FANART "FanArt"
#define SETTING_CONTAINERART "ContainerArt"
#define SETTING_SUBTITLE "SubTitle"
#define SETTING_METAFILE "MetaFile"
#define SETTING_RESOURCE "Resource"

/// \brief Provides information about one directory.
class DirectoryTweak {
public:
    DirectoryTweak() = default;
    explicit DirectoryTweak(fs::path location, bool inherit)
        : location(std::move(location))
        , inherit(inherit)
    {
    }

    void setLocation(fs::path location) { this->location = std::move(location); }
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

    void setMetaCharset(std::string metaCharset) { this->resourceFiles["MetaCharset"] = std::move(metaCharset); }
    bool hasMetaCharset() const { return resourceFiles.find("MetaCharset") != resourceFiles.end(); }
    std::string getMetaCharset() const { return resourceFiles.at("MetaCharset"); }

    void setFanArtFile(std::string file) { this->resourceFiles[SETTING_FANART] = std::move(file); }
    bool hasFanArtFile() const { return resourceFiles.find(SETTING_FANART) != resourceFiles.end(); }
    std::string getFanArtFile() const { return resourceFiles.at(SETTING_FANART); }

    void setSubTitleFile(std::string file) { this->resourceFiles[SETTING_SUBTITLE] = std::move(file); }
    bool hasSubTitleFile() const { return resourceFiles.find(SETTING_SUBTITLE) != resourceFiles.end(); }
    std::string getSubTitleFile() const { return resourceFiles.at(SETTING_SUBTITLE); }

    void setMetafile(std::string file) { this->resourceFiles[SETTING_METAFILE] = std::move(file); }
    bool hasMetafile() const { return resourceFiles.find(SETTING_METAFILE) != resourceFiles.end(); }
    std::string getMetafile() const { return resourceFiles.at(SETTING_METAFILE); }

    void setResourceFile(std::string file) { this->resourceFiles[SETTING_RESOURCE] = std::move(file); }
    bool hasResourceFile() const { return resourceFiles.find(SETTING_RESOURCE) != resourceFiles.end(); }
    std::string getResourceFile() const { return resourceFiles.at(SETTING_RESOURCE); }

    bool hasSetting(const std::string& setting) const { return resourceFiles.find(setting) != resourceFiles.end(); }
    std::string getSetting(const std::string& setting) const { return resourceFiles.at(setting); }

protected:
    fs::path location;
    bool isOrig {};
    bool inherit { true };
    std::map<std::string, std::string> resourceFiles;
    std::map<std::string, bool> flags;
};

#endif
