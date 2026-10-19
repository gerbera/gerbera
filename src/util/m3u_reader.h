/*GRB*
    Gerbera - https://gerbera.io/

    Copyright (C) 2026 Gerbera Contributors

    Gerbera is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License version 2
    as published by the Free Software Foundation.
*/

#ifndef GERBERA_M3U_READER_H
#define GERBERA_M3U_READER_H

#include <functional>
#include <istream>
#include <memory>
#include <optional>
#include <string>

class StringConverter;

enum class M3uCharset {
    Default,
    Auto,
    Utf8,
    Windows1252,
};

/// @brief Opt-in, whole-file charset selection before any playlist entries are imported.
class M3uReader {
public:
    M3uReader(std::istream& input, M3uCharset charset, bool m3u8, std::function<bool()> isValid = {});
    ~M3uReader();

    std::optional<std::string> readLine();
    const char* getCharset() const;

private:
    void rewind();
    std::optional<std::string> readRawLine();

    std::istream& input;
    std::function<bool()> isValid;
    M3uCharset charset;
    std::unique_ptr<StringConverter> converter;
    bool stripBom {};
    std::size_t lineNumber {};
};

#endif
