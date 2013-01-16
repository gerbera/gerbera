/*MT*
    
    MediaTomb - http://www.mediatomb.cc/
    
    import_script.h - this file is part of MediaTomb.
    
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

/// \file import_script.h

#ifndef __SCRIPTING_IMPORT_SCRIPT_H__
#define __SCRIPTING_IMPORT_SCRIPT_H__

#include "common.h"
#include "script.h"
#include "cds_objects.h"
#include "layout/layout.h"

class ImportScript : public Script
{
public:
    ImportScript(zmm::Ref<Runtime> runtime);
    ~ImportScript();
    void processCdsObject(zmm::Ref<CdsObject> obj, zmm::String rootpath);
    virtual script_class_t whoami() { return S_IMPORT; }
private:
    JSObject *root;
};

#endif // __SCRIPTING_IMPORT_SCRIPT_H__
