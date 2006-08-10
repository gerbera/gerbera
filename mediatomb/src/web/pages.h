/*  pages.h - this file is part of MediaTomb.
                                                                                
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

/// \file pages.h
/// \brief Defines the various WebRequestHandler subclasses which process requests coming to the ui.
#ifndef __WEB_PAGES_H__
#define __WEB_PAGES_H__

#include "common.h"
#include "dictionary.h"
#include "request_handler.h"
#include "web_request_handler.h"

namespace web
{

/// \brief Authentication handler (used over AJAX)
class auth : public WebRequestHandler
{
public:
    auth();
    virtual void process();
};

/// \brief Browser container tree
class containers : public WebRequestHandler
{
public:
    containers();
    virtual void process();
};

/// \brief Browser directory tree
class directories : public WebRequestHandler
{
public:
    directories();
    virtual void process();
};

/// \brief Browser file list
class files : public WebRequestHandler
{
public:
    files();
    virtual void process();
};

/// \brief Browser item list
class items : public WebRequestHandler
{
public:
    items();
    virtual void process();
};

} // namespace

/// \brief Chooses and creates the appropriate handler for processing the request.
/// \param page identifies what type of the request we are dealing with.
/// \return the appropriate request handler.
WebRequestHandler *create_web_request_handler(zmm::String page);

#endif // __WEB_PAGES_H__

