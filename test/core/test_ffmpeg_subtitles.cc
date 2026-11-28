/*GRB*

    Gerbera - https://gerbera.io/

    test_ffmpeg_subtitles.cc - this file is part of Gerbera.

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

    $Id$
*/

#ifdef HAVE_FFMPEG

#include "cds/cds_item.h"
#include "cds/cds_resource.h"
#include "config/config_setup.h"
#include "config/result/directory_tweak.h"
#include "content/content_manager.h"
#include "context.h"
#include "iohandler/io_handler.h"
#include "metadata/ffmpeg_handler.h"
#include "metadata/metadata_enums.h"
#include "metadata/metadata_service.h"
#include "request_handler/file_request_handler.h"
#include "upnp/clients.h"
#include "upnp/compat.h"
#include "upnp/xml_builder.h"
#include "util/mime.h"
#include "util/string_converter.h"
#ifdef HAVE_INOTIFY
#include "content/inotify/autoscan_inotify.h"
#endif

#include "../mock/config_mock.h"
#include "../mock/database_mock.h"

#include <array>
#include <gtest/gtest.h>

extern "C" {
#include <libavformat/version.h>
}

class FfmpegSubtitleConfig final : public ConfigMock {
public:
    std::string getOption(ConfigVal) const override { return {}; }

    bool getBoolOption(ConfigVal option) const override
    {
        if (option == ConfigVal::IMPORT_LIBOPTS_FFMPEG_ENABLED || option == ConfigVal::IMPORT_LIBOPTS_FFMPEG_STREAMS_ENABLED)
            return true;
        return ConfigMock::getBoolOption(option);
    }

    std::shared_ptr<DirectoryConfigList> getDirectoryTweakOption(ConfigVal) const override
    {
        return std::make_shared<DirectoryConfigList>();
    }
};

class FfmpegSubtitleDatabase final : public DatabaseMock {
public:
    using DatabaseMock::DatabaseMock;
    std::shared_ptr<CdsObject> object;

    std::shared_ptr<CdsObject> loadObject(int, const std::string&) override { return object; }
};

/// @brief read all data of an io handler
static std::string readAll(std::unique_ptr<IOHandler> io)
{
    std::string result;
    std::array<std::byte, 1024> buffer {};

    io->open(UPNP_READ);
    while (true) {
        auto count = io->read(buffer.data(), buffer.size());
        if (count <= 0)
            break;
        result.append(reinterpret_cast<const char*>(buffer.data()), count);
    }
    io->close();

    return result;
}

TEST(FfmpegSubtitleTest, ImportsAndServesEmbeddedSubtitles)
{
    auto config = std::make_shared<FfmpegSubtitleConfig>();
    auto database = std::make_shared<FfmpegSubtitleDatabase>(config);
    auto context = std::make_shared<Context>(nullptr, config, nullptr, std::make_shared<Mime>(config), database, nullptr, std::make_shared<ConverterManager>(config));

    // Video starting at 1s with embedded SubRip and ASS tracks, see fixtures/embedded-subtitles.txt
    auto item = std::make_shared<CdsItem>(CdsEntryType::File);
    item->setID(7);
    item->setLocation(fs::absolute("fixtures/embedded-subtitles.mkv"), CdsEntryType::File);
    item->setMimeType("video/x-matroska");
    item->setClass("object.item.videoItem");
    item->addResource(std::make_shared<CdsResource>(ContentHandler::DEFAULT, ResourcePurpose::Content));

    auto handler = std::make_shared<FfmpegHandler>(context);
    std::vector<int> newIds;
    ASSERT_TRUE(handler->fillMetadata(item, newIds));

    std::vector<std::shared_ptr<CdsResource>> subtitles;
    for (auto&& resource : item->getResources()) {
        if (resource->getPurpose() == ResourcePurpose::Subtitle)
            subtitles.push_back(resource);
    }

#if (LIBAVFORMAT_VERSION_INT < AV_VERSION_INT(60, 0, 0))
    // Embedded subtitles are disabled for FFmpeg before 6.0
    EXPECT_TRUE(subtitles.empty());
#else
    // Import lists the embedded SubRip track only, as SRT without size, and skips the ASS track
    ASSERT_EQ(subtitles.size(), 1);
    auto subtitle = subtitles.front();
    EXPECT_EQ(subtitle->getHandlerType(), ContentHandler::FFMPEG);
    EXPECT_EQ(subtitle->getAttribute(ResourceAttribute::LANGUAGE), "eng");
    EXPECT_EQ(subtitle->getAttribute(ResourceAttribute::TYPE), "srt");
    EXPECT_NE(subtitle->getAttribute(ResourceAttribute::PROTOCOLINFO).find(MIME_TYPE_SRT_SUBTITLE), std::string::npos);
    EXPECT_TRUE(subtitle->getAttribute(ResourceAttribute::SIZE).empty());

    // Served as SRT relative to the video start: the cue ending before it is dropped,
    // the cue crossing it is clipped, the last cue moves 1s earlier
    const std::string expected = "1\n00:00:00,000 --> 00:00:00,500\nCrossing start\n\n"
                                 "2\n00:00:01,000 --> 00:00:01,500\nAfter start\n\n";
    auto io = handler->serveContent(item, subtitle);
    ASSERT_NE(io, nullptr);
    EXPECT_EQ(readAll(std::move(io)), expected);

    // A repeated request returns the same SRT
    io = handler->serveContent(item, subtitle);
    ASSERT_NE(io, nullptr);
    EXPECT_EQ(readAll(std::move(io)), expected);

    // A size stored by older releases is ignored, the reported length is the served SRT
    subtitle->addAttribute(ResourceAttribute::SIZE, "4711");
    database->object = item;
    auto content = std::make_shared<ContentManager>(context, nullptr, nullptr);
    auto xml = std::make_shared<UpnpXMLBuilder>(context, "");
    FileRequestHandler requestHandler(content, xml, nullptr, std::make_shared<MetadataService>(context, content));

#if defined(USING_NPUPNP)
    auto info = std::make_unique<UpnpFileInfo>();
#else
    std::unique_ptr<UpnpFileInfo, decltype(&UpnpFileInfo_delete)> info(UpnpFileInfo_new(), UpnpFileInfo_delete);
#endif
    auto url = fmt::format("/media/object_id/{}/res_id/{}", item->getID(), subtitle->getResId());
    ASSERT_NO_THROW(requestHandler.getInfo(url.c_str(), info.get()));
#if defined(USING_NPUPNP)
    EXPECT_EQ(info->file_length, static_cast<off_t>(expected.size()));
#else
    EXPECT_EQ(UpnpFileInfo_get_FileLength(info.get()), static_cast<off_t>(expected.size()));
#endif
#endif
}

#endif // HAVE_FFMPEG
