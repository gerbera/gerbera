/*MT*
    
    MediaTomb - http://www.mediatomb.cc/
    
    exception.h - this file is part of MediaTomb.
    
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

/// \file exception.h

#ifndef __ZMMF_EXCEPTION_H__
#define __ZMMF_EXCEPTION_H__

#include <stdio.h>
#include <vector>
#include <string>

#include "zmm.h"
#include "array.h"

#include "../logger.h"

#define _Exception(format) zmm::Exception(format, __FILENAME__, __LINE__, __func__)

namespace zmm
{

class Exception
{
protected:
    std::string message;
    std::string file;
    std::string function;
    int line;
    std::vector<std::string> stackTrace;

public:
    Exception(std::string message, const char* file, int line, const char* function);
    Exception(std::string message);
    std::string getMessage() const;

    std::vector<std::string> getStackTrace();
#ifdef TOMBDEBUG
    void printStackTrace(FILE *file = LOG_FILE) const;
#else
    inline void printStackTrace(FILE *file = LOG_FILE) const {};
#endif

};

}

#endif // __ZMMF_EXCEPTION_H__
