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

TEST(ToolsTest, trimStringTest)
{
    EXPECT_EQ(trimString(""), "");
    EXPECT_EQ(trimString("AB"), "AB");
    EXPECT_EQ(trimString("  AB  "), "AB");
}

TEST(ToolsTest, startswithTest)
{
    EXPECT_EQ(startswith("AB", "AB"), true);
    EXPECT_EQ(startswith("ABCD", "AB"), true);
    EXPECT_EQ(startswith("AB", "ABC"), false);
    EXPECT_EQ(startswith("ABC", "BC"), false);
    EXPECT_EQ(startswith("ABAB", "AB"), true);
}

TEST(ToolsTest, toLowerTest)
{
    EXPECT_EQ(toLower("AbC"), "abc");

    std::string test = "TestString";
    EXPECT_EQ(toLowerInPlace(test), "teststring");
    EXPECT_EQ(test, "teststring");
}

TEST(ToolsTest, hexEncodeTest)
{
    const std::uint8_t data[] = { 0x60, 0x0D, 0xC0, 0xDE };
    EXPECT_EQ(hexEncode(data, sizeof(data)), "600dc0de");

    EXPECT_EQ(hexEncode(nullptr, 0), "");
}

TEST(ToolsTest, hexDecodeTest)
{
    EXPECT_EQ(hexDecodeString("476f6f64436f6465"), "GoodCode");
}

TEST(ToolsTest, dictEncodeTest)
{
    std::map<std::string, std::string> values;
    values.emplace("Key 1", "Value 1");
    values.emplace("Key2", "Value 2");
    EXPECT_EQ(dictEncode(values), "Key%201=Value%201&Key2=Value%202");
}

TEST(ToolsTest, dictDecodeTest)
{
    std::map<std::string, std::string> values = dictDecode("Key%201=Value%201&Key2=Value%202");
    ASSERT_EQ(values.size(), 2);
    EXPECT_EQ(values["Key 1"], "Value 1");
    EXPECT_EQ(values["Key2"], "Value 2");

    values = dictDecode("");
    EXPECT_EQ(values.size(), 0);
}

TEST(ToolsTest, dictMergeTest)
{
    std::map<std::string, std::string> result;
    result.emplace("A", "1");
    result.emplace("B", "2");

    std::map<std::string, std::string> source;
    source.emplace("A", "10");
    source.emplace("C", "3");

    EXPECT_EQ(result.size(), 2);

    dictMerge(result, source);

    EXPECT_EQ(result.size(), 3);
    EXPECT_EQ(result["A"], "1");
    EXPECT_EQ(result["B"], "2");
    EXPECT_EQ(result["C"], "3");
}

TEST(ToolsTest, pathToMapTest)
{
    std::map<std::string, std::string> values = pathToMap("Key 1/Value 1/Key 2/Value 2/Key3//");
    ASSERT_EQ(values.size(), 3);
    EXPECT_EQ(values["Key 1"], "Value 1");
    EXPECT_EQ(values["Key 2"], "Value 2");
    EXPECT_EQ(values["Key3"], "");

    values = pathToMap("Key 1/Value 1/Key 2/");
    ASSERT_EQ(values.size(), 2);
    EXPECT_EQ(values["Key 1"], "Value 1");
    EXPECT_EQ(values["Key 2"], "");

    values = pathToMap("Key 1/Value 1/Key 2");
    ASSERT_EQ(values.size(), 2);
    EXPECT_EQ(values["Key 1"], "Value 1");
    EXPECT_EQ(values["Key 2"], "");

    values = pathToMap("Key 1/");
    ASSERT_EQ(values.size(), 1);
    EXPECT_EQ(values["Key 1"], "");

    values = pathToMap("/");
    ASSERT_EQ(values.size(), 1);
    EXPECT_EQ(values[""], "");

    values = pathToMap("/meh");
    ASSERT_EQ(values.size(), 1);
    EXPECT_EQ(values[""], "meh");
}
