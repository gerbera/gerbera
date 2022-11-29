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

#include "common.h"
#include "request_handler.h"
#include "web_request_handler.h"

// forward declaration
class CdsItemExternalURL;
class CdsItem;
class Config;
class ConfigValue;
class Database;
class UpnpXMLBuilder;

namespace Web {

// forward declaration
class SessionManager;

/// \brief Authentication handler (used over AJAX)
class Auth : public WebRequestHandler {
protected:
    std::chrono::seconds timeout {};

public:
    explicit Auth(const std::shared_ptr<ContentManager>& content);
    void process() override;
};

/// \brief Browser container tree
class Containers : public WebRequestHandler {
protected:
    std::shared_ptr<UpnpXMLBuilder> xmlBuilder;

public:
    explicit Containers(const std::shared_ptr<ContentManager>& content, std::shared_ptr<UpnpXMLBuilder> xmlBuilder);
    void process() override;
};

/// \brief Browser directory tree
class Directories : public WebRequestHandler {
    using WebRequestHandler::WebRequestHandler;

public:
    void process() override;
};

/// \brief Browser file list
class Files : public WebRequestHandler {
    using WebRequestHandler::WebRequestHandler;

public:
    void process() override;
};

/// \brief Browser item list
class Items : public WebRequestHandler {
protected:
    std::shared_ptr<UpnpXMLBuilder> xmlBuilder;

public:
    explicit Items(const std::shared_ptr<ContentManager>& content, std::shared_ptr<UpnpXMLBuilder> xmlBuilder);
    void process() override;
};

/// \brief Browser add item
class Add : public WebRequestHandler {
    using WebRequestHandler::WebRequestHandler;

public:
    void process() override;
};

/// \brief Browser remove item
class Remove : public WebRequestHandler {
    using WebRequestHandler::WebRequestHandler;

public:
    void process() override;
};

/// \brief Browser remove item
class EditLoad : public WebRequestHandler {
protected:
    std::shared_ptr<UpnpXMLBuilder> xmlBuilder;

public:
    explicit EditLoad(const std::shared_ptr<ContentManager>& content, std::shared_ptr<UpnpXMLBuilder> xmlBuilder);
    void process() override;
};

/// \brief Browser remove item
class EditSave : public WebRequestHandler {
    using WebRequestHandler::WebRequestHandler;

public:
    void process() override;
};

/// \brief Browser add object.
class AddObject : public WebRequestHandler {
    using WebRequestHandler::WebRequestHandler;

public:
    void process() override;

protected:
    void addContainer(int parentID);
    std::shared_ptr<CdsItem> addItem(int parentID);
    std::shared_ptr<CdsItemExternalURL> addUrl(int parentID, bool addProtocol);
};

/// \brief autoscan add and remove
class Autoscan : public WebRequestHandler {
    using WebRequestHandler::WebRequestHandler;

public:
    void process() override;

protected:
    static void autoscan2XML(const std::shared_ptr<AutoscanDirectory>& adir, pugi::xml_node& element);
};

/// \brief nothing :)
class VoidType : public WebRequestHandler {
    using WebRequestHandler::WebRequestHandler;

public:
    void process() override;
};

/// \brief task list and task cancel
class Tasks : public WebRequestHandler {
    using WebRequestHandler::WebRequestHandler;

public:
    void process() override;
};

/// \brief UI action button
class Action : public WebRequestHandler {
    using WebRequestHandler::WebRequestHandler;

public:
    void process() override;
};

/// \brief Chooses and creates the appropriate handler for processing the request.
/// \param page identifies what type of the request we are dealing with.
/// \return the appropriate request handler.
std::unique_ptr<WebRequestHandler> createWebRequestHandler(
    const std::shared_ptr<Context>& context,
    const std::shared_ptr<ContentManager>& content,
    const std::shared_ptr<UpnpXMLBuilder>& xmlBuilder,
    const std::string& page);

/// \brief Browse clients list
class Clients : public WebRequestHandler {
    using WebRequestHandler::WebRequestHandler;

public:
    void process() override;
};

/// \brief load configuration
class ConfigLoad : public WebRequestHandler {
protected:
    std::vector<ConfigValue> dbEntries;
    std::map<std::string, std::string> allItems;
    void createItem(pugi::xml_node& item, const std::string& name, config_option_t id, config_option_t aid, const std::shared_ptr<ConfigSetup>& cs = nullptr);
    template <typename T>
    static void setValue(pugi::xml_node& item, const T& value);

    static void addTypeMeta(pugi::xml_node& meta, const std::shared_ptr<ConfigSetup>& cs);

public:
    explicit ConfigLoad(const std::shared_ptr<ContentManager>& content);
    void process() override;
};

/// \brief save configuration
class ConfigSave : public WebRequestHandler {
protected:
    std::shared_ptr<Context> context;

public:
    explicit ConfigSave(std::shared_ptr<Context> context, const std::shared_ptr<ContentManager>& content);
    void process() override;
};

} // namespace Web

#endif // __WEB_PAGES_H__
