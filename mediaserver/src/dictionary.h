/*  dictionary.h - this file is part of MediaTomb.
                                                                                
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

/// \file dictionary.h
/// \brief Definiton of the DictionaryElement and Dictionary classes.
#ifndef __DICTIONARY_H__
#define __DICTIONARY_H__

#include "zmmf/zmmf.h"

/// \brief This class should never be used directly, it is being used by the Dictionary class.
class DictionaryElement : public zmm::Object
{
public:
    /// \brief Constructor, stores the key and the value.
    DictionaryElement(zmm::String key, zmm::String value);

    /// \brief Changes the name of the key.
    /// \param key new key name.
    void setKey(zmm::String key);

    /// \brief Changes the key value.
    /// \param value new value.
    void setValue(zmm::String value);

    /// \brief Returns the key for this DictionaryElement.
    zmm::String getKey();

    /// \brief Returns the value for this DictionaryElement.
    zmm::String getValue();

protected:
    zmm::String key;
    zmm::String value;
};

/// \brief This class stores key:value pairs of String data and provides functions to access them.
class Dictionary : public zmm::Object
{
protected:
    /// \brief Array of DictionaryElements, representing our Dictionary.
    zmm::Ref<zmm::Array<DictionaryElement> > elements;
public:

    /// \brief Constructor, initializes the dictionary.
    Dictionary();

    /// \brief Adds a new key:value pair to the dictionary.
    void put(zmm::String key, zmm::String value);

    /// \brief Returns the value for a given key.
    zmm::String get(zmm::String key);

    /// \brief Returns the number of elements in the dictinary.
    int size();
    
    /// \brief Deletes a key value pair
    void remove(zmm::String key);
    
    /// \brief Returns an url encoded version of the whole dictionary.
    zmm::String encode();

    /// \brief Removes all elements from the dictionary.
    void clear();
    
    /// \brief Makes a dictionary out of url encoded data.
    void decode(zmm::String url);

    /// \brief Makes a shallow copy of the dictionary
    zmm::Ref<Dictionary> clone();

    /// \brief returns true if the dictionary is a subset of another dictionary.#
    bool isSubsetOf(zmm::Ref<Dictionary> other);

    /// \brief checks two dictionaries for equality
    bool equals(zmm::Ref<Dictionary> other);

    zmm::Ref<zmm::Array<DictionaryElement> > getElements();

    /// \brief Frees unnecessary memory
    inline void optimize() { elements->optimize(); }
};

#endif // __DICTIONARY_H__

