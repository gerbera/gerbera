/*GRB*

    Gerbera - https://gerbera.io/

    autoscan_list.h - this file is part of Gerbera.

    Copyright (C) 2021 Gerbera Contributors

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

/// \file autoscan_list.h

#ifndef __AUTOSCAN_LIST_H__
#define __AUTOSCAN_LIST_H__

#include "util/timer.h"

// forward declaration
class Database;
class AutoscanDirectory;

class AutoscanList {
public:
    explicit AutoscanList(std::shared_ptr<Database> database);
    virtual ~AutoscanList() = default;

    /// \brief Adds a new AutoscanDirectory to the list.
    ///
    /// The scanID of the directory is invalidated and set to
    /// the index in the AutoscanList.
    ///
    /// \param dir AutoscanDirectory to add to the list.
    /// \return scanID of the newly added AutoscanDirectory
    int add(const std::shared_ptr<AutoscanDirectory>& dir, size_t index = std::numeric_limits<std::size_t>::max());

    [[maybe_unused]] void addList(const std::shared_ptr<AutoscanList>& list);

    std::shared_ptr<AutoscanDirectory> get(size_t id, bool edit = false);

    std::shared_ptr<AutoscanDirectory> get(const fs::path& location);

    std::shared_ptr<AutoscanDirectory> getByObjectID(int objectID);

    size_t getEditSize() const;

    size_t size() const { return list.size(); }

    /// \brief removes the AutoscanDirectory given by its scan ID
    void remove(size_t id, bool edit = false);

    /// \brief removes the AutoscanDirectory if it is a subdirectory of a given location.
    /// \param parent parent directory.
    /// \param persistent also remove persistent directories.
    /// \return AutoscanList of removed directories, where each directory object in the list is a copy and not the original reference.
    std::shared_ptr<AutoscanList> removeIfSubdir(const fs::path& parent, bool persistent = false);

    /// \brief Send notification for each directory that is stored in the list.
    ///
    /// \param sub instance of the class that will receive the notifications.
    void notifyAll(Timer::Subscriber* sub);

    /// \brief updates the last_modified data for all AutoscanDirectories.
    void updateLMinDB();

    /// \brief returns a copy of the autoscan list in the form of an array
    std::vector<std::shared_ptr<AutoscanDirectory>> getArrayCopy();

protected:
    size_t origSize { 0 };
    std::map<size_t, std::shared_ptr<AutoscanDirectory>> indexMap;

    std::shared_ptr<Database> database;

    std::recursive_mutex mutex;
    using AutoLock = std::lock_guard<std::recursive_mutex>;

    std::vector<std::shared_ptr<AutoscanDirectory>> list;
    int _add(const std::shared_ptr<AutoscanDirectory>& dir, size_t index);
};

#endif //__AUTOSCAN_LIST_H__
