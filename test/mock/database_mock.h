#ifndef __DATABASE_MOCK_H__
#define __DATABASE_MOCK_H__

#include <gtest/gtest.h>

#include "config/config.h"
#include "config/config_setup.h"
#include "database/database.h"

using namespace ::testing;

class DatabaseMock : public Database {
public:
    DatabaseMock(std::shared_ptr<Config> config)
        : Database(config)
    {
    }

    void init() override { }
    void shutdown() override { }

    void addObject(std::shared_ptr<CdsObject> object, int* changedContainer) override { }
    void addContainerChain(std::string path, const std::string& lastClass, int lastRefID, int* containerID,
        std::vector<int>& updateID, const std::map<std::string, std::string>& lastMetadata) override { }
    fs::path buildContainerPath(int parentID, const std::string& title) override { return ""; }

    void updateObject(std::shared_ptr<CdsObject> object, int* changedContainer) override { }

    std::vector<std::shared_ptr<CdsObject>> browse(const std::unique_ptr<BrowseParam>& param) override { return std::vector<std::shared_ptr<CdsObject>>(); }
    std::vector<std::shared_ptr<CdsObject>> search(const std::unique_ptr<SearchParam>& param, int* numMatches) override { return std::vector<std::shared_ptr<CdsObject>>(); }

    std::vector<std::string> getMimeTypes() override { return std::vector<std::string>(); }
    std::shared_ptr<CdsObject> findObjectByPath(fs::path path, bool wasRegularFile = false) override { return nullptr; }
    int findObjectIDByPath(fs::path fullpath, bool wasRegularFile = false) override { return INVALID_OBJECT_ID; }
    std::string incrementUpdateIDs(const std::unique_ptr<std::unordered_set<int>>& ids) override { return ""; }

    std::shared_ptr<CdsObject> loadObject(int objectID) override { return nullptr; }
    int getChildCount(int contId, bool containers = true, bool items = true, bool hideFsRoot = false) override { return 0; }

    std::unique_ptr<ChangedContainers> removeObject(int objectID, bool all) override { return nullptr; }
    std::unique_ptr<std::unordered_set<int>> getObjects(int parentID, bool withoutContainer) override { return nullptr; }
    std::unique_ptr<ChangedContainers> removeObjects(const std::unique_ptr<std::unordered_set<int>>& list, bool all = false) override { return nullptr; }

    std::shared_ptr<CdsObject> loadObjectByServiceID(const std::string& serviceID) override { return nullptr; }
    std::unique_ptr<std::vector<int>> getServiceObjectIDs(char servicePrefix) override { return nullptr; }

    int getTotalFiles(bool isVirtual = false, const std::string& mimeType = "", const std::string& upnpClass = "") override { return 0; }

    std::string getInternalSetting(const std::string& key) override { return ""; }
    void storeInternalSetting(const std::string& key, const std::string& value) override { }

    std::vector<ConfigValue> getConfigValues() override
    {
        std::vector<ConfigValue> result;
        return result;
    }
    void removeConfigValue(const std::string& item) override { }
    void updateConfigValue(const std::string& key, const std::string& item, const std::string& value, const std::string& status = "unchanged") override { }

    std::shared_ptr<AutoscanList> getAutoscanList(ScanMode scanode) override { return nullptr; }
    void updateAutoscanList(ScanMode scanmode, std::shared_ptr<AutoscanList> list) override { }

    std::shared_ptr<AutoscanDirectory> getAutoscanDirectory(int objectID) override { return nullptr; }
    void addAutoscanDirectory(std::shared_ptr<AutoscanDirectory> adir) override { }
    void updateAutoscanDirectory(std::shared_ptr<AutoscanDirectory> adir) override { }
    void removeAutoscanDirectory(std::shared_ptr<AutoscanDirectory> adir) override { }
    void checkOverlappingAutoscans(std::shared_ptr<AutoscanDirectory> adir) override { }

    std::unique_ptr<std::vector<int>> getPathIDs(int objectID) override { return nullptr; }
    int ensurePathExistence(fs::path path, int* changedContainer) override { return 0; }

    void clearFlagInDB(int flag) override { }
    std::string getFsRootName() override { return ""; }

    void threadCleanup() override { }
    bool threadCleanupRequired() const override { return false; }
    void doMetadataMigration() override { }

protected:
    std::shared_ptr<Database> getSelf() override { return nullptr; }
};

#endif // __DATABASE_MOCK_H__
