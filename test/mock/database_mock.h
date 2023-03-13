/*GRB*
    Gerbera - https://gerbera.io/

    database_mock.h - this file is part of Gerbera.

    Copyright (C) 2016-2023 Gerbera Contributors

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
#ifndef __DATABASE_MOCK_H__
#define __DATABASE_MOCK_H__

#include <gtest/gtest.h>

#include "database/database.h"

class DatabaseMock : public Database {
public:
    DatabaseMock(std::shared_ptr<Config> config)
        : Database(std::move(config))
    {
    }

    void init() override { }
    void shutdown() override { }

    void addObject(const std::shared_ptr<CdsObject>& object, int* changedContainer) override { }
    bool addContainer(int parentContainerId, std::string virtualPath, const std::shared_ptr<CdsContainer>& cont, int* containerID) override { return true; }
    fs::path buildContainerPath(int parentID, const std::string& title) override { return {}; }

    void updateObject(const std::shared_ptr<CdsObject>& object, int* changedContainer) override { }

    std::vector<std::shared_ptr<CdsObject>> browse(BrowseParam& param) override { return {}; }
    std::vector<std::shared_ptr<CdsObject>> search(const SearchParam& param, int* numMatches) override { return {}; }

    std::vector<std::string> getMimeTypes() override { return {}; }
    std::vector<std::shared_ptr<CdsObject>> findObjectByContentClass(const std::string& contentClass) override { return {}; }
    std::shared_ptr<CdsObject> findObjectByPath(const fs::path& path, DbFileType fileType = DbFileType::Auto) override { return {}; }
    int findObjectIDByPath(const fs::path& fullpath, DbFileType fileType = DbFileType::Auto) override { return INVALID_OBJECT_ID; }
    std::string incrementUpdateIDs(const std::unordered_set<int>& ids) override { return {}; }

    std::shared_ptr<CdsObject> loadObject(int objectID) override { return nullptr; }
    std::shared_ptr<CdsObject> loadObject(const std::string& group, int objectID) override { return nullptr; }
    int getChildCount(int contId, bool containers = true, bool items = true, bool hideFsRoot = false) override { return 0; }
    std::map<int, int> getChildCounts(const std::vector<int>& contId, bool containers, bool items, bool hideFsRoot) override { return {}; }

    std::unique_ptr<ChangedContainers> removeObject(int objectID, bool all) override { return {}; }
    std::unordered_set<int> getObjects(int parentID, bool withoutContainer) override { return {}; }
    std::vector<std::pair<int, std::chrono::seconds>> getRefObjects(int objectId) override { return {}; }
    std::unique_ptr<ChangedContainers> removeObjects(const std::unordered_set<int>& list, bool all = false) override { return {}; }

    std::shared_ptr<CdsObject> loadObjectByServiceID(const std::string& serviceID) override { return {}; }
    std::vector<int> getServiceObjectIDs(char servicePrefix) override { return {}; }

    long long getFileStats(const StatsParam& stats) override { return 0; }
    std::map<std::string, long long> getGroupStats(const StatsParam& stats) override { return {}; };

    std::string getInternalSetting(const std::string& key) override { return {}; }
    void storeInternalSetting(const std::string& key, const std::string& value) override { }

    std::vector<ConfigValue> getConfigValues() override { return {}; }
    void removeConfigValue(const std::string& item) override { }
    void updateConfigValue(const std::string& key, const std::string& item, const std::string& value, const std::string& status = "unchanged") override { }

    /* clients methods */
    std::vector<ClientCacheEntry> getClients() override { return {}; }
    void saveClients(const std::vector<ClientCacheEntry>& cache) override { }
    std::shared_ptr<ClientStatusDetail> getPlayStatus(const std::string& group, int objectId) override { return {}; };
    void savePlayStatus(const std::shared_ptr<ClientStatusDetail>& detail) override { };
    std::vector<std::shared_ptr<ClientStatusDetail>> getPlayStatusList(int objectId) override { return {}; }
    std::vector<std::map<std::string, std::string>> getClientGroupStats() override { return {}; }

    std::shared_ptr<AutoscanList> getAutoscanList(AutoscanScanMode scanode) override { return {}; }
    void updateAutoscanList(AutoscanScanMode scanmode, const std::shared_ptr<AutoscanList>& list) override { }

    std::shared_ptr<AutoscanDirectory> getAutoscanDirectory(int objectID) override { return {}; }
    void addAutoscanDirectory(const std::shared_ptr<AutoscanDirectory>& adir) override { }
    void updateAutoscanDirectory(const std::shared_ptr<AutoscanDirectory>& adir) override { }
    void removeAutoscanDirectory(const std::shared_ptr<AutoscanDirectory>& adir) override { }
    void checkOverlappingAutoscans(const std::shared_ptr<AutoscanDirectory>& adir) override { }

    std::vector<int> getPathIDs(int objectID) override { return {}; }
    int ensurePathExistence(const fs::path& path, int* changedContainer) override { return 0; }

    void threadCleanup() override { }
    bool threadCleanupRequired() const override { return false; }

protected:
    std::shared_ptr<Database> getSelf() override { return {}; }
};

#endif // __DATABASE_MOCK_H__
