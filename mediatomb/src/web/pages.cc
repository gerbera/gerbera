/*  pages.cc - this file is part of MediaTomb.
                                                                                
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

#include "pages.h"

using namespace zmm;

WebRequestHandler *create_web_request_handler(String page)
{
    //if (page == "acct") return new web::acct();
    //if (page == "add") return new web::add();
    if (page == "auth") return new web::auth();
    //if (page == "browse") return new web::browse();
    if (page == "containers") return new web::containers();
    if (page == "directories") return new web::directories();
    //if (page == "edit_save") return new web::edit_save();
    //if (page == "edit_ui") return new web::edit_ui();
    //if (page == "error") return new web::error();
    if (page == "files") return new web::files();
    //if (page == "head") return new web::head();
    //if (page == "index") return new web::index();
    if (page == "items") return new web::items();
    //if (page == "login") return new web::login();
    //if (page == "new") return new web::new_ui();
    //if (page == "remove") return new web::remove();
    //if (page == "refresh") return new web::refresh();
    //if (page == "task") return new web::task();
    //if (page == "tree") return new web::tree();
    // ...
    
    throw Exception(_("Unknown page: ") + page);
}


