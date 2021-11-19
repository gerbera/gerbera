/* SPDX-License-Identifier: GPL-2.0-or-later */
/*
 * std::filesystem and fs namespace header
 *
 * Copyright (C) 2021 Gerbera Contributors
 */

#ifndef __GRB_FS_H__
#define __GRB_FS_H__

#include <filesystem>
#include <optional>
#include <vector>

namespace fs = std::filesystem;

/// \brief Checks if the given file is a regular file (imitate same behaviour as std::filesystem::is_regular_file)
bool isRegularFile(const fs::path& path, std::error_code& ec) noexcept;
bool isRegularFile(const fs::directory_entry& dirEnt, std::error_code& ec) noexcept;

/// \brief Returns file size of give file, if it does not exist it will throw an exception
off_t getFileSize(const fs::directory_entry& dirEnt);

/// \brief Checks if the given binary is executable by our process
/// \param path absolute path of the binary
/// \param err if not NULL err will contain the errno result of the check
/// \return true if the given binary is executable by our process, otherwise false
bool isExecutable(const fs::path& path, int* err = nullptr);

/// \brief Checks if the given executable exists in $PATH
/// \param exec filename of the executable that needs to be checked
/// \return aboslute path to the given executable or nullptr of it was not found
fs::path findInPath(const fs::path& exec);

/// \brief Reads the entire contents of a text file and returns it as a string.
std::string readTextFile(const fs::path& path);

/// \brief writes a string into a text file
void writeTextFile(const fs::path& path, std::string_view contents);

/// \brief Reads entire contents of a binary file into a buffer.
/// \return an empty optional if file can't be open and throws if read fails.
std::optional<std::vector<std::byte>> readBinaryFile(const fs::path& path);

/// \brief Writes data into a file. Throws if file can't be open or if write fails.
void writeBinaryFile(const fs::path& path, const std::byte* data, std::size_t size);

/// \brief Determines if the particular ogg file contains a video (theora)
bool isTheora(const fs::path& oggFilename);

/// \brief Gets an absolute filename as a parameter and returns the last parent
///
/// "/some/path/to/file.txt" -> "to"
fs::path getLastPath(const fs::path& path);

#endif // __GRB_FS_H__
