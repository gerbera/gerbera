/*  strings.h - this file is part of MediaTomb.
                                                                                
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

#ifndef __ZMM_STRINGS_H__
#define __ZMM_STRINGS_H__

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "object.h"
#include "ref.h"

#define _(str) zmm::String::refer(str)

#define MAX_INT_STRING_LENGTH 12
#define MAX_LONG_STRING_LENGTH 22 // max ulong: 12345678901234567890
#define MAX_DOUBLE_STRING_LENGTH 24

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


class StringBuffer;


class String
{
protected:
	StringBase *base;
public:
	String();
	explicit String(const char *str);
	String(const char *str, int len);
	String(const String &other);
	String(StringBase *other);
    String(Ref<StringBase> other);

    inline StringBase *getBase()
    {
        return base;
    }
    
	inline String(NIL_VAR)
	{
		base = NULL;
	}

	~String();

	String &operator=(String other);

	inline String &operator=(NIL_VAR)
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
	String operator+(double x);

	int operator==(String other);
	int operator==(const char *str);
	inline int operator!=(String other)
	{
		return ! operator==(other);
	}
	inline int operator!=(const char *str)
	{
		return ! operator==(str);
	}

	inline int operator==(NIL_VAR)
	{
		return (base == NULL);
	}
	inline int operator!=(NIL_VAR)
	{
		return (base != NULL);
	}


    inline operator Ref<StringBase>()
    {
        return Ref<StringBase>(base);
    }


	String substring(int from);
	String substring(int from, int count);
    char charAt(int index);
    int index(char ch);
    int rindex(char ch);

    long toLong();
    inline int toInt() { return (int)toLong(); }
	double toDouble();

	int length();
    inline void setLength(int length)
    {
        base->len = length;
    }
	char *c_str();
    inline void updateLength()
    {
        base->len = strlen(base->data);
    }

    bool startsWith(String str)
    {
        return base->startsWith(str.base);
    }
    
    static String from(int x);
    static String from(long x);
    static String from(double x);
    
    static String allocate(int size);
    static String take(const char *data, int length);
    static String take(const char *data);
    static String refer(const char *str);
    static String refer(const char *str, int len);
protected:
	String(int capacity);
	friend class StringBuffer;
};

}; // namespace

#endif // __STRINGS_H__
