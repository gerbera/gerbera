/* SPDX-License-Identifier: GPL-2.0-or-later */
/*
 * std::filesystem and fs namespace header
 *
 * Copyright (C) 2021 Gerbera Contributors
 */

#pragma once

#include <filesystem>
#include <optional>
#include <vector>

namespace fs = std::filesystem;

class GrbFile {
private:
    fs::path path;
    std::FILE* fd {};

public:
    explicit GrbFile(fs::path path);
    ~GrbFile();

    std::FILE* open(const char* mode, bool fail = true);
    /// \brief Reads the entire contents of a text file and returns it as a string.
    std::string readTextFile();
    /// \brief writes a string into a text file
    void writeTextFile(std::string_view contents);
    /// \brief Reads entire contents of a binary file into a buffer.
    /// \return an empty optional if file can't be open and throws if read fails.
    std::optional<std::vector<std::byte>> readBinaryFile();
    /// \brief Writes data into a file. Throws if file can't be open or if write fails.
    void writeBinaryFile(const std::byte* data, std::size_t size);
};

/// \brief Checks if the given file is a regular file (imitate same behaviour as std::filesystem::is_regular_file)
bool isRegularFile(const fs::path& path, std::error_code& ec) noexcept;
bool isRegularFile(const fs::directory_entry& dirEnt, std::error_code& ec) noexcept;

/// \brief Returns file size of give file, if it does not exist it will throw an exception
std::uintmax_t getFileSize(const fs::directory_entry& dirEnt);

/// \brief Checks if the given binary is executable by our process
/// \param path absolute path of the binary
/// \param err if not NULL err will contain the errno result of the check
/// \return true if the given binary is executable by our process, otherwise false
bool isExecutable(const fs::path& path, int* err = nullptr);

/// \brief Checks if the given executable exists in $PATH
/// \param exec filename of the executable that needs to be checked
/// \return aboslute path to the given executable or nullptr of it was not found
fs::path findInPath(const fs::path& exec);

/// \brief Determines if the particular ogg file contains a video (theora)
bool isTheora(const fs::path& oggFilename);

#ifndef HAVE_FFMPEG
/// \brief Fallback code to retrieve the used fourcc from an AVI file.
///
/// This code is based on offsets, so we will use it only if ffmpeg is not
/// available.
std::string getAVIFourCC(const fs::path& aviFilename);
#endif

/// \brief Gets an absolute filename as a parameter and returns the last parent
///
/// "/some/path/to/file.txt" -> "to"
fs::path getLastPath(const fs::path& path);
