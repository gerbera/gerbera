/*GRB*
Gerbera - https://gerbera.io/

    grb_fs.cc - this file is part of Gerbera.

    Copyright (C) 2022-2025 Gerbera Contributors

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

/// \file grb_fs.cc
#define GRB_LOG_FAC GrbLogFacility::content
#include "grb_fs.h" // API

#include "exceptions.h"
#include "util/logger.h"
#include "util/tools.h"

#include <algorithm>
#include <fcntl.h>
#include <fstream>
#include <sstream>
#include <sys/stat.h>
#include <unistd.h>

#ifdef __HAIKU__
#define _DEFAULT_SOURCE
#endif

std::string rtrimPath(std::string& s, unsigned char sep)
{
    if (!s.empty()) {
        s.erase(std::find_if(s.rbegin(), s.rend() - 1, [&](unsigned char ch) {
            return ch != sep;
        }).base(),
            s.end());
    }
    return s;
}

bool isSubDir(const fs::path& path, const fs::path& parent)
{
    auto pathStr = path.string();
    auto chkStr = parent.string();
    rtrimPath(pathStr);
    rtrimPath(chkStr);
    chkStr += "/";
    return startswith(pathStr, chkStr);
}

bool isRegularFile(const fs::path& path, std::error_code& ec) noexcept
{
    // unfortunately fs::is_regular_file(path, ec) is broken with old libstdc++ on 32bit systems (see #737)
#if defined(__GLIBCXX__) && (__GLIBCXX__ <= 20190406)
    struct stat statbuf;
    int ret = stat(path.c_str(), &statbuf);
    if (ret != 0) {
        ec = std::make_error_code(std::errc(errno));
        return false;
    }

    ec.clear();
    return S_ISREG(statbuf.st_mode);
#else
    return fs::is_regular_file(path, ec);
#endif
}

bool isRegularFile(const fs::directory_entry& dirEnt, std::error_code& ec) noexcept
{
    // unfortunately fs::is_regular_file(path, ec) is broken with old libstdc++ on 32bit systems (see #737)
#if defined(__GLIBCXX__) && (__GLIBCXX__ <= 20190406)
    struct stat statbuf;
    int ret = stat(dirEnt.path().c_str(), &statbuf);
    if (ret != 0) {
        ec = std::make_error_code(std::errc(errno));
        return false;
    }

    ec.clear();
    return S_ISREG(statbuf.st_mode);
#else
    return dirEnt.is_regular_file(ec);
#endif
}

std::uintmax_t getFileSize(const fs::directory_entry& dirEnt)
{
    // unfortunately fs::file_size() is broken with old libstdc++ on 32bit systems (see #737)
#if defined(__GLIBCXX__) && (__GLIBCXX__ <= 20190406)
    struct stat statbuf;
    int ret = stat(dirEnt.path().c_str(), &statbuf);
    if (ret != 0) {
        throw_fmt_system_error("Could not stat file {}", dirEnt.path().c_str());
    }

    return statbuf.st_size;
#else
    return dirEnt.file_size();
#endif
}

bool isExecutable(const fs::path& path, int* err)
{
    int ret = access(path.c_str(), R_OK | X_OK);
    if (err)
        *err = errno;

    return ret == 0;
}

fs::path findInPath(const fs::path& exec)
{
    auto p = getenv("PATH");
    if (!p)
        return {};

    std::string envPath = p;
    std::error_code ec;
    auto pathAr = splitString(envPath, ':');
    for (auto&& path : pathAr) {
        fs::path check = fs::path(path) / exec;
        if (isRegularFile(check, ec))
            return check;
    }

    return {};
}

GrbFile::GrbFile(fs::path path)
    : path(std::move(path))
{
}

GrbFile::~GrbFile()
{
    if (fd && std::fclose(fd) != 0) {
        log_error("fclose {} failed", path.c_str());
    }
    fd = nullptr;
}

#ifdef __linux__
#define GrbFileExtra "e"
#else
#define GrbFileExtra ""
#endif

std::FILE* GrbFile::open(const char* mode, bool fail)
{
    if (fd && std::fclose(fd) != 0) {
        log_error("fclose {} failed", path.c_str());
    }
    fd = std::fopen(path.c_str(), fmt::format("{}{}", mode, GrbFileExtra).c_str());
    if (!fd) {
        if (fail)
            throw_fmt_system_error("Could not open {}", path.c_str());
        log_error("Could not open {}: {}", path.c_str(), std::strerror(errno));
    }
    return fd;
}

std::string GrbFile::readTextFile()
{
    open("rt");
    std::ostringstream buf;
    char buffer[1024];
    std::size_t bytesRead;
    while ((bytesRead = std::fread(buffer, 1, std::size(buffer), fd)) > 0) {
        buf << std::string(buffer, bytesRead);
    }
    return buf.str();
}

void GrbFile::writeTextFile(std::string_view contents)
{
    open("wt");
    setPermissions();

    std::size_t bytesWritten = std::fwrite(contents.data(), 1, contents.length(), fd);
    if (bytesWritten < contents.length()) {
        throw_std_runtime_error("Error writing to {}", path.c_str());
    }
}

std::optional<std::vector<std::byte>> GrbFile::readBinaryFile()
{
    static_assert(sizeof(std::byte) == sizeof(std::ifstream::char_type));

    auto file = std::ifstream(path, std::ios::in | std::ios::binary);
    if (!file)
        return std::nullopt;

    auto& fb = *file.rdbuf();

    // Somewhat portable way to read file.
    // sgetn loops internally, so we need to check only the final result.
    // Also assume file size doesn't change while reading,
    // and no line conversion happens (therefore lseek returns result close to file size).
    auto size = fb.pubseekoff(0, std::ios::end);
    if (size < 0)
        throw_std_runtime_error("Can't determine file size of {}", path.c_str());

    fb.pubseekoff(0, std::ios::beg);

    auto result = std::optional<std::vector<std::byte>>(size);
    size = fb.sgetn(reinterpret_cast<char*>(result->data()), size);
    if (size < 0 || !file)
        throw_std_runtime_error("Failed to read from file {}", path.c_str());

    result->resize(size);

    return result;
}

void GrbFile::writeBinaryFile(const std::byte* data, std::size_t size)
{
    static_assert(sizeof(std::byte) == sizeof(std::ifstream::char_type));

    auto file = std::ofstream(path, std::ios::out | std::ios::binary | std::ios::trunc);
    if (!file)
        throw_std_runtime_error("Failed to open {}", path.c_str());

    setPermissions();

    file.rdbuf()->sputn(reinterpret_cast<const char*>(data), size);

    if (!file)
        throw_std_runtime_error("Failed to write to file {}", path.c_str());
}

void GrbFile::setPermissions()
{
    auto err = chmod(path.c_str(), S_IWUSR | S_IRUSR | S_IRGRP | S_IROTH);
    if (err != 0) {
        log_error("Failed to change location {} permissions: {}", path.c_str(), std::strerror(errno));
    }
}

bool GrbFile::isWritable()
{
    auto err = access(path.c_str(), R_OK | W_OK);
    if (err != 0 && errno != ENOENT) {
        log_error("Failed to check file {} write permissions: {}", path.c_str(), std::strerror(errno));
        return false;
    }
    return true;
}

bool GrbFile::isReadable(bool warn)
{
    auto err = access(path.c_str(), R_OK);
    if (err != 0) {
        if (warn)
            log_error("Failed to check file {} read permissions: {}", path.c_str(), std::strerror(errno));
        return false;
    }
    return true;
}

bool isTheora(const fs::path& oggFilename)
{
    char buffer[7];
    GrbFile file(oggFilename);
    auto f = file.open("rb");

    if (std::fread(buffer, 1, 4, f) != 4) {
        throw_std_runtime_error("Error reading {}", oggFilename.c_str());
    }

    if (std::memcmp(buffer, "OggS", 4) != 0) {
        return false;
    }

    if (fseeko(f, 28, SEEK_SET) != 0) {
        throw_std_runtime_error("Incomplete file {}", oggFilename.c_str());
    }

    if (std::fread(buffer, 1, 7, f) != 7) {
        throw_std_runtime_error("Error reading {}", oggFilename.c_str());
    }

    return std::memcmp(buffer, "\x80theora", 7) == 0;
}

fs::path getLastPath(const fs::path& path)
{
    return path.parent_path().filename();
}

#ifndef HAVE_FFMPEG
std::string getAVIFourCC(const fs::path& aviFilename)
{
#define FCC_OFFSET 0xbc
    GrbFile file(aviFilename);
    auto f = file.open("rb");

    char buffer[FCC_OFFSET + 6];

    std::size_t rb = std::fread(buffer, 1, FCC_OFFSET + 4, f);
    if (rb != FCC_OFFSET + 4) {
        throw_std_runtime_error("Could not read header of {}", aviFilename.c_str());
    }

    buffer[FCC_OFFSET + 5] = '\0';

    if (std::strncmp(buffer, "RIFF", 4) != 0) {
        return {};
    }

    if (std::strncmp(buffer + 8, "AVI ", 4) != 0) {
        return {};
    }

    return { buffer + FCC_OFFSET, 4 };
}
#endif
