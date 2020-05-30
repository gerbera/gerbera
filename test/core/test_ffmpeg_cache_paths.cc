#include "../mock/config_mock.h"
#include <metadata/ffmpeg_handler.h>

#include <gtest/gtest.h>

using namespace testing;

#if defined(HAVE_FFMPEG) && defined(HAVE_FFMPEGTHUMBNAILER)

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

TEST(Thumbnailer_Cache, CacheUniquePaths)
{
    const auto cache_base = fs::path { "/storage/cache" };
    // 2 similar paths with same file name.
    auto path1 = getThumbnailCachePath(cache_base, "/storage/images/2020/04/image-1.jpg");
    auto path2 = getThumbnailCachePath(cache_base, "/storage/images/2020/05/image-1.jpg");
    EXPECT_NE(path1, path2);
}

#endif