/*MT*
    
    MediaTomb - http://www.mediatomb.cc/
    
    stringtokenizer.h - this file is part of MediaTomb.
    
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

/// \file stringtokenizer.h

#ifndef __STRING_TOKENIZER_H__
#define __STRING_TOKENIZER_H__

#include <string>

#include "zmm/zmmf.h"

class StringTokenizer : public zmm::Object
{
public:
    StringTokenizer(std::string str);
    std::string nextToken(std::string seps);
protected:
    std::string str;
    int len;
    int pos;
};

#endif // __STRING_TOKENIZER_H__
