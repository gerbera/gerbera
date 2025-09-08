/*MT*

    MediaTomb - http://www.mediatomb.cc/

    web/auth.cc - this file is part of MediaTomb.

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

/// @file web/auth.cc
#define GRB_LOG_FAC GrbLogFacility::web

#include "pages.h" // API

#include "config/config.h"
#include "config/config_val.h"
#include "database/sql_database.h"
#include "exceptions.h"
#include "session_manager.h"
#include "util/tools.h"

static constexpr auto loginTimeout = std::chrono::seconds(10);
const std::string_view Web::Auth::PAGE = "auth";

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
    : PageRequest(content, server, xmlBuilder, quirks)
    , timeout(std::chrono::minutes(config->getLongOption(ConfigVal::SERVER_UI_SESSION_TIMEOUT)))
    , accountsEnabled(config->getBoolOption(ConfigVal::SERVER_UI_ACCOUNTS_ENABLED))
{
    doCheck = false;
}

bool Web::Auth::processPageAction(Json::Value& element, const std::string& action)
{
    if (action.empty()) {
        element["error"] = "req_type auth: no action given";
        return doCheck;
    }

    if (action == "get_config") {
        return getConfig(element);
    } else if (action == "get_sid") {
        return getSid(element);
    } else if (action == "logout") {
        return logout();
    } else if (action == "get_token") {
        return getToken(element);
    } else if (action == "login") {
        return login();
    } else {
        throw_std_runtime_error("illegal action {} given to req_type auth", action);
    }

    return doCheck;
}

bool Web::Auth::getConfig(Json::Value& element)
{
    Json::Value cfg;
    cfg["accounts"] = accountsEnabled;
    cfg["enableNumbering"] = config->getBoolOption(ConfigVal::SERVER_UI_ENABLE_NUMBERING);
    cfg["enableThumbnail"] = config->getBoolOption(ConfigVal::SERVER_UI_ENABLE_THUMBNAIL);
    cfg["enableVideo"] = config->getBoolOption(ConfigVal::SERVER_UI_ENABLE_VIDEO);
    cfg["show-tooltips"] = config->getBoolOption(ConfigVal::SERVER_UI_SHOW_TOOLTIPS);
    cfg["poll-when-idle"] = config->getBoolOption(ConfigVal::SERVER_UI_POLL_WHEN_IDLE);
    cfg["poll-interval"] = static_cast<Json::Int64>(config->getLongOption(ConfigVal::SERVER_UI_POLL_INTERVAL));
    cfg["fsAddItem"] = config->getBoolOption(ConfigVal::SERVER_UI_FS_SUPPORT_ADD_ITEM);
    cfg["editSortKey"] = config->getBoolOption(ConfigVal::SERVER_UI_EDIT_SORTKEY);
    cfg["sourceDocs"] = config->getOption(ConfigVal::SERVER_UI_DOCUMENTATION_SOURCE);
    cfg["userDocs"] = config->getOption(ConfigVal::SERVER_UI_DOCUMENTATION_USER);
    cfg["searchCaps"] = SQLDatabase::getSearchCapabilities();
    cfg["sortCaps"] = SQLDatabase::getSortCapabilities();

    Json::Value ipp;
    ipp["default"] = config->getIntOption(ConfigVal::SERVER_UI_DEFAULT_ITEMS_PER_PAGE);

    Json::Value opt(Json::arrayValue);
    auto menuOpts = config->getArrayOption(ConfigVal::SERVER_UI_ITEMS_PER_PAGE_DROPDOWN);
    for (auto&& menuOpt : menuOpts) {
        opt.append(stoiString(menuOpt));
    }
    ipp["option"] = opt;
    cfg["items-per-page"] = ipp;

#ifdef HAVE_INOTIFY
    cfg["have-inotify"] = config->getBoolOption(ConfigVal::IMPORT_AUTOSCAN_USE_INOTIFY);
#else
    cfg["have-inotify"] = false;
#endif

    Json::Value action(Json::arrayValue);
    element["actions"]["action"] = action;
    cfg["friendlyName"] = config->getOption(ConfigVal::SERVER_NAME);
    cfg["version"] = GERBERA_VERSION;
    element["config"] = cfg;
    return false;
}

bool Web::Auth::getSid(Json::Value& element)
{
    log_debug("checking/getting sid...");
    std::shared_ptr<Session> session;
    std::string sid = param(SID);

    if (sid.empty() || !(session = sessionManager->getSession(sid))) {
        session = sessionManager->createSession(timeout);
        element["sid_was_valid"] = false;
    } else {
        session->clearUpdateIDs();
        element["sid_was_valid"] = true;
    }
    element[SID] = session->getID().c_str();

    if (!session->isLoggedIn() && !accountsEnabled) {
        session->logIn();
    }
    element["logged_in"] = session->isLoggedIn();

    return false;
}

bool Web::Auth::getToken(Json::Value& element)
{
    checkRequest(false);

    // sending token
    std::string token = generateToken();
    session->put("token", token);
    element["token"] = token;

    return true;
}

bool Web::Auth::login()
{
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

    return true;
}

bool Web::Auth::logout()
{
    checkRequest();

    std::string sid = param(SID);
    auto session = sessionManager->getSession(sid);
    if (!session)
        throw_std_runtime_error("illegal session id");
    sessionManager->removeSession(sid);

    return true;
}
