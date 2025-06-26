/*GRB*

    Gerbera - https://gerbera.io/

    inotify_handler.h - this file is part of Gerbera.

    Copyright (C) 2024-2025 Gerbera Contributors

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

/// \file inotify_handler.h

#ifndef __INOTIFY_HANDLER_H__
#define __INOTIFY_HANDLER_H__

#ifdef HAVE_INOTIFY

#include "inotify_types.h"
#include "util/grb_fs.h"

class AutoscanInotify;
class AutoScanSetting;
class AutoscanDirectory;
class CdsObject;
class Content;
class Database;
class DirectoryWatch;
class WatchAutoscan;
enum class ImportMode;

class InotifyHandler {
public:
    InotifyHandler(AutoscanInotify* ai, struct inotify_event* event, InotifyFlags maskedEvent);

    static std::string mapFlags(InotifyFlags flags);
    static InotifyFlags makeFlags(const std::string& optValue);
    static InotifyFlags remapFlag(const std::string& flag);

    /// @brief Get File Path for new event
    fs::path getPath(const std::shared_ptr<DirectoryWatch>& wdObj);

    /// @brief Get Autoscan DIrectory for new event
    std::pair<bool, std::shared_ptr<AutoscanDirectory>> getAutoscanDirectory(const std::shared_ptr<DirectoryWatch>& wdObj);

    /// @brief Handle file or folder being moved
    void doMove(const std::shared_ptr<DirectoryWatch>& wdObj) const;

    /// @brief Handle directory
    void doDirectory(
        AutoScanSetting& asSetting,
        const std::shared_ptr<Content>& content,
        const std::shared_ptr<DirectoryWatch>& wdObj) const;

    /// @brief Handle existing filesystem entry
    int doExistingEntry(
        const std::shared_ptr<Database>& database,
        const std::shared_ptr<Content>& content,
        const std::shared_ptr<DirectoryWatch>& wdObj,
        ImportMode importMode,
        bool& isDir);

    /// @brief Handle new filesystem entry
    void doNewEntry(
        AutoScanSetting& asSetting,
        const std::shared_ptr<Content>& content,
        bool isDir) const;

    /// @brief handle ignoring filesystem entries
    void doIgnored() const;

    /// @brief Check whether event is actively monitored
    bool hasEvent() const { return mask; }

    /// @brief Get Watch Descriptor for current event
    int getWd() const { return wd; }

    /// @brief Get File Path for current event
    fs::path getPath() const { return path; }

private:
    AutoscanInotify* ai;
    int wd;
    InotifyFlags mask;
    std::string name;
    std::shared_ptr<AutoscanDirectory> adir;
    std::shared_ptr<CdsObject> changedObject;
    fs::directory_entry dirEnt;
    fs::path path;
};

#endif
#endif
