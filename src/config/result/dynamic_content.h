/*GRB*

    Gerbera - https://gerbera.io/

    dynamic_content.h - this file is part of Gerbera.

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

/// @file config/result/dynamic_content.h
/// @brief Definitions of the DynamicContent class.

#ifndef __DYNAMICCONTENT_H__
#define __DYNAMICCONTENT_H__

#include "edit_helper.h"
#include "util/grb_fs.h"

// forward declarations
class Config;
class DynamicContent;
using EditHelperDynamicContent = EditHelper<DynamicContent>;

class DynamicContentList : public EditHelperDynamicContent {
public:
    std::shared_ptr<DynamicContent> getKey(const fs::path& location) const;
};

/// @brief dynamic content reader
class DynamicContent : public Editable {
public:
    DynamicContent() = default;
    explicit DynamicContent(fs::path location)
        : location(std::move(location))
    {
        if (this->location.empty())
            this->location = "/Auto";
    }
    bool equals(const std::shared_ptr<DynamicContent>& other) { return this->location == other->location; }

    void setLocation(const fs::path& location)
    {
        if (!location.empty())
            this->location = location;
    }
    fs::path getLocation() const { return location; }

    void setFilter(std::string filter) { this->filter = std::move(filter); }
    std::string getFilter() const { return filter; }

    void setSort(std::string sort) { this->sort = std::move(sort); }
    std::string getSort() const { return sort; }

    void setImage(fs::path image) { this->image = std::move(image); }
    fs::path getImage() const { return image; }

    void setTitle(std::string title) { this->title = std::move(title); }
    std::string getTitle() const { return title; }

    void setUpnpShortcut(std::string upnpShortcut) { this->upnpShortcut = std::move(upnpShortcut); }
    std::string getUpnpShortcut() const { return upnpShortcut; }

    void setMaxCount(int maxCount) { this->maxCount = maxCount; }
    int getMaxCount() const { return maxCount; }

private:
    /// @brief virtual tree location
    fs::path location { "/Auto" };

    /// @brief title
    std::string title;

    /// @brief search filter
    std::string filter;

    /// @brief sort criteria
    std::string sort;

    /// @brief shortcut name for upnp shortcuts list
    std::string upnpShortcut;

    /// @brief maximum request count
    int maxCount {};

    /// @brief folder image
    fs::path image;
};

#endif
