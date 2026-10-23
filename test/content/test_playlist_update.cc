/*GRB*
    Gerbera - https://gerbera.io/

    Copyright (C) 2026 Gerbera Contributors

    Gerbera is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License version 2
    as published by the Free Software Foundation.

    Gerbera is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with Gerbera.  If not, see <http://www.gnu.org/licenses/>.
*/

#include "../mock/config_mock.h"
#include "../mock/database_mock.h"
#include "cds/cds_container.h"
#include "cds/cds_item.h"
#include "config/config_option_enum.h"
#include "config/config_setup.h"
#include "config/result/autoscan.h"
#include "content/content_manager.h"
#include "content/layout/layout.h"
#include "content/update_manager.h"
#include "context.h"
#include "upnp/clients.h"
#include "web/session_manager.h"

#ifdef ONLINE_SERVICES
#include "content/onlineservice/online_service.h"
#endif

#ifdef HAVE_INOTIFY
#include "content/inotify/autoscan_inotify.h"
#include "content/inotify/directory_watch.h"
#include "content/inotify/inotify_handler.h"
#ifdef HAVE_JS
#include "content/inotify/scripting_inotify.h"
#endif
#include <sys/inotify.h>
#endif

#include <gtest/gtest.h>

class PlaylistUpdateConfig : public ConfigMock {
public:
    std::string getOption(ConfigVal) const override { return {}; }
    std::shared_ptr<ConfigOption> getConfigOption(ConfigVal option) const override
    {
#ifdef HAVE_JS
        if (option == ConfigVal::IMPORT_SCRIPTING_SCAN_MODE)
            return std::make_shared<EnumOption<AutoscanScanMode>>(AutoscanScanMode::Manual, "manual");
#endif
        return ConfigMock::getConfigOption(option);
    }
};

class PlaylistUpdateDatabase : public DatabaseMock {
public:
    using DatabaseMock::DatabaseMock;
    std::map<int, std::shared_ptr<CdsObject>> objects;
    std::vector<int> removed;
    std::shared_ptr<CdsObject> source;

    std::shared_ptr<CdsObject> loadObject(int id, const std::string&) override { return objects.at(id); }
    std::shared_ptr<CdsObject> findObjectByPath(const fs::path&, const std::string&, DbFileType) override { return source; }
    std::unique_ptr<ChangedContainers> removeObject(int id, const fs::path&, bool) override
    {
        removed.push_back(id);
        objects.erase(id);
        return {};
    }
};

class PlaylistUpdateContent : public ContentManager {
public:
    explicit PlaylistUpdateContent(const std::shared_ptr<Context>& context)
        : ContentManager(context, nullptr, nullptr)
    {
    }
    std::vector<int> removed;
    std::vector<int> removeObject(const std::shared_ptr<AutoscanDirectory>&, const std::shared_ptr<CdsObject>& obj, const fs::path&, bool, bool, bool) override
    {
        removed.push_back(obj->getID());
        return {};
    }
};

class PlaylistUpdateLayout : public Layout {
public:
    using Layout::cleanUp;
    using Layout::Layout;

protected:
    std::vector<int> addAudio(const std::shared_ptr<CdsObject>&, const std::shared_ptr<CdsContainer>&, const fs::path&, const std::map<AutoscanMediaMode, std::string>&) override { return {}; }
    std::vector<int> addVideo(const std::shared_ptr<CdsObject>&, const std::shared_ptr<CdsContainer>&, const fs::path&, const std::map<AutoscanMediaMode, std::string>&) override { return {}; }
    std::vector<int> addImage(const std::shared_ptr<CdsObject>&, const std::shared_ptr<CdsContainer>&, const fs::path&, const std::map<AutoscanMediaMode, std::string>&) override { return {}; }
#ifdef ONLINE_SERVICES
    std::vector<int> addOnlineItem(const std::shared_ptr<CdsObject>&, OnlineServiceType, const fs::path&, const std::map<AutoscanMediaMode, std::string>&) override { return {}; }
#endif
};

class PlaylistUpdateTest : public ::testing::Test {
protected:
    void SetUp() override
    {
        config = std::make_shared<PlaylistUpdateConfig>();
        database = std::make_shared<PlaylistUpdateDatabase>(config);
        auto context = std::make_shared<Context>(nullptr, config, nullptr, nullptr, database, nullptr, nullptr);
        content = std::make_shared<PlaylistUpdateContent>(context);
    }
    std::shared_ptr<PlaylistUpdateConfig> config;
    std::shared_ptr<PlaylistUpdateDatabase> database;
    std::shared_ptr<PlaylistUpdateContent> content;
};

TEST_F(PlaylistUpdateTest, LayoutCleanupPreservesPlaylistPositionsAndRemovesStaleAlbumEntries)
{
    database->objects[10] = std::make_shared<CdsContainer>(CdsEntryType::VirtualContainer);
    database->objects[20] = std::make_shared<CdsContainer>(CdsEntryType::VirtualContainer);
    database->objects[20]->setClass(UPNP_CLASS_PLAYLIST_CONTAINER);
    for (int id : { 100, 101, 102, 103 }) {
        auto item = std::make_shared<CdsItem>(CdsEntryType::VirtualItem);
        item->setID(id);
        item->setRefID(1);
        item->setParentID(id == 101 || id == 102 ? 20 : 10);
        item->setTrackNumber(id == 101 ? 1 : 3);
        database->objects[id] = item;
    }
    PlaylistUpdateLayout layout(content);
    layout.cleanUp({ 100, 101, 102, 103 }, { 103 });
    EXPECT_EQ(database->removed, (std::vector<int> { 100 }));
    EXPECT_EQ(std::static_pointer_cast<CdsItem>(database->objects.at(101))->getTrackNumber(), 1);
    EXPECT_EQ(std::static_pointer_cast<CdsItem>(database->objects.at(102))->getTrackNumber(), 3);
}

