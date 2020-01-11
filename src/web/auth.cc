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

#include "pages.h"
#include "config/config_manager.h"
#include "session_manager.h"
#include "util/tools.h"
#include <sys/time.h>

using namespace zmm;
using namespace mxml;

#define LOGIN_TIMEOUT 10 // in seconds

static long get_time()
{
    struct timeval tv;
    gettimeofday(&tv, nullptr);
    return tv.tv_sec;
}

static std::string generate_token()
{
    long expiration = get_time() + LOGIN_TIMEOUT;
    std::string salt = generate_random_id();
    return std::to_string(expiration) + '_' + salt;
}

static bool check_token(std::string token, std::string password, std::string encPassword)
{
    std::vector<std::string> parts = split_string(token, '_');
    if (parts.size() != 2)
        return false;
    long expiration = std::stol(parts[0]);
    if (expiration < get_time())
        return false;
    std::string checksum = hex_string_md5(token + password);
    return (checksum == encPassword);
}

web::auth::auth(std::shared_ptr<ConfigManager> config, std::shared_ptr<Storage> storage,
    std::shared_ptr<ContentManager> content, std::shared_ptr<SessionManager> sessionManager)
    : WebRequestHandler(config, storage, content, sessionManager)
{
    timeout = 60 * config->getIntOption(CFG_SERVER_UI_SESSION_TIMEOUT);
}
void web::auth::process()
{
    std::string action = param("action");

    if (!string_ok(action)) {
        root->appendTextChild("error", "req_type auth: no action given");
        return;
    }

    if (action == "get_config") {
        Ref<Element> cfg(new Element("config"));
        root->appendElementChild(cfg);
        cfg->setAttribute("accounts", accountsEnabled() ? "1" : "0", mxml_bool_type);
        cfg->setAttribute("show-tooltips",
            (config->getBoolOption(
                 CFG_SERVER_UI_SHOW_TOOLTIPS)
                    ? "1"
                    : "0"),
            mxml_bool_type);
        cfg->setAttribute("poll-when-idle",
            (config->getBoolOption(
                 CFG_SERVER_UI_POLL_WHEN_IDLE)
                    ? "1"
                    : "0"),
            mxml_bool_type);
        cfg->setAttribute("poll-interval",
            std::to_string(config->getIntOption(CFG_SERVER_UI_POLL_INTERVAL)), mxml_int_type);
        /// CREATE XML FRAGMENT FOR ITEMS PER PAGE
        Ref<Element> ipp(new Element("items-per-page"));
        ipp->setArrayName("option");
        ipp->setAttribute("default",
            std::to_string(config->getIntOption(CFG_SERVER_UI_DEFAULT_ITEMS_PER_PAGE)), mxml_int_type);

        std::vector<std::string> menu_opts = config->getStringArrayOption(CFG_SERVER_UI_ITEMS_PER_PAGE_DROPDOWN);

        for (size_t i = 0; i < menu_opts.size(); i++) {
            ipp->appendTextChild("option", menu_opts[i], mxml_int_type);
        }

        cfg->appendElementChild(ipp);
#ifdef HAVE_INOTIFY
        if (config->getBoolOption(CFG_IMPORT_AUTOSCAN_USE_INOTIFY))
            cfg->setAttribute("have-inotify", "1", mxml_bool_type);
        else
            cfg->setAttribute("have-inotify", "0", mxml_bool_type);
#else
        cfg->setAttribute("have-inotify", "0", mxml_bool_type);
#endif

        Ref<Element> actions(new Element("actions"));
        actions->setArrayName("action");
        //actions->appendTextChild("action", "fokel1");
        //actions->appendTextChild("action", "fokel2");

        cfg->appendElementChild(actions);

        Ref<Element> friendlyName(new Element("friendlyName"));
        friendlyName->setText(config->getOption(CFG_SERVER_NAME));
        cfg->appendElementChild(friendlyName);

        Ref<Element> gerberaVersion(new Element("version"));
        gerberaVersion->setText(VERSION);
        cfg->appendElementChild(gerberaVersion);
    } else if (action == "get_sid") {
        log_debug("checking/getting sid...");
        std::shared_ptr<Session> session = nullptr;
        std::string sid = param("sid");

        if (sid.empty() || (session = sessionManager->getSession(sid)) == nullptr) {
            session = sessionManager->createSession(timeout);
            root->setAttribute("sid_was_valid", "0", mxml_bool_type);
        } else {
            session->clearUpdateIDs();
            root->setAttribute("sid_was_valid", "1", mxml_bool_type);
        }
        root->setAttribute("sid", session->getID());

        if (!session->isLoggedIn() && !accountsEnabled()) {
            session->logIn();
            //throw SessionException("not logged in");
        }
        root->setAttribute("logged_in", session->isLoggedIn() ? "1" : "0", mxml_bool_type);
    } else if (action == "logout") {
        check_request();
        std::string sid = param("sid");
        auto session = sessionManager->getSession(sid);
        if (session == nullptr)
            throw _Exception("illegal session id");
        sessionManager->removeSession(sid);
    } else if (action == "get_token") {
        check_request(false);

        // sending token
        std::string token = generate_token();
        session->put("token", token);
        root->appendTextChild("token", token);
    } else if (action == "login") {
        check_request(false);

        // authentication
        std::string username = param("username");
        std::string encPassword = param("password");
        std::string sid = param("sid");

        if (!string_ok(username) || !string_ok(encPassword))
            throw LoginException("Missing username or password");

        auto session = sessionManager->getSession(sid);
        if (session == nullptr)
            throw _Exception("illegal session id");

        std::string correctPassword = sessionManager->getUserPassword(username);

        if (!string_ok(correctPassword) || !check_token(session->get("token"), correctPassword, encPassword))
            throw LoginException("Invalid username or password");

        session->logIn();
    } else
        throw _Exception("illegal action given to req_type auth");
}
