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

#include "zmm/zmmf.h"

#define EXCEPTION_DATA __FILENAME__, __LINE__, __func__
#define _UpnpException(code, msg) UpnpException(code, msg, EXCEPTION_DATA)
#define _StorageException(usermsg, debugmsg) StorageException(usermsg, debugmsg, EXCEPTION_DATA)
#define _ObjectNotFoundException(msg) ObjectNotFoundException(msg, EXCEPTION_DATA)
#define _ServerShutdownException(msg) ServerShutdownException(msg, EXCEPTION_DATA)
#define _TryAgainException(msg) TryAgainException(msg, EXCEPTION_DATA)

class UpnpException : public zmm::Exception {
protected:
    int errCode;

public:
    UpnpException(int errCode, zmm::String message);
    UpnpException(int errCode, zmm::String message, const char* file, int line, const char* function);
    inline int getErrorCode() const { return errCode; }
};

class StorageException : public zmm::Exception {
protected:
    zmm::String userMessage;

public:
    inline StorageException(zmm::String _userMessage, zmm::String message)
        : zmm::Exception(message)
    {
        userMessage = _userMessage;
    }
    inline StorageException(zmm::String _userMessage, zmm::String message, const char* file, int line, const char* function)
        : zmm::Exception(message, file, line, function)
    {
        userMessage = _userMessage;
    }
    zmm::String getUserMessage() const { return (userMessage != nullptr ? userMessage : message); }
};

class ObjectNotFoundException : public StorageException {
public:
    inline ObjectNotFoundException(zmm::String message)
        : StorageException(message, message)
    {
    }
    inline ObjectNotFoundException(zmm::String message, const char* file, int line, const char* function)
        : StorageException(message, message, file, line, function)
    {
    }
};

class SubtitlesNotFoundException : public zmm::Exception {
public:
    inline SubtitlesNotFoundException(zmm::String message)
        : zmm::Exception(message)
    {
    }
    inline SubtitlesNotFoundException(zmm::String message, const char* file, int line, const char* function)
        : zmm::Exception(message, file, line, function)
    {
    }
};

class ServerShutdownException : public zmm::Exception {
public:
    inline ServerShutdownException(zmm::String message)
        : zmm::Exception(message)
    {
    }
    inline ServerShutdownException(zmm::String message, const char* file, int line, const char* function)
        : zmm::Exception(message, file, line, function)
    {
    }
};

class TryAgainException : public zmm::Exception {
public:
    inline TryAgainException(zmm::String message)
        : zmm::Exception(message)
    {
    }
    inline TryAgainException(zmm::String message, const char* file, int line, const char* function)
        : zmm::Exception(message, file, line, function)
    {
    }
};

class SingletonException : public zmm::Exception {
public:
    inline SingletonException(zmm::String message)
        : zmm::Exception(message)
    {
    }
    inline SingletonException(zmm::String message, const char* file, int line, const char* function)
        : zmm::Exception(message, file, line, function)
    {
    }
};

#endif // __EXCEPTIONS_H__
