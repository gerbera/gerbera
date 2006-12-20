/*MT*
    
    MediaTomb - http://www.mediatomb.org/
    
    rexp.h - this file is part of MediaTomb.
    
    Copyright (C) 2005 Gena Batyan <bgeradz@mediatomb.org>,
                       Sergey Bostandzhyan <jin@mediatomb.org>
    Copyright (C) 2006 Gena Batyan <bgeradz@mediatomb.org>,
                       Sergey Bostandzhyan <jin@mediatomb.org>,
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

/// \file rexp.h

#ifndef __REXP_H__
#define __REXP_H__

#include "common.h"
#include <sys/types.h>
#include <regex.h>

#define DEFAULT_NMATCH 10

class Matcher;

class RExp : public zmm::Object
{
public:
    RExp();
    virtual ~RExp();
    void compile(zmm::String pattern, int flags = 0);
    void compile(zmm::String pattern, const char *flags);
    zmm::Ref<Matcher> matcher(zmm::String text, int nmatch = DEFAULT_NMATCH);
    zmm::Ref<Matcher> match(zmm::String text, int nmatch = DEFAULT_NMATCH);
    bool matches(zmm::String text);
    zmm::String getPattern();
protected:
    bool isCompiled;
    zmm::String pattern;
    regex_t regex;

    friend class Matcher;
};


class Matcher : public zmm::Object
{
public:
    virtual ~Matcher();
    zmm::String group(int i);
    bool next();
    bool matches();
protected:
    Matcher(zmm::Ref<RExp> rexp, zmm::String text, int nmatch);
protected:    
    zmm::Ref<RExp> rexp;
    zmm::String text;
    char *ptr;
    int nmatch;
    regmatch_t *pmatch;
    
    friend class RExp;
};


#endif // __REXP_H__

