/*GRB*
Gerbera - https://gerbera.io/

    enum_iterator.h - this file is part of Gerbera.

    Copyright (C) 2022-2023 Gerbera Contributors

    Gerbera is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License version 2
    as published by the Free Software Foundation.

    Gerbera is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with Gerbera.  If not, see <http://www.gnu.org/licenses/>.
*/

/// \file enum_iterator.h
/// \brief EnumIterator template class

#ifndef __ENUM_ITERATOR_H__
#define __ENUM_ITERATOR_H__

/// \brief Iterator over values of a sequential enum between begin and end
template <typename C, C beginVal, C endVal>
class EnumIterator {
private:
    using val_t = std::underlying_type_t<C>;
    val_t val;

public:
    explicit EnumIterator(const C& f)
        : val(static_cast<val_t>(f))
    {
    }
    EnumIterator()
        : val(static_cast<val_t>(beginVal))
    {
    }

    EnumIterator operator++()
    {
        ++val;
        return *this;
    }

    C operator*() { return static_cast<C>(val); }

    EnumIterator begin() const { return *this; }
    EnumIterator end() const
    {
        static const EnumIterator endIter = EnumIterator(endVal);
        return endIter;
    }
    bool operator!=(const EnumIterator& i) const
    {
        return val != i.val;
    }
};

#endif // __ENUM_ITERATOR_H__
