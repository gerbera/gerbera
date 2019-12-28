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

DictionaryElement::DictionaryElement(std::string key, std::string value)
    : Object()
{
    this->key = key;
    this->value = value;
}

void DictionaryElement::setKey(std::string key)
{
    this->key = key;
}

void DictionaryElement::setValue(std::string value)
{
    this->value = value;
}

std::string DictionaryElement::getKey()
{
    return key;
}

std::string DictionaryElement::getValue()
{
    return value;
}

Dictionary::Dictionary()
    : Object()
{
    elements = Ref<Array<DictionaryElement>>(new Array<DictionaryElement>());
}

void Dictionary::put(std::string key, std::string value)
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

std::string Dictionary::get(std::string key)
{
    for (int i = 0; i < elements->size(); i++) {
        Ref<DictionaryElement> el = elements->get(i);
        if (el->getKey() == key) {
            return el->getValue();
        }
    }
    return "";
}

int Dictionary::size()
{
    return elements->size();
}

void Dictionary::remove(std::string key)
{
    for (int i = 0; i < elements->size(); i++) {
        Ref<DictionaryElement> el = elements->get(i);
        if (el->getKey() == key) {
            elements->remove(i, 1);
            return;
        }
    }
}

std::string Dictionary::_encode(char sep1, char sep2)
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

std::string Dictionary::encode()
{
    return _encode('&', '=');
}

std::string Dictionary::encodeSimple()
{
    return _encode('/', '/');
}

void Dictionary::decode(std::string url)
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
            std::string key(data, eqPos - data);
            std::string value(eqPos + 1, ampPos - eqPos - 1);
            key = urlUnescape(key);
            value = urlUnescape(value);

            put(key, value);
        }
        data = ampPos + 1;
    }
}

// this is somewhat tricky as we need an exact amount of pairs
// object_id=720&res_id=0
void Dictionary::decodeSimple(std::string url)
{
    std::string encoded;
    size_t pos;
    size_t last_pos = 0;
    do {
        pos = url.find('/', last_pos);
        if (pos == std::string::npos || pos < last_pos + 1)
            break;

        std::string key = urlUnescape(url.substr(last_pos, pos - last_pos));
        last_pos = pos + 1;
        pos = url.find('/', last_pos);
        if (pos == std::string::npos)
            pos = url.length();
        if (pos < last_pos + 1)
            break;

        std::string value = urlUnescape(url.substr(last_pos, pos - last_pos));
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
