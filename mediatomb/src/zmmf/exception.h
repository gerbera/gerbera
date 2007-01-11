/*MT*
    
    MediaTomb - http://www.mediatomb.org/
    
    exception.h - this file is part of MediaTomb.
    
    Copyright (C) 2005 Gena Batyan <bgeradz@mediatomb.org>,
                       Sergey 'Jin' Bostandzhyan <jin@mediatomb.org>
    
    Copyright (C) 2006-2007 Gena Batyan <bgeradz@mediatomb.org>,
                            Sergey 'Jin' Bostandzhyan <jin@mediatomb.org>,
                            Leonhard Wimmer <leo@mediatomb.org>
    
    MediaTomb is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License version 2
    as published by the Free Software Foundation.
    
    MediaTomb is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.
    
    You should have received a copy of the GNU General Public License
    version 2 along with MediaTomb; if not, write to the Free Software
    Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
    
    $Id$
*/

/// \file exception.h

#ifndef __ZMMF_EXCEPTION_H__
#define __ZMMF_EXCEPTION_H__

#include "zmm/zmm.h"

#include "array.h"
#include <stdio.h>
#include "logger.h"

#define _Exception(format) zmm::Exception(format, __FILE__, __LINE__, __func__)

namespace zmm
{

class Exception
{
protected:
    String message;
    String file;
    String function;
    int    line;

    Ref<Array<StringBase> > stackTrace;

public:
    Exception(String message, const char* file, int line, const char* function);
    Exception(String message);
    String getMessage();

    Ref<Array<StringBase> > getStackTrace();
#ifdef LOG_ENABLED
    void printStackTrace(FILE *file = LOG_FILE);
#else
    inline void printStackTrace(FILE *file = LOG_FILE) {}; 
#endif

};

}

#endif // __ZMMF_EXCEPTION_H__
