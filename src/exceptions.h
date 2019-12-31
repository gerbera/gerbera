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

#include "util/exception.h"

#define EXCEPTION_DATA __FILENAME__, __LINE__, __func__
#define _UpnpException(code, msg) UpnpException(code, msg, EXCEPTION_DATA)
#define _StorageException(usermsg, debugmsg) StorageException(usermsg, debugmsg, EXCEPTION_DATA)
#define _ObjectNotFoundException(msg) ObjectNotFoundException(msg, EXCEPTION_DATA)
#define _ServerShutdownException(msg) ServerShutdownException(msg, EXCEPTION_DATA)
#define _TryAgainException(msg) TryAgainException(msg, EXCEPTION_DATA)

class UpnpException : public Exception {
protected:
    int errCode;

public:
    UpnpException(int errCode, std::string message);
    UpnpException(int errCode, std::string message, const char* file, int line, const char* function);
    inline int getErrorCode() const { return errCode; }
};

class StorageException : public Exception {
protected:
    std::string userMessage;

public:
    inline StorageException(std::string _userMessage, std::string message)
        : Exception(message)
    {
        userMessage = _userMessage;
    }
    inline StorageException(std::string _userMessage, std::string message, const char* file, int line, const char* function)
        : Exception(message, file, line, function)
    {
        userMessage = _userMessage;
    }
    std::string getUserMessage() const { return (!userMessage.empty() ? userMessage : message); }
};

class ObjectNotFoundException : public StorageException {
public:
    inline ObjectNotFoundException(std::string message)
        : StorageException(message, message)
    {
    }
    inline ObjectNotFoundException(std::string message, const char* file, int line, const char* function)
        : StorageException(message, message, file, line, function)
    {
    }
};

class SubtitlesNotFoundException : public Exception {
public:
    inline SubtitlesNotFoundException(std::string message)
        : Exception(message)
    {
    }
    inline SubtitlesNotFoundException(std::string message, const char* file, int line, const char* function)
        : Exception(message, file, line, function)
    {
    }
};

class ServerShutdownException : public Exception {
public:
    inline ServerShutdownException(std::string message)
        : Exception(message)
    {
    }
    inline ServerShutdownException(std::string message, const char* file, int line, const char* function)
        : Exception(message, file, line, function)
    {
    }
};

class TryAgainException : public Exception {
public:
    inline TryAgainException(std::string message)
        : Exception(message)
    {
    }
    inline TryAgainException(std::string message, const char* file, int line, const char* function)
        : Exception(message, file, line, function)
    {
    }
};

#endif // __EXCEPTIONS_H__
