/*MT*
    
    MediaTomb - http://www.mediatomb.cc/
    
    string_converter.cc - this file is part of MediaTomb.
    
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

/// \file string_converter.cc

#include "string_converter.h"
#include "config/config_manager.h"

StringConverter::StringConverter(std::string from, std::string to)
{
    dirty = false;

    cd = iconv_open(to.c_str(), from.c_str());
    if (cd == (iconv_t)(-1)) {
        cd = (iconv_t) nullptr;
        throw _Exception(std::string("iconv: ") + strerror(errno));
    }
}

StringConverter::~StringConverter()
{
    if (cd != (iconv_t) nullptr)
        iconv_close(cd);
}

std::string StringConverter::convert(std::string str, bool validate)
{
    size_t stoppedAt = 0;
    std::string ret;

    if (!string_ok(str))
        return str;

    do {
        ret = ret + _convert(str, validate, &stoppedAt);
        if ((ret.length() > 0) && (stoppedAt == 0))
            break;

        ret = ret + "?";
        if ((stoppedAt + 1) < (size_t)str.length())
            str = str.substr (stoppedAt + 1);
        else
            break;

        stoppedAt = 0;
    } while (true);

    return ret;
}

bool StringConverter::validate(std::string str)
{
    try {
        _convert(str, true);
        return true;
    } catch (const Exception& e) {
        return false;
    }
}

std::string StringConverter::_convert(std::string str, bool validate,
    size_t* stoppedAt)
{
    std::string ret_str;

    int buf_size = str.length() * 4;

    const char* input = str.c_str();
    auto* output = (char*)MALLOC(buf_size);
    if (!output) {
        log_debug("Could not allocate memory for string conversion!\n");
        throw _Exception("Could not allocate memory for string conversion!");
    }

    const char* input_copy = input;
    char* output_copy = output;

    const char** input_ptr = &input_copy;
    char** output_ptr = &output_copy;

    auto input_bytes = (size_t)str.length();
    auto output_bytes = (size_t)buf_size;

    int ret;

    // reset to initial state
    if (dirty) {
        iconv(cd, nullptr, nullptr, nullptr, nullptr);
        dirty = false;
    }

    //log_debug(("iconv: BEFORE: input bytes left: %d  output bytes left: %d\n",
    //       input_bytes, output_bytes));
#if defined(ICONV_CONST) || defined(SOLARIS)
    ret = iconv(cd, input_ptr, &input_bytes,
        output_ptr, &output_bytes);
#else
    ret = iconv(cd, (char**)input_ptr, &input_bytes,
        output_ptr, &output_bytes);
#endif

    if (ret == -1) {
        log_error("iconv: %s\n", strerror(errno));
        std::string err;
        switch (errno) {
        case EILSEQ:
        case EINVAL:
            if (errno == EILSEQ)
                log_error("iconv: %s could not be converted to new encoding: invalid character sequence!\n", str.c_str());
            else
                log_error("iconv: Incomplete multibyte sequence\n");
            if (validate)
                throw _Exception(err);

            if (stoppedAt)
                *stoppedAt = (size_t)str.length() - input_bytes;
            ret_str = std::string(output, output_copy - output);
            dirty = true;
            *output_copy = 0;
            FREE(output);
            return ret_str;
            break;
        case E2BIG:
            /// \todo should encode the whole string anyway
            err = "iconv: Insufficient space in output buffer";
            break;
        default:
            err = std::string("iconv: ") + strerror(errno);
            break;
        }
        *output_copy = 0;
        log_error("%s\n", err.c_str());
        //        log_debug("iconv: input: %s\n", input);
        //        log_debug("iconv: converted part:  %s\n", output);
        dirty = true;
        if (output)
            FREE(output);
        throw _Exception(err);
    }

    //log_debug("iconv: AFTER: input bytes left: %d  output bytes left: %d\n",
    //       input_bytes, output_bytes);
    //log_debug("iconv: returned %d\n", ret);

    ret_str = std::string(output, output_copy - output);
    FREE(output);
    if (stoppedAt)
        *stoppedAt = 0; // no error
    return ret_str;
}

/// \todo iconv caching
std::unique_ptr<StringConverter> StringConverter::i2f(std::shared_ptr<ConfigManager> cm)
{
    auto conv = std::make_unique<StringConverter>(
        DEFAULT_INTERNAL_CHARSET, cm->getOption(CFG_IMPORT_FILESYSTEM_CHARSET));
    //        INTERNAL_CHARSET, cm->getFilesystemCharset()));
    return conv;
}
std::unique_ptr<StringConverter> StringConverter::f2i(std::shared_ptr<ConfigManager> cm)
{
    auto conv = std::make_unique<StringConverter>(
        cm->getOption(CFG_IMPORT_FILESYSTEM_CHARSET), DEFAULT_INTERNAL_CHARSET);
    return conv;
}
std::unique_ptr<StringConverter> StringConverter::m2i(std::shared_ptr<ConfigManager> cm)
{
    auto conv = std::make_unique<StringConverter>(
        cm->getOption(CFG_IMPORT_METADATA_CHARSET),
        DEFAULT_INTERNAL_CHARSET);
    return conv;
}

#ifdef HAVE_JS
std::unique_ptr<StringConverter> StringConverter::j2i(std::shared_ptr<ConfigManager> cm)
{
    auto conv = std::make_unique<StringConverter>(
        cm->getOption(CFG_IMPORT_SCRIPTING_CHARSET),
        DEFAULT_INTERNAL_CHARSET);
    return conv;
}

std::unique_ptr<StringConverter> StringConverter::p2i(std::shared_ptr<ConfigManager> cm)
{
    auto conv = std::make_unique<StringConverter>(
        cm->getOption(CFG_IMPORT_PLAYLIST_CHARSET),
        DEFAULT_INTERNAL_CHARSET);
    return conv;
}
#endif

#if defined(HAVE_JS) || defined(HAVE_TAGLIB) || defined(ATRAILERS) || defined(HAVE_MATROSKA)
std::unique_ptr<StringConverter> StringConverter::i2i(std::shared_ptr<ConfigManager> cm)
{
    auto conv = std::make_unique<StringConverter>(
        DEFAULT_INTERNAL_CHARSET,
        DEFAULT_INTERNAL_CHARSET);
    return conv;
}
#endif
