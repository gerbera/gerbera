/*MT*

    MediaTomb - http://www.mediatomb.cc/

    string_converter.h - this file is part of MediaTomb.

    Copyright (C) 2005 Gena Batyan <bgeradz@mediatomb.cc>,
                       Sergey 'Jin' Bostandzhyan <jin@mediatomb.cc>

    Copyright (C) 2006-2010 Gena Batyan <bgeradz@mediatomb.cc>,
                            Sergey 'Jin' Bostandzhyan <jin@mediatomb.cc>,
                            Leonhard Wimmer <leo@mediatomb.cc>

    Copyright (C) 2016-2025 Gerbera Contributors

    MediaTomb is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License version 2
    as published by the Free Software Foundation.

    MediaTomb is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    version 2 along with MediaTomb; if not, write to the Free Software
    Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301, USA.

    $Id$
*/

/// \file string_converter.h

#ifndef __STRING_CONVERTER_H__
#define __STRING_CONVERTER_H__

#include "util/grb_fs.h"

#include <iconv.h>
#include <map>
#include <memory>
#include <string>

class Config;
enum class ConfigVal;

class StringConverter {
public:
    /// \brief create a new converter
    /// \param from source character set
    /// \param to target character set
    StringConverter(const std::string& from, const std::string& to);
    virtual ~StringConverter();

    StringConverter(const StringConverter&) = delete;
    StringConverter& operator=(const StringConverter&) = delete;
    /// \brief Uses the from and to values that were passed
    /// to the constructor to convert the string str to a specific character
    /// set.
    /// \param str String to be converted.
    /// \param validate if this parameter is true then an exception will be
    /// thrown if illegal input is encountered. If false, illegal characters
    /// will be padded with '?' and the function will return the string.
    std::pair<std::string, std::string> convert(
        std::string str,
        bool validate = false);
    bool validate(const std::string& str);

protected:
    iconv_t cd;
    bool dirty {};
    std::string to;

    std::pair<std::string, std::string> _convert(
        const std::string& str,
        bool validate,
        std::size_t* stoppedAt = nullptr);
};

class ConverterManager {
public:
    ConverterManager(const std::shared_ptr<Config>& cm);
    virtual ~ConverterManager() = default;

    /// \brief metadata to internal
    const std::shared_ptr<StringConverter> m2i(ConfigVal option, const fs::path& location);
    /// \brief filesystem to internal
    const std::shared_ptr<StringConverter> f2i() const;
#if defined(HAVE_JS) || defined(HAVE_TAGLIB) || defined(HAVE_MATROSKA)
    /// \brief safeguard - internal to internal - needed to catch some
    /// scenarious where the user may have forgotten to add proper conversion
    /// in the script.
    const std::shared_ptr<StringConverter> i2i() const;
#endif
#ifdef HAVE_JS
    /// \brief scripting to internal
    const std::shared_ptr<StringConverter> j2i() const;
    /// \brief playlist to internal
    const std::shared_ptr<StringConverter> p2i() const;
#endif

protected:
    std::shared_ptr<Config> config;
    std::map<ConfigVal, std::string> charsets {};
    std::map<std::string, std::shared_ptr<StringConverter>> converters {};
};

#endif // __STRING_CONVERTER_H__
