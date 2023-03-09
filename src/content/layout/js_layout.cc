/*MT*

    MediaTomb - http://www.mediatomb.cc/

    js_layout.cc - this file is part of MediaTomb.

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

/// \file js_layout.cc
#define LOG_FAC log_facility_t::layout

#ifdef HAVE_JS
#include "js_layout.h" // API

#include "content/scripting/import_script.h"

JSLayout::JSLayout(const std::shared_ptr<ContentManager>& content, const std::string& parent)
    : Layout(content)
    , import_script(std::make_unique<ImportScript>(content, parent))
{
}

void JSLayout::processCdsObject(const std::shared_ptr<CdsObject>& obj, const fs::path& rootpath, const std::string& contentType, const std::map<AutoscanMediaMode, std::string>& containerMap)
{
    if (!import_script)
        return;

    import_script->processCdsObject(obj, rootpath, containerMap);
}

#endif // HAVE_JS
