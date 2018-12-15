/*MT*
    
    MediaTomb - http://www.mediatomb.cc/
    
    object_dictionary.h - this file is part of MediaTomb.
    
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

/// \file object_dictionary.h
/// \brief Definiton of the DictionaryElement and Dictionary classes.
#ifndef __OBJECT_DICTIONARY_H__
#define __OBJECT_DICTIONARY_H__

#include "zmm/zmmf.h"

template <class T>
class ObjectDictionaryElement : public zmm::Object {
public:
    /// \brief Constructor, stores the key and the value.
    ObjectDictionaryElement(zmm::String key, zmm::Ref<T> value)
        : zmm::Object()
    {
        this->key = key;
        this->value = value;
    }

    /// \brief Changes the name of the key.
    /// \param key new key name.
    void setKey(zmm::String key) { this->key = key; }

    /// \brief Changes the key value.
    /// \param value new value.
    void setValue(zmm::Ref<T> value) { this->value = value; }

    /// \brief Returns the key for this DictionaryElement.
    zmm::String getKey() { return key; }

    /// \brief Returns the value for this DictionaryElement.
    zmm::Ref<T> getValue() { return value; }

protected:
    zmm::String key;
    zmm::Ref<T> value;
};

template <class T>
class ObjectDictionary : public zmm::Object {
public:
    /// \brief Constructor, initializes the dictionary.
    ObjectDictionary()
        : zmm::Object()
    {
        elements = zmm::Ref<zmm::Array<ObjectDictionaryElement<T>>>(new zmm::Array<ObjectDictionaryElement<T>>());
    }

    /// \brief Adds a new key:value pair to the dictionary.
    void put(zmm::String key, zmm::Ref<T> value)
    {
        for (int i = 0; i < elements->size(); i++) {
            zmm::Ref<ObjectDictionaryElement<T>> el = elements->get(i);
            if (el->getKey() == key) {
                el->setValue(value);
                return;
            }
        }
        zmm::Ref<ObjectDictionaryElement<T>> newEl(new ObjectDictionaryElement<T>(key, value));
        elements->append(newEl);
    }

    /// \brief Returns the value for a given key.
    zmm::Ref<T> get(zmm::String key)
    {
        for (int i = 0; i < elements->size(); i++) {
            zmm::Ref<ObjectDictionaryElement<T>> el = elements->get(i);
            if (el->getKey() == key) {
                return el->getValue();
            }
        }
        return nullptr;
    }

    /// \brief Returns the number of elements in the dictinary.
    int size() { return elements->size(); }

    /// \brief Deletes a key value pair
    void remove(zmm::String key)
    {
        for (int i = 0; i < elements->size(); i++) {
            zmm::Ref<ObjectDictionaryElement<T>> el = elements->get(i);
            if (el->getKey() == key) {
                elements->remove(i, 1);
                return;
            }
        }
    }

    /// \brief Removes all elements from the dictionary.
    void clear()
    {
        elements->remove(0, elements->size());
    }

    /// \brief Makes a shallow copy of the dictionary
    zmm::Ref<ObjectDictionary> clone()
    {
        zmm::Ref<ObjectDictionary> ret(new ObjectDictionary());
        int len = elements->size();
        for (int i = 0; i < len; i++) {
            zmm::Ref<ObjectDictionaryElement<T>> el = elements->get(i);
            ret->put(el->getKey(), el->getValue());
        }
        return ret;
    }

    zmm::Ref<zmm::Array<ObjectDictionaryElement<T>>> getElements()
    {
        return elements;
    }

    /// \brief Frees unnecessary memory
    inline void optimize() { elements->optimize(); }

protected:
    /// \brief Array of DictionaryElements, representing our Dictionary.
    zmm::Ref<zmm::Array<ObjectDictionaryElement<T>>> elements;
};
#endif // __OBJECT_DICTIONARY_H__
