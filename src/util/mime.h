/*GRB*

    Gerbera - https://gerbera.io/

    mime.h - this file is part of Gerbera.

    Copyright (C) 2021-2026 Gerbera Contributors

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

/// @file util/mime.h

#ifndef __MIME_H__
#define __MIME_H__

#include "util/grb_fs.h"

#include <map>

#ifdef HAVE_MAGIC
#include <mutex>
// for older versions of filemagic
extern "C" {
#include <magic.h>
}
#endif

// forward declaration
class Config;

// @brief class encapsulating handling of file types
class Mime {
public:
    explicit Mime(const std::shared_ptr<Config>& config);
#ifdef HAVE_MAGIC
    virtual ~Mime();

    Mime(const Mime&) = delete;
    Mime& operator=(const Mime&) = delete;

    /// @brief Extracts mimetype from a buffer using filemagic
    std::string bufferToMimeType(const void* buffer, std::size_t length);
#endif // HAVE_MAGIC

    // @brief get mime type for file
    std::pair<bool, std::string> getMimeType(const fs::path& path, const std::string& defval = "");

    // @brief get extension not mappable to mime types
    std::vector<std::string> getUnhandledExtensions() const { return unhandledExtensions; }

private:
    // @brief cache configuration value extension-mimetype case-sensitive
    bool extension_map_case_sensitive;

    // @brief cache configuration value extension-mimetype ignore-unknown
    bool ignore_unknown_extensions;

    // @brief cache configuration value extension-mimetype
    std::map<std::string, std::string> extension_mimetype_map;

    // @brief cache configuration value ignore-extensions
    std::vector<std::string> ignoredExtensions;

    // @brief collect extensions with no mime type
    std::vector<std::string> unhandledExtensions;

#ifdef HAVE_MAGIC
    // @brief magic cookie file descriptor
    magic_t magicCookie;

    /// @brief Extracts mimetype from a file using filemagic
    std::string fileToMimeType(const fs::path& path, const std::string& defval = "");

    // @brief magic thread mutex
    std::mutex mime_mutex;
#endif
};

#endif // __MIME_H__
