/*MT*

    MediaTomb - http://www.mediatomb.cc/

    playlist_parser_script.cc - this file is part of MediaTomb.

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

/// \file playlist_parser_script.cc

#ifdef HAVE_JS
#include "playlist_parser_script.h" // API

#include "config/config_manager.h"
#include "content/content_manager.h"
#include "database/database.h"
#include "js_functions.h"
#include "scripting_runtime.h"
#include "util/string_converter.h"

#define ONE_TEXTLINE_BYTES 1024

script_class_t PlaylistParserScript::whoami()
{
    return S_PLAYLIST;
}

extern "C" {

static duk_ret_t jsReadXml(duk_context* ctx)
{
    auto self = dynamic_cast<PlaylistParserScript*>(Script::getContextScript(ctx));
    if (!self) {
        return 0;
    }

    if (!duk_is_number(ctx, 0))
        return 0;

    int direction = duk_to_int(ctx, 0);
    duk_pop(ctx);

    try {
        auto node = self->readXml(direction);
        if (!node) {
            log_debug("No matching node {} found.", direction);
            return 0;
        }
        duk_push_object(ctx);
        for (auto&& attrib : node.attributes()) {
            duk_push_string(ctx, attrib.value());
            duk_put_prop_string(ctx, -2, toLower(attrib.name()).c_str());
        }

        duk_push_string(ctx, node.text().as_string());
        duk_put_prop_string(ctx, -2, "VALUE");
        duk_push_string(ctx, toLower(node.name()).c_str());
        duk_put_prop_string(ctx, -2, "NAME");
    } catch (const ServerShutdownException&) {
        log_warning("Aborting script execution due to server shutdown.");
        return duk_error(ctx, DUK_ERR_ERROR, "Aborting script execution due to server shutdown.");
    } catch (const std::runtime_error& e) {
        log_error("DUK exception: {}", e.what());
        return 0;
    }

    return 1;
}

static duk_ret_t jsReadln(duk_context* ctx)
{
    auto self = dynamic_cast<PlaylistParserScript*>(Script::getContextScript(ctx));
    if (!self) {
        return 0;
    }

    try {
        auto line = self->readLine();
        duk_push_string(ctx, line.c_str());
    } catch (const ServerShutdownException&) {
        log_warning("Aborting script execution due to server shutdown.");
        return duk_error(ctx, DUK_ERR_ERROR, "Aborting script execution due to server shutdown.");
    } catch (const std::runtime_error& e) {
        log_error("DUK exception: {}", e.what());
        return 0;
    }

    return 1;
}

static duk_ret_t jsGetCdsObject(duk_context* ctx)
{
    auto self = dynamic_cast<PlaylistParserScript*>(Script::getContextScript(ctx));
    if (!self) {
        return 0;
    }

    if (!duk_is_string(ctx, 0))
        return 0;

    std::error_code ec;
    fs::path path = fs::weakly_canonical(duk_to_string(ctx, 0), ec);
    duk_pop(ctx);

    if (ec || path.empty())
        return 0;

    auto database = self->getDatabase();
    auto obj = database->findObjectByPath(path);
    if (!obj) {
        auto cm = self->getContent();
        ec.clear();
        auto dirEnt = fs::directory_entry(path, ec);
        if (!ec) {
            obj = cm->createObjectFromFile(dirEnt, false);
        } else {
            log_error("Failed to read {}: {}", path.c_str(), ec.message());
        }
        if (!obj) { // object ignored
            return 0;
        }
    }
    self->cdsObject2dukObject(obj);
    return 1;
}

} // extern "C"

PlaylistParserScript::PlaylistParserScript(const std::shared_ptr<ContentManager>& content,
    const std::shared_ptr<ScriptingRuntime>& runtime)
    : Script(content, runtime, "playlist", "playlist", StringConverter::p2i(content->getContext()->getConfig()))
{
    ScriptingRuntime::AutoLock lock(runtime->getMutex());
    defineFunction("readln", jsReadln, 0);
    defineFunction("readXml", jsReadXml, 1);
    defineFunction("getCdsObject", jsGetCdsObject, 1);

    std::string scriptPath = config->getOption(CFG_IMPORT_SCRIPTING_PLAYLIST_SCRIPT);
    load(scriptPath);
}

static pugi::xml_node nullNode;
pugi::xml_node& PlaylistParserScript::readXml(int direction)
{
    if (!root)
        throw_std_runtime_error("readXml not yet setup for use");

    if (currentTask && !currentTask->isValid())
        return root;

    if (direction > 0 && root.first_child()) {
        root = root.first_child();
        return root;
    }
    if (direction < -1 && root.root()) {
        root = xmlDoc->document_element();
        return root;
    }
    if (direction < 0 && root.parent()) {
        root = root.parent();
        return root;
    }
    if (root.next_sibling()) {
        root = root.next_sibling();
        return root;
    }
    return nullNode;
}

std::string PlaylistParserScript::readLine()
{
    if (!currentHandle)
        throw_std_runtime_error("readLine not yet setup for use");

    if (currentTask && !currentTask->isValid())
        return {};

    while (true) {
        if (!fgets(currentLine, ONE_TEXTLINE_BYTES, currentHandle))
            return {};

        auto ret = trimString(currentLine);
        if (!ret.empty())
            return ret;
    }
}

void PlaylistParserScript::processPlaylistObject(const std::shared_ptr<CdsObject>& obj, std::shared_ptr<GenericTask> task, const std::string& scriptPath)
{
    if ((currentObjectID != INVALID_OBJECT_ID) || currentHandle || currentLine) {
        throw_std_runtime_error("Recursion in playlists not allowed");
    }

    if (!obj->isPureItem()) {
        throw_std_runtime_error("Calling processPlaylistObject only allowed for pure items");
    }
    auto item = std::static_pointer_cast<CdsItem>(obj);

    log_debug("Checking playlist {} ...", obj->getLocation().string());
    if (item->getMimeType() != MIME_TYPE_ASX_PLAYLIST) {
        GrbFile file(item->getLocation());
        currentHandle = file.open("r");
    } else {
        pugi::xml_parse_result result = xmlDoc->load_file(item->getLocation().c_str());
        if (result.status != pugi::xml_parse_status::status_ok) {
            throw ConfigParseException(result.description());
        }
        root = xmlDoc->document_element();
    }

    currentTask = std::move(task);
    currentObjectID = item->getID();
    currentLine = new char[ONE_TEXTLINE_BYTES];
    currentLine[0] = '\0';

    ScriptingRuntime::AutoLock lock(runtime->getMutex());
    try {
        execute(obj, scriptPath);
    } catch (const std::runtime_error&) {
        cleanup();
        duk_pop(ctx);

        currentHandle = nullptr;

        delete[] currentLine;
        currentLine = nullptr;

        currentObjectID = INVALID_OBJECT_ID;
        currentTask = nullptr;

        throw;
    }

    currentHandle = nullptr;

    delete[] currentLine;
    currentLine = nullptr;
    xmlDoc->reset();
    root = nullNode;

    currentObjectID = INVALID_OBJECT_ID;
    currentTask = nullptr;

    ++gc_counter;
    if (gc_counter > JS_CALL_GC_AFTER_NUM) {
        duk_gc(ctx, 0);
        gc_counter = 0;
    }
}
#endif // HAVE_JS
