/*GRB*

    Gerbera - https://gerbera.io/

    autoscan_list.h - this file is part of Gerbera.

    Copyright (C) 2021-2025 Gerbera Contributors

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

/// @file content/autoscan_list.h

#ifndef __AUTOSCAN_LIST_H__
#define __AUTOSCAN_LIST_H__

#include "config/result/edit_helper.h"
#include "util/timer.h"

// forward declaration
class AutoscanDirectory;
class Content;
class Context;
class Database;
class Timer;
using EditHelperAutoscanDirectory = EditHelper<AutoscanDirectory>;

#ifdef HAVE_INOTIFY
class AutoscanInotify;
#endif

class AutoscanList : public EditHelperAutoscanDirectory {
public:
    std::shared_ptr<AutoscanDirectory> getKey(const fs::path& location) const;

    std::shared_ptr<AutoscanDirectory> getByObjectID(int objectID) const;

    /// @brief removes the AutoscanDirectory if it is a subdirectory of a given location.
    /// @param parent parent directory.
    /// @param persistent also remove persistent directories.
    /// @return AutoscanList of removed directories, where each directory object in the list is a copy and not the original reference.
    std::shared_ptr<AutoscanList> removeIfSubdir(const fs::path& parent, bool persistent = false);

    /// @brief Send notification for each directory that is stored in the list.
    ///
    /// @param sub instance of the class that will receive the notifications.
    void notifyAll(Timer::Subscriber* sub);

    /// @brief updates the last_modified data for all AutoscanDirectories.
    void updateLMinDB(Database& database);

    void initTimer(
        std::shared_ptr<Content>& content,
        std::shared_ptr<Timer>& timer
#ifdef HAVE_INOTIFY
        ,
        bool doInotify, std::unique_ptr<AutoscanInotify>& inotify
#endif
    );
};

#endif //__AUTOSCAN_LIST_H__
