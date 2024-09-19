/*MT*

    MediaTomb - http://www.mediatomb.cc/

    web/auth.cc - this file is part of MediaTomb.

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

/// \file web/auth.cc
#define GRB_LOG_FAC GrbLogFacility::web

#include "pages.h" // API

#include "config/config.h"
#include "config/config_val.h"
#include "exceptions.h"
#include "session_manager.h"
#include "util/tools.h"
#include "util/xml_to_json.h"

static constexpr auto loginTimeout = std::chrono::seconds(10);

static std::string generateToken()
{
    auto expiration = currentTime() + loginTimeout;
    std::string salt = generateRandomId();
    return fmt::format("{}_{}", expiration.count(), salt);
}

static bool checkToken(const std::string& token, const std::string& password, const std::string& encPassword)
{
    std::vector<std::string> parts = splitString(token, '_');
    if (parts.size() != 2)
        return false;
    auto expiration = std::chrono::seconds(std::stol(parts[0]));
    if (expiration < currentTime())
        return false;
    std::string checksum = hexStringMd5(token + password);
    return (checksum == encPassword);
}

Web::Auth::Auth(const std::shared_ptr<Content>& content,
    const std::shared_ptr<Server>& server,
    const std::shared_ptr<UpnpXMLBuilder>& xmlBuilder,
    const std::shared_ptr<Quirks>& quirks)
    : WebRequestHandler(content, server, xmlBuilder, quirks)
    , timeout(std::chrono::minutes(config->getIntOption(ConfigVal::SERVER_UI_SESSION_TIMEOUT)))
    , accountsEnabled(config->getBoolOption(ConfigVal::SERVER_UI_ACCOUNTS_ENABLED))
{
}

void Web::Auth::process()
{
    std::string action = param("action");
    auto root = xmlDoc->document_element();

    if (action.empty()) {
        root.append_child("error").append_child(pugi::node_pcdata).set_value("req_type auth: no action given");
        return;
    }

    if (action == "get_config") {
        auto cfg = root.append_child("config");
        cfg.append_attribute("accounts") = accountsEnabled;
        cfg.append_attribute("enableNumbering") = config->getBoolOption(ConfigVal::SERVER_UI_ENABLE_NUMBERING);
        cfg.append_attribute("enableThumbnail") = config->getBoolOption(ConfigVal::SERVER_UI_ENABLE_THUMBNAIL);
        cfg.append_attribute("enableVideo") = config->getBoolOption(ConfigVal::SERVER_UI_ENABLE_VIDEO);
        cfg.append_attribute("show-tooltips") = config->getBoolOption(ConfigVal::SERVER_UI_SHOW_TOOLTIPS);
        cfg.append_attribute("poll-when-idle") = config->getBoolOption(ConfigVal::SERVER_UI_POLL_WHEN_IDLE);
        cfg.append_attribute("poll-interval") = config->getIntOption(ConfigVal::SERVER_UI_POLL_INTERVAL);
        cfg.append_child("sourceDocs").append_child(pugi::node_pcdata).set_value(config->getOption(ConfigVal::SERVER_UI_DOCUMENTATION_SOURCE).c_str());
        cfg.append_child("userDocs").append_child(pugi::node_pcdata).set_value(config->getOption(ConfigVal::SERVER_UI_DOCUMENTATION_USER).c_str());

        /// CREATE XML FRAGMENT FOR ITEMS PER PAGE
        auto ipp = cfg.append_child("items-per-page");
        xml2Json->setArrayName(ipp, "option");
        ipp.append_attribute("default") = config->getIntOption(ConfigVal::SERVER_UI_DEFAULT_ITEMS_PER_PAGE);

        auto menuOpts = config->getArrayOption(ConfigVal::SERVER_UI_ITEMS_PER_PAGE_DROPDOWN);
        for (auto&& menuOpt : menuOpts) {
            ipp.append_child("option").append_child(pugi::node_pcdata).set_value(menuOpt.c_str());
        }

#ifdef HAVE_INOTIFY
        cfg.append_attribute("have-inotify") = config->getBoolOption(ConfigVal::IMPORT_AUTOSCAN_USE_INOTIFY);
#else
        cfg.append_attribute("have-inotify") = false;
#endif

        auto actions = cfg.append_child("actions");
        xml2Json->setArrayName(actions, "action");
        // actions->appendTextChild("action", "fokel1");
        // actions->appendTextChild("action", "fokel2");

        auto friendlyName = cfg.append_child("friendlyName").append_child(pugi::node_pcdata);
        friendlyName.set_value(config->getOption(ConfigVal::SERVER_NAME).c_str());

        auto gerberaVersion = cfg.append_child("version").append_child(pugi::node_pcdata);
        gerberaVersion.set_value(GERBERA_VERSION);
    } else if (action == "get_sid") {
        log_debug("checking/getting sid...");
        std::shared_ptr<Session> session;
        std::string sid = param(SID);

        if (sid.empty() || !(session = sessionManager->getSession(sid))) {
            session = sessionManager->createSession(timeout);
            root.append_attribute("sid_was_valid") = false;
        } else {
            session->clearUpdateIDs();
            root.append_attribute("sid_was_valid") = true;
        }
        root.append_attribute(SID) = session->getID().c_str();

        if (!session->isLoggedIn() && !accountsEnabled) {
            session->logIn();
            // throw SessionException("not logged in");
        }
        root.append_attribute("logged_in") = session->isLoggedIn();
    } else if (action == "logout") {
        checkRequest();
        std::string sid = param(SID);
        auto session = sessionManager->getSession(sid);
        if (!session)
            throw_std_runtime_error("illegal session id");
        sessionManager->removeSession(sid);
    } else if (action == "get_token") {
        checkRequest(false);

        // sending token
        std::string token = generateToken();
        session->put("token", token);
        root.append_child("token").append_child(pugi::node_pcdata).set_value(token.c_str());
    } else if (action == "login") {
        checkRequest(false);

        // authentication
        std::string username = param("username");
        std::string encPassword = param("password");
        std::string sid = param(SID);

        if (username.empty() || encPassword.empty())
            throw LoginException("Missing username or password");

        auto session = sessionManager->getSession(sid);
        if (!session)
            throw_std_runtime_error("illegal session id");

        std::string correctPassword = sessionManager->getUserPassword(username);

        if (correctPassword.empty() || !checkToken(session->get("token"), correctPassword, encPassword))
            throw LoginException("Invalid username or password");

        session->logIn();
    } else
        throw_std_runtime_error("illegal action given to req_type auth");
}
