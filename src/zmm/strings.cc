/*MT*
    
    MediaTomb - http://www.mediatomb.cc/
    
    strings.cc - this file is part of MediaTomb.
    
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

/// \file strings.cc
#include <cctype>

#include "memory.h"
#include "strings.h"

using namespace zmm;

StringBase::StringBase(int capacity) : Object()
{
    len = capacity;
    data = (char *)MALLOC((len + 1) * sizeof(char));
    store = true;
}
StringBase::StringBase(const char *str) : Object()
{
    len = (int)strlen(str);
    data = (char *)MALLOC((len + 1) * sizeof(char));
    strcpy(data, str);
    store = true;
}
StringBase::StringBase(const char *str, int len) : Object()
{
    this->len = len;
    data = (char *)MALLOC((len + 1) * sizeof(char));
    memcpy(data, str, len);
    data[len] = 0;
    store = true;
}

StringBase::~StringBase()
{
    if (store)
        FREE(data);
}

bool StringBase::startsWith(StringBase *other)
{
    return (!strncmp(data, other->data, other->len));
}

String::String()
{
    base = nullptr;
}
String::String(int capacity)
{
    base = new StringBase(capacity);
    base->retain();
}
String::String(const char *str)
{
    if(str)
    {
        base = new StringBase(str);
        base->retain();
    }
    else
        base = nullptr;
}
String::String(const char *str, int len)
{
    if(str)
    {
        base = new StringBase(str, len);
        base->retain();
    }
    else
        base = nullptr;
}
String::String(char ch)
{
    base = new StringBase(&ch, 1);
    base->retain();
}

String::String(const String &other)
{
    base = other.base;
    if(base)
        base->retain();
}
String::String(StringBase *other)
{
    base = other;
    if(base)
        base->retain();
}
String::String(Ref<StringBase> other)
{
    base = other.getPtr();
    if(base)
        base->retain();
}
String::String(const std::string &other) : String(other.c_str(), other.length())
{}

String::~String()
{
    if(base)
        base->release();
}

int String::length()
{
    if(base)
        return base->len;
    else
        return 0;
}
const char *String::c_str() const
{
    if(base)
        return base->data;
    else
        return nullptr;
}
String String::operator+(String other)
{
    if(! other.base)
        return *this;
    if(! base)
        return other;
    int len = base->len;
    int otherLen = other.base->len;

    String res(len + otherLen);
    strcpy(res.base->data, base->data);
    strcpy(res.base->data + len, other.base->data);
    return res;
}
String String::operator+(const char *other)
{
    if(! other)
        return *this;
    if(! base)
        return String::copy(other);
    int len = base->len;
    auto otherLen = (int)strlen(other);
    String res(len + otherLen);
    strcpy(res.base->data, base->data);
    strcpy(res.base->data + len, other);
    return res;
}
String String::operator+(char chr)
{
    char str[2];
    str[0] = chr;
    str[1] = 0;
    return operator+(str);
}

String String::operator+(int x)
{
    return operator+(from(x));
}
String String::operator+(unsigned int x)
{
    return operator+(from(x));
}
String String::operator+(double x)
{
    return operator+(from(x));
}

String String::from(int x)
{
    auto *b = new StringBase(MAX_INT_STRING_LENGTH);
    b->len = sprintf(b->data, "%d", x);
    return (String(b));
}
String String::from(unsigned int x)
{
    auto *b = new StringBase(MAX_INT_STRING_LENGTH);
    b->len = sprintf(b->data, "%u", x);
    return (String(b));
}
String String::from(long x)
{
    auto *b = new StringBase(MAX_LONG_STRING_LENGTH);
    b->len = sprintf(b->data, "%ld", x);
    return (String(b));
}
String String::from(unsigned long x)
{
    auto *b = new StringBase(MAX_LONG_STRING_LENGTH);
    b->len = sprintf(b->data, "%ld", x);
    return (String(b));
}
String String::from(double x)
{
    auto *b = new StringBase(MAX_DOUBLE_STRING_LENGTH);
    b->len = sprintf(b->data, "%lf", x);
    return (String(b));
}
String String::from(long long x)
{
    auto *b = new StringBase(MAX_LONG_LONG_STRING_LENGTH);
    b->len = sprintf(b->data, "%lld", x);
    return (String(b));
}
/*
String String::from(int64_t x)
{
    StringBase *b = new StringBase(MAX_INT64_T_STRING_LENGTH);
    b->len = sprintf(b->data, "%"PRIx64, x);
    return (String(b));
}
*/
String String::allocate(int size)
{
    return String(new StringBase(size));
}
String String::take(const char *data)
{
    auto *base = new StringBase();
    base->data = (char *)data;
    base->len = strlen(data);
    base->store = true;
    return String(base);
}
String String::take(const char *data, int length)
{
    auto *base = new StringBase();
    base->data = (char *)data;
    base->len = length;
    base->store = true;
    return String(base);
}
String String::refer(const char *data)
{
    auto *base = new StringBase();
    base->data = (char *)data;
    base->len = strlen(data);
    base->store = false;
    return String(base);
}
String String::refer(const char *data, int len)
{
    auto *base = new StringBase();
    base->data = (char *)data;
    base->len = len;
    base->store = false;
    return String(base);
}
String String::refer(const std::optional<std::string> &str)
{
    if (str.has_value()) {
        auto base = new StringBase(str.value().c_str(), str.value().length());
        base->retain();
        return String(base);
    }
    return nullptr;
}
String String::copy(const char *data)
{
    if (data)
    {
        auto *base = new StringBase(data);
        return String(base);
    }
    else
        return String();
}
int String::operator==(String other) const
{
    if(! base && ! other.base)
        return 1;
    if(base && other.base)
        return ( ! strcmp(base->data, other.base->data) );
    return 0;
}
int String::operator==(const char *other) const
{
    if(! base && ! other)
        return 1;
    if(base && other)
        return ( ! strcmp(base->data, other ) );
    return 0;
}
int String::operator==(char c) const
{
    if(! base || base->len != 1)
        return 0;
    return (c == *(base->data));
}
String& String::operator=(const char *str)
{
    if (base)
        base->release();
    if (str)
    {
        base = new StringBase(str);
        base->retain();
    }
    else
        base = nullptr;
    return *this;
}
String& String::operator=(String other)
{
    if (this == &other)
        return *this;
    if(base)
        base->release();
    base = other.base;
    if(base)
        base->retain();
    return *this;
}

