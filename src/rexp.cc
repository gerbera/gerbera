/*MT*
    
    MediaTomb - http://www.mediatomb.cc/
    
    rexp.cc - this file is part of MediaTomb.
    
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

/// \file rexp.cc

#include "rexp.h"

using namespace zmm;

static String error_string(int code, regex_t* regex)
{
    int size = regerror(code, regex, nullptr, 0);
    String buf = String::allocate(size);
    regerror(code, regex, const_cast<char*>(buf.c_str()), size);
    buf.setLength(size - 1);
    return buf;
}

RExp::RExp()
    : Object()
{
    isCompiled = false;
}
RExp::~RExp()
{
    if (isCompiled) {
        regfree(&regex);
    }
}
String RExp::getPattern()
{
    return pattern;
}
void RExp::compile(String pattern, int flags)
{
    int ret;
    flags |= REG_EXTENDED; // alsays use extended regexps
    this->pattern = pattern;
    ret = regcomp(&regex, pattern.c_str(), flags);
    if (ret != 0)
        throw _Exception(error_string(ret, &regex));
    isCompiled = true;
}

void RExp::compile(zmm::String pattern, const char* sflags)
{
    int flags = 0;
    auto* p = (char*)sflags;
    char c;
    while ((c = *p) != 0) {
        switch (c) {
        case 'i':
            flags |= REG_ICASE;
            break;
        case 's':
            flags |= REG_NEWLINE;
            break;
        default:
            throw _Exception(_("RExp: unknown flag: ") + c);
        }
        p++;
    }
    compile(pattern, flags);
}

Ref<Matcher> RExp::matcher(String text, int nmatch)
{
    return Ref<Matcher>(new Matcher(Ref<RExp>(this), text, nmatch));
}
Ref<Matcher> RExp::match(String text, int nmatch)
{
    Ref<Matcher> m = matcher(text, nmatch);
    if (m->next())
        return m;
    else
        return nullptr;
}

bool RExp::matches(String text)
{
    Ref<Matcher> matcher(new Matcher(Ref<RExp>(this), text, 0));
    return matcher->next();
}

Matcher::Matcher(zmm::Ref<RExp> rexp, String text, int nmatch)
{
    this->rexp = rexp;
    this->text = text;
    this->ptr = nullptr;
    this->nmatch = nmatch++;
    if (this->nmatch)
        this->pmatch = (regmatch_t*)MALLOC(this->nmatch * sizeof(regmatch_t));
    else
        this->pmatch = nullptr;
}
Matcher::~Matcher()
{
    if (pmatch)
        FREE(pmatch);
}
String Matcher::group(int i)
{
    regmatch_t* m = pmatch + i;
    if (m->rm_so >= 0)
        return String(ptr + m->rm_so, m->rm_eo - m->rm_so);
    else
        return nullptr;
}
bool Matcher::next()
{
    int ret;

    if (ptr == nullptr) // first match
    {
        ptr = const_cast<char*>(text.c_str());
    } else {
        if (!*ptr)
            return false;
        ptr += pmatch->rm_eo;
        if (!*ptr)
            return false;
    }

    int flags = ((nmatch == 0) ? REG_NOSUB : 0);
    ret = regexec(&rexp->regex, ptr, nmatch, pmatch, flags);
    switch (ret) {
    case REG_NOMATCH:
        return false;
    case 0:
        return true;
    default:
        throw _Exception(error_string(ret, &rexp->regex));
    }
}
