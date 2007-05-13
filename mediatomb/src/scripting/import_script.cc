/*MT*
    
    MediaTomb - http://www.mediatomb.cc/
    
    import_script.cc - this file is part of MediaTomb.
    
    Copyright (C) 2005 Gena Batyan <bgeradz@mediatomb.cc>,
                       Sergey 'Jin' Bostandzhyan <jin@mediatomb.cc>
    
    Copyright (C) 2006-2007 Gena Batyan <bgeradz@mediatomb.cc>,
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

/// \file import_script.cc

#ifdef HAVE_CONFIG_H
    #include "autoconfig.h"
#endif

#ifdef HAVE_JS

#include "import_script.h"
#include "config_manager.h"
#include "js_functions.h"

using namespace zmm;

static JSFunctionSpec global_functions[] = {
    {"addCdsObject",    js_addCdsObject,   3},
    {"copyObject",      js_copyObject,     1},
    {0}
};

ImportScript::ImportScript(Ref<Runtime> runtime) : Script(runtime)
{
    defineFunctions(global_functions);
    
    String scriptPath = ConfigManager::getInstance()->getOption(_("/import/scripting/virtual-layout/import-script")); 
    load(scriptPath);
}

void ImportScript::processCdsObject(Ref<CdsObject> obj)
{
   JSObject *orig = JS_NewObject(cx, NULL, NULL, glob);
   setObjectProperty(glob, _("orig"), orig);
   cdsObject2jsObject(obj, orig);
   execute();
}

#endif // HAVE_JS

