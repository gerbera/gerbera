/*GRB*

    Gerbera - https://gerbera.io/

    test_ffmpeg_cach_paths.cc - this file is part of Gerbera.

    Copyright (C) 2016-2024 Gerbera Contributors

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

#include "config/config_val.h"
#include "context.h"
#include "metadata/ffmpeg_thumbnailer_handler.h"
#include "metadata/ffmpeg_handler.h"

#include "../mock/config_mock.h"

#include <gtest/gtest.h>

#ifdef HAVE_FFMPEGTHUMBNAILER

using ::testing::Return;

class FfmpegThumbnailerHandlerTest : public FfmpegThumbnailerHandler
{
    using FfmpegThumbnailerHandler::FfmpegThumbnailerHandler;

public:
    const fs::path& getCacheBasePath() const { return getThumbnailCacheBasePath(); }
    static fs::path getCachePath(const fs::path& base, const fs::path& movie) { return getThumbnailCachePath(base, movie); }
};

TEST(Thumbnailer_Cache, BaseDirFromConfig)
{
    auto ctx = std::make_shared<Context>(std::make_shared<ConfigMock>(), nullptr, nullptr, nullptr, nullptr, nullptr);
    auto cfg = std::static_pointer_cast<ConfigMock>(ctx->getConfig());
    EXPECT_CALL(*cfg, getOption(ConfigVal::SERVER_EXTOPTS_FFMPEGTHUMBNAILER_CACHE_DIR))
        .WillOnce(Return("/var/lib/cache"));
    EXPECT_EQ(FfmpegThumbnailerHandlerTest(ctx, ConfigVal::SERVER_EXTOPTS_FFMPEGTHUMBNAILER_ENABLED).getCacheBasePath(), fs::path { "/var/lib/cache" });
}

TEST(Thumbnailer_Cache, BaseDirDefaultFromUserHome)
{
    auto ctx = std::make_shared<Context>(std::make_shared<ConfigMock>(), nullptr, nullptr, nullptr, nullptr, nullptr);
    auto cfg = std::static_pointer_cast<ConfigMock>(ctx->getConfig());
    EXPECT_CALL(*cfg, getOption(ConfigVal::SERVER_EXTOPTS_FFMPEGTHUMBNAILER_CACHE_DIR))
        .WillOnce(Return(""));
    EXPECT_CALL(*cfg, getOption(ConfigVal::SERVER_HOME))
        .WillOnce(Return("/var/lib/gerbera"));
    EXPECT_EQ(FfmpegThumbnailerHandlerTest(ctx, ConfigVal::SERVER_EXTOPTS_FFMPEGTHUMBNAILER_ENABLED).getCacheBasePath(), fs::path { "/var/lib/gerbera/cache-dir" });
}

TEST(Thumbnailer_Cache, CachePathAppendsAbsolute)
{
    auto result = FfmpegThumbnailerHandlerTest::getCachePath("/data/cache", "/data/video/file.avi");
    EXPECT_EQ(result, fs::path { "/data/cache/data/video/file.avi-thumb.jpg" });
}

TEST(Thumbnailer_Cache, CacheUniquePaths)
{
    const auto cacheBase = fs::path { "/database/cache" };
    // 2 similar paths with same file name.
    auto path1 = FfmpegThumbnailerHandlerTest::getCachePath(cacheBase, "/database/images/2020/04/image-1.jpg");
    auto path2 = FfmpegThumbnailerHandlerTest::getCachePath(cacheBase, "/database/images/2020/05/image-1.jpg");
    EXPECT_NE(path1, path2);
}

#endif
