/*MT*

    MediaTomb - http://www.mediatomb.cc/

    web/web_request_handler.cc - this file is part of MediaTomb.

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

/// \file web/web_request_handler.cc
#define GRB_LOG_FAC GrbLogFacility::web

#include "web_request_handler.h" // API

#include "cds/cds_enums.h"
#include "common.h"
#include "config/config.h"
#include "config/config_setup.h"
#include "config/config_val.h"
#include "content/content.h"
#include "context.h"
#include "exceptions.h"
#include "iohandler/mem_io_handler.h"
#include "server.h"
#include "upnp/compat.h"
#include "upnp/headers.h"
#include "upnp/quirks.h"
#include "util/url_utils.h"
#include "util/xml_to_json.h"
#include "web/pages.h"
#include "web/session_manager.h"

namespace Web {

WebRequestHandler::WebRequestHandler(const std::shared_ptr<Content>& content,
    std::shared_ptr<Server> server,
    const std::shared_ptr<UpnpXMLBuilder>& xmlBuilder,
    const std::shared_ptr<Quirks>& quirks)
    : RequestHandler(content, xmlBuilder, quirks)
    , sessionManager(this->content->getContext()->getSessionManager())
    , server(std::move(server))
{
}

std::string WebRequestHandler::getGroup() const
{
    return quirks ? quirks->getGroup() : DEFAULT_CLIENT_GROUP;
}

bool WebRequestHandler::getInfo(const char* filename, UpnpFileInfo* info)
{
    this->filename = filename;
    auto&& parameters = URLUtils::getQuery(this->filename);
    auto decodedParams = URLUtils::dictDecode(parameters);
    if (params.empty()) {
        params = std::move(decodedParams);
    } else {
        params.merge(decodedParams);
    }

    UpnpFileInfo_set_FileLength(info, -1); // length is unknown

    UpnpFileInfo_set_LastModified(info, currentTime().count()); // always use current time
    UpnpFileInfo_set_IsDirectory(info, 0);
    UpnpFileInfo_set_IsReadable(info, 1);

    std::string contentType = "application/json; charset=UTF-8";

    Headers headers;
    headers.addHeader("Cache-Control", "no-cache, must-revalidate");
    headers.addHeader("SameSite", "Lax");
    GrbUpnpFileInfoSetContentType(info, contentType);
#ifdef UPNP_NEEDS_CORS
    headers.addHeader("Access-Control-Allow-Origin", fmt::format("{}", fmt::join(server->getCorsHosts(), " ")));
#endif

    if (quirks)
        quirks->updateHeaders(headers);
    headers.writeHeaders(info);
    return quirks && quirks->getClient();
}

std::unique_ptr<IOHandler> WebRequestHandler::open(const char* filename, const std::shared_ptr<Quirks>& quirks, enum UpnpOpenFileMode mode)
{
    this->filename = filename;
    auto&& parameters = URLUtils::getQuery(this->filename);
    auto decodedParams = URLUtils::dictDecode(parameters);
    if (params.empty()) {
        params = std::move(decodedParams);
    } else {
        params.merge(decodedParams);
    }

    xmlDoc = std::make_unique<pugi::xml_document>();
    auto decl = xmlDoc->prepend_child(pugi::node_declaration);
    decl.append_attribute("version") = "1.0";
    decl.append_attribute("encoding") = "UTF-8";
    auto root = xmlDoc->append_child("root");

    xml2Json = std::make_unique<Xml2Json>();

    std::string error;
    int errorCode = 0;
    log_debug("start");

    std::string output;
    // processing page, creating output
    try {
        if (!config->getBoolOption(ConfigVal::SERVER_UI_ENABLED)) {
            log_warning("The UI is disabled in the configuration file. See README.");
            error = "The UI is disabled in the configuration file. See README.";
            errorCode = 900;
        } else {
            process(root);
        }
    } catch (const LoginException& e) {
        error = e.what();
        errorCode = 300;
    } catch (const ObjectNotFoundException& e) {
        error = e.what();
        errorCode = 200;
    } catch (const SessionException& e) {
        error = e.what();
        errorCode = 400;
    } catch (const DatabaseException& e) {
        error = e.getUserMessage();
        errorCode = 500;
    } catch (const std::runtime_error& e) {
        error = fmt::format("Error: {}", e.what());
        errorCode = 800;
    }

    if (error.empty()) {
        root.append_attribute("success") = true;
    } else {
        root.append_attribute("success") = false;

        auto errorEl = root.append_child("error");
        errorEl.append_attribute("text") = error.c_str();

        if (errorCode == 0)
            errorCode = 899;
        errorEl.append_attribute("code") = errorCode;

        log_warning("Web Error: {} {}", errorCode, error);
    }

    try {
        output = xml2Json->getJson(root);
    } catch (const std::runtime_error& e) {
        log_error("Web marshalling error: {}", e.what());
    }

    log_debug("output-----------------------{}", output);

    auto ioHandler = std::make_unique<MemIOHandler>(output);
    ioHandler->open(mode);
    return ioHandler;
}

std::string_view WebRequestHandler::mapAutoscanType(AutoscanType type)
{
    switch (type) {
    case AutoscanType::Ui:
        return "ui";
    case AutoscanType::Config:
        return "persistent";
    default:
        return "none";
    }
}

std::unique_ptr<WebRequestHandler> createWebRequestHandler(
    const std::shared_ptr<Context>& context,
    const std::shared_ptr<Content>& content,
    const std::shared_ptr<Server>& server,
    const std::shared_ptr<UpnpXMLBuilder>& xmlBuilder,
    const std::shared_ptr<Quirks>& quirks,
    const std::string& page)
{
    if (page == Web::Add::PAGE)
        return std::make_unique<Web::Add>(content, server, xmlBuilder, quirks);
    if (page == Web::Remove::PAGE)
        return std::make_unique<Web::Remove>(content, server, xmlBuilder, quirks);
    if (page == Web::AddObject::PAGE)
        return std::make_unique<Web::AddObject>(content, server, xmlBuilder, quirks);
    if (page == Web::Auth::PAGE)
        return std::make_unique<Web::Auth>(content, server, xmlBuilder, quirks);
    if (page == Web::Containers::PAGE)
        return std::make_unique<Web::Containers>(content, server, xmlBuilder, quirks);
    if (page == Web::Directories::PAGE)
        return std::make_unique<Web::Directories>(content, context->getConverterManager(), server, xmlBuilder, quirks);
    if (page == Web::Files::PAGE)
        return std::make_unique<Web::Files>(content, context->getConverterManager(), server, xmlBuilder, quirks);
    if (page == Web::Items::PAGE)
        return std::make_unique<Web::Items>(content, server, xmlBuilder, quirks);
    if (page == Web::EditLoad::PAGE)
        return std::make_unique<Web::EditLoad>(content, server, xmlBuilder, quirks);
    if (page == Web::EditSave::PAGE)
        return std::make_unique<Web::EditSave>(content, server, xmlBuilder, quirks);
    if (page == Web::Autoscan::PAGE)
        return std::make_unique<Web::Autoscan>(content, server, xmlBuilder, quirks);
    if (page == Web::VoidType::PAGE)
        return std::make_unique<Web::VoidType>(content, server, xmlBuilder, quirks);
    if (page == Web::Tasks::PAGE)
        return std::make_unique<Web::Tasks>(content, server, xmlBuilder, quirks);
    if (page == Web::Action::PAGE)
        return std::make_unique<Web::Action>(content, server, xmlBuilder, quirks);
    if (page == Web::Clients::PAGE)
        return std::make_unique<Web::Clients>(content, server, xmlBuilder, quirks);
    if (page == Web::ConfigLoad::PAGE)
        return std::make_unique<Web::ConfigLoad>(content, server, xmlBuilder, quirks);
    if (page == Web::ConfigSave::PAGE)
        return std::make_unique<Web::ConfigSave>(context, content, server, xmlBuilder, quirks);

    throw_std_runtime_error("Unknown page: {}", page);
}

} // namespace Web
