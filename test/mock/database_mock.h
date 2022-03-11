#ifndef __DATABASE_MOCK_H__
#define __DATABASE_MOCK_H__

#include <gtest/gtest.h>

#include "config/config.h"
#include "config/config_setup.h"
#include "database/database.h"
#include "util/upnp_clients.h"

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
    std::shared_ptr<CdsObject> findObjectByPath(const fs::path& path, bool wasRegularFile = false) override { return {}; }
    int findObjectIDByPath(const fs::path& fullpath, bool wasRegularFile = false) override { return INVALID_OBJECT_ID; }
    std::string incrementUpdateIDs(const std::unordered_set<int>& ids) override { return {}; }

    std::shared_ptr<CdsObject> loadObject(int objectID) override { return nullptr; }
    std::shared_ptr<CdsObject> loadObject(const std::string& group, int objectID) override { return nullptr; }
    int getChildCount(int contId, bool containers = true, bool items = true, bool hideFsRoot = false) override { return 0; }
    std::map<int, int> getChildCounts(const std::vector<int>& contId, bool containers, bool items, bool hideFsRoot) override { return {}; }

    std::unique_ptr<ChangedContainers> removeObject(int objectID, bool all) override { return {}; }
    std::unordered_set<int> getObjects(int parentID, bool withoutContainer) override { return {}; }
    std::unique_ptr<ChangedContainers> removeObjects(const std::unordered_set<int>& list, bool all = false) override { return {}; }

    std::shared_ptr<CdsObject> loadObjectByServiceID(const std::string& serviceID) override { return {}; }
    std::vector<int> getServiceObjectIDs(char servicePrefix) override { return {}; }

    int getTotalFiles(bool isVirtual = false, const std::string& mimeType = "", const std::string& upnpClass = "") override { return 0; }

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

    std::shared_ptr<AutoscanList> getAutoscanList(ScanMode scanode) override { return {}; }
    void updateAutoscanList(ScanMode scanmode, const std::shared_ptr<AutoscanList>& list) override { }

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
