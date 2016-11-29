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

#include "js_layout.h"
#include "scripting/runtime.h"

using namespace zmm;

JSLayout::JSLayout() : Layout()
{
    import_script = Ref<ImportScript>(new ImportScript(Runtime::getInstance()));
}

JSLayout::~JSLayout()
{
}

void JSLayout::processCdsObject(Ref<CdsObject> obj, String rootpath)
{
    if (import_script == nil)
        return;

    import_script->processCdsObject(obj, rootpath);
}


#endif // HAVE_JS
