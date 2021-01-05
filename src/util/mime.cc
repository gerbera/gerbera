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

/// \file mime.cc

#include "mime.h" // API

#include "config/config_manager.h"
#include "util/tools.h"

Mime::Mime(std::shared_ptr<Config> config)
//    : config(std::move(config))
{
    extension_map_case_sensitive = config->getBoolOption(CFG_IMPORT_MAPPINGS_EXTENSION_TO_MIMETYPE_CASE_SENSITIVE);
    extension_mimetype_map = config->getDictionaryOption(CFG_IMPORT_MAPPINGS_EXTENSION_TO_MIMETYPE_LIST);
    mimetype_upnpclass_map = config->getDictionaryOption(CFG_IMPORT_MAPPINGS_MIMETYPE_TO_UPNP_CLASS_LIST);

#ifdef HAVE_MAGIC
    // init filemagic
    int magicFlags = config->getBoolOption(CFG_IMPORT_FOLLOW_SYMLINKS) ? MAGIC_MIME_TYPE | MAGIC_SYMLINK : MAGIC_MIME_TYPE;
    magicCookie = magic_open(magicFlags);
    if (magicCookie == nullptr) {
        throw_std_runtime_error("magic_open failed");
    }

    std::string optMagicFile = config->getOption(CFG_IMPORT_MAGIC_FILE);
    const char* magicFile = !optMagicFile.empty() ? optMagicFile.c_str() : nullptr;
    if (magic_load(magicCookie, magicFile) == -1) {
        std::string errMsg = std::string(magic_error(magicCookie));
        magic_close(magicCookie);
        magicCookie = nullptr;
        throw_std_runtime_error("magic_load failed: " + errMsg);
    }
#endif // HAVE_MAGIC
}

Mime::~Mime()
{
    if (magicCookie) {
        magic_close(magicCookie);
        magicCookie = nullptr;
    }
}

#ifdef HAVE_MAGIC
std::string Mime::fileToMimeType(const fs::path& path, const std::string& defval)
{
    const char* mimeType = magic_file(magicCookie, path.c_str());
    if (mimeType[0] == '\0') {
        return defval;
    }

    return mimeType;
}

std::string Mime::bufferToMimeType(const void* buffer, size_t length)
{
    const char* mimeType = magic_buffer(magicCookie, buffer, length);
    return mimeType;
}
#endif

std::string Mime::extensionToMimeType(const fs::path& path)
{
    std::string extension = path.extension();
    if (!extension.empty())
        extension.erase(0, 1); // remove leading .

    if (!extension_map_case_sensitive)
        extension = toLower(extension);

    return getValueOrDefault(extension_mimetype_map, extension);
}

std::string Mime::mimeTypeToUpnpClass(const std::string& mimeType)
{
    auto it = mimetype_upnpclass_map.find(mimeType);
    if (it != mimetype_upnpclass_map.end())
        return it->second;

    // try to match foo
    std::vector<std::string> parts = splitString(mimeType, '/');
    if (parts.size() != 2)
        return "";
    return getValueOrDefault(mimetype_upnpclass_map, parts[0] + "/*");
}
