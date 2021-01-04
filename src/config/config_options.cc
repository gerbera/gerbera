/*GRB*

    Gerbera - https://gerbera.io/

    config_options.cc - this file is part of Gerbera.

    Copyright (C) 2020-2021 Gerbera Contributors

    Gerbera is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License version 2
    as published by the Free Software Foundation.

    Gerbera is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with Gerbera.  If not, see <http://www.gnu.org/licenses/>.

    $Id$
*/

/// \file config_options.cc

#include "config_options.h" // API

#include <algorithm>
#include <fmt/core.h>

size_t DictionaryOption::getEditSize() const
{
    if (indexMap.empty()) {
        return 0;
    }
    return (*std::max_element(indexMap.begin(), indexMap.end(), [&](auto a, auto b) { return (a.first < b.first); })).first + 1;
}

std::map<std::string, std::string> DictionaryOption::getDictionaryOption(bool forEdit) const
{
    if (!forEdit)
        return option;

    std::map<std::string, std::string> editOption;
    auto editSize = getEditSize();
    for (size_t i = 0; i < editSize; i++) {
        // add index in front of key to get items correct sorting in map
        if (!indexMap.at(i).empty()) {
            editOption[fmt::format("{:05d}{}", i, indexMap.at(i))] = option.at(indexMap.at(i));
        } else {
            editOption[fmt::format("{:05d}", i)] = "";
        }
    }
    return editOption;
}

void DictionaryOption::setKey(size_t keyIndex, const std::string& newKey)
{
    if (!indexMap[keyIndex].empty()) {
        auto oldValue = option[indexMap[keyIndex]];
        option.erase(indexMap[keyIndex]);
        indexMap[keyIndex] = newKey;
        if (!newKey.empty()) {
            option[newKey] = oldValue;
        } else {
            for (size_t i = getEditSize() - 1; i >= origSize; i--) {
                if (indexMap.find(i) != indexMap.end() && indexMap[keyIndex].empty()) {
                    option.erase(indexMap[i]);
                } else {
                    break;
                }
            }
        }
    } else if (!newKey.empty()) {
        indexMap[keyIndex] = newKey;
    }
}

void DictionaryOption::setValue(size_t keyIndex, const std::string& value)
{
    if (!indexMap[keyIndex].empty()) {
        option[indexMap[keyIndex]] = value;
    }
}

std::vector<std::string> ArrayOption::getArrayOption(bool forEdit) const
{
    if (!forEdit)
        return option;

    std::vector<std::string> editOption;
    auto editSize = getEditSize();
    for (size_t i = 0; i < editSize; i++) {
        if (indexMap.at(i) < SIZE_MAX) {
            editOption.emplace_back(option[indexMap.at(i)]);
        } else {
            editOption.emplace_back("");
        }
    }
    return editOption;
}

void ArrayOption::setItem(size_t index, const std::string& value)
{
    auto editSize = getEditSize();
    if (indexMap.find(index) != indexMap.end() && value.empty()) {
        option.erase(option.begin() + indexMap[index]);
        indexMap[index] = SIZE_MAX;
        for (size_t i = index + 1; i < editSize; i++) {
            if (indexMap[i] < SIZE_MAX) {
                indexMap[i]--;
            }
        }
        for (size_t i = editSize - 1; i >= origSize; i--) {
            if (indexMap[i] == SIZE_MAX)
                indexMap.erase(i);
            else {
                break;
            }
        }
    } else if (indexMap.find(index) != indexMap.end()) {
        if (indexMap[index] == SIZE_MAX) {
            for (size_t i = editSize - 1; i > index; i--) {
                if (indexMap[i] < SIZE_MAX) {
                    indexMap[index] = indexMap[i];
                    indexMap[i]++;
                }
            }
            if (indexMap[index] == SIZE_MAX) {
                for (size_t i = 0; i < index; i++) {
                    if (indexMap[i] < SIZE_MAX) {
                        indexMap[index] = indexMap[i] + 1;
                    }
                }
            }
            if (indexMap[index] == SIZE_MAX || indexMap[index] > option.size()) {
                indexMap[index] = option.size();
            }
            option.insert(option.begin() + indexMap[index], value);
        } else {
            if (indexMap.find(index) == indexMap.end() || indexMap[index] == SIZE_MAX || indexMap[index] >= option.size()) {
                indexMap[index] = option.size();
                option.push_back(value);
            } else {
                option.at(indexMap[index]) = value;
            }
        }
    } else if (!value.empty()) {
        option.push_back(value);
        indexMap[index] = option.size() - 1;
    }
}

size_t ArrayOption::getEditSize() const
{
    if (indexMap.empty()) {
        return 0;
    }
    return (*std::max_element(indexMap.begin(), indexMap.end(), [&](auto a, auto b) { return (a.first < b.first); })).first + 1;
}
