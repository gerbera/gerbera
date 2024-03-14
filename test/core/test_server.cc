/*GRB*

Gerbera - https://gerbera.io/

    test_server.cc - this file is part of Gerbera.

    Copyright (C) 2016-2024 Gerbera Contributors

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

#include <array>
#include <filesystem>
#include <fstream>
#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include <memory>
#include <regex>

#include "common.h"
#include "util/tools.h"

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
#ifdef GRBDEBUG
    std::string expectedOutput = mockText("fixtures/mock-help-debug.out");
#else
    std::string expectedOutput = mockText("fixtures/mock-help.out");
#endif
    std::smatch base_match;
    auto result = std::vector<std::string>();
    auto patternList = std::vector<std::string> {
#ifndef HAVE_MAGIC
        ".*--magic FILE.*Set magic FILE.*",
#endif
#ifndef GRBDEBUG
        ".*--debug-mode FACILITY.*Set debugging mode to FACILITY.*",
#endif
    };
    for (auto&& line : splitString(expectedOutput, '\n', true)) {
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

    ASSERT_THAT(output, HasSubstr("Compile info\n-------------\nWITH_"));
    if constexpr (!gitBranch.empty()) {
        ASSERT_THAT(output, HasSubstr("Git info:\n-------------\n"));
        ASSERT_THAT(output, HasSubstr("Git Branch: "));
    } else {
        ASSERT_THAT(output, Not(HasSubstr("Git info:\n-------------\n")));
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
    std::string topOutput = R"(<config version="2" xmlns="http://mediatomb.cc/config/2" xmlns:xsi="http://www.w3.org/2001/XMLSchema-instance" xsi:schemaLocation="http://mediatomb.cc/config/2 http://mediatomb.cc/config/2.xsd">)";
    std::string bottomOutput = "</profile>\n    </profiles>\n  </transcoding>\n</config>";

    fs::path cmd = fs::path(CMAKE_BINARY_DIR) / "gerbera --create-config 2>&1";
    std::string output = exec(cmd.c_str());

    ASSERT_THAT(output, HasSubstr(topOutput));
    ASSERT_THAT(output, HasSubstr(bottomOutput));
}
