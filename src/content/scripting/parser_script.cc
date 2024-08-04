/*MT*

    MediaTomb - http://www.mediatomb.cc/

    parser_script.cc - this file is part of MediaTomb.

    Copyright (C) 2005 Gena Batyan <bgeradz@mediatomb.cc>,
                       Sergey 'Jin' Bostandzhyan <jin@mediatomb.cc>

    Copyright (C) 2006-2010 Gena Batyan <bgeradz@mediatomb.cc>,
                            Sergey 'Jin' Bostandzhyan <jin@mediatomb.cc>,
                            Leonhard Wimmer <leo@mediatomb.cc>

    Copyright (C) 2016-2024 Gerbera Contributors

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

/// \file parser_script.cc

#ifdef HAVE_JS
#define GRB_LOG_FAC GrbLogFacility::script

#include "parser_script.h" // API

#include "content/content.h"
#include "context.h"
#include "database/database.h"
#include "exceptions.h"
#include "util/generic_task.h"
#include "util/string_converter.h"
#include "util/tools.h"

duk_ret_t jsReadXml(duk_context* ctx)
{
    auto self = dynamic_cast<ParserScript*>(Script::getContextScript(ctx));
    if (!self) {
        log_error("readXml can only be called for ParserScript.");
        return 0;
    }

    if (!duk_is_number(ctx, 0)) {
        log_error("readXml must be called with integer argument.");
        return 0;
    }

    int direction = duk_to_int(ctx, 0);
    duk_pop(ctx);

    log_debug("jsReadXml {}.", direction);
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
            log_debug("{} {}={} found", node.name(), attrib.name(), attrib.value());
        }

        duk_push_string(ctx, node.text().as_string());
        duk_put_prop_string(ctx, -2, "VALUE");
        duk_push_string(ctx, toLower(node.name()).c_str());
        duk_put_prop_string(ctx, -2, "NAME");
        log_debug("{}={} found", node.name(), node.text().as_string());
    } catch (const ServerShutdownException&) {
        log_warning("Aborting script execution due to server shutdown.");
        return duk_error(ctx, DUK_ERR_ERROR, "Aborting script execution due to server shutdown.");
    } catch (const std::runtime_error& e) {
        log_error("DUK exception: {}", e.what());
        return 0;
    }

    return 1;
}

duk_ret_t jsReadln(duk_context* ctx)
{
    auto self = dynamic_cast<ParserScript*>(Script::getContextScript(ctx));
    if (!self) {
        log_error("readln can only be called for ParserScript.");
        return 0;
    }

    try {
        auto [line, trimmed] = self->readLine();
        duk_push_string(ctx, line.c_str());
        return trimmed ? 1 : 0;
    } catch (const ServerShutdownException&) {
        log_warning("Aborting script execution due to server shutdown.");
        return duk_error(ctx, DUK_ERR_ERROR, "Aborting script execution due to server shutdown.");
    } catch (const std::runtime_error& e) {
        log_error("DUK exception: {}", e.what());
        return 0;
    }
}

duk_ret_t jsGetCdsObject(duk_context* ctx)
{
    auto self = dynamic_cast<ParserScript*>(Script::getContextScript(ctx));
    if (!self) {
        log_error("getCdsObject can only be called for ParserScript.");
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
            auto adir = cm->findAutoscanDirectory(path);
            obj = cm->createObjectFromFile(adir, dirEnt, false);
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

duk_ret_t jsUpdateCdsObject(duk_context* ctx)
{
    auto self = dynamic_cast<Script*>(Script::getContextScript(ctx));
    if (!self) {
        log_error("updateCdsObject can only be called for MetafileParserScript.");
        return 0;
    }

    if (duk_is_undefined(ctx, -1)) {
        log_debug("Could not retrieve {} object", self->getOrigName());
        return 0;
    }
    auto origObject = self->dukObject2cdsObject(self->getProcessedObject());
    if (!origObject) {
        log_debug("Could not load {} object", self->getOrigName());
        return 0;
    }
    return 1;
}

ParserScript::ParserScript(const std::shared_ptr<Content>& content, const std::string& parent,
    const std::string& name, const std::string& objName)
    : Script(content, parent, name, objName, content->getContext()->getConverterManager()->p2i())
{
    defineFunction("readln", jsReadln, 0);
    defineFunction("readXml", jsReadXml, 1);
    defineFunction("getCdsObject", jsGetCdsObject, 1);
}

pugi::xml_node ParserScript::nullNode;

pugi::xml_node& ParserScript::readXml(int direction)
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

std::pair<std::string, bool> ParserScript::readLine()
{
    if (!currentHandle)
        throw_std_runtime_error("readLine not yet setup for use");

    if (currentTask && !currentTask->isValid())
        return { {}, false };

    while (true) {
        log_debug("Scanning {}", currentLine);
        if (!fgets(currentLine, ONE_TEXTLINE_BYTES, currentHandle))
            return { {}, false };
        auto ret = trimString(currentLine);
        if (!ret.empty())
            return { ret, true };
    }
}

#endif // HAVE_JS
