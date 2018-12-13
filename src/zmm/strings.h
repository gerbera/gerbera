/*MT*
    
    MediaTomb - http://www.mediatomb.cc/
    
    strings.h - this file is part of MediaTomb.
    
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

/// \file strings.h

#ifndef __ZMM_STRINGS_H__
#define __ZMM_STRINGS_H__

#include <optional>
#include <string>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "object.h"
#include "ref.h"

#define _(str) zmm::String::refer(str)
//#define _(str) str

#define MAX_INT_STRING_LENGTH 12

//max ulong: 12345678901234567890  ???
#define MAX_LONG_STRING_LENGTH 22

#define MAX_DOUBLE_STRING_LENGTH 24

#define MAX_LONG_LONG_STRING_LENGTH 24

#define MAX_INT64_T_STRING_LENGTH 24

namespace zmm
{

class StringBase : public Object
{
public:
    char *data;
    int len;
    bool store; // if true, the object is responsible for freeing data

    StringBase(int capacity);
    StringBase(const char *str);
    StringBase(const char *str, int len);
    bool startsWith(StringBase *other);
    virtual ~StringBase();
protected:
    inline StringBase() : Object() {}
    friend class String;
};


class String
{
protected:
    StringBase *base;
public:
    String();
    String(const char *str);
    explicit String(char ch);
    String(const char *str, int len);
    String(const String &other);
    String(StringBase *other);
    String(Ref<StringBase> other);
    String(const std::string &other);

    inline StringBase *getBase()
    {
        return base;
    }

    inline String(std::nullptr_t)
    {
        base = NULL;
    }

    ~String();

    String &operator=(const char *str);

    String &operator=(String other);

    String &operator=(const std::string &other);

    inline String &operator=(std::nullptr_t)
    {
        if(base)
            base->release();
        base = NULL;
        return *this;
    }

    String operator+(String other);
    String operator+(const char *str);
    String operator+(char chr);
    String operator+(int x);
    String operator+(unsigned int x);
    String operator+(double x);

    int operator==(String other) const;
    int operator==(const char *str) const;
    int operator==(char c) const;

    inline int operator!=(String other) const
    {
        return ! operator==(other);
    }
    inline int operator!=(const char *str) const
    {
        return ! operator==(str);
    }
    inline int operator!=(char c) const
    {
        return ! operator==(c);
    }

    inline int operator==(std::nullptr_t) const
    {
        return (base == nullptr);
    }
    inline int operator!=(std::nullptr_t) const
    {
        return (base != nullptr);
    }


    inline operator Ref<StringBase>()
    {
        return Ref<StringBase>(base);
    }

    int equals(String other, bool ignoreCase = false) const;
    String toLower();
    String toUpper();

    String substring(int from);
    String substring(int from, int count);

    /// \brief reduces multiple consecutive occurences of the character ch
    /// to one occurence
    /// \param ch the character to reduce
    /// \return the new string, with ch reduced
    String reduce(char ch);

    inline char charAt(int index) {  return base->data[index]; }
    inline char operator[](int index) { return base->data[index]; }
    inline char *charPtrAt(int index) { return base->data + index; }
    inline int index(char ch) { return index(0, ch); }
    int index(int start, char ch);
    int rindex(char ch);
    int rindex(int end, char ch);

    long toLong();
    off_t toOFF_T();
    inline int toInt() { return (int)toLong(); }
    inline unsigned int toUInt() { return (unsigned int)toLong(); }
    double toDouble();

    int length();
    inline void setLength(int length)
    {
        base->len = length;
    }
    const char *c_str() const;
    inline void updateLength()
    {
        base->len = strlen(base->data);
    }

    bool startsWith(String str)
    {
        return base->startsWith(str.base);
    }

    int find(const char *needle);
    int find(String needle);
    String replace(String needle, String replacement);
    String replaceChar(char needle , char replacement);

    static String from(int x);
    static String from(unsigned int x);
    static String from(long x);
    static String from(unsigned long x);
    static String from(double x);
    static String from(long long x);

    static String allocate(int size);
    static String take(const char *data, int length);
    static String take(const char *data);
    static String refer(const char *str);
    static String refer(const char *str, int len);
    static String refer(const std::optional<std::string> &str);
    static String copy(const char *str);
protected:
    String(int capacity);
};

}; // namespace

// custom specialization of std::hash so we can be used in std::hash'd maps/sets
namespace std
{
template<>
struct hash<zmm::String>
{
    typedef zmm::String argument_type;
    typedef std::size_t value_type;

    value_type operator()(argument_type const& s) const
    {
        return std::hash<std::string>{}(s.c_str());
    }
};
}

template <typename T>
std::basic_ostream<T> &operator<<(std::basic_ostream<T> &oss, const zmm::String &s) {
    oss << (s.c_str() == nullptr ? "" : s.c_str());
    return oss;
}

#endif // __STRINGS_H__
