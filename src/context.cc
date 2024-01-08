/*GRB*

    Gerbera - https://gerbera.io/

    context.cc - this file is part of Gerbera.

    Copyright (C) 2021-2024 Gerbera Contributors

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

/// \file context.cc

#include "context.h" // API

Context::Context(std::shared_ptr<Config> config,
    std::shared_ptr<ClientManager> clients,
    std::shared_ptr<Mime> mime,
    std::shared_ptr<Database> database,
    std::shared_ptr<Web::SessionManager> sessionManager)
    : config(std::move(config))
    , clients(std::move(clients))
    , mime(std::move(mime))
    , database(std::move(database))
    , session_manager(std::move(sessionManager))
{
}
