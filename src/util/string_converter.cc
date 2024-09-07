/*MT*

    MediaTomb - http://www.mediatomb.cc/

    string_converter.cc - this file is part of MediaTomb.

    Copyright (C) 2005 Gena Batyan <bgeradz@mediatomb.cc>,
                       Sergey 'Jin' Bostandzhyan <jin@mediatomb.cc>

    Copyright (C) 2006-2010 Gena Batyan <bgeradz@mediatomb.cc>,
                            Sergey 'Jin' Bostandzhyan <jin@mediatomb.cc>,
                            Leonhard Wimmer <leo@mediatomb.cc>

    Copyright (C) 2016-2024 Gerbera Contributors

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

/// \file string_converter.cc
#define GRB_LOG_FAC GrbLogFacility::content
#include "string_converter.h" // API

#include "common.h"
#include "config/config_val.h"
#include "config/result/directory_tweak.h"
#include "exceptions.h"
#include "util/logger.h"

StringConverter::StringConverter(const std::string& from, const std::string& to)
    : cd(iconv_open(to.c_str(), from.c_str()))
{
    if (!cd) {
        cd = {};
        throw_fmt_system_error("iconv");
    }
}

StringConverter::~StringConverter()
{
    if (cd)
        iconv_close(cd);
}

std::string StringConverter::convert(std::string str, bool validate)
{
    std::size_t stoppedAt = 0;
    std::string ret;

    if (str.empty())
        return str;

    do {
        ret = ret + _convert(str, validate, &stoppedAt);
        if ((ret.length() > 0) && (stoppedAt == 0))
            break;

        ret = ret + "?";
        if ((stoppedAt + 1) < str.length())
            str = str.substr(stoppedAt + 1);
        else
            break;

        stoppedAt = 0;
    } while (true);

    return ret;
}

bool StringConverter::validate(const std::string& str)
{
    try {
        _convert(str, true);
        return true;
    } catch (const std::runtime_error& e) {
        return false;
    }
}

std::string StringConverter::_convert(const std::string& str, bool validate,
    std::size_t* stoppedAt)
{
    // reset to initial state
    if (dirty) {
        iconv(cd, nullptr, nullptr, nullptr, nullptr);
        dirty = false;
    }

    auto length = str.length();
    if (length < (std::string::npos / 4)) {
        length *= 4;
    } else {
        log_debug("Could not determine memory for string conversion!");
        throw_std_runtime_error("Could not determine memory for string conversion");
    }

    char* output;
    try {
        output = new char[length];
        output[0] = '\0';
    } catch (const std::bad_alloc& ex) {
        log_debug("Could not allocate memory for string conversion!\n{}", ex.what());
        throw_std_runtime_error("Could not allocate memory for string conversion");
    }

    auto input = str.c_str();
    const char* inputCopy = input;
    char* outputCopy = output;

    const char** inputPtr = &inputCopy;
    char** outputPtr = &outputCopy;

    auto inputBytes = str.length();
    auto outputBytes = length;

    // log_debug("iconv: BEFORE: input bytes left: {}  output bytes left: {}", inputBytes, outputBytes);
#if defined(ICONV_CONST) || defined(SOLARIS)
    int ret = iconv(cd, inputPtr, &inputBytes,
        outputPtr, &outputBytes);
#else
    int ret = iconv(cd, const_cast<char**>(inputPtr), &inputBytes,
        outputPtr, &outputBytes);
#endif

    if (ret == -1) {
        log_error("iconv: {}", std::strerror(errno));
        std::string err;
        switch (errno) {
        case EILSEQ:
        case EINVAL: {
            if (errno == EILSEQ) {
                log_error("iconv: {} could not be converted to new encoding: invalid character sequence!", str.c_str());
            } else {
                log_error("iconv: Incomplete multibyte sequence");
            }
            if (validate) {
                throw_std_runtime_error(err);
            }

            if (stoppedAt)
                *stoppedAt = str.length() - inputBytes;
            auto retStr = std::string(output, outputCopy - output);
            dirty = true;
            *outputCopy = 0;
            delete[] output;
            return retStr;
        }
        case E2BIG:
            /// \todo should encode the whole string anyway
            err = "iconv: Insufficient space in output buffer";
            break;
        default:
            err = fmt::format("iconv: {}", std::strerror(errno));
            break;
        }
        *outputCopy = 0;
        log_error(err);
        //        log_debug("iconv: input: {}", input);
        //        log_debug("iconv: converted part:  {}", output);
        dirty = true;
        delete[] output;
        throw_std_runtime_error(err);
    }

    // log_debug("iconv: AFTER: input bytes left: {}  output bytes left: {}", inputBytes, outputBytes);
    // log_debug("iconv: returned {}", ret);

    auto retStr = std::string(output, outputCopy - output);
    delete[] output;
    if (stoppedAt)
        *stoppedAt = 0; // no error
    return retStr;
}

ConverterManager::ConverterManager(const std::shared_ptr<Config>& cm)
    : config(cm)
{
#if defined(HAVE_JS) || defined(HAVE_TAGLIB) || defined(HAVE_MATROSKA)
    charsets[ConfigVal::MAX] = DEFAULT_INTERNAL_CHARSET;
#endif
    for (auto cv : std::array {
#ifdef HAVE_LIBEXIF
             ConfigVal::IMPORT_LIBOPTS_EXIF_CHARSET,
#endif
#ifdef HAVE_EXIV2
             ConfigVal::IMPORT_LIBOPTS_EXIV2_CHARSET,
#endif
#ifdef HAVE_TAGLIB
             ConfigVal::IMPORT_LIBOPTS_ID3_CHARSET,
#endif
#ifdef HAVE_FFMPEG
             ConfigVal::IMPORT_LIBOPTS_FFMPEG_CHARSET,
#endif
#ifdef HAVE_MATROSKA
             ConfigVal::IMPORT_LIBOPTS_MKV_CHARSET,
#endif
#ifdef HAVE_WAVPACK
             ConfigVal::IMPORT_LIBOPTS_WVC_CHARSET,
#endif
#ifdef HAVE_JS
             ConfigVal::IMPORT_SCRIPTING_CHARSET,
             ConfigVal::IMPORT_PLAYLIST_CHARSET,
#endif
             ConfigVal::IMPORT_METADATA_CHARSET,
             ConfigVal::IMPORT_FILESYSTEM_CHARSET }) {
        charsets[cv] = cm->getOption(cv);
    }

    for (auto&& [config, cs] : charsets) {
        converters[cs] = std::make_shared<StringConverter>(
            cs,
            DEFAULT_INTERNAL_CHARSET);
    }
}

const std::shared_ptr<StringConverter> ConverterManager::m2i(ConfigVal option, const fs::path& location)
{
    auto charset = charsets.at(option);
    if (charset.empty()) {
        charset = config->getOption(ConfigVal::IMPORT_METADATA_CHARSET);
        charsets[option] = charset;
    }
    auto tweak = config->getDirectoryTweakOption(ConfigVal::IMPORT_DIRECTORIES_LIST)->get(!location.empty() ? location : "/");
    if (tweak && tweak->hasMetaCharset()) {
        charset = tweak->getMetaCharset();
        if (converters.find(charset) == converters.end()) {
            converters[charset] = std::make_shared<StringConverter>(
                charset,
                DEFAULT_INTERNAL_CHARSET);
        }
        log_debug("Using charset {} for {}", charset, location.string());
    }

    return converters.at(charset);
}

const std::shared_ptr<StringConverter> ConverterManager::f2i() const
{
    return converters.at(charsets.at(ConfigVal::IMPORT_FILESYSTEM_CHARSET));
}

#if defined(HAVE_JS) || defined(HAVE_TAGLIB) || defined(HAVE_MATROSKA)
const std::shared_ptr<StringConverter> ConverterManager::i2i() const
{
    return converters.at(charsets.at(ConfigVal::MAX));
}
#endif

#ifdef HAVE_JS
const std::shared_ptr<StringConverter> ConverterManager::j2i() const
{
    return converters.at(charsets.at(ConfigVal::IMPORT_SCRIPTING_CHARSET));
}

const std::shared_ptr<StringConverter> ConverterManager::p2i() const
{
    return converters.at(charsets.at(ConfigVal::IMPORT_PLAYLIST_CHARSET));
}
#endif
