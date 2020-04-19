/*GRB*

    Gerbera - https://gerbera.io/
    
    upnp_quirks.h - this file is part of Gerbera.
    
    Copyright (C) 2020 Gerbera Contributors
    
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

/// \file upnp_quirks.h

#ifndef __UPNP_QUIRKS_H__
#define __UPNP_QUIRKS_H__

#include <filesystem>
#include <memory>
#include <pugixml.hpp>
namespace fs = std::filesystem;

typedef uint32_t QuirkFlags;

#define QUIRK_FLAG_NONE 0x00000000
#define QUIRK_FLAG_SAMSUNG 0x00000001

// forward declaration
class Config;
class CdsItem;
class Headers;
struct ClientInfo;

class Quirks {
public:
    Quirks(std::shared_ptr<Config> config, const struct sockaddr_storage* addr, const std::string& userAgent);

    // Look for subtitle file and returns it's URL in CaptionInfo.sec response header.
    // To be more compliant with original Samsung server we should check for getCaptionInfo.sec: 1 request header.
    void addCaptionInfo(std::shared_ptr<CdsItem> item, std::unique_ptr<Headers>& headers);

    // add additional headers in DIDL-Lite when doing doBrowse or doSearch
    // currentlly used for Samsung
    void appendSpecialNamespace(pugi::xml_node* parent);

private:
    std::shared_ptr<Config> config;
    const ClientInfo* pClientInfo;
};

#endif // __UPNP_QUIRKS_H__
