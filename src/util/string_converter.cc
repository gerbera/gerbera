/*MT*

    MediaTomb - http://www.mediatomb.cc/

    string_converter.cc - this file is part of MediaTomb.

    Copyright (C) 2005 Gena Batyan <bgeradz@mediatomb.cc>,
                       Sergey 'Jin' Bostandzhyan <jin@mediatomb.cc>

    Copyright (C) 2006-2010 Gena Batyan <bgeradz@mediatomb.cc>,
                            Sergey 'Jin' Bostandzhyan <jin@mediatomb.cc>,
                            Leonhard Wimmer <leo@mediatomb.cc>

    Copyright (C) 2016-2023 Gerbera Contributors

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

#include "string_converter.h" // API

#include "config/directory_tweak.h"

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
    std::string retStr;

    auto input = str.c_str();
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
    } catch (const std::bad_alloc& ex) {
        log_debug("Could not allocate memory for string conversion!\n{}", ex.what());
        throw_std_runtime_error("Could not allocate memory for string conversion");
    }
    const char* inputCopy = input;
    char* outputCopy = output;

    const char** inputPtr = &inputCopy;
    char** outputPtr = &outputCopy;

    auto inputBytes = str.length();
    auto outputBytes = length;

    // reset to initial state
    if (dirty) {
        iconv(cd, nullptr, nullptr, nullptr, nullptr);
        dirty = false;
    }

    // log_debug(("iconv: BEFORE: input bytes left: {}  output bytes left: {}",
    //        input_bytes, output_bytes));
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
        case EINVAL:
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
            retStr = std::string(output, outputCopy - output);
            dirty = true;
            *outputCopy = 0;
            delete[] output;
            return retStr;
        case E2BIG:
            /// \todo should encode the whole string anyway
            err = "iconv: Insufficient space in output buffer";
            break;
        default:
            err = fmt::format("iconv: {}", std::strerror(errno));
            break;
        }
        *outputCopy = 0;
        log_error("{}", err);
        //        log_debug("iconv: input: {}", input);
        //        log_debug("iconv: converted part:  {}", output);
        dirty = true;
        delete[] output;
        throw_std_runtime_error(err);
    }

    // log_debug("iconv: AFTER: input bytes left: {}  output bytes left: {}",
    //        input_bytes, output_bytes);
    // log_debug("iconv: returned {}", ret);

    retStr = std::string(output, outputCopy - output);
    delete[] output;
    if (stoppedAt)
        *stoppedAt = 0; // no error
    return retStr;
}

/// \todo iconv caching
std::unique_ptr<StringConverter> StringConverter::i2f(const std::shared_ptr<Config>& cm)
{
    return std::make_unique<StringConverter>(
        DEFAULT_INTERNAL_CHARSET, cm->getOption(CFG_IMPORT_FILESYSTEM_CHARSET));
}
std::unique_ptr<StringConverter> StringConverter::f2i(const std::shared_ptr<Config>& cm)
{
    return std::make_unique<StringConverter>(
        cm->getOption(CFG_IMPORT_FILESYSTEM_CHARSET), DEFAULT_INTERNAL_CHARSET);
}
std::unique_ptr<StringConverter> StringConverter::m2i(config_option_t option, const fs::path& location, const std::shared_ptr<Config>& cm)
{
    auto charset = cm->getOption(option);
    if (charset.empty()) {
        charset = cm->getOption(CFG_IMPORT_METADATA_CHARSET);
    }
    auto tweak = cm->getDirectoryTweakOption(CFG_IMPORT_DIRECTORIES_LIST)->get(!location.empty() ? location : "/");
    if (tweak && tweak->hasMetaCharset()) {
        charset = tweak->getMetaCharset();
        log_debug("Using charset {} for {}", charset, location.string());
    }

    return std::make_unique<StringConverter>(charset, DEFAULT_INTERNAL_CHARSET);
}

#ifdef HAVE_JS
std::unique_ptr<StringConverter> StringConverter::j2i(const std::shared_ptr<Config>& cm)
{
    return std::make_unique<StringConverter>(
        cm->getOption(CFG_IMPORT_SCRIPTING_CHARSET),
        DEFAULT_INTERNAL_CHARSET);
}

std::unique_ptr<StringConverter> StringConverter::p2i(const std::shared_ptr<Config>& cm)
{
    return std::make_unique<StringConverter>(
        cm->getOption(CFG_IMPORT_PLAYLIST_CHARSET),
        DEFAULT_INTERNAL_CHARSET);
}
#endif

#if defined(HAVE_JS) || defined(HAVE_TAGLIB) || defined(ATRAILERS) || defined(HAVE_MATROSKA)
std::unique_ptr<StringConverter> StringConverter::i2i(const std::shared_ptr<Config>& cm)
{
    return std::make_unique<StringConverter>(
        DEFAULT_INTERNAL_CHARSET,
        DEFAULT_INTERNAL_CHARSET);
}
#endif
