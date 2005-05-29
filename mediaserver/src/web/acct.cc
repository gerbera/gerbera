/*  browse.cc - this file is part of MediaTomb.
                                                                                
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

#include "common.h"
#include "pages.h"
#include "content_manager.h"

#define AUTO_RELOAD_MILLIS (1000 * 5)

using namespace zmm;

web::acct::acct() : WebRequestHandler()
{}

void web::acct::process()
{
    Ref<ContentManager> cm = ContentManager::getInstance();
    
    Ref<CMAccounting> a = cm->getAccounting();
    Ref<CMTask> task = cm->getCurrentTask();

    String taskDesc = (task == nil) ? "" : task->getDescription();

//    check_request();

    /// \todo JS_escape status
    
    *out << "<html><body><script>\n"
         << "  top.head.updateAccounting({\n"
         << "    acctTotalFiles: " << a->totalFiles << ",\n"
         << "    currentTask: '" << taskDesc << "'\n"
         << "  });\n";
    if (true) /// \todo do not schedule reload if no asynchronous tasks
    {
        *out
         << "  setTimeout('location.href = location.href;', "
                           << AUTO_RELOAD_MILLIS << ");\n";
    }
    *out << "</script></body></html>\n";
}

void web::acct::get_info(IN const char *filename, OUT struct File_Info *info)
{
    info->file_length = -1; // length is unknown
    info->last_modified = time(NULL);
    info->is_directory = 0; 
    info->is_readable = 1;
    String content_type = String("text/html; charset=") + DEFAULT_INTERNAL_CHARSET;
    info->content_type = ixmlCloneDOMString(content_type.c_str());
}


