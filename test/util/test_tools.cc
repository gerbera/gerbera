#include "util/tools.h"

#include <gtest/gtest.h>

using namespace ::testing;

TEST(ToolsTest, millisecondsToHMSF)
{
    EXPECT_EQ(millisecondsToHMSF(0), "0:00:00.000");
    EXPECT_EQ(millisecondsToHMSF(1 * 1000), "0:00:01.000");
    EXPECT_EQ(millisecondsToHMSF(62 * 1000), "0:01:02.000");
    EXPECT_EQ(millisecondsToHMSF((3600 + 60 + 2) * 1000 + 123), "1:01:02.123");
}

TEST(ToolsTest, readWriteBinaryFile)
{
    std::string source = "This is a test, \0, binary";
    auto test_file = fs::temp_directory_path() / "gerbera-binary-test";
    EXPECT_FALSE(fs::exists(test_file)) << "Can't test existing file";
    writeBinaryFile(test_file, reinterpret_cast<const std::byte*>(source.data()), source.size());
    auto result = readBinaryFile(test_file);
    EXPECT_TRUE(result.has_value());

    std::vector<std::byte> expected(source.size());
    std::transform(source.begin(), source.end(), expected.begin(),
        [](char c) { return std::byte(c); });

    EXPECT_EQ(*result, expected);
    fs::remove(test_file);
}

TEST(ToolsTest, readBinaryReturnsEmptyIfFileMissing)
{
    fs::path test_file("/some/unexisting/file");
    EXPECT_FALSE(fs::exists(test_file));
    auto res = readBinaryFile(test_file);
    EXPECT_FALSE(res.has_value());
}

TEST(ToolsTest, writeFileThrowsIfCantOpenFile)
{
    fs::path test_file("/some/unexisting/file");
    EXPECT_FALSE(fs::exists(test_file));

    std::vector<std::byte> data { std::byte { 1 } };

    EXPECT_THROW(writeBinaryFile(test_file, data.data(), data.size()), std::runtime_error);
}

TEST(ToolsTest, renderWebUriV4)
{
    EXPECT_EQ(renderWebUri("192.168.5.5", 7777), "192.168.5.5:7777");
}

TEST(ToolsTest, renderWebUriV6)
{
    EXPECT_EQ(renderWebUri("2001:0db8:85a3:0000:0000:8a2e:0370:7334", 7777), "[2001:0db8:85a3:0000:0000:8a2e:0370:7334]:7777");
}

TEST(ToolsTest, splitStringTest)
{
    auto parts = splitString("", ',');
    EXPECT_EQ(parts.size(), 0);

    parts = splitString("", ',', true);
    EXPECT_EQ(parts.size(), 1);

    parts = splitString("A", ',');
    ASSERT_EQ(parts.size(), 1);
    EXPECT_EQ(parts[0], "A");

    parts = splitString("A,", ',', false);
    ASSERT_EQ(parts.size(), 1);
    EXPECT_EQ(parts[0], "A");

    parts = splitString("A,", ',', true);
    ASSERT_EQ(parts.size(), 2);
    EXPECT_EQ(parts[0], "A");
    EXPECT_EQ(parts[1], "");

    parts = splitString(",A", ',', true);
    ASSERT_EQ(parts.size(), 2);
    EXPECT_EQ(parts[0], "");
    EXPECT_EQ(parts[1], "A");

    parts = splitString("A,B,C", ',');
    ASSERT_EQ(parts.size(), 3);
    EXPECT_EQ(parts[0], "A");
    EXPECT_EQ(parts[1], "B");
    EXPECT_EQ(parts[2], "C");

    parts = splitString("A,,C", ',', false);
    ASSERT_EQ(parts.size(), 2);
    EXPECT_EQ(parts[0], "A");
    EXPECT_EQ(parts[1], "C");

    parts = splitString("A,,C", ',', true);
    ASSERT_EQ(parts.size(), 3);
    EXPECT_EQ(parts[0], "A");
    EXPECT_EQ(parts[1], "");
    EXPECT_EQ(parts[2], "C");

    // Test splitString on last usecase where 'empty' is in use
    auto resource_string = "0~protocolInfo=http-get%3A%2A%3Aimage%2Fjpeg%3A%2A&resolution=3327x2039&size=732150~~|1~protocolInfo=http-get%3A%2A%3Aimage%2Fjpeg%3A%2A&resolution=170x256~rct=EX_TH~";
    auto resource_parts = splitString(resource_string, '|');
    ASSERT_EQ(resource_parts.size(), 2);
    auto parts0 = splitString(resource_parts[0], '~', true);
    auto parts1 = splitString(resource_parts[1], '~', true);
    ASSERT_EQ(parts0.size(), 4);
    ASSERT_EQ(parts1.size(), 4);
}
