/*GRB*

 Gerbera - https://gerbera.io/

 gerbera_fs_path.cc - this file is part of Gerbera.

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

/// \file gerbera_fs_path.cc

#include "gerbera_fs_path.h"
#include <memory>
#include <mutex>

gerbera_fs_path::gerbera_fs_path(const std::filesystem::directory_entry& de)
    : path(de)
{
}

gerbera_fs_path::gerbera_fs_path(const std::filesystem::path& fsp)
    : path(fsp)
{
}

gerbera_fs_path::gerbera_fs_path(const std::string& p)
    : path(p)
{
}

gerbera_fs_path::gerbera_fs_path(const std::filesystem::path::value_type* p)
    : path(p)
{
}

std::vector<std::filesystem::directory_entry>::iterator gerbera_fs_path::begin()
{
    return init_lazy_iterator()->begin();
}

std::vector<std::filesystem::directory_entry>::iterator gerbera_fs_path::end()
{
    return init_lazy_iterator()->end();
}

std::shared_ptr<gerbera_directory_iterator> gerbera_fs_path::init_lazy_iterator() const
{
    static std::mutex init_mutex;
    std::unique_lock lock(init_mutex);
    if (lazy_iterator.get() == nullptr) {
        std::error_code ec;
        lazy_iterator = std::make_shared<gerbera_directory_iterator>(path, ec);
    }

    return lazy_iterator;
}

bool gerbera_fs_path::operator==(const gerbera_fs_path& other) const
{
    return path == std::filesystem::path(other);
}

bool gerbera_fs_path::operator==(const std::filesystem::path& other) const
{
    return path == other;
}

bool gerbera_fs_path::operator!=(const gerbera_fs_path& other) const
{
    return path != std::filesystem::path(other);
}

bool gerbera_fs_path::operator!=(const std::filesystem::path& other) const
{
    return path != other;
}

gerbera_fs_path::operator std::filesystem::path() const
{
    return path;
}

gerbera_fs_path::operator std::string() const
{
    return path;
}

const std::filesystem::path::value_type* gerbera_fs_path::c_str() const
{
    return path.c_str();
}

std::string gerbera_fs_path::string() const
{
    return path;
}

gerbera_fs_path gerbera_fs_path::parent_path() const
{
    return gerbera_fs_path(path.parent_path());
}

bool gerbera_fs_path::empty() const
{
    return path.empty();
}

gerbera_fs_path gerbera_fs_path::filename() const
{
    return gerbera_fs_path(path.filename());
}

gerbera_fs_path gerbera_fs_path::stem() const
{
    return gerbera_fs_path(path.stem());
}

size_t gerbera_fs_path::distance(const gerbera_fs_path& entry) const
{
    std::filesystem::directory_entry dirEntry;
    dirEntry.assign(entry);
    return init_lazy_iterator()->distance(dirEntry);
}
