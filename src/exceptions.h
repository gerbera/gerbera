/*MT*
    
    MediaTomb - http://www.mediatomb.cc/
    
    exceptions.h - this file is part of MediaTomb.
    
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

/// \file exceptions.h

#ifndef __EXCEPTIONS_H__
#define __EXCEPTIONS_H__

#include <stdexcept>
#include <utility>
#include <string>

class ConfigParseException : public std::runtime_error {
public:
    inline ConfigParseException(std::string message)
        : std::runtime_error(message)
    {
    }
};

class UpnpException : public std::runtime_error {
protected:
    int errCode;

public:
    UpnpException(int errCode, const std::string& message);
    [[nodiscard]] inline int getErrorCode() const { return errCode; }
};

class StorageException : public std::runtime_error {
protected:
    std::string userMessage;

public:
    inline StorageException(std::string _userMessage, std::string message)
        : std::runtime_error(message)
    {
        userMessage = std::move(_userMessage);
    }
    std::string getUserMessage() const { return (!userMessage.empty() ? userMessage : what()); }
};

class ObjectNotFoundException : public StorageException {
public:
    inline explicit ObjectNotFoundException(std::string message)
        : StorageException(message, message)
    {
    }
};

class SubtitlesNotFoundException : public std::runtime_error {
public:
    inline explicit SubtitlesNotFoundException(std::string message)
        : std::runtime_error(message)
    {
    }
};

class ServerShutdownException : public std::runtime_error {
public:
    inline explicit ServerShutdownException(std::string message)
        : std::runtime_error(message)
    {
    }
};

class TryAgainException : public std::runtime_error {
public:
    inline explicit TryAgainException(std::string message)
        : std::runtime_error(message)
    {
    }
};

#endif // __EXCEPTIONS_H__