class PlaybackMetadataDatabase : public PlaylistUpdateDatabase {
public:
    using PlaylistUpdateDatabase::PlaylistUpdateDatabase;
    int mediaUpdates {};
    std::shared_ptr<ClientStatusDetail> played;

    void updateObject(const std::shared_ptr<CdsObject>& object, int* changedContainer) override
    {
        ++mediaUpdates;
        auto saved = CdsObject::createObject(object->getEntryType());
        object->copyTo(saved);
        objects[object->getID()] = saved;
        *changedContainer = object->getParentID();
    }

    void savePlayStatus(const std::shared_ptr<ClientStatusDetail>& detail) override
    {
        played = detail->clone();
    }
};

class PlaybackMetadataContent : public PlaylistUpdateContent {
public:
    explicit PlaybackMetadataContent(const std::shared_ptr<Context>& context)
        : PlaylistUpdateContent(context)
    {
        update_manager->run();
    }

    ~PlaybackMetadataContent() override { update_manager->shutdown(); }
};

TEST(PlaybackMetadataTest, PlaybackDoesNotOverwriteNewerPlaylistMetadata)
{
    auto config = std::make_shared<PlaylistUpdateConfig>();
    auto database = std::make_shared<PlaybackMetadataDatabase>(config);
    auto sessions = std::make_shared<Web::SessionManager>(config, nullptr);
    auto context = std::make_shared<Context>(nullptr, config, nullptr, nullptr, database, sessions, nullptr);
    PlaybackMetadataContent content(context);

    auto requested = std::make_shared<CdsItem>(CdsEntryType::VirtualItem);
    requested->setID(42);
    requested->setRefID(41);
    requested->setParentID(20);
    requested->setTitle("Before playlist refresh");
    requested->setClass(UPNP_CLASS_MUSIC_TRACK);
    requested->setMimeType("audio/x-wav");
    requested->setLocation(__FILE__, CdsEntryType::VirtualItem);
    requested->setPlayStatus(std::make_shared<ClientStatusDetail>("default", 42, 7, 1, 1234, 5678));

    // The request already holds its snapshot when a playlist refresh finishes.
    auto refreshed = std::make_shared<CdsItem>(CdsEntryType::VirtualItem);
    requested->copyTo(refreshed);
    refreshed->setTitle("After playlist refresh");
    refreshed->addMetaData(MetadataFields::M_DESCRIPTION, "Entry description");
    refreshed->addMetaData(MetadataFields::M_CONTENT_CLASS, UPNP_CLASS_PLAYLIST_ITEM);
    database->objects[42] = refreshed;

    content.triggerPlayHook("default", requested);
    content.triggerPlayHook("default", requested);

    EXPECT_EQ(database->mediaUpdates, 0);
    EXPECT_EQ(database->objects.at(42)->getTitle(), refreshed->getTitle());
    EXPECT_EQ(database->objects.at(42)->getMetaData(), refreshed->getMetaData());
    ASSERT_NE(database->played, nullptr);
    EXPECT_EQ(database->played->getPlayCount(), 9);
    EXPECT_EQ(database->played->getLastPlayedPosition().count(), 1234);
    EXPECT_EQ(database->played->getBookMarkPosition().count(), 5678);
}

#ifdef HAVE_INOTIFY
TEST_F(PlaylistUpdateTest, WritableCloseKeepsMediaIdentityButDeletionAndPlaylistReplacementStillRemove)
{
    auto source = std::make_shared<CdsItem>(CdsEntryType::File);
    source->setID(42);
    database->source = source;
    auto watch = std::make_shared<DirectoryWatch>("/fixture/file", 1, 0);
    for (auto mode : { ImportMode::MediaTomb, ImportMode::Gerbera }) {
        for (const auto& cls : { UPNP_CLASS_AUDIO_ITEM, UPNP_CLASS_VIDEO_ITEM, UPNP_CLASS_IMAGE_ITEM }) {
            for (auto mask : { IN_CLOSE_WRITE, IN_MOVED_TO }) {
                source->setClass(cls);
                inotify_event event {};
                event.wd = 1;
                event.mask = mask;
                InotifyHandler handler(nullptr, &event, event.mask);
                handler.getPath(watch);
                bool isDir = false;
                handler.doExistingEntry(database, content, watch, mode, isDir);
                EXPECT_TRUE(content->removed.empty());
            }
        }
    }
    for (auto eventMask : { IN_CLOSE_WRITE, IN_DELETE, IN_MOVED_FROM }) {
        source->setClass(eventMask == IN_CLOSE_WRITE ? "object.item.playlistItem" : UPNP_CLASS_AUDIO_ITEM);
        inotify_event event {};
        event.wd = 1;
        event.mask = eventMask;
        InotifyHandler handler(nullptr, &event, event.mask);
        handler.getPath(watch);
        bool isDir = false;
        handler.doExistingEntry(database, content, watch, ImportMode::MediaTomb, isDir);
        ASSERT_EQ(content->removed, (std::vector<int> { 42 }));
        content->removed.clear();
    }
}
#endif
