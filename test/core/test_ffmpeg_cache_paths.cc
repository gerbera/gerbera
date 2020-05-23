#include "../mock/config_mock.h"
#include <metadata/ffmpeg_handler.h>

#include <gtest/gtest.h>

using namespace testing;

TEST(Thumbnailer_Cache, BaseDirFromConfig)
{
    auto cfg = ConfigMock {};
    EXPECT_CALL(cfg, getOption(CFG_SERVER_EXTOPTS_FFMPEGTHUMBNAILER_CACHE_DIR))
        .WillOnce(Return("/var/lib/cache"));
    EXPECT_EQ(getThumbnailCacheBasePath(cfg), fs::path { "/var/lib/cache" });
}

TEST(Thumbnailer_Cache, BaseDirDefaultFromUserHome)
{
    auto cfg = ConfigMock {};
    EXPECT_CALL(cfg, getOption(CFG_SERVER_EXTOPTS_FFMPEGTHUMBNAILER_CACHE_DIR))
        .WillOnce(Return(""));
    EXPECT_CALL(cfg, getOption(CFG_SERVER_HOME))
        .WillOnce(Return("/var/lib/gerbera"));
    EXPECT_EQ(getThumbnailCacheBasePath(cfg), fs::path { "/var/lib/gerbera/cache-dir" });
}

TEST(Thumbnailer_Cache, CachePathAppendsAbsolute)
{
    auto result = getThumbnailCachePath("/data/cache", "/data/video/file.avi");
    EXPECT_EQ(result, fs::path { "/data/cache/data/video/file.avi-thumb.jpg" });
}
