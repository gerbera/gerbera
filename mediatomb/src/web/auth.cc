/*  auth.cc - this file is part of MediaTomb.
                                                                                
    Copyright (C) 2005 Gena Batyan <bgeradz@deadlock.dhs.org>,
                       Sergey Bostandzhyan <jin@deadlock.dhs.org>
                                                                                
    MediaTomb is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.
                                                                                
    MediaTomb is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.
                                                                                
    You should have received a copy of the GNU General Public License
    along with MediaTomb; if not, write to the Free Software
    Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
*/

#ifdef HAVE_CONFIG_H
    #include "autoconfig.h"
#endif

#include <sys/time.h>
#include "pages.h"
#include "tools.h"
#include "session_manager.h"

using namespace zmm;
using namespace mxml;

#define LOGIN_TIMEOUT 10 // in seconds

class LoginException : public zmm::Exception
{
public:
    LoginException(zmm::String message) : zmm::Exception(message) {}
};

static long get_time()
{
    struct timeval tv;
    gettimeofday(&tv, NULL);
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
    Ref<Array<StringBase> > parts = split_string(token, '_');
    if (parts->size() != 2)
        return false;
    long expiration = String(parts->get(0)).toLong();
    if (expiration < get_time())
        return false;
    String checksum = hex_string_md5(token + password);
    return (checksum == encPassword);
}

web::auth::auth() : WebRequestHandler()
{
}
void web::auth::process()
{
    if (param(_("checkSID")) != nil)
    {
        check_request();
    }
    else if (param(_("logout")) != nil)
    {
        check_request();
        String sid = param(_("sid"));
        Ref<SessionManager> sessionManager = SessionManager::getInstance();
        Ref<Session> session = SessionManager::getInstance()->getSession(sid);
        if (session == nil)
            throw _Exception(_("illegal session id"));
        sessionManager->removeSession(sid);
        root->appendTextChild(_("redirect"), _("/"));
    }
    else if (param(_("auth")) == nil)
    {
        Ref<SessionManager> sessionManager = SessionManager::getInstance();
        Ref<Session> session = sessionManager->createSession();
        
        // sending token
        String token = generate_token();
        session->put(_("token"), token);
        root->appendTextChild(_("token"), token);
        root->addAttribute(_("sid"), session->getID());
    }
    else
    {
        // login
        try
        {
            check_request(false);
            
            // authentication
            String username = param(_("username"));
            String encPassword = param(_("password"));
            String sid = param(_("sid"));
            
            if (! string_ok(username) || ! string_ok(encPassword))
                throw LoginException(_("Missing username or password"));
            
            Ref<SessionManager> sessionManager = SessionManager::getInstance();
            Ref<Session> session = sessionManager->getSession(sid);
            if (session == nil)
                throw _Exception(_("illegal session id"));
            
            //Ref<Array<StringBase> > parts = split_string(password, ':');
            //if (parts->size() != 2)
            //    throw LoginException(_("Invalid password"));
            //String token = parts->get(0);
            //String encPassword = parts->get(1);
            
            String correctPassword = sessionManager->getUserPassword(username);
            
            if (! string_ok(correctPassword) || ! check_token(session->get(_("token")), correctPassword, encPassword))
                throw LoginException(_("Invalid username or password"));
            
            session->logIn();
        }
        catch (LoginException le)
        {
            root->appendTextChild(_("error"), le.getMessage());
        }
    }
}

