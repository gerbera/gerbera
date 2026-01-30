/*GRB*

    Gerbera - https://gerbera.io/

    db_param.h - this file is part of Gerbera.

    Copyright (C) 2024-2026 Gerbera Contributors

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

/// @file database/db_param.h

#ifndef __DB_PARAM_H__
#define __DB_PARAM_H__

#include <memory>
#include <string>
#include <vector>

class CdsObject;
enum class ObjectSource;

/// @brief Base class for actions performed based on UPnP requests
class ActionParam {
protected:
    std::string sortCrit;
    int startingIndex {};
    int requestedCount {};
    std::string group;
    std::vector<std::string> forbiddenDirectories;

    // output parameters
    int totalMatches {};

public:
    ActionParam(std::string sortCriteria, int startingIndex,
        int requestedCount, std::string group)
        : sortCrit(std::move(sortCriteria))
        , startingIndex(startingIndex)
        , requestedCount(requestedCount)
        , group(std::move(group))
    {
    }

    int getStartingIndex() const { return startingIndex; }
    void setStartingIndex(int startingIndex) { this->startingIndex = startingIndex; }

    int getRequestedCount() const { return requestedCount; }
    void setRequestedCount(int requestedCount) { this->requestedCount = requestedCount; }

    void setRange(int startingIndex, int requestedCount)
    {
        this->startingIndex = startingIndex;
        this->requestedCount = requestedCount;
    }

    const std::string& getSortCriteria() const { return sortCrit; }
    void setSortCriteria(const std::string& sortCrit) { this->sortCrit = sortCrit; }

    /// @brief set forbidden directories
    void setForbiddenDirectories(const std::vector<std::string>& directories) { forbiddenDirectories = directories; }
    /// @brief get list of forbidden directories
    const std::vector<std::string>& getForbiddenDirectories() const { return forbiddenDirectories; }

    const std::string& getGroup() const { return group; }
    void setGroup(const std::string& group) { this->group = group; }

    int getTotalMatches() const { return totalMatches; }
    void setTotalMatches(int totalMatches)
    {
        this->totalMatches = totalMatches;
    }
};

/// @brief Parameters for UPnP browse request
class BrowseParam : public ActionParam {
protected:
    unsigned int flags;
    std::shared_ptr<CdsObject> object;

    bool showDynamicContainers { true };
    std::vector<ObjectSource> sources;

public:
    BrowseParam(std::shared_ptr<CdsObject> object, unsigned int flags)
        : ActionParam("", 0, 0, "")
        , flags(flags)
        , object(std::move(object))
    {
    }

    unsigned int getFlag(unsigned int mask) const { return flags & mask; }
    void setFlag(unsigned int mask) { flags |= mask; }
    void clearFlag(unsigned int mask) { flags &= ~mask; }

    std::shared_ptr<CdsObject> getObject() const { return object; }

    void setDynamicContainers(bool showDynamicContainers)
    {
        this->showDynamicContainers = showDynamicContainers;
    }

    bool getDynamicContainers() const { return showDynamicContainers; }

    void addSource(ObjectSource source) { this->sources.push_back(source); }
    const std::vector<ObjectSource>& getSources() const { return sources; }
};

/// @brief Parameters for UPnP search request
class SearchParam : public ActionParam {
protected:
    std::string containerID;
    std::string searchCrit;
    bool searchableContainers;
    bool searchContainers;
    bool searchItems;

public:
    SearchParam(std::string containerID, std::string searchCriteria, const std::string& sortCriteria, int startingIndex,
        int requestedCount, bool searchableContainers, const std::string& group,
        bool searchContainers = true,
        bool searchItems = true)
        : ActionParam(sortCriteria, startingIndex, requestedCount, group)
        , containerID(std::move(containerID))
        , searchCrit(std::move(searchCriteria))
        , searchableContainers(searchableContainers)
        , searchContainers(searchContainers)
        , searchItems(searchItems)
    {
    }
    const std::string& getSearchCriteria() const { return searchCrit; }
    const std::string& getContainerID() const { return containerID; }
    bool getSearchableContainers() const { return searchableContainers; }
    bool getContainers() const { return searchContainers; }
    bool getItems() const { return searchItems; }
};

class StatsParam {
public:
    enum class StatsMode {
        Count,
        Size,
    };

protected:
    std::string mimeType;
    std::string upnpClass;
    StatsMode mode { StatsMode::Count };
    bool isVirtual { false };

public:
    StatsParam(StatsMode mode, std::string mimeType, std::string upnpClass, bool isVirtual)
        : mimeType(std::move(mimeType))
        , upnpClass(std::move(upnpClass))
        , mode(mode)
        , isVirtual(isVirtual)
    {
    }
    const std::string& getMimeType() const { return mimeType; }
    const std::string& getUpnpClass() const { return upnpClass; }
    StatsMode getMode() const { return mode; }
    bool getVirtual() const { return isVirtual; }
};

#endif
