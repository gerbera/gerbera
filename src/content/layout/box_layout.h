/*GRB*

    Gerbera - https://gerbera.io/

    box_layout.h - this file is part of Gerbera.

    Copyright (C) 2023 Gerbera Contributors

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

#include <map>
#include <memory>
#include <mutex>
#include <vector>

#include "common.h"

// forward declaration
class BoxLayout;

class BoxLayoutList {
public:
    /// \brief Adds a new BoxLayout to the list.
    ///
    /// \param box BoxLayout to add to the list.
    /// \return scanID of the newly added BoxLayout
    void add(const std::shared_ptr<BoxLayout>& layout, std::size_t index = std::numeric_limits<std::size_t>::max());

    std::shared_ptr<BoxLayout> get(std::size_t id, bool edit = false) const;

    std::shared_ptr<BoxLayout> get(const std::string& key) const;

    std::size_t getEditSize() const;

    std::size_t size() const { return list.size(); }

    /// \brief removes the BoxLayout given by its ID
    void remove(std::size_t id, bool edit = false);

    /// \brief returns a copy of the config list in the form of an array
    std::vector<std::shared_ptr<BoxLayout>> getArrayCopy() const;

protected:
    std::size_t origSize {};
    std::map<std::size_t, std::shared_ptr<BoxLayout>> indexMap;

    mutable std::recursive_mutex mutex;
    using AutoLock = std::scoped_lock<std::recursive_mutex>;

    std::vector<std::shared_ptr<BoxLayout>> list;
    void _add(const std::shared_ptr<BoxLayout>& dir, std::size_t index);
};

/// \brief Define properties of a tree entry in virtual layout
class BoxLayout {
public:
    BoxLayout() = default;
    explicit BoxLayout(std::string key, std::string title, std::string objClass, bool enabled = true, int size = 1)
        : key(std::move(key))
        , title(std::move(title))
        , objClass(std::move(objClass))
        , enabled(enabled)
        , size(size)
    {
    }

    void setKey(std::string key) { this->key = std::move(key); }
    std::string getKey() const { return key; }

    void setOrig(bool orig) { this->isOrig = orig; }
    bool getOrig() const { return isOrig; }

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
    bool isOrig {};
    std::string key;
    std::string title;
    std::string objClass;
    bool enabled { true };
    int size { 1 };
    int id { INVALID_OBJECT_ID };
};

#endif
