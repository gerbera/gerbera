/*GRB*

    Gerbera - https://gerbera.io/

    gerbera_directory_iterator.h - this file is part of Gerbera.

    Copyright (C) 2024 Gerbera Contributors

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

/// \file gerbera_directory_iterator.h

#ifndef GERBERA_DIRECTORY_ITERATOR_H
#define GERBERA_DIRECTORY_ITERATOR_H

#include <filesystem>
#include <map>
#include <string>
#include <vector>

/// Custom iterator for human-oriented sorting (right order for numbers without leading zeros)
///  if option HAVE_HUMAN_SORTING is disabled, using dictionary sort
class gerbera_directory_iterator {
public:
    gerbera_directory_iterator(const std::filesystem::directory_entry& de, std::error_code& ec);
    gerbera_directory_iterator(const std::string& path, std::error_code& ec);

    std::vector<std::filesystem::directory_entry>::iterator begin() { return sortedContents.begin(); }
    std::vector<std::filesystem::directory_entry>::iterator end() { return sortedContents.end(); }

    size_t distance(const std::filesystem::directory_entry& path) const;

private:
    void process_sort();

    std::vector<std::filesystem::directory_entry> sortedContents;
    std::map<std::filesystem::directory_entry, size_t> sortedContentsMap;
};

std::vector<std::filesystem::directory_entry>::iterator begin(gerbera_directory_iterator& it);
std::vector<std::filesystem::directory_entry>::iterator end(gerbera_directory_iterator& it);

#endif // GERBERA_DIRECTORY_ITERATOR_H
