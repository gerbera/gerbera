/*MT*
    
    MediaTomb - http://www.mediatomb.cc/
    
    rexp.h - this file is part of MediaTomb.
    
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

/// \file rexp.h

#ifndef __REXP_H__
#define __REXP_H__

#include "common.h"
#include <regex.h>
#include <sys/types.h>

#define DEFAULT_NMATCH 10

class Matcher;

class RExp : public zmm::Object {
public:
    RExp();
    virtual ~RExp();
    void compile(zmm::String pattern, int flags = 0);
    void compile(zmm::String pattern, const char* flags);
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

class Matcher : public zmm::Object {
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
    char* ptr;
    int nmatch;
    regmatch_t* pmatch;

    friend class RExp;
};

#endif // __REXP_H__
