/*GRB*

    Gerbera - https://gerbera.io/

    mime.h - this file is part of Gerbera.

    Copyright (C) 2021-2024 Gerbera Contributors

    Gerbera is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License version 2
    as published by the Free Software Foundation.

    Gerbera is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with Gerbera.  If not, see <http://www.gnu.org/licenses/>.

    $Id$
*/

/// \file mime.h

#ifndef __MIME_H__
#define __MIME_H__

#include <map>

#include "util/grb_fs.h"

#ifdef HAVE_MAGIC
// for older versions of filemagic
extern "C" {
#include <magic.h>
}
#endif

// forward declaration
class Config;

class Mime {
public:
    explicit Mime(const std::shared_ptr<Config>& config);
#ifdef HAVE_MAGIC
    virtual ~Mime();

    Mime(const Mime&) = delete;
    Mime& operator=(const Mime&) = delete;

    /// \brief Extracts mimetype from a buffer using filemagic
    std::string bufferToMimeType(const void* buffer, std::size_t length);
#endif // HAVE_MAGIC

    std::string getMimeType(const fs::path& path, const std::string& defval = "");

private:
    bool extension_map_case_sensitive;
    bool ignore_unknown_extensions;

    std::map<std::string, std::string> extension_mimetype_map;
    std::vector<std::string> ignoredExtensions;

#ifdef HAVE_MAGIC
    magic_t magicCookie;

    /// \brief Extracts mimetype from a file using filemagic
    std::string fileToMimeType(const fs::path& path, const std::string& defval = "");
#endif
};

#endif // __MIME_H__
