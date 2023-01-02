/*GRB*

    Gerbera - https://gerbera.io/

    config_options.cc - this file is part of Gerbera.

    Copyright (C) 2020-2023 Gerbera Contributors

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

#include "content/autoscan.h"

#include <algorithm>
#include <fmt/core.h>

std::size_t DictionaryOption::getEditSize() const
{
    if (indexMap.empty()) {
        return 0;
    }
    return std::max_element(indexMap.begin(), indexMap.end(), [](auto a, auto b) { return (a.first < b.first); })->first + 1;
}

std::map<std::string, std::string> DictionaryOption::getDictionaryOption(bool forEdit) const
{
    if (!forEdit)
        return option;

    std::map<std::string, std::string> editOption;
    auto editSize = getEditSize();
    for (std::size_t i = 0; i < editSize; i++) {
        // add index in front of key to get items correct sorting in map
        if (indexMap.find(i) != indexMap.end() && !indexMap.at(i).empty() && option.find(indexMap.at(i)) != option.end()) {
            editOption[fmt::format("{:05d}{}", i, indexMap.at(i))] = option.at(indexMap.at(i));
        } else {
            editOption[fmt::format("{:05d}", i)] = "";
        }
    }
    return editOption;
}

void DictionaryOption::setKey(std::size_t keyIndex, const std::string& newKey)
{
    if (!indexMap[keyIndex].empty()) {
        auto oldValue = option[indexMap[keyIndex]];
        option.erase(indexMap[keyIndex]);
        indexMap[keyIndex] = newKey;
        if (!newKey.empty()) {
            option[newKey] = oldValue;
        } else {
            for (std::size_t i = getEditSize() - 1; i >= origSize; i--) {
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

void DictionaryOption::setValue(std::size_t keyIndex, std::string value)
{
    if (!indexMap.at(keyIndex).empty()) {
        option[indexMap.at(keyIndex)] = std::move(value);
    }
}

std::vector<std::vector<std::pair<std::string, std::string>>> VectorOption::getVectorOption(bool forEdit) const
{
    if (!forEdit)
        return option;

    std::vector<std::vector<std::pair<std::string, std::string>>> editOption;
    auto editSize = getEditSize();
    editOption.reserve(editSize);
    for (std::size_t i = 0; i < editSize; i++) {
        editOption.push_back((indexMap.find(i) != indexMap.end() && indexMap.at(i) < std::numeric_limits<std::size_t>::max()) ? option.at(indexMap.at(i)) : std::vector<std::pair<std::string, std::string>>());
    }
    return editOption;
}

std::size_t VectorOption::getEditSize() const
{
    if (indexMap.empty()) {
        return 0;
    }
    return std::max_element(indexMap.begin(), indexMap.end(), [](auto a, auto b) { return (a.first < b.first); })->first + 1;
}

void VectorOption::setValue(std::size_t optionIndex, std::size_t entryIndex, std::string value)
{
    if (optionIndex < indexMap.size() && entryIndex < option[indexMap.at(optionIndex)].size()) {
        option[indexMap.at(optionIndex)][entryIndex].second = std::move(value);
    }
}

std::vector<std::string> ArrayOption::getArrayOption(bool forEdit) const
{
    if (!forEdit)
        return option;

    auto editSize = getEditSize();
    std::vector<std::string> editOption;
    editOption.reserve(editSize);
    for (std::size_t i = 0; i < editSize; i++) {
        editOption.push_back((indexMap.find(i) != indexMap.end() && indexMap.at(i) < std::numeric_limits<std::size_t>::max()) ? option[indexMap.at(i)] : "");
    }
    return editOption;
}

void ArrayOption::setItem(std::size_t index, const std::string& value)
{
    auto editSize = getEditSize();
    if (indexMap.find(index) != indexMap.end() && value.empty()) {
        option.erase(option.begin() + indexMap[index]);
        indexMap[index] = std::numeric_limits<std::size_t>::max();
        for (std::size_t i = index + 1; i < editSize; i++) {
            if (indexMap[i] < std::numeric_limits<std::size_t>::max()) {
                indexMap[i]--;
            }
        }
        for (std::size_t i = editSize - 1; i >= origSize; i--) {
            if (indexMap[i] == std::numeric_limits<std::size_t>::max())
                indexMap.erase(i);
            else {
                break;
            }
        }
    } else if (indexMap.find(index) != indexMap.end()) {
        if (indexMap[index] == std::numeric_limits<std::size_t>::max()) {
            for (std::size_t i = editSize - 1; i > index; i--) {
                if (indexMap[i] < std::numeric_limits<std::size_t>::max()) {
                    indexMap[index] = indexMap[i];
                    indexMap[i]++;
                }
            }
            if (indexMap[index] == std::numeric_limits<std::size_t>::max()) {
                for (std::size_t i = 0; i < index; i++) {
                    if (indexMap[i] < std::numeric_limits<std::size_t>::max()) {
                        indexMap[index] = indexMap[i] + 1;
                    }
                }
            }
            if (indexMap[index] == std::numeric_limits<std::size_t>::max() || indexMap[index] > option.size()) {
                indexMap[index] = option.size();
            }
            option.insert(option.begin() + indexMap[index], value);
        } else {
            if (indexMap.find(index) == indexMap.end() || indexMap[index] == std::numeric_limits<std::size_t>::max() || indexMap[index] >= option.size()) {
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

std::size_t ArrayOption::getEditSize() const
{
    if (indexMap.empty()) {
        return 0;
    }
    return std::max_element(indexMap.begin(), indexMap.end(), [](auto a, auto b) { return (a.first < b.first); })->first + 1;
}
