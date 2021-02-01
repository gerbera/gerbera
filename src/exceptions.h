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
#include <string>

#ifdef __PRETTY_FUNCTION__
// GCC + llvm
#define PRETTY_FUNCTION __PRETTY_FUNCTION__
#else
// C99
#define PRETTY_FUNCTION __func__
#endif

#define throw_std_runtime_error(...) throw std::runtime_error(fmt::format("[{}:{}] {} Error: {}", __FILE__, __LINE__, PRETTY_FUNCTION, fmt::format(__VA_ARGS__)))

class ConfigParseException : public std::runtime_error {
public:
    explicit ConfigParseException(const std::string& message)
        : std::runtime_error(message)
    {
    }
};

class UpnpException : public std::runtime_error {
protected:
    int errCode;

public:
    UpnpException(int errCode, const std::string& message);
    [[nodiscard]] int getErrorCode() const { return errCode; }
};

class DatabaseException : public std::runtime_error {
protected:
    std::string userMessage;

public:
    DatabaseException(std::string _userMessage, const std::string& message)
        : std::runtime_error(message)
        , userMessage(std::move(_userMessage))
    {
    }
    std::string getUserMessage() const { return (!userMessage.empty() ? userMessage : what()); }
};

class ObjectNotFoundException : public DatabaseException {
public:
    explicit ObjectNotFoundException(const std::string& message)
        : DatabaseException(message, message)
    {
    }
};

class SubtitlesNotFoundException : public std::runtime_error {
public:
    explicit SubtitlesNotFoundException(const std::string& message)
        : std::runtime_error(message)
    {
    }
};

class ServerShutdownException : public std::runtime_error {
public:
    explicit ServerShutdownException(const std::string& message)
        : std::runtime_error(message)
    {
    }
};

class TryAgainException : public std::runtime_error {
public:
    explicit TryAgainException(const std::string& message)
        : std::runtime_error(message)
    {
    }
};

#endif // __EXCEPTIONS_H__
