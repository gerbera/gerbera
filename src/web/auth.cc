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
#include "session_manager.h"
#include "tools.h"
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

static String generate_token()
{
    long expiration = get_time() + LOGIN_TIMEOUT;
    String salt = generate_random_id();
    return String::from(expiration) + '_' + salt;
}

static bool check_token(String token, String password, String encPassword)
{
    Ref<Array<StringBase>> parts = split_string(token, '_');
    if (parts->size() != 2)
        return false;
    long expiration = String(parts->get(0)).toLong();
    if (expiration < get_time())
        return false;
    String checksum = hex_string_md5(token + password);
    return (checksum == encPassword);
}

web::auth::auth()
    : WebRequestHandler()
{
    timeout = 60 * ConfigManager::getInstance()->getIntOption(CFG_SERVER_UI_SESSION_TIMEOUT);
}
void web::auth::process()
{
    String action = param(_("action"));
    Ref<SessionManager> sessionManager = SessionManager::getInstance();

    if (!string_ok(action)) {
        root->appendTextChild(_("error"), _("req_type auth: no action given"));
        return;
    }

    if (action == "get_config") {
        Ref<ConfigManager> cm = ConfigManager::getInstance();
        Ref<Element> config(new Element(_("config")));
        root->appendElementChild(config);
        config->setAttribute(_("accounts"), accountsEnabled() ? _("1") : _("0"), mxml_bool_type);
        config->setAttribute(_("show-tooltips"),
            (cm->getBoolOption(
                 CFG_SERVER_UI_SHOW_TOOLTIPS)
                    ? _("1")
                    : _("0")),
            mxml_bool_type);
        config->setAttribute(_("poll-when-idle"),
            (cm->getBoolOption(
                 CFG_SERVER_UI_POLL_WHEN_IDLE)
                    ? _("1")
                    : _("0")),
            mxml_bool_type);
        config->setAttribute(_("poll-interval"),
            String::from(cm->getIntOption(CFG_SERVER_UI_POLL_INTERVAL)), mxml_int_type);
        /// CREATE XML FRAGMENT FOR ITEMS PER PAGE
        Ref<Element> ipp(new Element(_("items-per-page")));
        ipp->setArrayName(_("option"));
        ipp->setAttribute(_("default"),
            String::from(cm->getIntOption(CFG_SERVER_UI_DEFAULT_ITEMS_PER_PAGE)), mxml_int_type);

        Ref<Array<StringBase>> menu_opts = cm->getStringArrayOption(CFG_SERVER_UI_ITEMS_PER_PAGE_DROPDOWN);

        for (int i = 0; i < menu_opts->size(); i++) {
            ipp->appendTextChild(_("option"), menu_opts->get(i), mxml_int_type);
        }

        config->appendElementChild(ipp);
#ifdef HAVE_INOTIFY
        if (cm->getBoolOption(CFG_IMPORT_AUTOSCAN_USE_INOTIFY))
            config->setAttribute(_("have-inotify"), _("1"), mxml_bool_type);
        else
            config->setAttribute(_("have-inotify"), _("0"), mxml_bool_type);
#else
        config->setAttribute(_("have-inotify"), _("0"), mxml_bool_type);
#endif

        Ref<Element> actions(new Element(_("actions")));
        actions->setArrayName(_("action"));
        //actions->appendTextChild(_("action"), _("fokel1"));
        //actions->appendTextChild(_("action"), _("fokel2"));

        config->appendElementChild(actions);

        Ref<Element> friendlyName(new Element(_("friendlyName")));
        friendlyName->setText(cm->getOption(CFG_SERVER_NAME));
        config->appendElementChild(friendlyName);

        Ref<Element> gerberaVersion(new Element(_("version")));
        gerberaVersion->setText(VERSION);
        config->appendElementChild(gerberaVersion);
    } else if (action == "get_sid") {
        log_debug("checking/getting sid...\n");
        Ref<Session> session = nullptr;
        String sid = param(_("sid"));

        if (sid == nullptr || (session = sessionManager->getSession(sid)) == nullptr) {
            session = sessionManager->createSession(timeout);
            root->setAttribute(_("sid_was_valid"), _("0"), mxml_bool_type);
        } else {
            session->clearUpdateIDs();
            root->setAttribute(_("sid_was_valid"), _("1"), mxml_bool_type);
        }
        root->setAttribute(_("sid"), session->getID());

        if (!session->isLoggedIn() && !accountsEnabled()) {
            session->logIn();
            //throw SessionException(_("not logged in"));
        }
        root->setAttribute(_("logged_in"), session->isLoggedIn() ? _("1") : _("0"), mxml_bool_type);
    } else if (action == "logout") {
        check_request();
        String sid = param(_("sid"));
        Ref<Session> session = SessionManager::getInstance()->getSession(sid);
        if (session == nullptr)
            throw _Exception(_("illegal session id"));
        sessionManager->removeSession(sid);
    } else if (action == "get_token") {
        check_request(false);

        // sending token
        String token = generate_token();
        session->put(_("token"), token);
        root->appendTextChild(_("token"), token);
    } else if (action == "login") {
        check_request(false);

        // authentication
        String username = param(_("username"));
        String encPassword = param(_("password"));
        String sid = param(_("sid"));

        if (!string_ok(username) || !string_ok(encPassword))
            throw LoginException(_("Missing username or password"));

        Ref<Session> session = sessionManager->getSession(sid);
        if (session == nullptr)
            throw _Exception(_("illegal session id"));

        String correctPassword = sessionManager->getUserPassword(username);

        if (!string_ok(correctPassword) || !check_token(session->get(_("token")), correctPassword, encPassword))
            throw LoginException(_("Invalid username or password"));

        session->logIn();
    } else
        throw _Exception(_("illegal action given to req_type auth"));
}
