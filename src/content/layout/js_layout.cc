/*MT*
    
    MediaTomb - http://www.mediatomb.cc/
    
    js_layout.cc - this file is part of MediaTomb.
    
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

/// \file js_layout.cc

#ifdef HAVE_JS
#include "js_layout.h" // API

#include "content/layout/layout.h" // for Layout
#include "content/scripting/import_script.h" // for ImportScript

class CdsObject;

JSLayout::JSLayout(const std::shared_ptr<ContentManager>& content,
    const std::shared_ptr<ScriptingRuntime>& runtime)
    : Layout(content)
    , import_script(std::make_unique<ImportScript>(content, runtime))
{
}

JSLayout::~JSLayout() = default;

void JSLayout::processCdsObject(std::shared_ptr<CdsObject> obj, fs::path rootpath)
{
    if (import_script == nullptr)
        return;

    import_script->processCdsObject(obj, rootpath);
}

#endif // HAVE_JS
