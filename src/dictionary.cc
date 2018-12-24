/*MT*
    
    MediaTomb - http://www.mediatomb.cc/
    
    dictionary.cc - this file is part of MediaTomb.
    
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

/// \file dictionary.cc

#include "dictionary.h"

#include "tools.h"
#include <cstring>
#include <sstream>

using namespace zmm;

DictionaryElement::DictionaryElement(String key, String value)
    : Object()
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

Dictionary::Dictionary()
    : Object()
{
    elements = Ref<Array<DictionaryElement>>(new Array<DictionaryElement>());
}

void Dictionary::put(String key, String value)
{
    for (int i = 0; i < elements->size(); i++) {
        Ref<DictionaryElement> el = elements->get(i);
        if (el->getKey() == key) {
            el->setValue(value);
            return;
        }
    }
    Ref<DictionaryElement> newEl(new DictionaryElement(key, value));
    elements->append(newEl);
}

String Dictionary::get(String key)
{
    for (int i = 0; i < elements->size(); i++) {
        Ref<DictionaryElement> el = elements->get(i);
        if (el->getKey() == key) {
            return el->getValue();
        }
    }
    return nullptr;
}

int Dictionary::size()
{
    return elements->size();
}

void Dictionary::remove(String key)
{
    for (int i = 0; i < elements->size(); i++) {
        Ref<DictionaryElement> el = elements->get(i);
        if (el->getKey() == key) {
            elements->remove(i, 1);
            return;
        }
    }
}

String Dictionary::_encode(char sep1, char sep2)
{
    std::ostringstream buf;
    int len = elements->size();
    for (int i = 0; i < len; i++) {
        if (i > 0)
            buf << sep1;
        Ref<DictionaryElement> el = elements->get(i);
        buf << url_escape(el->getKey()) << sep2
            << url_escape(el->getValue());
    }
    return buf.str();
}

String Dictionary::encode()
{
    return _encode('&', '=');
}

String Dictionary::encodeSimple()
{
    return _encode('/', '/');
}

void Dictionary::decode(String url)
{
    const char* data = url.c_str();
    const char* dataEnd = data + url.length();
    while (data < dataEnd) {
        const char* ampPos = strchr(data, '&');
        if (!ampPos) {
            ampPos = dataEnd;
        }
        const char* eqPos = strchr(data, '=');
        if (eqPos && eqPos < ampPos) {
            String key(data, eqPos - data);
            String value(eqPos + 1, ampPos - eqPos - 1);
            key = urlUnescape(key);
            value = urlUnescape(value);

            put(key, value);
        }
        data = ampPos + 1;
    }
}

// this is somewhat tricky as we need an exact amount of pairs
// object_id=720&res_id=0
void Dictionary::decodeSimple(String url)
{
    String encoded;
    int pos;
    int last_pos = 0;
    do {
        pos = url.index(last_pos, '/');
        if (pos < last_pos + 1)
            break;

        String key = urlUnescape(url.substring(last_pos, pos - last_pos));
        last_pos = pos + 1;
        pos = url.index(last_pos, '/');
        if (pos == -1)
            pos = url.length();
        if (pos < last_pos + 1)
            break;

        String value = urlUnescape(url.substring(last_pos, pos - last_pos));
        last_pos = pos + 1;
        put(key, value);
    } while (last_pos < url.length());
}

void Dictionary::clear()
{
    elements->remove(0, elements->size());
}

Ref<Dictionary> Dictionary::clone()
{
    Ref<Dictionary> ret(new Dictionary());
    int len = elements->size();
    for (int i = 0; i < len; i++) {
        Ref<DictionaryElement> el = elements->get(i);
        ret->put(el->getKey(), el->getValue());
    }
    return ret;
}

void Dictionary::merge(Ref<Dictionary> other)
{
    if (other == nullptr)
        return;

    Ref<Array<DictionaryElement>> other_el = other->getElements();
    int len = other_el->size();
    for (int i = 0; i < len; i++) {
        Ref<DictionaryElement> el = other_el->get(i);
        this->put(el->getKey(), el->getValue());
    }
}

bool Dictionary::isSubsetOf(Ref<Dictionary> other)
{
    int len = elements->size();
    for (int i = 0; i < len; i++) {
        Ref<DictionaryElement> el = elements->get(i);
        if (el->getValue() != other->get(el->getKey()))
            return false;
    }
    return true;
}
bool Dictionary::equals(Ref<Dictionary> other)
{
    return (isSubsetOf(other) && other->isSubsetOf(Ref<Dictionary>(this)));
}

Ref<Array<DictionaryElement>> Dictionary::getElements()
{
    return elements;
}