String &String::operator=(const std::string &other) {
    base = new StringBase(other.c_str(), other.length());
    base->retain();
    return *this;
}

int String::equals(String other, bool ignoreCase) const
{
    if(! base && ! other.base)
        return 1;
    
    if (ignoreCase)
    {
        if(base && other.base)
            return ( ! strcasecmp(base->data, other.base->data) );
    }
    else
        return (operator==(other));

    return 0;
}

String String::toLower()
{
    if (!base)
        return nullptr;

    String result = String(base->data, base->len);
    for (int i = 0; i < result.base->len; i++)
    {
        result.base->data[i] = tolower(result.base->data[i]);
    }
    return result;
}

String String::toUpper()
{
    if (!base)
        return nullptr;

    String result = String(base->data, base->len);
    for (int i = 0; i < base->len; i++)
    {
        result.base->data[i] = toupper(result.base->data[i]);
    }

    return result;
}

long String::toLong()
{
    if(! base)
        return 0;
    char *endptr;
    long res = (int)strtol(base->data, &endptr, 10);
    if(*endptr)
        return 0;
    // TODO: throw exception
    return res;
}
double String::toDouble()
{
    if(! base)
        return 0;
    char *endptr;
    double res = strtod(base->data, &endptr);
    if(*endptr)
        return 0;
    // TODO: throw exception
    return res;
}


