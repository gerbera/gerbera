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
#include "cds/cds_resource.h"
#include "config/config_definition.h"
#include "config/config_option_enum.h"
#include "config/config_setup.h"
#include "config/result/autoscan.h"
#include "content/content_manager.h"
#include "content/layout/layout.h"
#include "context.h"
#include "exceptions.h"
#include "iohandler/io_handler.h"
#include "request_handler/file_request_handler.h"
#include "upnp/clients.h"
#include "upnp/compat.h"
#include "upnp/xml_builder.h"
#include "util/grb_fs.h"
#include "util/tools.h"

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

#include <array>
#include <cstddef>
#include <gtest/gtest.h>
#include <memory>
#include <string>
#include <system_error>
#include <vector>

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

    std::shared_ptr<CdsObject> loadObject(int id, const std::string&) override
    {
        auto object = objects.find(id);
        if (object == objects.end())
            throw ObjectNotFoundException("Removed diagnostic object " + std::to_string(id));
        return object->second;
    }
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
    std::vector<int> played;
    std::string playedGroup;
    void triggerPlayHook(const std::string& group, const std::shared_ptr<CdsObject>& object) override
    {
        played.push_back(object->getID());
        playedGroup = group;
    }
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
    void TearDown() override
    {
        if (!resourceFile.empty()) {
            std::error_code error;
            fs::remove(resourceFile, error);
        }
    }
    fs::path resourceFile;
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

