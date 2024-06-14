/*GRB*

    Gerbera - https://gerbera.io/

    inotify_handler.h - this file is part of Gerbera.

    Copyright (C) 2024 Gerbera Contributors

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
class Content;
class Database;
class DirectoryWatch;
class WatchAutoscan;
enum class ImportMode;

class InotifyHandler {
public:
    InotifyHandler(AutoscanInotify* ai, struct inotify_event* event, InotifyFlags monitoredEvents);

    static std::string mapFlags(InotifyFlags flags);
    static InotifyFlags makeFlags(const std::string& optValue);
    static InotifyFlags remapFlag(const std::string& flag);

    fs::path getPath(const std::shared_ptr<DirectoryWatch>& wdObj);
    std::pair<bool, std::shared_ptr<AutoscanDirectory>> getAutoscanDirectory(const std::shared_ptr<DirectoryWatch>& wdObj);
    void doMove(const std::shared_ptr<DirectoryWatch>& wdObj);
    void doDirectory(AutoScanSetting& asSetting, const std::shared_ptr<Content>& content, const std::shared_ptr<DirectoryWatch>& wdObj);
    int doExistingFile(const std::shared_ptr<Database>& database, const std::shared_ptr<Content>& content, const std::shared_ptr<DirectoryWatch>& wdObj, ImportMode importMode, bool isDir);
    void doNewFile(AutoScanSetting& asSetting, const std::shared_ptr<Content>& content, bool isDir);
    void doIgnored();

    bool hasEvent() const { return mask; }
    int getWd() const { return wd; }
    fs::path getPath() const { return path; }

private:
    AutoscanInotify* ai;
    int wd;
    InotifyFlags mask;
    std::string name;
    std::shared_ptr<AutoscanDirectory> adir;
    fs::directory_entry dirEnt;
    fs::path path;
};

#endif
#endif
