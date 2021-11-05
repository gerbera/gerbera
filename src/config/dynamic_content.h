/*GRB*

    Gerbera - https://gerbera.io/

    dynamic_content.h - this file is part of Gerbera.

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

/// \file dynamic_content.h
///\brief Definitions of the DynamicContent class.

#ifndef __DYNAMICCONTENT_H__
#define __DYNAMICCONTENT_H__

#include <filesystem>
#include <map>
#include <mutex>
#include <vector>
namespace fs = std::filesystem;

// forward declarations
class Config;
class DynamicContent;

class DynamicContentList {
public:
    /// \brief Adds a new DynamicContent to the list.
    ///
    /// \param cont DynamicContent to add to the list.
    /// \return index of the newly added DynamicContent
    void add(const std::shared_ptr<DynamicContent>& cont, std::size_t index = std::numeric_limits<std::size_t>::max());

    std::shared_ptr<DynamicContent> get(std::size_t id, bool edit = false);

    std::shared_ptr<DynamicContent> get(const fs::path& location);

    std::size_t getEditSize() const;

    std::size_t size() const { return list.size(); }

    /// \brief removes the DynamicContent given by its ID
    void remove(std::size_t id, bool edit = false);

    /// \brief returns a copy of the directory config list in the form of an array
    std::vector<std::shared_ptr<DynamicContent>> getArrayCopy();

protected:
    std::size_t origSize {};
    std::map<std::size_t, std::shared_ptr<DynamicContent>> indexMap;

    std::recursive_mutex mutex;
    using AutoLock = std::scoped_lock<std::recursive_mutex>;

    std::vector<std::shared_ptr<DynamicContent>> list;
    void _add(const std::shared_ptr<DynamicContent>& cont, std::size_t index);
};

/// \brief dynamic content reader
class DynamicContent {
public:
    DynamicContent() = default;
    explicit DynamicContent(fs::path location)
        : location(std::move(location))
    {
        if (this->location.empty())
            this->location = "/Auto";
    }

    void setLocation(const fs::path& location)
    {
        if (!location.empty())
            this->location = location;
    }
    fs::path getLocation() const { return location; }

    void setOrig(bool orig) { this->isOrig = orig; }
    bool getOrig() const { return isOrig; }

    void setFilter(const std::string& filter) { this->filter = filter; }
    std::string getFilter() const { return filter; }

    void setSort(const std::string& sort) { this->sort = sort; }
    std::string getSort() const { return sort; }

    void setImage(const fs::path& image) { this->image = image; }
    fs::path getImage() const { return image; }

    void setTitle(const std::string& title) { this->title = title; }
    std::string getTitle() const { return title; }

private:
    /// \brief virtual tree location
    fs::path location { "/Auto" };

    /// \brief title
    std::string title;

    /// \brief search filter
    std::string filter;

    /// \brief sort criteria
    std::string sort;

    /// \brief folder image
    fs::path image;

    bool isOrig {};
};

#endif
