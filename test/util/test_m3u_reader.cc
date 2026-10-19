/*GRB*
    Gerbera - https://gerbera.io/

    Copyright (C) 2026 Gerbera Contributors

    Gerbera is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License version 2
    as published by the Free Software Foundation.
*/

#include "util/m3u_reader.h"

#include <gtest/gtest.h>
#include <sstream>
#include <vector>

static std::vector<std::string> readPlaylist(const std::string& bytes, M3uCharset charset, bool m3u8 = false)
{
    std::istringstream input(bytes);
    M3uReader reader(input, charset, m3u8);
    std::vector<std::string> lines;
    while (auto line = reader.readLine())
        lines.push_back(*line);
    return lines;
}

TEST(M3uReaderTest, DecodesWindows1252PathsAndExtinfWithoutLosingOrderOrRepeats)
{
    const std::string bytes = "#EXTM3U\r\n#EXTINF:12,Gr\xfc\xdf"
                              "e \x96 \x80\r\n../V\xe4th.flac\r\n../a.mp3\r\n../V\xe4th.flac";
    const std::vector<std::string> expected { "#EXTM3U\r", u8"#EXTINF:12,Grüße – €\r", u8"../Väth.flac\r", "../a.mp3\r", u8"../Väth.flac" };
    EXPECT_EQ(readPlaylist(bytes, M3uCharset::Auto), expected);
    EXPECT_EQ(readPlaylist(bytes, M3uCharset::Windows1252), expected);
}

TEST(M3uReaderTest, AcceptsUtf8InternationalAndSupplementaryCharacters)
{
    const std::string bytes = u8"#EXTINF:1,日本語 😀\n../Grüße.flac\n\n../日本語 😀.mp3\n";
    const std::vector<std::string> expected { u8"#EXTINF:1,日本語 😀", u8"../Grüße.flac", "", u8"../日本語 😀.mp3" };
    for (auto mode : { M3uCharset::Auto, M3uCharset::Utf8 }) {
        EXPECT_EQ(readPlaylist(bytes, mode), expected);
        EXPECT_EQ(readPlaylist("\xef\xbb\xbf" + bytes, mode), expected);
    }
}

TEST(M3uReaderTest, SelectsOneCharsetForTheEntireFile)
{
    // First line alone is valid UTF-8 but the later line requires Windows-1252.
    EXPECT_EQ(readPlaylist("\xc3\xa4.mp3\n\xe4.mp3", M3uCharset::Auto),
        (std::vector<std::string> { u8"Ã¤.mp3", u8"ä.mp3" }));
    EXPECT_EQ(readPlaylist("\xc3\xa4.mp3", M3uCharset::Auto), (std::vector<std::string> { u8"ä.mp3" }));
    EXPECT_EQ(readPlaylist("\xc3\xa4.mp3", M3uCharset::Windows1252), (std::vector<std::string> { u8"Ã¤.mp3" }));
}

TEST(M3uReaderTest, RejectsMalformedUtf8BeforeReturningAnyEntries)
{
    for (const auto& invalid : std::vector<std::string> { "\xc0\xaf", "\xed\xa0\x80", "\xf4\x90\x80\x80", "\xe2\x82", "\x80", "\xf8\x88\x80\x80\x80" }) {
        SCOPED_TRACE(invalid);
        EXPECT_THROW(readPlaylist("valid.mp3\n" + invalid, M3uCharset::Utf8), std::runtime_error);
        EXPECT_THROW(readPlaylist("\xef\xbb\xbfvalid.mp3\n" + invalid, M3uCharset::Auto), std::runtime_error);
    }
}

TEST(M3uReaderTest, NeverFallsBackForM3u8)
{
    for (auto mode : { M3uCharset::Auto, M3uCharset::Utf8, M3uCharset::Windows1252 }) {
        EXPECT_THROW(readPlaylist("valid.mp3\nV\xe4th.flac", mode, true), std::runtime_error);
        EXPECT_EQ(readPlaylist(u8"Väth.flac", mode, true), (std::vector<std::string> { u8"Väth.flac" }));
    }
}

TEST(M3uReaderTest, PreservesLongLinesAcrossMultibyteBoundaries)
{
    auto path = std::string(1022, 'a') + u8"日本語😀.flac";
    EXPECT_EQ(readPlaylist(path + "\n" + path, M3uCharset::Auto), (std::vector<std::string> { path, path }));
}

TEST(M3uReaderTest, HandlesEmptyFilesAndRejectsNulBytes)
{
    EXPECT_TRUE(readPlaylist("", M3uCharset::Auto).empty());
    EXPECT_EQ(readPlaylist("\xef\xbb\xbf", M3uCharset::Auto), (std::vector<std::string> { "" }));
    for (auto mode : { M3uCharset::Auto, M3uCharset::Utf8, M3uCharset::Windows1252 })
        EXPECT_THROW(readPlaylist(std::string("a\0b", 3), mode), std::runtime_error);
}

TEST(M3uReaderTest, RejectsUnavailableInput)
{
    std::istringstream input("a.mp3");
    input.setstate(std::ios::badbit);
    EXPECT_THROW(M3uReader reader(input, M3uCharset::Auto, false), std::runtime_error);
    EXPECT_THROW(M3uReader reader(input, M3uCharset::Default, false), std::runtime_error);
}

TEST(M3uReaderTest, RejectsNonSeekableInput)
{
    class NonSeekableBuffer : public std::stringbuf {
        pos_type seekpos(pos_type, std::ios_base::openmode) override { return pos_type(off_type(-1)); }
    } buffer;
    buffer.str("a.mp3\n");
    std::istream input(&buffer);
    EXPECT_THROW(M3uReader reader(input, M3uCharset::Auto, false), std::runtime_error);
}

TEST(M3uReaderTest, CanCancelDuringWholeFileValidation)
{
    std::istringstream input("a.mp3\nb.mp3\na.mp3\n");
    unsigned int calls = 0;
    EXPECT_THROW(M3uReader reader(input, M3uCharset::Auto, false, [&]() { return ++calls < 3; }), std::runtime_error);
    EXPECT_EQ(calls, 3);
}
