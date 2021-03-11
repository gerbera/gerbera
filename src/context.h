/*GRB*

    Gerbera - https://gerbera.io/
    
    context.h - this file is part of Gerbera.
    
    Copyright (C) 2021 Gerbera Contributors
    
    Gerbera is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License version 2
    as published by the Free Software Foundation.

    Gerbera is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with Gerbera.  If not, see <http://www.gnu.org/licenses/>.

    $Id$
*/

/// \file context.h

#ifndef __CONTEXT_H__
#define __CONTEXT_H__

#include <memory>

// forward declaration
class Config;
class Clients;
class Database;
class Mime;
class Server;
class UpdateManager;
namespace web {
class SessionManager;
} // namespace web

class Context {
public:
    Context(std::shared_ptr<Config> config,
        std::shared_ptr<Clients> clients,
        std::shared_ptr<Mime> mime,
        std::shared_ptr<Database> database,
        std::shared_ptr<Server> server,
        std::shared_ptr<web::SessionManager> session_manager);

    virtual ~Context() = default;

    std::shared_ptr<Config> getConfig() const
    {
        return config;
    }

    std::shared_ptr<Clients> getClients() const
    {
        return clients;
    }

    std::shared_ptr<Mime> getMime() const
    {
        return mime;
    }

    std::shared_ptr<Database> getDatabase() const
    {
        return database;
    }

    std::shared_ptr<Server> getServer() const
    {
        return server;
    }

    std::shared_ptr<web::SessionManager> getSessionManager() const
    {
        return session_manager;
    }

private:
    std::shared_ptr<Config> config;
    std::shared_ptr<Clients> clients;
    std::shared_ptr<Mime> mime;
    std::shared_ptr<Database> database;
    std::shared_ptr<Server> server;
    std::shared_ptr<web::SessionManager> session_manager;
};

#endif // __CONTEXT_H__