TEST_F(PlaylistUpdateTest, SourceHintPreservesLivePlaylistIdentityAndSurvivesPositionRemoval)
{
    const std::string payload(1672, 'P');
    resourceFile = fs::temp_directory_path() / ("grb-playlist-http-" + generateRandomId() + ".wav");
    GrbFile(resourceFile).writeBinaryFile(reinterpret_cast<const std::byte*>(payload.data()), payload.size());
    auto physical = std::make_shared<CdsItem>(CdsEntryType::File);
    physical->setID(42);
    physical->setTitle("Physical track");
    physical->setLocation(resourceFile, CdsEntryType::File);
    physical->setMimeType("audio/x-wav");
    auto resource = std::make_shared<CdsResource>(ContentHandler::DEFAULT, ResourcePurpose::Content);
    resource->addAttribute(ResourceAttribute::PROTOCOLINFO, "http-get:*:audio/x-wav:*");
    resource->addAttribute(ResourceAttribute::SIZE, std::to_string(payload.size()));
    physical->addResource(resource);
    database->objects[42] = physical;
    auto position = std::make_shared<CdsItem>(CdsEntryType::VirtualItem);
    physical->copyTo(position);
    position->setID(64);
    position->setRefID(42);
    position->setVirtual(true);
    database->objects[64] = position;
    auto definition = std::make_shared<ConfigDefinition>();
    definition->init(definition);
    auto context = std::make_shared<Context>(definition, config, nullptr, nullptr, database, nullptr, nullptr);
    content = std::make_shared<PlaylistUpdateContent>(context);
    auto xml = std::make_shared<UpnpXMLBuilder>(context, "");
    const auto positionUrl = xml->renderResourceURL(*position, *position->getResource(ResourcePurpose::Content), {}, "diagnostic");
    EXPECT_NE(positionUrl.find("/object_id/64/"), std::string::npos);
    EXPECT_NE(positionUrl.find("/source_id/42/"), std::string::npos);
#if defined(USING_NPUPNP)
    auto info = std::make_unique<UpnpFileInfo>();
#else
    std::unique_ptr<UpnpFileInfo, decltype(&UpnpFileInfo_delete)> info(UpnpFileInfo_new(), UpnpFileInfo_delete);
#endif
    ASSERT_NE(info, nullptr);
    for (int playingId : { 64, 42 }) {
        FileRequestHandler infoHandler(content, xml, nullptr, nullptr);
        ASSERT_NO_THROW(infoHandler.getInfo(positionUrl.c_str(), info.get()));
#if defined(USING_NPUPNP)
        EXPECT_EQ(info->file_length, static_cast<off_t>(payload.size()));
#else
        EXPECT_EQ(UpnpFileInfo_get_FileLength(info.get()), static_cast<off_t>(payload.size()));
#endif
        if (playingId == 42)
            database->removeObject(64, "", false);
        FileRequestHandler openHandler(content, xml, nullptr, nullptr);
        std::unique_ptr<IOHandler> reader;
        ASSERT_NO_THROW(reader = openHandler.open(positionUrl.c_str(), nullptr, UPNP_READ));
        ASSERT_NE(reader, nullptr);
        reader->open(UPNP_READ);
        std::array<std::byte, 1672> bytes {};
        const auto count = reader->read(bytes.data(), bytes.size());
        const auto end = reader->read(bytes.data(), 1);
        reader->close();
        EXPECT_EQ(count, static_cast<grb_read_t>(payload.size()));
        EXPECT_EQ(end, GRB_READ_END);
        EXPECT_EQ(std::string(reinterpret_cast<const char*>(bytes.data()), bytes.size()), payload);
        ASSERT_FALSE(content->played.empty());
        EXPECT_EQ(content->played.back(), playingId);
        EXPECT_EQ(content->playedGroup, "diagnostic");
    }
    FileRequestHandler handler(content, xml, nullptr, nullptr);
    EXPECT_NO_THROW(handler.getInfo(positionUrl.c_str(), info.get()));
    EXPECT_EQ(content->played, (std::vector<int> { 64, 42 }));
    for (const auto* suffix : { "", "/source_id/0", "/source_id/64", "/source_id/invalid",
             "/source_id/999999999999999999999", "/source_id/42/pr_name/test", "/source_id/42/zip/1", "/source_id/42/tr/1" }) {
        const std::string rejected = std::string("/media/object_id/64/res_id/0") + suffix + "/group/diagnostic";
        EXPECT_THROW(handler.open(rejected.c_str(), nullptr, UPNP_READ), ObjectNotFoundException);
    }
    for (const auto* resId : { "1", "none", "invalid", "999999999999999999999" }) {
        const auto rejected = std::string("/media/object_id/64/res_id/") + resId + "/source_id/42/group/diagnostic";
        EXPECT_THROW(handler.open(rejected.c_str(), nullptr, UPNP_READ), ObjectNotFoundException);
    }
    for (auto attribute : { ResourceAttribute::OFFSET, ResourceAttribute::RESOURCE_FILE }) {
        auto customized = std::make_shared<CdsItem>(CdsEntryType::VirtualItem);
        position->copyTo(customized);
        customized->getResource(ResourcePurpose::Content)->addAttribute(attribute, "1");
        const auto url = xml->getFirstResourcePath(customized);
        EXPECT_EQ(url.find("/source_id/"), std::string::npos);
    }
    auto customized = std::make_shared<CdsItem>(CdsEntryType::VirtualItem);
    position->copyTo(customized);
    customized->getResource(ResourcePurpose::Content)->addParameter("custom", "value");
    EXPECT_EQ(xml->getFirstResourcePath(customized).find("/source_id/"), std::string::npos);
    customized = std::make_shared<CdsItem>(CdsEntryType::VirtualItem);
    position->copyTo(customized);
    customized->getResource(ResourcePurpose::Content)->addOption("custom", "value");
    EXPECT_EQ(xml->getFirstResourcePath(customized).find("/source_id/"), std::string::npos);
    customized = std::make_shared<CdsItem>(CdsEntryType::VirtualItem);
    position->copyTo(customized);
    customized->setMimeType("audio/other");
    EXPECT_EQ(xml->getFirstResourcePath(customized).find("/source_id/"), std::string::npos);
    customized->setMimeType(physical->getMimeType());
    customized->setLocation(resourceFile.string() + ".other", CdsEntryType::VirtualItem);
    EXPECT_EQ(xml->getFirstResourcePath(customized).find("/source_id/"), std::string::npos);
    for (auto purpose : { ResourcePurpose::Transcode, ResourcePurpose::Thumbnail }) {
        auto other = std::make_shared<CdsResource>(ContentHandler::DEFAULT, purpose);
        EXPECT_EQ(xml->renderResourceURL(*position, *other, {}, "diagnostic").find("/source_id/"), std::string::npos);
    }
    resource->addAttribute(ResourceAttribute::OFFSET, "1");
    EXPECT_THROW(handler.open(positionUrl.c_str(), nullptr, UPNP_READ), ObjectNotFoundException);
    physical->setResources(position->getResources());
    physical->setLocation(resourceFile, CdsEntryType::ExtraFile);
    EXPECT_THROW(handler.open(positionUrl.c_str(), nullptr, UPNP_READ), ObjectNotFoundException);
}
