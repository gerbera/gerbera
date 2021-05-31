/*MT*
    
    MediaTomb - http://www.mediatomb.cc/
    
    string_converter.h - this file is part of MediaTomb.
    
    Copyright (C) 2005 Gena Batyan <bgeradz@mediatomb.cc>,
                       Sergey 'Jin' Bostandzhyan <jin@mediatomb.cc>
    
    Copyright (C) 2006-2010 Gena Batyan <bgeradz@mediatomb.cc>,
                            Sergey 'Jin' Bostandzhyan <jin@mediatomb.cc>,
                            Leonhard Wimmer <leo@mediatomb.cc>
    
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

#include <iconv.h>
#include <memory>

#include "common.h"
#include "config/config.h"

class StringConverter {
public:
    StringConverter(const std::string& from, const std::string& to);
    virtual ~StringConverter();

    StringConverter(const StringConverter&) = delete;
    StringConverter& operator=(const StringConverter&) = delete;
    /// \brief Converts uses the from and to values that were passed
    /// to the constructor to convert the string str to a specific character
    /// set.
    /// \param str String to be converted.
    /// \param validate if this parameter is true then an exception will be
    /// thrown if illegal input is encountered. If false, illegal characters
    /// will be padded with '?' and the function will return the string.
    std::string convert(std::string str, bool validate = false);
    bool validate(const std::string& str);

    /// \brief internal (UTF-8) to filesystem
    static std::unique_ptr<StringConverter> i2f(const std::shared_ptr<Config>& cm);

    /// \brief filesystem to internal
    static std::unique_ptr<StringConverter> f2i(const std::shared_ptr<Config>& cm);

    /// \brief metadata to internal
    static std::unique_ptr<StringConverter> m2i(config_option_t option, const fs::path& location, const std::shared_ptr<Config>& cm);
#ifdef HAVE_JS
    /// \brief scripting to internal
    static std::unique_ptr<StringConverter> j2i(const std::shared_ptr<Config>& cm);

    /// \brief playlist to internal
    static std::unique_ptr<StringConverter> p2i(const std::shared_ptr<Config>& cm);
#endif
#if defined(HAVE_JS) || defined(HAVE_TAGLIB) || defined(ATRAILERS) || defined(HAVE_MATROSKA)
    /// \brief safeguard - internal to internal - needed to catch some
    /// scenarious where the user may have forgotten to add proper conversion
    /// in the script.
    static std::unique_ptr<StringConverter> i2i([[maybe_unused]] const std::shared_ptr<Config>& cm);
#endif

protected:
    iconv_t cd;
    bool dirty { false };

    std::string _convert(const std::string& str, bool validate,
        size_t* stoppedAt = nullptr);
};

#endif // __STRING_CONVERTER_H__
