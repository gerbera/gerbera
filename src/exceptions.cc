/*MT*

    MediaTomb - http://www.mediatomb.cc/

    exceptions.cc - this file is part of MediaTomb.

    Copyright (C) 2005 Gena Batyan <bgeradz@mediatomb.cc>,
                       Sergey 'Jin' Bostandzhyan <jin@mediatomb.cc>

    Copyright (C) 2006-2010 Gena Batyan <bgeradz@mediatomb.cc>,
                            Sergey 'Jin' Bostandzhyan <jin@mediatomb.cc>,
                            Leonhard Wimmer <leo@mediatomb.cc>

    Copyright (C) 2016-2026 Gerbera Contributors

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

/// @file exceptions.cc

#include "exceptions.h" // API

DatabaseException::DatabaseException(std::string userMessage, const std::string& message)
    : std::runtime_error(fmt::format("{}: Error: {}", message, userMessage))
    , userMessage(std::move(userMessage))
{
}

ObjectNotFoundException::ObjectNotFoundException(const std::string& message)
    : DatabaseException(message, message)
{
}

UpnpException::UpnpException(int errCode, const std::string& message)
    : std::runtime_error(message)
    , errCode(errCode)
{
}
