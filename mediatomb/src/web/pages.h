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

/// \brief Show accounting information.
/*
class acct : public WebRequestHandler
{
public:
    acct();
    virtual void process();
};

/// \brief Add a file or directory to database.
class add : public WebRequestHandler
{
public:
    add();
    virtual void process();
};
*/
/// \brief Authentication handler (used over AJAX)
class auth : public WebRequestHandler
{
public:
    auth();
    virtual void process();
};
/*
/// \brief Browser frameset
class browse : public WebRequestHandler
{
public:
    browse();
    virtual void process();
};
*/
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
/*
/// \brief Save the changes that were made to an item or container to the database.
class edit_save : public WebRequestHandler
{
public:
    edit_save();
    virtual void process();
};

/// \brief Provide user interface for editing item or container properties.
class edit_ui : public WebRequestHandler
{
public:
    edit_ui();
    virtual void process();
};
*/
/// \brief Browser file list
class files : public WebRequestHandler
{
public:
    files();
    virtual void process();
};
/*
/// \brief UI head
class head : public WebRequestHandler
{
public:
    head();
    virtual void process();
};
*/
/// \brief main frameset
class index : public WebRequestHandler
{
public:
    index();
    virtual void process();
};

/// \brief Browser item list
class items : public WebRequestHandler
{
public:
    items();
    virtual void process();
};

/// \brief Validate user login and create a session.
class login : public WebRequestHandler
{
public:
    login();
    virtual void process();
};
/*
/// \brief Remove an item or container from the database.
class remove : public WebRequestHandler
{
public:
    remove();
    virtual void process();
};

/// \brief Render tree of containers/directories.
class task : public WebRequestHandler
{
public:
    task();
    virtual void process();
};

/// \brief Render tree of containers/directories.
class tree : public WebRequestHandler
{
public:
    tree();
    virtual void process();
};


/// \brief provide user interface for creating new (virtual) items or containers.
class new_ui : public WebRequestHandler
{
protected:
    void addContainer();
    void addItem();
    void addUrl();
    void addInternalUrl();
    void addActiveItem();
    void addObject();
public:
    new_ui();
    virtual void process();
};

/// \brief Refresh current container.
class refresh : public WebRequestHandler
{
public:
    refresh();
    virtual void process();
};


/// \brief Display a page with an error message.
class error : public WebRequestHandler
{
public:
    error();
    virtual void process();
};
*/

} // namespace

/// \brief Chooses and creates the appropriate handler for processing the request.
/// \param page identifies what type of the request we are dealing with.
/// \return the appropriate request handler.
WebRequestHandler *create_web_request_handler(zmm::String page);

#endif // __WEB_PAGES_H__

