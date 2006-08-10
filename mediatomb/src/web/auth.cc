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
#include "common.h"
#include "pages.h"
#include "tools.h"
#include "session_manager.h"

using namespace zmm;
using namespace mxml;

#define LOGIN_PASSWORD "102ec827703eeb509f12a2a0eb5b4e2d"
#define LOGIN_TIMEOUT 10L // in seconds

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
    pagename = _("auth");
    plainXML = true;
}
void web::auth::process()
{
    if (param(_("checkSID")) != nil)
    {
        check_request();
    }
    else if (param(_("auth")) == nil)
    {
        // generating token
        String token = generate_token();
        root->appendTextChild(_("token"), token);
    }
    else
    {
        // login
        try
        {
            // authentication
            String username = param(_("username"));
            String password = param(_("password"));

            if (! string_ok(username) || ! string_ok(password))
                throw LoginException(_("Missing username or password"));
            Ref<Array<StringBase> > parts = split_string(password, ':');
            if (parts->size() != 2)
                throw LoginException(_("Invalid password"));
            String token = parts->get(0);
            String encPassword = parts->get(1);

            String correctPassword = String(LOGIN_PASSWORD);
            
            if (! check_token(token, correctPassword, encPassword))
                throw LoginException(_("Invalid username or password"));
           
            // generate sid, the new ID must be the one that has never been sent
            // over network before and which is not possible to derive from anything
            // however it must be derivable from md5'd password and given token
            String sid = hex_string_md5(correctPassword + token);
 
            // check whether the session already exists
            Ref<SessionManager> sm = SessionManager::getInstance();
            if (sm->getSession(sid) != nil)
                throw LoginException(_("Session already exists"));
            
            /// \todo create session with the calculated id
            Ref<Session> s = sm->createSession(DEFAULT_SESSION_TIMEOUT, sid);
            s->put(_("object_id"), _("0"));
            s->put(_("requested_count"), _("15"));
            s->put(_("starting_index"), _("0"));
            
            root->addAttribute(_("sid"), sid);
        }
        catch (LoginException le)
        {
            root->appendTextChild(_("error"), le.getMessage());
        }
    }
}

