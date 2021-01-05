/*GRB*

    Gerbera - https://gerbera.io/
    
    mime.h - this file is part of Gerbera.
    
    Copyright (C) 2021 Gerbera Contributors
    
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

#include <filesystem>
#include <map>
#include <memory>
#include <string>
namespace fs = std::filesystem;

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
    Mime(std::shared_ptr<Config> config);
    virtual ~Mime();

#ifdef HAVE_MAGIC
    /// \brief Extracts mimetype from a file using filemagic
    std::string fileToMimeType(const fs::path& path, const std::string& defval = "");

    /// \brief Extracts mimetype from a buffer using filemagic
    std::string bufferToMimeType(const void* buffer, size_t length);
#endif // HAVE_MAGIC

    std::string extensionToMimeType(const fs::path& path);
    std::string mimeTypeToUpnpClass(const std::string& mimeType);

private:
    bool extension_map_case_sensitive;

    std::map<std::string, std::string> extension_mimetype_map;
    std::map<std::string, std::string> mimetype_upnpclass_map;

#ifdef HAVE_MAGIC
    magic_t magicCookie;
#endif
};

#endif // __MIME_H__
