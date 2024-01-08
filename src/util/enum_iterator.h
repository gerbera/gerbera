/*GRB*
Gerbera - https://gerbera.io/

    enum_iterator.h - this file is part of Gerbera.

    Copyright (C) 2022-2024 Gerbera Contributors

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
template <typename En, En beginVal, En endVal>
class EnumIterator {
private:
    using val_t = std::underlying_type_t<En>;
    val_t val;

public:
    explicit EnumIterator(const En& f)
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

    En operator*() { return static_cast<En>(val); }

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

template <class En, class Under = typename std::underlying_type<En>::type>
class FlagEnum {
public:
    FlagEnum()
        : _flags(0)
    {
    }
    FlagEnum(En flag)
        : _flags(flag)
    {
    }
    FlagEnum(const FlagEnum& original)
        : _flags(original._flags)
    {
    }

    FlagEnum& operator|=(En addFlag)
    {
        _flags |= addFlag;
        return *this;
    }
    FlagEnum operator|(En addFlag)
    {
        FlagEnum result(*this);
        result |= addFlag;
        return result;
    }
    FlagEnum& operator&=(En maskFlag)
    {
        _flags &= maskFlag;
        return *this;
    }
    FlagEnum operator&(En maskFlag)
    {
        FlagEnum result(*this);
        result &= maskFlag;
        return result;
    }
    FlagEnum operator~()
    {
        FlagEnum result(*this);
        result._flags = ~result._flags;
        return result;
    }
    explicit operator bool()
    {
        return _flags != 0;
    }

protected:
    Under _flags;
};

#endif // __ENUM_ITERATOR_H__
