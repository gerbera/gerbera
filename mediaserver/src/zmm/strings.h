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


#define MAX_INT_STRING_LENGTH 12

namespace zmm
{


class StringBase : public Object
{
public:
	char *data;
	int len;
	StringBase(int capacity);
	StringBase(char *str);
	StringBase(char *str, int len);
	bool startsWith(StringBase *other);
	virtual ~StringBase();
protected:
    StringBase();
	friend class String;
};


class StringBuffer;


class String
{
protected:
	StringBase *base;
public:
	String();
	String(char *str);
	String(char *str, int len);
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
	String operator+(char *str);
	String operator+(char chr);
	String operator+(int x);

	int operator==(String other);
	int operator==(char *str);
	inline int operator!=(String other)
	{
		return ! operator==(other);
	}
	inline int operator!=(char *str)
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

	int toInt();
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
    static String from(double x);
    
    static String allocate(int size);
    static String take(char *data, int length);
    static String take(char *data);
protected:
	String(int capacity);
	friend class StringBuffer;
};


}; // namespace

#endif // __STRINGS_H__
