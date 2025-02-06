/*GRB*

    Gerbera - https://gerbera.io/

    edit_helper.h - this file is part of Gerbera.

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

/// \file edit_helper.h
/// \brief Definitions of the EditHelper class.

#ifndef __GRB_EDIT_HELPER_H__
#define __GRB_EDIT_HELPER_H__

#include "util/logger.h"

#include <algorithm>
#include <map>
#include <memory>
#include <mutex>
#include <vector>

#define EDIT_CAST(tt, obj) std::dynamic_pointer_cast<tt>((obj))

/// \brief base class editable objects
class Editable {
public:
    virtual ~Editable() = default;
    /// \brief make object valid
    virtual void setValid(bool newEntry, std::size_t index) { }
    /// \brief make object invalid
    virtual void setInvalid() { }

    void setOrig(bool orig) { this->isOrig = orig; }
    bool getOrig() const { return isOrig; }

protected:
    bool isOrig {};
};

/// \brief base class for lists of editable objects
template <class Editable>
class EditHelper {
public:
    /// \brief Adds a new Editable to the list.
    ///
    /// \param editable new item.
    /// \param index position of new entry
    void add(const std::shared_ptr<Editable>& editable, std::size_t index = std::numeric_limits<std::size_t>::max());

    std::shared_ptr<Editable> get(std::size_t id, bool edit = false) const;

    std::size_t getEditSize() const;

    std::size_t size() const { return list.size(); }

    /// \brief removes the Editable given by its scan ID
    void remove(std::size_t id, bool edit = false);
    void remove(const std::shared_ptr<Editable>& editable);

    /// \brief returns a copy of the Editable list in the form of an array
    std::vector<std::shared_ptr<Editable>> getArrayCopy() const;

protected:
    mutable std::recursive_mutex mutex;
    using AutoLock = std::scoped_lock<std::recursive_mutex>;

    std::size_t origSize {};
    std::map<std::size_t, std::shared_ptr<Editable>> indexMap;
    std::vector<std::shared_ptr<Editable>> list;

    void _add(const std::shared_ptr<Editable>& editable, std::size_t index);
};

template <class Editable>
std::shared_ptr<Editable> EditHelper<Editable>::get(std::size_t id, bool edit) const
{
    AutoLock lock(mutex);
    if (!edit) {
        if (id >= list.size())
            return nullptr;

        return list.at(id);
    }
    if (indexMap.find(id) != indexMap.end()) {
        return indexMap.at(id);
    }
    return nullptr;
}

template <class Editable>
std::size_t EditHelper<Editable>::getEditSize() const
{
    if (indexMap.empty()) {
        return 0;
    }
    return std::max_element(indexMap.begin(), indexMap.end(), [](auto a, auto b) { return (a.first < b.first); })->first + 1;
}

template <class Editable>
std::vector<std::shared_ptr<Editable>> EditHelper<Editable>::getArrayCopy() const
{
    AutoLock lock(mutex);
    return list;
}

template <class Editable>
void EditHelper<Editable>::remove(const std::shared_ptr<Editable>& editable)
{
    AutoLock lock(mutex);

    auto entry = std::find_if(list.begin(), list.end(), [editable](auto&& item) { return item->equals(editable); });
    if (entry == list.end()) {
        log_debug("No such {} to remove!", typeid(Editable).name());
        return;
    }
    editable->setInvalid();
    auto index = entry - list.begin();
    list.erase(entry);
    log_debug("{} index {} removed!", typeid(Editable).name(), index);
}

template <class Editable>
void EditHelper<Editable>::remove(std::size_t id, bool edit)
{
    AutoLock lock(mutex);

    if (!edit) {
        if (id >= list.size()) {
            log_debug("No such {} ID {}!", typeid(Editable).name(), id);
            return;
        }
        auto&& editable = list.at(id);

        editable->setInvalid();
        list.erase(list.begin() + id);

        log_debug("{} ID {} removed!", typeid(Editable).name(), id);
    } else {
        if (indexMap.find(id) == indexMap.end()) {
            log_debug("No such {} index ID {}!", typeid(Editable).name(), id);
            return;
        }
        auto&& editable = indexMap.at(id);
        auto entry = std::find_if(list.begin(), list.end(), [editable](auto&& item) { return editable->equals(item); });

        editable->setInvalid();
        list.erase(entry);

        if (id >= origSize) {
            indexMap.erase(id);
        }
        log_debug("{} ID {} removed!", typeid(Editable).name(), id);
    }
}

template <class Editable>
void EditHelper<Editable>::add(const std::shared_ptr<Editable>& editable, std::size_t index)
{
    AutoLock lock(mutex);
    _add(editable, index);
}

template <class Editable>
void EditHelper<Editable>::_add(const std::shared_ptr<Editable>& editable, std::size_t index)
{
    if (std::any_of(list.begin(), list.end(), [editable](auto&& d) { return d->equals(editable); })) {
        log_error("Duplicate {} entry [{}]", typeid(Editable).name(), index);
        return;
    }
    if (index == std::numeric_limits<std::size_t>::max()) {
        index = getEditSize();
        origSize = list.size() + 1;
        editable->setOrig(true);
        editable->setValid(true, index);
    } else
        editable->setValid(false, index);

    list.push_back(editable);
    indexMap[index] = editable;
}
#endif
