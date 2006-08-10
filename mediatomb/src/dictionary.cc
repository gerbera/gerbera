/*  dictionary.cc - this file is part of MediaTomb.
                                                                                
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

#ifdef HAVE_CONFIG_H
    #include "autoconfig.h"
#endif

#include "dictionary.h"

#include <string.h>
#include "tools.h"

using namespace zmm;

DictionaryElement::DictionaryElement(String key, String value) : Object()
{
    this->key = key;
    this->value = value;
}

void DictionaryElement::setKey(String key)
{
    this->key = key;
}

void DictionaryElement::setValue(String value)
{
    this->value = value;
}

String DictionaryElement::getKey()
{
    return key;
}

String DictionaryElement::getValue()
{
    return value;
}



Dictionary::Dictionary() : Object()
{
    elements = Ref<Array<DictionaryElement> >(new Array<DictionaryElement>());
}

void Dictionary::put(String key, String value)
{
    for (int i = 0; i < elements->size(); i++)
    {
        Ref<DictionaryElement> el = elements->get(i);
        if(el->getKey() == key)
        {
            el->setValue(value);
            return;
        }
    }
    Ref<DictionaryElement> newEl(new DictionaryElement(key, value));
    elements->append(newEl);
}

String Dictionary::get(String key)
{
    for (int i = 0; i < elements->size(); i++)
    {
        Ref<DictionaryElement> el = elements->get(i);
        if (el->getKey() == key)
        {
            return el->getValue();
        }
    }
    return nil;
}

int Dictionary::size()
{
    return elements->size();
}

void Dictionary::remove(String key)
{
    for (int i = 0; i < elements->size(); i++)
    {
        Ref<DictionaryElement> el = elements->get(i);
        if (el->getKey() == key)
        {
            elements->remove(i, 1);
            return;
        }
    }
}

String Dictionary::encode()
{
    Ref<StringBuffer> buf(new StringBuffer());
    int len = elements->size();
    for (int i = 0; i < len; i++)
    {
        if(i > 0)
            *buf << '&';
        Ref<DictionaryElement> el = elements->get(i);
        *buf << url_escape(el->getKey()) << '='
             << url_escape(el->getValue());
    }
    return buf->toString();
}

void Dictionary::decode(String url)
{
    char *data = url.c_str();
    char *dataEnd = data + url.length();
    while (data < dataEnd)
    {
        char *ampPos = index(data, '&');
        if (!ampPos)
        {
            ampPos = dataEnd;
        }
        char *eqPos = index(data, '=');
        if(eqPos && eqPos < ampPos)
        {
            String key(data, eqPos - data);
            String value(eqPos + 1, ampPos - eqPos - 1);
            key = url_unescape(key);
            value = url_unescape(value);

            put(key, value);
        }
        data = ampPos + 1;
    }
}

void Dictionary::clear()
{
    elements->remove(0, elements->size());
}

Ref<Dictionary> Dictionary::clone()
{
    Ref<Dictionary> ret(new Dictionary());
    int len = elements->size();
    for (int i = 0; i < len; i++)
    {
        Ref<DictionaryElement> el = elements->get(i);
        ret->put(el->getKey(), el->getValue());
    }
    return ret;
}

bool Dictionary::isSubsetOf(Ref<Dictionary> other)
{
    int len = elements->size();
    for (int i = 0; i < len; i++)
    {
        Ref<DictionaryElement> el = elements->get(i);
        if (el->getValue() != other->get(el->getKey()))
            return 0;
    }
    return 1;
}
bool Dictionary::equals(Ref<Dictionary> other)
{
    return (isSubsetOf(other) && other->isSubsetOf(Ref<Dictionary>(this)));
}

Ref<Array<DictionaryElement> > Dictionary::getElements()
{
    return elements;
}

