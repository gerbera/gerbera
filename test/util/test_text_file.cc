/*GRB*
    Gerbera - https://gerbera.io/

    Copyright (C) 2026 Gerbera Contributors

    Gerbera is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License version 2
    as published by the Free Software Foundation.
*/

#include "util/grb_fs.h"
#include "util/tools.h"

#include <gtest/gtest.h>
#include <vector>

struct TemporaryTextFile {
    fs::path path = fs::temp_directory_path() / ("grb-text-" + generateRandomId());
    ~TemporaryTextFile()
    {
        std::error_code error;
        fs::remove(path, error);
    }
};

static std::vector<std::string> readText(const std::string& bytes, TextFileCharset charset)
{
    TemporaryTextFile temporary;
    GrbFile file(temporary.path);
    file.writeTextFile(bytes);
    file.open("rb");
    file.setTextCharset(charset);
    std::vector<std::string> lines;
    while (auto line = file.readLine()) {
        if (!line->empty() && line->back() == '\n')
            line->pop_back();
        lines.push_back(*line);
    }
    return lines;
}

TEST(TextFileTest, DecodesWindows1252PathsAndExtinfWithoutLosingOrderOrRepeats)
{
    const std::string bytes = "#EXTM3U\r\n#EXTINF:12,Gr\xfc\xdf"
                              "e \x96 \x80\r\n../V\xe4th.flac\r\n../a.mp3\r\n../V\xe4th.flac";
    const std::vector<std::string> expected { "#EXTM3U\r", u8"#EXTINF:12,Grüße – €\r", u8"../Väth.flac\r", "../a.mp3\r", u8"../Väth.flac" };
    EXPECT_EQ(readText(bytes, TextFileCharset::Auto), expected);
    EXPECT_EQ(readText(bytes, TextFileCharset::Windows1252), expected);
}

TEST(TextFileTest, AcceptsUtf8InternationalAndSupplementaryCharacters)
{
    const std::string bytes = u8"#EXTINF:1,日本語 😀\n../Grüße.flac\n\n../日本語 😀.mp3\n";
    const std::vector<std::string> expected { u8"#EXTINF:1,日本語 😀", u8"../Grüße.flac", "", u8"../日本語 😀.mp3" };
    for (auto mode : { TextFileCharset::Auto, TextFileCharset::Utf8 }) {
        EXPECT_EQ(readText(bytes, mode), expected);
        EXPECT_EQ(readText("\xef\xbb\xbf" + bytes, mode), expected);
    }
}

TEST(TextFileTest, SelectsOneCharsetForTheEntireFile)
{
    // First line alone is valid UTF-8 but the later line requires Windows-1252.
    EXPECT_EQ(readText("\xc3\xa4.mp3\n\xe4.mp3", TextFileCharset::Auto),
        (std::vector<std::string> { u8"Ã¤.mp3", u8"ä.mp3" }));
    EXPECT_EQ(readText("\xc3\xa4.mp3", TextFileCharset::Auto), (std::vector<std::string> { u8"ä.mp3" }));
    EXPECT_EQ(readText("\xc3\xa4.mp3", TextFileCharset::Windows1252), (std::vector<std::string> { u8"Ã¤.mp3" }));
}

TEST(TextFileTest, RejectsMalformedUtf8BeforeReturningAnyEntries)
{
    for (const auto& invalid : std::vector<std::string> { "\xc0\xaf", "\xed\xa0\x80", "\xf4\x90\x80\x80", "\xe2\x82", "\x80", "\xf8\x88\x80\x80\x80" }) {
        SCOPED_TRACE(invalid);
        EXPECT_THROW(readText("valid.mp3\n" + invalid, TextFileCharset::Utf8), std::runtime_error);
        EXPECT_THROW(readText("\xef\xbb\xbfvalid.mp3\n" + invalid, TextFileCharset::Auto), std::runtime_error);
    }
}

TEST(TextFileTest, PreservesLongLinesAcrossMultibyteBoundaries)
{
    auto path = std::string(1022, 'a') + u8"日本語😀.flac";
    EXPECT_EQ(readText(path + "\n" + path, TextFileCharset::Auto), (std::vector<std::string> { path, path }));
}

TEST(TextFileTest, HandlesEmptyFilesAndRejectsNulBytes)
{
    EXPECT_TRUE(readText("", TextFileCharset::Auto).empty());
    EXPECT_EQ(readText("\xef\xbb\xbf", TextFileCharset::Auto), (std::vector<std::string> { "" }));
    for (auto mode : { TextFileCharset::Auto, TextFileCharset::Utf8, TextFileCharset::Windows1252 })
        EXPECT_THROW(readText(std::string("a\0b", 3), mode), std::runtime_error);
}

TEST(TextFileTest, RejectsUnavailableInput)
{
    TemporaryTextFile temporary;
    GrbFile file(temporary.path);
    EXPECT_THROW(file.open("rb"), std::runtime_error);
    EXPECT_THROW(file.readLine(), std::runtime_error);
    EXPECT_THROW(file.setTextCharset(TextFileCharset::Auto), std::runtime_error);
}

TEST(TextFileTest, CanCancelDuringWholeFileValidation)
{
    TemporaryTextFile temporary;
    GrbFile file(temporary.path);
    file.writeTextFile("a\nb\na\n");
    file.open("rb");
    unsigned int calls = 0;
    EXPECT_THROW(file.setTextCharset(TextFileCharset::Auto, [&]() { return ++calls < 3; }), std::runtime_error);
    EXPECT_EQ(calls, 3);
}

TEST(TextFileTest, RetainsRawTerminatorsAndLegacyChunkBoundaries)
{
    TemporaryTextFile temporary;
    GrbFile file(temporary.path);
    file.writeTextFile("abc\r\ndef\nlast");
    file.open("rb");
    ASSERT_EQ(file.readLine(), std::make_optional<std::string>("abc\r\n"));
    ASSERT_EQ(file.readLine(2), std::make_optional<std::string>("de"));
    ASSERT_EQ(file.readLine(), std::make_optional<std::string>("f\n"));
    ASSERT_EQ(file.readLine(), std::make_optional<std::string>("last"));
    EXPECT_FALSE(file.readLine());
}

TEST(TextFileTest, DefaultModeDoesNotDetectStripOrReinterpretBytes)
{
    const std::string bytes = "\xef\xbb\xbf\xe4\n";
    EXPECT_EQ(readText(bytes, TextFileCharset::Default), (std::vector<std::string> { bytes.substr(0, bytes.size() - 1) }));
}
