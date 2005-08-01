/*  login.cc - this file is part of MediaTomb.
                                                                                
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

#include "server.h"
#include "common.h"
#include "storage.h"
#include "cds_objects.h"
#include "dictionary.h"
#include "pages.h"
#include "session_manager.h"

using namespace zmm;
using namespace mxml;

web::login::login() : WebRequestHandler()
{}

void web::login::get_info(IN const char *filename, OUT struct File_Info *info)
{
    info->file_length = -1; // length is unknown
    info->last_modified = time(NULL);
    info->is_directory = 0;
    info->is_readable = 1;
    info->content_type = ixmlCloneDOMString(MIMETYPE_HTML);
}


void web::login::process()
{
    // well... at the moment - unsupported
    /*
    String username = param("username");
    String password = param("password");

    log_info(("username: %s, password %s\n", username.c_str(), password.c_str()));

    if ((username == nil) || (password == nil) || 
        (username == "") || (password == ""))
    {
        *out << "<html><head><meta http-equiv=\"Refresh\" content=\"0;URL=/noaccess.html\"></head></html>";
        return;
    }

    */

    Ref<SessionManager> sm = SessionManager::getInstance();
    Ref<Session> s = sm->createSession(DEFAULT_SESSION_TIMEOUT);
    
    String sid = s->getID();

    s->put("object_id", "0");
    s->put("requested_count", "15");
    s->put("starting_index", "0");
    
    *out << "<html><head><meta http-equiv=\"Refresh\" content=\"0;URL="; 
//    *out << server->getVirtualURL();
    *out << "/content/";
    *out << CONTENT_UI_HANDLER;
    *out << URL_REQUEST_SEPARATOR;
    *out << "req_type=browse&object_id=0&requested_count=15&driver=1&sid=";
    *out << sid;
    *out << "\"></head></html>";
//    log_info(("%s\n", out->toString().c_str()));
}

