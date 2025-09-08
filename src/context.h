/*GRB*

    Gerbera - https://gerbera.io/

    context.h - this file is part of Gerbera.

    Copyright (C) 2021-2025 Gerbera Contributors

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

/// @file context.h

#ifndef __CONTEXT_H__
#define __CONTEXT_H__

#include <memory>

// forward declarations
class Config;
class ConfigDefinition;
class ClientManager;
class Database;
class Mime;
class Server;
class UpdateManager;
class ConverterManager;
namespace Web {
class SessionManager;
} // namespace Web

class Context {
public:
    Context(
        std::shared_ptr<ConfigDefinition> definition,
        std::shared_ptr<Config> config,
        std::shared_ptr<ClientManager> clients,
        std::shared_ptr<Mime> mime,
        std::shared_ptr<Database> database,
        std::shared_ptr<Web::SessionManager> sessionManager,
        std::shared_ptr<ConverterManager> converterManager);

    std::shared_ptr<ConfigDefinition> getDefinition() const
    {
        return definition;
    }

    std::shared_ptr<Config> getConfig() const
    {
        return config;
    }

    std::shared_ptr<ClientManager> getClients() const
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

    std::shared_ptr<Web::SessionManager> getSessionManager() const
    {
        return session_manager;
    }

    std::shared_ptr<ConverterManager> getConverterManager() const
    {
        return converterManager;
    }

private:
    std::shared_ptr<ConfigDefinition> definition;
    std::shared_ptr<Config> config;
    std::shared_ptr<ClientManager> clients;
    std::shared_ptr<Mime> mime;
    std::shared_ptr<Database> database;
    std::shared_ptr<Web::SessionManager> session_manager;
    std::shared_ptr<ConverterManager> converterManager;
};

#endif // __CONTEXT_H__
