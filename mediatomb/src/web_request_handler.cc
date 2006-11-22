/*MT*
    
    MediaTomb - http://www.mediatomb.org/
    
    web_request_handler.cc - this file is part of MediaTomb.
    
    Copyright (C) 2005 Gena Batyan <bgeradz@mediatomb.org>,
                       Sergey Bostandzhyan <jin@mediatomb.org>
    Copyright (C) 2006 Gena Batyan <bgeradz@mediatomb.org>,
                       Sergey Bostandzhyan <jin@mediatomb.org>,
                       Leonhard Wimmer <leo@mediatomb.org>
    
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
    
    $Id$
*/

/// \file web_request_handler.cc

#ifdef HAVE_CONFIG_H
    #include "autoconfig.h"
#endif

#include <time.h>
#include "mem_io_handler.h"
#include "web_request_handler.h"
#include "config_manager.h"
#include "web/pages.h"
#include "tools.h"
#include "hash.h"

using namespace zmm;
using namespace mxml;

#define DEFAULT_PAGE_BUFFER_SIZE 4096

//static JSObject *shared_global_object = NULL;

//static Ref<DSOHash<WebScript> > script_hash(new DSOHash<WebScript>(37));

WebRequestHandler::WebRequestHandler() : RequestHandler()
{
}

int WebRequestHandler::intParam(String name, int invalid)
{
    String value = param(name);
    if (!string_ok(value))
        return invalid;
    else
        return value.toInt();
}

/*String WebRequestHandler::buildScriptPath(String filename)
{
    String scriptRoot = ConfigManager::getInstance()->getOption(_("/server/webroot"));
    return String(scriptRoot + "/jssp/") + filename;
}*/

void WebRequestHandler::check_request(bool checkLogin)
{
    // we have a minimum set of parameters that are "must have"

    // check if the session parameter was supplied and if we have
    // a session with that id
    String sid = param(_("sid"));
    if (sid == nil)
    {
        throw SessionException(_("no session id given"));
    }
    Ref<Session> session;
    if ((session = SessionManager::getInstance()->getSession(sid)) == nil)
    {
        throw SessionException(_("invalid session id"));
    }
    if (checkLogin && ! session->isLoggedIn())
    {
        throw SessionException(_("not logged in"));
    }
    session->access();
    String uiUpdate = param(_("get_update_ids"));
    if ((string_ok(uiUpdate) && uiUpdate == _("1")))
        addUpdateIDs(session, root);
}

String WebRequestHandler::renderXMLHeader()
{
    return _("<?xml version=\"1.0\" encoding=\"") +
            DEFAULT_INTERNAL_CHARSET +"\"?>\n";
}

void WebRequestHandler::get_info(IN const char *filename, OUT struct File_Info *info)
{
    info->file_length = -1; // length is unknown
    info->last_modified = time(NULL);
    info->is_directory = 0;
    info->is_readable = 1;
    
    String contentType;
    
    contentType = _(MIMETYPE_XML) + "; charset=" + DEFAULT_INTERNAL_CHARSET;
    
    info->content_type = ixmlCloneDOMString(contentType.c_str());
    info->http_header = ixmlCloneDOMString("Cache-Control: no-cache, must-revalidate");
}

Ref<IOHandler> WebRequestHandler::open(Ref<Dictionary> params, IN enum UpnpOpenFileMode mode)
{
    this->params = params;
    
    root = Ref<Element>(new Element(_("root")));
    out = Ref<StringBuffer>(new StringBuffer());
    
    String output;
    // processing page, creating output
    try
    {
        process();
        output = renderXMLHeader() + root->print();
    }
    catch (SessionException se)
    {
        //String url = _("/content/interface?req_type=login&slt=") +
        //                    generate_random_id();
        
        root->appendTextChild(_("redirect"), _("/"));
        output = renderXMLHeader() + root->print();
    }
    catch (Exception e)
    {
        e.printStackTrace();
        // Ref<Dictionary> par(new Dictionary());
        // par->put("message", e.getMessage());
        // output = subrequest("error", par);
        String errmsg = _("Exception: ") + e.getMessage();
        root->appendTextChild(_("error"), errmsg);
        output = renderXMLHeader() + root->print();
    }
    root = nil;

    Ref<MemIOHandler> io_handler(new MemIOHandler(output));
    io_handler->open(mode);
    return RefCast(io_handler, IOHandler);
}

Ref<IOHandler> WebRequestHandler::open(IN const char *filename,
                                       IN enum UpnpOpenFileMode mode)
{
    log_debug("request: %s\n", filename);
    this->filename = String((char *)filename);
    this->mode = mode;

    String path, parameters;
    split_url(filename, URL_UI_PARAM_SEPARATOR, path, parameters);

    Ref<Dictionary> params = Ref<Dictionary>(new Dictionary());
    params->decode(parameters);
    return open(params, mode);
}

void WebRequestHandler::addUpdateIDs(Ref<Session> session, Ref<Element> root)
{
    String updateIDs = session->getUIUpdateIDs();
    if (string_ok(updateIDs))
    {
        log_debug("UI: sending update ids: %s\n", updateIDs.c_str());
        root->appendTextChild(_("updateIDs"), updateIDs);
    }
}
