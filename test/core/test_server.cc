/*GRB*

    Gerbera - https://gerbera.io/

    test_server.cc - this file is part of Gerbera.

    Copyright (C) 2016-2025 Gerbera Contributors

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

#include "common.h"
#include "exceptions.h"
#include "util/tools.h"

#include <array>
#include <filesystem>
#include <fmt/format.h>
#if FMT_VERSION >= 100202
#include <fmt/ranges.h>
#endif
#include <fstream>
#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include <memory>
#include <regex>

namespace fs = std::filesystem;

using ::testing::HasSubstr;

static constexpr auto gitBranch = std::string_view(GIT_BRANCH);
static constexpr auto gitCommitHash = std::string_view(GIT_COMMIT_HASH);

class ServerTest : public ::testing::Test {
public:
    ServerTest() = default;
    ~ServerTest() override = default;

    void SetUp() override
    {
    }

    void TearDown() override { }

    static std::string exec(const char* cmd)
    {
        std::array<char, 128> buffer;
        std::string result;
        std::shared_ptr<FILE> pipe(popen(cmd, "r"), pclose);
        if (!pipe)
            throw_std_runtime_error("popen() failed");
        while (!feof(pipe.get())) {
            if (fgets(buffer.data(), 128, pipe.get()))
                result += buffer.data();
        }
        return result;
    }

    static std::string mockText(const std::string& mockFile)
    {
        std::ifstream t(mockFile);
        std::string str((std::istreambuf_iterator<char>(t)), std::istreambuf_iterator<char>());
        return str;
    }
};

TEST_F(ServerTest, ServerOutputsHelpInformation)
{
    std::string expectedOutput = mockText("fixtures/mock-help.out");
    std::smatch base_match;
    auto result = std::vector<std::string>();
    auto patternList = std::vector<std::string> {
        "-h, --help *Print this help and exit",
        "-D, --debug *Enable debugging",
        "-v, --version *Print version info and exit",
        "    --compile-info *Print compile info and exit",
        "    --create-config [=Section(=All)]",
        " *Print a default config.xml file and exit",
        "    --create-example-config [=Section(=All)]",
        " *Print a example config.xml file and exit",
        "    --create-advanced-config [=Section(=All)]",
        " *Print a advanced example config.xml file ",
        " *and exit",
        "    --check-config *Check config.xml file and exit",
        "    --print-options *Print simple config options and exit",
        "    --offline *Do not answer UPnP content requests",
        "    --drop-tables *Drop all database tables and exit",
#ifdef HAVE_LASTFM
#ifndef HAVE_LASTFMLIB
        ".*--init-lastfm.*Get Last\\.FM session key.*",
#endif
#endif
        "-d, --daemon *Daemonize after startup",
        "-u, --user UID *Drop privs to user UID",
        "-P, --pidfile FILE *Write a pidfile to the specified location, ",
        " *e.g. /run/gerbera.pid",
        "-c, --config DIR *Path to config file",
        "-f, --cfgdir DIR *Override name of config folder ",
        " *(.config/gerbera) in home folder",
        "-l, --logfile FILE *Set log location",
        "    --rotatelog FILE *Set rotating log location",
        "    --syslog [=LOG(=USER)] *Log to syslog",
        "    --add-file FILE *Scan a file into the DB on startup, can be ",
        " *specified multiple times",
        "    --set-option OPT=VAL *Set simple config option OPT to value VAL, ",
        " *can be specified multiple times use ",
        " *--print-options for value OPTs",
        "-p, --port PORT *Port to bind with, must be >=49152",
        "-i, --ip IP *IP to bind with",
        "-e, --interface IF *Interface to bind with",
        "-m, --home DIR *Search this directory for a .config/gerbera ",
        " *folder containing a config file",
#ifndef HAVE_MAGIC
        "    --magic FILE *Set magic FILE",
#endif
        "    --import-mode MODE *Activate import MODE ('mt' or 'grb')",
#ifndef GRBDEBUG
        "    --debug-mode FACILITY *Set debugging mode to FACILITY.*",
#endif
    };
    for (auto&& line : splitString(expectedOutput, '\n', '\0', true)) {
        bool found = false;
        for (auto&& pattern : patternList) {
            if (std::regex_match(line, base_match, std::regex(pattern)))
                found = true;
        }
        if (!found)
            result.push_back(line);
    }
    expectedOutput = fmt::format("{}", fmt::join(result, "\n"));
    fs::path cmd = fs::path(CMAKE_BINARY_DIR) / "gerbera --help 2>&1";
    std::string output = exec(cmd.c_str());

    ASSERT_THAT(output.c_str(), HasSubstr(expectedOutput));
}

TEST_F(ServerTest, ServerOutputsCompileInformationIncludingGit)
{
    fs::path cmd = fs::path(CMAKE_BINARY_DIR) / "gerbera --compile-info 2>&1";
    std::string output = exec(cmd.c_str());

    ASSERT_THAT(output, HasSubstr("Compile info:\n-------------\nWITH_"));
    ASSERT_THAT(output, HasSubstr("Build info:\n-----------\nFMT"));
    if constexpr (!gitBranch.empty()) {
        ASSERT_THAT(output, HasSubstr("Git info:\n---------\n"));
        ASSERT_THAT(output, HasSubstr("Git Branch: "));
    } else {
        ASSERT_THAT(output, Not(HasSubstr("Git info:\n---------\n")));
        ASSERT_THAT(output, Not(HasSubstr("Git Branch: ")));
    }
    if constexpr (!gitCommitHash.empty()) {
        ASSERT_THAT(output, HasSubstr("Git Commit: "));
    } else {
        ASSERT_THAT(output, Not(HasSubstr("Git Commit: ")));
    }
}

TEST_F(ServerTest, GeneratesFullConfigFromServerCommand)
{
    // simple check to ensure complete generation from server
    // rely on ConfigGeneratorTest for validation.
    std::string topOutput = R"(<config version="2" xmlns="http://gerbera.io/config/2" xmlns:xsi="http://www.w3.org/2001/XMLSchema-instance" xsi:schemaLocation="http://gerbera.io/config/2 http://gerbera.io/config/2.xsd">)";
    std::string bottomOutput = "</profile>\n    </profiles>\n  </transcoding>\n</config>";

    fs::path cmd = fs::path(CMAKE_BINARY_DIR) / "gerbera --create-config 2>&1";
    std::string output = exec(cmd.c_str());

    ASSERT_THAT(output, HasSubstr(topOutput));
    ASSERT_THAT(output, HasSubstr(bottomOutput));
}
