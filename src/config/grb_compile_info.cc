/*GRB*
Gerbera - https://gerbera.io/

    grb_compile_info.cc - this file is part of Gerbera.

    Copyright (C) 2024-2025 Gerbera Contributors

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

/// @file config/grb_compile_info.cc
#define GRB_LOG_FAC GrbLogFacility::server

#include "grb_runtime.h" // API

#include "upnp/compat.h"

#include <fmt/format.h>
#if FMT_VERSION >= 100202
#include <fmt/ranges.h>
#endif
#include <json/version.h>
#include <pugixml.hpp>
#include <spdlog/version.h>
#include <sqlite3.h>

#ifdef HAVE_JS
#include <duktape.h>
#endif

#ifdef HAVE_CURL
#include <curl/curlver.h>
#endif

#ifdef HAVE_MAGIC
#include <magic.h>
#endif

#ifdef HAVE_ICU
#include <unicode/uvernum.h>
#endif

#ifdef HAVE_MYSQL
#include <mysql.h>
#endif

#ifdef HAVE_PGSQL
extern "C" {
#include <libpq-fe.h>
}
#include <pqxx/version>
#endif

#ifdef HAVE_FFMPEG
extern "C" {

#include <libavformat/avformat.h>
#include <libavutil/avutil.h>

} // extern "C"
#endif

#ifdef HAVE_TAGLIB
#include <taglib.h>
#endif

#ifdef HAVE_EXIV2
#include <exiv2/exv_conf.h>
#endif

#ifdef HAVE_WAVPACK
#include <wavpack/wavpack.h>
#endif

#ifdef HAVE_MATROSKA
#include <ebml/EbmlVersion.h>
#include <matroska/KaxVersion.h>
#endif

static constexpr auto gitBranch = std::string_view(GIT_BRANCH);
static constexpr auto gitCommitHash = std::string_view(GIT_COMMIT_HASH);

bool GerberaRuntime::printCompileInfo(const std::string& arg)
{
    printCopyright(arg);
    // Info from cmake
    fmt::print(
        "Compile info:\n"
        "-------------\n"
        "{}\n\n",
        COMPILE_INFO);

    // Active defines
    std::vector<std::pair<std::string_view, std::string>> buildOptions {
        { "FMT    ", fmt::to_string(FMT_VERSION) },
        { "SPDLOG ", fmt::to_string(SPDLOG_VERSION) },
#ifdef SPDLOG_FMT_EXTERNAL
        { "", "SPDLOG_FMT_EXTERNAL" },
#endif
        { "SQLITE ", fmt::to_string(SQLITE_VERSION) },
        { "PUGIXML", fmt::to_string(PUGIXML_VERSION) },
        { "JSONCPP", JSONCPP_VERSION_STRING },
#ifdef PACKAGE_DATADIR
        { "PACKAGE_DATADIR", PACKAGE_DATADIR },
#endif
#ifdef USING_NPUPNP
        { "NPUPNP ", fmt::to_string(UPNP_VERSION) },
#else
        { "PUPNP  ", fmt::to_string(UPNP_VERSION) },
#endif
#ifdef UPNP_HAVE_TOOLS
        { "", "UPNP_HAVE_TOOLS" },
#endif
#ifdef HAVE_NL_LANGINFO
        { "HAVE_NL_LANGINFO", "" },
#endif
#ifdef HAVE_SETLOCALE
        { "HAVE_SETLOCALE", "" },
#endif
#ifdef HAVE_EXECINFO
        { "HAVE_EXECINFO", "" },
#endif
#ifdef HAVE_INOTIFY
        { "HAVE_INOTIFY", "" },
#endif
#ifdef HAVE_SYSTEMD
        { "HAVE_SYSTEMD", LIBSYSTEMD_VERSION },
#endif
#ifdef HAVE_ICU
        { "HAVE_ICU", U_ICU_VERSION },
#endif
#ifdef HAVE_JS
        { "HAVE_JS", fmt::to_string(DUK_VERSION) },
#endif
#ifdef HAVE_MYSQL
        { "HAVE_MYSQL", MYSQL_SERVER_VERSION },
#endif
#ifdef HAVE_PGSQL
        { "HAVE_PGSQL", PQXX_VERSION },
        { "", fmt::to_string(PQlibVersion()) },
#endif
#ifdef HAVE_CURL
        { "HAVE_CURL", LIBCURL_VERSION },
#endif
#ifdef HAVE_TAGLIB
        { "HAVE_TAGLIB", fmt::format("{}.{}.{}", TAGLIB_MAJOR_VERSION, TAGLIB_MINOR_VERSION, TAGLIB_PATCH_VERSION) },
#endif
#ifdef HAVE_WAVPACK
        { "HAVE_WAVPACK", WavpackGetLibraryVersionString() },
#endif
#ifdef HAVE_MAGIC
        { "HAVE_MAGIC", fmt::to_string(MAGIC_VERSION) },
#endif
#ifdef HAVE_FFMPEG
        { "HAVE_FFMPEG", LIBAVUTIL_IDENT ", " LIBAVFORMAT_IDENT },
#endif
#ifdef HAVE_AVSTREAM_CODECPAR
        { "", "HAVE_AVSTREAM_CODECPAR" },
#endif
#ifdef HAVE_FFMPEGTHUMBNAILER
        { "HAVE_FFMPEGTHUMBNAILER", FFMPEGTHUMBNAILER_VERSION },
#endif
#ifdef HAVE_LIBEXIF
        { "HAVE_LIBEXIF", EXIF_VERSION },
#endif
#ifdef HAVE_EXIV2
        { "HAVE_EXIV2", EXV_PACKAGE_VERSION },
#endif
#ifdef HAVE_MATROSKA
        { "HAVE_MATROSKA", fmt::format("Matroska {:x}, Ebml {:x}", LIBMATROSKA_VERSION, LIBEBML_VERSION) },
#endif
#ifdef ONLINE_SERVICES
        { "ONLINE_SERVICES", "" },
#endif
#ifdef HAVE_LASTFM
        { "HAVE_LASTFM",
#ifdef HAVE_LASTFMLIB
            "HAVE_LASTFMLIB"
#else
            ""
#endif
        },
#endif
#ifdef ICONV_CONST
        { "ICONV_CONST", "" },
#endif
#ifdef GRBDEBUG
        { "GRBDEBUG", "" },
#endif
#ifdef TOMBDEBUG
        { "TOMBDEBUG", "" },
#endif
#ifdef __sun__
        { "SOLARIS", "" },
#endif
#ifdef BSD
        { "BSD", "" },
#endif
#ifdef HAVE_ATOMIC
        { "HAVE_ATOMIC", "" },
#endif
    };
    // Print active defines
    fmt::print(
        "Build info:\n"
        "-----------\n");
    std::size_t len = 0;
    for (auto&& opt : buildOptions) {
        len = std::max(opt.first.size(), len);
    }
    for (auto&& [lib, vers] : buildOptions) {
        if (vers.empty())
            fmt::print("{}\n", lib);
        else
            fmt::print("{1:{0}}  {2}\n", len, lib, vers);
    }
    fmt::print("\n\n");

    // Print git branch
    if (!gitBranch.empty()) {
        fmt::print(
            "Git info:\n"
            "---------\n"
            "Git Branch: {}\n",
            gitBranch);
    }

    // Print git commit hash
    if (!gitCommitHash.empty()) {
        fmt::print("Git Commit: {}\n", gitCommitHash);
    }

    return true;
}
