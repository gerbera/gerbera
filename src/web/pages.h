/*MT*
    
    MediaTomb - http://www.mediatomb.cc/
    
    pages.h - this file is part of MediaTomb.
    
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

/// \file pages.h
/// \brief Defines the various WebRequestHandler subclasses which process requests coming to the ui.
#ifndef __WEB_PAGES_H__
#define __WEB_PAGES_H__

#include "cds_objects.h"
#include "common.h"
#include "request_handler.h"
#include "web_request_handler.h"

// forward declaration
class ConfigManager;
class Storage;

namespace web {

// forward declaration
class SessionManager;

/// \brief Authentication handler (used over AJAX)
class auth : public WebRequestHandler {
protected:
    int timeout;

public:
    auth(const std::shared_ptr<ConfigManager>& config, std::shared_ptr<Storage> storage,
        std::shared_ptr<ContentManager> content, std::shared_ptr<SessionManager> sessionManager);
    void process() override;
};

/// \brief Browser container tree
class containers : public WebRequestHandler {
public:
    containers(std::shared_ptr<ConfigManager> config, std::shared_ptr<Storage> storage,
        std::shared_ptr<ContentManager> content, std::shared_ptr<SessionManager> sessionManager);
    void process() override;
};

/// \brief Browser directory tree
class directories : public WebRequestHandler {
public:
    directories(std::shared_ptr<ConfigManager> config, std::shared_ptr<Storage> storage,
        std::shared_ptr<ContentManager> content, std::shared_ptr<SessionManager> sessionManager);
    void process() override;
};

/// \brief Browser file list
class files : public WebRequestHandler {
public:
    files(std::shared_ptr<ConfigManager> config, std::shared_ptr<Storage> storage,
        std::shared_ptr<ContentManager> content, std::shared_ptr<SessionManager> sessionManager);
    void process() override;
};

/// \brief Browser item list
class items : public WebRequestHandler {
public:
    items(std::shared_ptr<ConfigManager> config, std::shared_ptr<Storage> storage,
        std::shared_ptr<ContentManager> content, std::shared_ptr<SessionManager> sessionManager);
    void process() override;
};

/// \brief Browser add item
class add : public WebRequestHandler {
public:
    add(std::shared_ptr<ConfigManager> config, std::shared_ptr<Storage> storage,
        std::shared_ptr<ContentManager> content, std::shared_ptr<SessionManager> sessionManager);
    void process() override;
};

/// \brief Browser remove item
class remove : public WebRequestHandler {
public:
    remove(std::shared_ptr<ConfigManager> config, std::shared_ptr<Storage> storage,
        std::shared_ptr<ContentManager> content, std::shared_ptr<SessionManager> sessionManager);
    void process() override;
};

/// \brief Browser remove item
class edit_load : public WebRequestHandler {
public:
    edit_load(std::shared_ptr<ConfigManager> config, std::shared_ptr<Storage> storage,
        std::shared_ptr<ContentManager> content, std::shared_ptr<SessionManager> sessionManager);
    void process() override;
};

/// \brief Browser remove item
class edit_save : public WebRequestHandler {
public:
    edit_save(std::shared_ptr<ConfigManager> config, std::shared_ptr<Storage> storage,
        std::shared_ptr<ContentManager> content, std::shared_ptr<SessionManager> sessionManager);
    void process() override;
};

/// \brief Browser add object.
class addObject : public WebRequestHandler {
public:
    addObject(std::shared_ptr<ConfigManager> config, std::shared_ptr<Storage> storage,
        std::shared_ptr<ContentManager> content, std::shared_ptr<SessionManager> sessionManager);
    void process() override;

protected:
    void addContainer(int parentID);
    std::shared_ptr<CdsObject> addItem(int parentID, std::shared_ptr<CdsItem> item);
    std::shared_ptr<CdsObject> addUrl(int parentID, std::shared_ptr<CdsItemExternalURL> item, bool addProtocol);
    std::shared_ptr<CdsObject> addActiveItem(int parentID);
};

/// \brief autoscan add and remove
class autoscan : public WebRequestHandler {
public:
    autoscan(std::shared_ptr<ConfigManager> config, std::shared_ptr<Storage> storage,
        std::shared_ptr<ContentManager> content, std::shared_ptr<SessionManager> sessionManager);
    void process() override;

protected:
    static void autoscan2XML(const std::shared_ptr<AutoscanDirectory>& adir, pugi::xml_node* element);
};

/// \brief nothing :)
class voidType : public WebRequestHandler {
public:
    voidType(std::shared_ptr<ConfigManager> config, std::shared_ptr<Storage> storage,
        std::shared_ptr<ContentManager> content, std::shared_ptr<SessionManager> sessionManager)
        : WebRequestHandler(config, storage, content, sessionManager)
    {
    }
    void process() override;
};

/// \brief task list and task cancel
class tasks : public WebRequestHandler {
public:
    tasks(std::shared_ptr<ConfigManager> config, std::shared_ptr<Storage> storage,
        std::shared_ptr<ContentManager> content, std::shared_ptr<SessionManager> sessionManager);
    void process() override;
};

/// \brief UI action button
class action : public WebRequestHandler {
public:
    action(std::shared_ptr<ConfigManager> config, std::shared_ptr<Storage> storage,
        std::shared_ptr<ContentManager> content, std::shared_ptr<SessionManager> sessionManager);
    void process() override;
};

/// \brief Chooses and creates the appropriate handler for processing the request.
/// \param page identifies what type of the request we are dealing with.
/// \return the appropriate request handler.
std::unique_ptr<WebRequestHandler> createWebRequestHandler(
    const std::shared_ptr<ConfigManager>& config, const std::shared_ptr<Storage>& storage,
    const std::shared_ptr<ContentManager>& content, const std::shared_ptr<SessionManager>& sessionManager,
    const std::string& page);

} // namespace

#endif // __WEB_PAGES_H__
