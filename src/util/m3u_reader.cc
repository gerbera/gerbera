/*GRB*
    Gerbera - https://gerbera.io/

    Copyright (C) 2026 Gerbera Contributors

    Gerbera is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License version 2
    as published by the Free Software Foundation.
*/

#include "m3u_reader.h" // API

#include "exceptions.h"
#include "string_converter.h"

// RFC 3629: reject overlong sequences, surrogates and values above U+10FFFF.
static bool isUtf8(const std::string& text)
{
    for (std::size_t i = 0; i < text.size();) {
        auto first = static_cast<unsigned char>(text[i++]);
        if (first < 0x80)
            continue;
        unsigned int count;
        unsigned int value;
        unsigned int minimum;
        if (first >= 0xc2 && first <= 0xdf) {
            count = 1;
            value = first & 0x1f;
            minimum = 0x80;
        } else if (first >= 0xe0 && first <= 0xef) {
            count = 2;
            value = first & 0x0f;
            minimum = 0x800;
        } else if (first >= 0xf0 && first <= 0xf4) {
            count = 3;
            value = first & 0x07;
            minimum = 0x10000;
        } else {
            return false;
        }
        if (text.size() - i < count)
            return false;
        while (count--) {
            auto next = static_cast<unsigned char>(text[i++]);
            if ((next & 0xc0) != 0x80)
                return false;
            value = (value << 6) | (next & 0x3f);
        }
        if (value < minimum || value > 0x10ffff || (value >= 0xd800 && value <= 0xdfff))
            return false;
    }
    return true;
}

M3uReader::M3uReader(std::istream& input, M3uCharset charset, bool m3u8, std::function<bool()> isValid)
    : input(input)
    , isValid(std::move(isValid))
    , charset(m3u8 ? M3uCharset::Utf8 : charset)
{
    if (charset == M3uCharset::Default)
        throw_std_runtime_error("M3U charset selection requires an explicit option");

    bool validUtf8 = true;
    std::size_t invalidLine = 0;
    while (auto line = readRawLine()) {
        if (lineNumber == 1)
            stripBom = line->compare(0, 3, "\xef\xbb\xbf") == 0 && this->charset != M3uCharset::Windows1252;
        if (!isUtf8(*line) && validUtf8) {
            validUtf8 = false;
            invalidLine = lineNumber;
        }
    }
    if (this->charset == M3uCharset::Auto)
        this->charset = (stripBom || validUtf8) ? M3uCharset::Utf8 : M3uCharset::Windows1252;
    if (this->charset == M3uCharset::Utf8 && !validUtf8)
        throw_std_runtime_error("Invalid UTF-8 in M3U playlist at line {}", invalidLine);

    rewind();
    if (this->charset == M3uCharset::Windows1252) {
        converter = std::make_unique<StringConverter>("WINDOWS-1252", "UTF-8");
        // Validate the selected fallback before the script can create partial playlists.
        while (readLine()) { }
        rewind();
    }
}

M3uReader::~M3uReader() = default;

void M3uReader::rewind()
{
    input.clear();
    input.seekg(0);
    if (!input)
        throw_std_runtime_error("Cannot rewind M3U playlist");
    lineNumber = 0;
}

std::optional<std::string> M3uReader::readRawLine()
{
    if (isValid && !isValid())
        throw_std_runtime_error("M3U playlist import cancelled");
    std::string line;
    if (!std::getline(input, line)) {
        if (input.bad() || !input.eof())
            throw_std_runtime_error("Cannot read M3U playlist at line {}", lineNumber + 1);
        return std::nullopt;
    }
    ++lineNumber;
    if (line.find('\0') != std::string::npos)
        throw_std_runtime_error("NUL byte in M3U playlist at line {}", lineNumber);
    return line;
}

std::optional<std::string> M3uReader::readLine()
{
    auto line = readRawLine();
    if (!line)
        return std::nullopt;
    if (lineNumber == 1 && stripBom)
        line->erase(0, 3);
    if (converter) {
        auto [decoded, error] = converter->convert(*line);
        if (!error.empty())
            throw_std_runtime_error("Invalid Windows-1252 in M3U playlist at line {}: {}", lineNumber, error);
        return decoded;
    }
    if (!isUtf8(*line))
        throw_std_runtime_error("Invalid UTF-8 in M3U playlist at line {}", lineNumber);
    return line;
}

const char* M3uReader::getCharset() const
{
    return charset == M3uCharset::Windows1252 ? "WINDOWS-1252" : "UTF-8";
}