off_t String::toOFF_T()
{
    if(! base)
        return 0;

#if SIZEOF_OFF_T > 4
    char *endptr;
    off_t res = strtoll(base->data, &endptr, 10);
    if(*endptr)
        return 0;

    return res;
    // TODO: throw exceptoin
#else
    return toLong();
#endif    
     
}


String String::substring(int from)
{
    if(! base)
        return nullptr;
    int count = base->len - from;
    return substring(from, count);
}
String String::substring(int from, int count)
{
    if(! base || count < 0)
        return nullptr;
    if(count == 0)
        return _("");
    if(from + count > base->len)
        count = base->len - from;
    String res(count);
    strncpy(res.base->data, base->data + from, count);
    *(res.base->data + count) = 0;
    return res;
}
int String::index(int start, char ch)
{
    if(! base)
        return -1;
    if (start < 0 || start + 1 > base->len)
        return -1;
    char *pos = ::strchr(base->data + start, ch);
    if (pos)
        return pos - base->data;
    else
        return -1;
}
int String::rindex(char ch)
{
    if(! base)
        return -1;
    char *pos = ::strrchr(base->data, ch);
    if (pos)
        return pos - base->data;
    else
        return -1;
}
int String::rindex(int end, char ch)
{
    if(! base)
        return -1;
    if (end < 0 || end + 1 > base->len)
        return -1;
    for (char *pos = base->data + end; pos >= base->data; pos--)
    {
        if (*pos == ch)
            return pos - base->data;
    }
    return -1;
}
String String::reduce(char ch)
{ 
    if (!base)
        return nullptr;

    char *pos = ::strchr(base->data, ch);
    if (!pos)
        return *this;
  
    String res(base->len);

    char *pos2 = res.base->data;
    char *pos3 = base->data;
    do
    {
        if (*(pos + 1) == ch)
        {
            if (pos-pos3 == 0)
            {
                *pos2 = ch;
                pos2++;
            }
            else
            {
                strncpy(pos2, pos3, (pos-pos3)+1);
                pos2 = pos2 + ((pos-pos3)+1);
            }
            while (*pos == ch) pos++;
            pos3 = pos;
            if (*pos == '\0')
            {
                *pos2 = '\0';
                break;
            }
            pos = ::strchr(pos, ch);
        }
        else
        {
            pos++;
            if (*pos == '\0')
                break;
            pos = ::strchr(pos, ch);
        }
    } while (pos);
    if ((base->data)+(base->len)-pos3)
        strncpy(pos2, pos3, (base->data)+(base->len)-pos3);
    pos2[(base->data)+(base->len)-pos3] = 0;
    res.updateLength();
    return res;
}

int String::find(const char *needle)
{
    if (!base)
        return -1;
    
    char *pos = strstr(base->data, needle);
    if (pos)
    {
        return (pos-(base->data));
    }
    else
        return -1;

}
int String::find(String needle)
{
    return find(needle.base->data);
}

String String::replace(String needle, String replacement)
{
    if (! replacement.base || ! needle.base)
        return nullptr;
    int pos = find(needle);
    if (pos < 0)
        return *this; 
    String res(base->len + replacement.base->len - needle.base->len);
    strncpy(res.base->data, base->data, pos);
    strncpy(res.base->data + pos, replacement.base->data, replacement.base->len);
    strncpy(res.base->data + pos + replacement.base->len, base->data + pos + needle.base->len, base->len - pos - needle.base->len);
    res.base->data[res.base->len] = 0;
    return res;
}

String String::replaceChar(char needle, char replacement)
{
    if ((needle == '\0') || (replacement == '\0'))
        return *this;

    String res(base->len);
    for (int i = 0; i < res.base->len; i++)
    {
        if (base->data[i] == needle)
            res.base->data[i] = replacement;
        else
            res.base->data[i] = base->data[i];
    }
    res.base->data[res.base->len] = 0;
    return res;
}

