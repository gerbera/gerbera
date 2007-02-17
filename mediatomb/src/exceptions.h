/*MT*
    
    MediaTomb - http://www.mediatomb.cc/
    
    exceptions.h - this file is part of MediaTomb.
    
    Copyright (C) 2005 Gena Batyan <bgeradz@mediatomb.cc>,
                       Sergey 'Jin' Bostandzhyan <jin@mediatomb.cc>
    
    Copyright (C) 2006-2007 Gena Batyan <bgeradz@mediatomb.cc>,
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

#include "zmmf/zmmf.h"

#define _UpnpException(code, format) UpnpException(code, format, __FILE__, __LINE__, __func__)
#define _StorageException(format) StorageException(format, __FILE__, __LINE__, __func__)

class UpnpException : public zmm::Exception
{
protected:
    int errCode;
public:
    UpnpException(int errCode, zmm::String message);
    UpnpException(int errCode, zmm::String message, const char *file, int line, const char *function);
    inline int getErrorCode() { return errCode; }
};

class StorageException : public zmm::Exception
{
public:
    inline StorageException(zmm::String message) : zmm::Exception(message) {}
    inline StorageException(zmm::String message, const char *file, int line, const char* function) : 
        zmm::Exception(message, file, line, function) {}
};

#endif // __EXCEPTIONS_H__

