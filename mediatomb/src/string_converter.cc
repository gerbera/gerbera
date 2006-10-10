/*  string_converter.cc - this file is part of MediaTomb.
                                                                                
    Copyright (C) 2005 Gena Batyan <bgeradz@deadlock.dhs.org>,
                       Sergey Bostandzhyan <jin@deadlock.dhs.org>
                                                                                
    MediaTomb is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.
                                                                                
    MediaTomb is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.
                                                                                
    You should have received a copy of the GNU General Public License
    along with MediaTomb; if not, write to the Free Software
    Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
*/

/// \file string_converter.cc

#ifdef HAVE_CONFIG_H
    #include "autoconfig.h"
#endif

#include "string_converter.h"
#include "config_manager.h"

using namespace zmm;

#ifdef LIBICONV_NAMING
    #define iconv_open libiconv_open
    #define iconv_close libiconv_close
    #define iconv libiconv
#endif

StringConverter::StringConverter(String from, String to) : Object()
{
    dirty = false;

    cd = iconv_open(to.c_str(), from.c_str());
    if (cd == (iconv_t)(-1))
    {
        cd = (iconv_t)(0);
        throw _Exception(_("iconv: ") + strerror(errno));
    }
}

StringConverter::~StringConverter()
{
    if (cd != (iconv_t)(0))
        iconv_close(cd);
}

zmm::String StringConverter::convert(String str)
{
    String ret_str;

    int buf_size = str.length() * 4;

    char *input = str.c_str();
    char *output = (char *)MALLOC(buf_size);
    if (!output)
    {
        log_debug("Could not allocate memory for string conversion!\n");
        throw _Exception(_("Could not allocate memory for string conversion!"));
    }

    char *input_copy = input;
    char *output_copy = output;
    
    char **input_ptr = &input_copy;
    char **output_ptr = &output_copy;
    
    size_t input_bytes = (size_t)str.length();
    size_t output_bytes = (size_t)buf_size;

    int ret;
  
    // reset to initial state
    if (dirty)
    {
        iconv(cd, NULL, 0, NULL, 0);
        dirty = false;
    }
    
    //log_debug(("iconv: BEFORE: input bytes left: %d  output bytes left: %d\n",
    //       input_bytes, output_bytes));
#ifdef __FreeBSD__
    ret = iconv(cd, (const char**)input_ptr, &input_bytes,
            output_ptr, &output_bytes);
#else
    ret = iconv(cd, input_ptr, &input_bytes,
            output_ptr, &output_bytes);
#endif

    if (ret == -1)
    {
        log_error("iconv: %s\n", strerror(errno));
        String err;
        switch (errno)
        {
            case EILSEQ:
                log_error("%s could not be converted to new encoding: invalid character sequence!\n", str.c_str());
                err = _("iconv: Invalid character sequence");
                ret_str = String(output, output_copy - output);
                if (ret_str == nil)
                    ret_str = _("");

                    // pad rest with "?"
                    for (int i = ret_str.length(); i < str.length(); i++)
                    {
                        ret_str = ret_str + _("?");
                    }
                    FREE(output);
                    return ret_str;
                break;
            case EINVAL:
                err = _("iconv: Incomplete multibyte sequence");
                break;
            case E2BIG:
                /// \todo should encode the whole string anyway
                err = _("iconv: Insufficient space in output buffer");
                break;
            default:
                err = _("iconv: ") + strerror(errno);
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

    ret_str = String(output, output_copy - output);
    FREE(output);
    
    return ret_str;
}

String StringConverter::validSubstring(String str, String encoding)
{
    /// \todo: validate string
    return str;
}

/// \todo iconv caching
Ref<StringConverter> StringConverter::i2f()
{
    Ref<StringConverter> conv(new StringConverter(
        _(DEFAULT_INTERNAL_CHARSET), ConfigManager::getInstance()->getOption(_("/import/filesystem-charset"))));
//        INTERNAL_CHARSET, ConfigManager::getInstance()->getFilesystemCharset()));
    return conv;
}
Ref<StringConverter> StringConverter::f2i()
{
    Ref<StringConverter> conv(new StringConverter(
        ConfigManager::getInstance()->getOption(_("/import/filesystem-charset")),
                                                _(DEFAULT_INTERNAL_CHARSET)));
    return conv;
}
Ref<StringConverter> StringConverter::m2i()
{
    Ref<StringConverter> conv(new StringConverter(
        ConfigManager::getInstance()->getOption(_("/import/metadata-charset")),
                                                _(DEFAULT_INTERNAL_CHARSET)));
    return conv;
}
