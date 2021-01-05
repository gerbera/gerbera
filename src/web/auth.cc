/*MT*
    
    MediaTomb - http://www.mediatomb.cc/
    
    auth.cc - this file is part of MediaTomb.
    
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

/// \file auth.cc

#include "pages.h" // API

#include <chrono>
#include <sys/time.h>
#include <utility>

#include "config/config_manager.h"
#include "session_manager.h"
#include "util/tools.h"

#define LOGIN_TIMEOUT 10 // in seconds

static time_t get_time()
{
    return std::chrono::time_point_cast<std::chrono::seconds>(std::chrono::system_clock::now()).time_since_epoch().count();
}

static std::string generate_token()
{
    const time_t expiration = get_time() + LOGIN_TIMEOUT;
    std::string salt = generateRandomId();
    return std::to_string(expiration) + '_' + salt;
}

static bool check_token(const std::string& token, const std::string& password, const std::string& encPassword)
{
    std::vector<std::string> parts = splitString(token, '_');
    if (parts.size() != 2)
        return false;
    auto expiration = static_cast<time_t>(std::stol(parts[0]));
    if (expiration < get_time())
        return false;
    std::string checksum = hexStringMd5(token + password);
    return (checksum == encPassword);
}

web::auth::auth(std::shared_ptr<ContentManager> content)
    : WebRequestHandler(std::move(content))
{
    timeout = 60 * config->getIntOption(CFG_SERVER_UI_SESSION_TIMEOUT);
}
void web::auth::process()
{
    std::string action = param("action");
    auto root = xmlDoc->document_element();

    if (action.empty()) {
        root.append_child("error").append_child(pugi::node_pcdata).set_value("req_type auth: no action given");
        return;
    }

    if (action == "get_config") {
        auto cfg = root.append_child("config");
        cfg.append_attribute("accounts") = accountsEnabled();
        cfg.append_attribute("show-tooltips") = config->getBoolOption(CFG_SERVER_UI_SHOW_TOOLTIPS);
        cfg.append_attribute("poll-when-idle") = config->getBoolOption(CFG_SERVER_UI_POLL_WHEN_IDLE);
        cfg.append_attribute("poll-interval") = config->getIntOption(CFG_SERVER_UI_POLL_INTERVAL);

        /// CREATE XML FRAGMENT FOR ITEMS PER PAGE
        auto ipp = cfg.append_child("items-per-page");
        xml2JsonHints->setArrayName(ipp, "option");
        ipp.append_attribute("default") = config->getIntOption(CFG_SERVER_UI_DEFAULT_ITEMS_PER_PAGE);

        auto menu_opts = config->getArrayOption(CFG_SERVER_UI_ITEMS_PER_PAGE_DROPDOWN);
        for (const auto& menu_opt : menu_opts) {
            ipp.append_child("option").append_child(pugi::node_pcdata).set_value(menu_opt.c_str());
        }

#ifdef HAVE_INOTIFY
        cfg.append_attribute("have-inotify") = config->getBoolOption(CFG_IMPORT_AUTOSCAN_USE_INOTIFY);
#else
        cfg.append_attribute("have-inotify") = false;
#endif

        auto actions = cfg.append_child("actions");
        xml2JsonHints->setArrayName(actions, "action");
        //actions->appendTextChild("action", "fokel1");
        //actions->appendTextChild("action", "fokel2");

        auto friendlyName = cfg.append_child("friendlyName").append_child(pugi::node_pcdata);
        friendlyName.set_value(config->getOption(CFG_SERVER_NAME).c_str());

        auto gerberaVersion = cfg.append_child("version").append_child(pugi::node_pcdata);
        gerberaVersion.set_value(GERBERA_VERSION);
    } else if (action == "get_sid") {
        log_debug("checking/getting sid...");
        std::shared_ptr<Session> session = nullptr;
        std::string sid = param("sid");

        if (sid.empty() || (session = sessionManager->getSession(sid)) == nullptr) {
            session = sessionManager->createSession(timeout);
            root.append_attribute("sid_was_valid") = false;
        } else {
            session->clearUpdateIDs();
            root.append_attribute("sid_was_valid") = true;
        }
        root.append_attribute("sid") = session->getID().c_str();

        if (!session->isLoggedIn() && !accountsEnabled()) {
            session->logIn();
            //throw SessionException("not logged in");
        }
        root.append_attribute("logged_in") = session->isLoggedIn();
    } else if (action == "logout") {
        check_request();
        std::string sid = param("sid");
        auto session = sessionManager->getSession(sid);
        if (session == nullptr)
            throw_std_runtime_error("illegal session id");
        sessionManager->removeSession(sid);
    } else if (action == "get_token") {
        check_request(false);

        // sending token
        std::string token = generate_token();
        session->put("token", token);
        root.append_child("token").append_child(pugi::node_pcdata).set_value(token.c_str());
    } else if (action == "login") {
        check_request(false);

        // authentication
        std::string username = param("username");
        std::string encPassword = param("password");
        std::string sid = param("sid");

        if (username.empty() || encPassword.empty())
            throw LoginException("Missing username or password");

        auto session = sessionManager->getSession(sid);
        if (session == nullptr)
            throw_std_runtime_error("illegal session id");

        std::string correctPassword = sessionManager->getUserPassword(username);

        if (correctPassword.empty() || !check_token(session->get("token"), correctPassword, encPassword))
            throw LoginException("Invalid username or password");

        session->logIn();
    } else
        throw_std_runtime_error("illegal action given to req_type auth");
}
