/*MT*

    MediaTomb - http://www.mediatomb.cc/

    import_script.cc - this file is part of MediaTomb.

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

/// \file import_script.cc

#ifdef HAVE_JS
#define LOG_FAC log_facility_t::script
#include "import_script.h" // API

#include "cds/cds_objects.h"
#include "content/content_manager.h"
#include "js_functions.h"
#include "util/string_converter.h"

ImportScript::ImportScript(const std::shared_ptr<ContentManager>& content, const std::string& parent)
    : Script(content, parent, "import", "orig", StringConverter::i2i(content->getContext()->getConfig()))
{
    std::string scriptPath = config->getOption(CFG_IMPORT_SCRIPTING_IMPORT_SCRIPT);
    load(scriptPath);
}

void ImportScript::processCdsObject(const std::shared_ptr<CdsObject>& obj, const std::string& scriptPath, const std::map<AutoscanMediaMode, std::string>& containerMap)
{
    processed = obj;
    try {
        execute(obj, scriptPath);
    } catch (const std::runtime_error&) {
        cleanup();
        processed = nullptr;
        throw;
    }

    processed = nullptr;

    gc_counter++;
    if (gc_counter > JS_CALL_GC_AFTER_NUM) {
        duk_gc(ctx, 0);
        gc_counter = 0;
    }
}

bool ImportScript::setRefId(const std::shared_ptr<CdsObject>& cdsObj, const std::shared_ptr<CdsObject>& origObject, int pcdId)
{
    if (!cdsObj->isExternalItem()) {
        cdsObj->setRefID(origObject->getID());
        cdsObj->setFlag(OBJECT_FLAG_USE_RESOURCE_REF);
    }
    return true;
}
#endif // HAVE_JS
