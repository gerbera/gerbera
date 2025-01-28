/*GRB*
Gerbera - https://gerbera.io/

    grb_fs.h - this file is part of Gerbera.

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

/// \file grb_fs.h
/// \brief std::filesystem and fs namespace header

#ifndef __GRB_FS_H__
#define __GRB_FS_H__

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

    GrbFile(const GrbFile&) = delete;
    GrbFile& operator=(const GrbFile&) = delete;

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
    /// \brief Ensure that a file has permissions 644 (-rw-r--r--)
    void setPermissions();
    bool isReadable(bool warn = false);
    bool isWritable();
    const fs::path& getPath() { return path; }
};

std::string rtrimPath(std::string& s, unsigned char sep = '/');
/// \brief Checks if the given path is a subdirectory of the other
/// \param path directory to check
/// \param parent potential parent directory
/// \return true if path is subdirectory of parent
bool isSubDir(const fs::path& path, const fs::path& parent);

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

#endif // __GRB_FS_H__
