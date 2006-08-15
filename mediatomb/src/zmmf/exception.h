/*  exception.h - this file is part of MediaTomb.
                                                                                
    Copyright (C) 2005 Gena Batyan <bgeradz@deadlock.dhs.org>,
                       Sergey Bostandzhyan <jin@deadlock.dhs.org>
                                                                                
    MediaTomb is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.
                                                                                
    MediaTomb is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.
                                                                                
    You should have received a copy of the GNU General Public License
    along with MediaTomb; if not, write to the Free Software
    Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
*/

#ifndef __ZMMF_EXCEPTION_H__
#define __ZMMF_EXCEPTION_H__

#include "zmm/zmm.h"

#include "array.h"
#include <stdio.h>
#include "logger.h"

#define _Exception(format) Exception(format, __FILE__, __LINE__, __func__)

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
