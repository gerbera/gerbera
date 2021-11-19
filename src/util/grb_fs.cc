/* SPDX-License-Identifier: GPL-2.0-or-later */
/*
 * std::filesystem and fs namespace header
 *
 * Copyright (C) 2021 Gerbera Contributors
 */

/// \file grb_fs.cc

#include "grb_fs.h" // API

#include <fstream>
#include <sstream>
#include <sys/stat.h>
#include <unistd.h>

#ifdef __HAIKU__
#define _DEFAULT_SOURCE
#endif

#ifdef SOLARIS
#include <fcntl.h>
#endif

#include "util/tools.h"

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

off_t getFileSize(const fs::directory_entry& dirEnt)
{
    // unfortunately fs::file_size() is broken with old libstdc++ on 32bit systems (see #737)
#if defined(__GLIBCXX__) && (__GLIBCXX__ <= 20190406)
    struct stat statbuf;
    int ret = stat(dirEnt.path().c_str(), &statbuf);
    if (ret != 0) {
        throw_std_runtime_error("{}: {}", std::strerror(errno), dirEnt.path().c_str());
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

std::string readTextFile(const fs::path& path)
{
#ifdef __linux__
    auto f = std::fopen(path.c_str(), "rte");
#else
    auto f = std::fopen(path.c_str(), "rt");
#endif
    if (!f) {
        throw_std_runtime_error("Could not open {}: {}", path.c_str(), std::strerror(errno));
    }
    std::ostringstream buf;
    char buffer[1024];
    std::size_t bytesRead;
    while ((bytesRead = std::fread(buffer, 1, std::size(buffer), f)) > 0) {
        buf << std::string(buffer, bytesRead);
    }
    std::fclose(f);
    return buf.str();
}

void writeTextFile(const fs::path& path, std::string_view contents)
{
#ifdef __linux__
    auto f = std::fopen(path.c_str(), "wte");
#else
    auto f = std::fopen(path.c_str(), "wt");
#endif
    if (!f) {
        throw_std_runtime_error("Could not open {}: {}", path.c_str(), std::strerror(errno));
    }

    std::size_t bytesWritten = std::fwrite(contents.data(), 1, contents.length(), f);
    if (bytesWritten < contents.length()) {
        std::fclose(f);

        throw_std_runtime_error("Error writing to {}: {}", path.c_str(), std::strerror(errno));
    }
    std::fclose(f);
}

std::optional<std::vector<std::byte>> readBinaryFile(const fs::path& path)
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

void writeBinaryFile(const fs::path& path, const std::byte* data, std::size_t size)
{
    static_assert(sizeof(std::byte) == sizeof(std::ifstream::char_type));

    auto file = std::ofstream(path, std::ios::out | std::ios::binary | std::ios::trunc);
    if (!file)
        throw_std_runtime_error("Failed to open {}", path.c_str());

    file.rdbuf()->sputn(reinterpret_cast<const char*>(data), size);

    if (!file)
        throw_std_runtime_error("Failed to write to file {}", path.c_str());
}

bool isTheora(const fs::path& oggFilename)
{
    char buffer[7];

#ifdef __linux__
    auto f = std::fopen(oggFilename.c_str(), "rbe");
#else
    auto f = std::fopen(oggFilename.c_str(), "rb");
#endif
    if (!f) {
        throw_std_runtime_error("Error opening {}: {}", oggFilename.c_str(), std::strerror(errno));
    }

    if (std::fread(buffer, 1, 4, f) != 4) {
        std::fclose(f);
        throw_std_runtime_error("Error reading {}", oggFilename.c_str());
    }

    if (std::memcmp(buffer, "OggS", 4) != 0) {
        std::fclose(f);
        return false;
    }

    if (std::fseek(f, 28, SEEK_SET) != 0) {
        std::fclose(f);
        throw_std_runtime_error("Incomplete file {}", oggFilename.c_str());
    }

    if (std::fread(buffer, 1, 7, f) != 7) {
        std::fclose(f);
        throw_std_runtime_error("Error reading {}", oggFilename.c_str());
    }

    if (std::memcmp(buffer, "\x80theora", 7) != 0) {
        std::fclose(f);
        return false;
    }

    std::fclose(f);
    return true;
}

fs::path getLastPath(const fs::path& path)
{
    auto it = std::prev(path.end()); // filename
    if (it != path.end())
        it = std::prev(it); // last path
    if (it != path.end())
        return *it;

    return {};
}
