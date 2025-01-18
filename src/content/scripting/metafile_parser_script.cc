/*MT*

    MediaTomb - http://www.mediatomb.cc/

    metafile_parser_script.cc - this file is part of MediaTomb.

    Copyright (C) 2005 Gena Batyan <bgeradz@mediatomb.cc>,
                       Sergey 'Jin' Bostandzhyan <jin@mediatomb.cc>

    Copyright (C) 2006-2010 Gena Batyan <bgeradz@mediatomb.cc>,
                            Sergey 'Jin' Bostandzhyan <jin@mediatomb.cc>,
                            Leonhard Wimmer <leo@mediatomb.cc>

    Copyright (C) 2016-2025 Gerbera Contributors

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

/// \file metafile_parser_script.cc

#ifdef HAVE_JS
#define GRB_LOG_FAC GrbLogFacility::script

#include "metafile_parser_script.h" // API

#include "cds/cds_item.h"
#include "config/config.h"
#include "config/config_val.h"
#include "exceptions.h"
#include "script_property.h"
#include "scripting_runtime.h"
#include "util/string_converter.h"

MetafileParserScript::MetafileParserScript(const std::shared_ptr<Content>& content, const std::string& parent)
    : ParserScript(content, parent, "metafile", "obj", false)
{
    std::string scriptPath = config->getOption(ConfigVal::IMPORT_SCRIPTING_METAFILE_SCRIPT);
    defineFunction("updateCdsObject", jsUpdateCdsObject, 1);
    if (!scriptPath.empty()) {
        load(scriptPath);
        scriptMode = true;
    } else {
        metafileFunction = config->getOption(ConfigVal::IMPORT_SCRIPTING_IMPORT_FUNCTION_METAFILE);
    }
}

void MetafileParserScript::processObject(const std::shared_ptr<CdsObject>& obj, const fs::path& path)
{
    if (currentObjectID != INVALID_OBJECT_ID || currentHandle || currentLine) {
        throw_std_runtime_error("Recursion in metafiles is not allowed");
    }

    if (!obj->isPureItem()) {
        throw_std_runtime_error("Calling processObject only allowed for pure items");
    }
    auto item = std::static_pointer_cast<CdsItem>(obj);

    log_debug("Checking metafile {} for {}...", path.string(), obj->getLocation().string());
    GrbFile file(path);
    pugi::xml_parse_result result = xmlDoc->load_file(path.c_str());
    if (result.status == pugi::xml_parse_status::status_ok) {
        root = xmlDoc->document_element();
    }
    currentHandle = file.open("r");
    processed = obj;

    currentObjectID = item->getID();
    currentLine = new char[ONE_TEXTLINE_BYTES];
    currentLine[0] = '\0';

    ScriptingRuntime::AutoLock lock(runtime->getMutex());
    try {
        if (scriptMode) {
            execute(obj, path);
        } else {
            call(obj, nullptr, metafileFunction, path, "");
        }
    } catch (const std::runtime_error&) {
        if (scriptMode) {
            cleanup();
            duk_pop(ctx);
        }

        currentHandle = nullptr;

        delete[] currentLine;
        currentLine = nullptr;

        currentObjectID = INVALID_OBJECT_ID;

        throw;
    }

    currentHandle = nullptr;

    log_debug("Done metafile {} ...", path.string());

    delete[] currentLine;
    currentLine = nullptr;
    xmlDoc->reset();
    root = nullNode;

    currentObjectID = INVALID_OBJECT_ID;

    gc_counter++;
    if (gc_counter > JS_CALL_GC_AFTER_NUM) {
        duk_gc(ctx, 0);
        gc_counter = 0;
    }
}

std::pair<std::shared_ptr<CdsObject>, int> MetafileParserScript::createObject2cdsObject(const std::shared_ptr<CdsObject>& origObject, const std::string& rootPath)
{
    return { {}, INVALID_OBJECT_ID };
}

bool MetafileParserScript::setRefId(const std::shared_ptr<CdsObject>& cdsObj, const std::shared_ptr<CdsObject>& origObject, int pcdId)
{
    if (!cdsObj->isExternalItem()) {
        cdsObj->setRefID(origObject->getID());
        cdsObj->setFlag(OBJECT_FLAG_USE_RESOURCE_REF);
    }
    return true;
}

void MetafileParserScript::handleObject2cdsItem(duk_context* ctx, const std::shared_ptr<CdsObject>& pcd, const std::shared_ptr<CdsItem>& item)
{
    item->setTrackNumber(ScriptNamedProperty(ctx, "trackNumber").getIntValue(0));
    item->setPartNumber(ScriptNamedProperty(ctx, "partNumber").getIntValue(0));
}

std::shared_ptr<CdsObject> MetafileParserScript::createObject(const std::shared_ptr<CdsObject>& pcd)
{
    ScriptNamedProperty(ctx, "aux").getObject([&]() {
        auto keys = ScriptProperty(ctx).getPropertyNames();
        for (auto&& sym : keys) {
            auto val = ScriptNamedProperty(ctx, sym).getStringValue();
            if (!val.empty()) {
                auto [mval, err] = sc->convert(val);
                if (!err.empty()) {
                    log_warning("{}: {}", sym, err);
                }
                pcd->setAuxData(sym, mval);
            }
        }
    });
    return pcd;
}

#endif // HAVE_JS
