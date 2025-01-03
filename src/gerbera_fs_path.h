/*GRB*

 Gerbera - https://gerbera.io/

 gerbera_fs_path.h - this file is part of Gerbera.

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

/// \file gerbera_fs_path.h

#ifndef GERBERA_FS_PATH_H
#define GERBERA_FS_PATH_H

#include "gerbera_directory_iterator.h"
#include <memory>

/// Custom filesystem path with iterators for human-oriented sorting (right order for numbers without leading zeros)
///  if option HAVE_HUMAN_SORTING is disabled, using dictionary sort
class gerbera_fs_path {
public:
    gerbera_fs_path() = default;
    gerbera_fs_path(const std::filesystem::directory_entry& de);
    gerbera_fs_path(const std::filesystem::path& fsp);
    gerbera_fs_path(const std::string& p);
    gerbera_fs_path(const std::filesystem::path::value_type* p);

    operator std::filesystem::path() const;
    operator std::string() const;

    std::string string() const;
    gerbera_fs_path parent_path() const;
    bool empty() const;
    const std::filesystem::path::value_type* c_str() const;

    bool operator==(const gerbera_fs_path& other) const;
    bool operator==(const std::filesystem::path& other) const;

    bool operator!=(const gerbera_fs_path& other) const;
    bool operator!=(const std::filesystem::path& other) const;

    std::vector<std::filesystem::directory_entry>::iterator begin();
    std::vector<std::filesystem::directory_entry>::iterator end();

    gerbera_fs_path filename() const;
    gerbera_fs_path stem() const;

    size_t distance(const std::filesystem::directory_entry& entry) const { return init_lazy_iterator()->distance(entry); };
    size_t distance(const gerbera_fs_path& entry) const;

private:
    std::shared_ptr<gerbera_directory_iterator> init_lazy_iterator() const;

    std::filesystem::path path;
    mutable std::shared_ptr<gerbera_directory_iterator> lazy_iterator;
};

#endif // GERBERA_FS_PATH_H
