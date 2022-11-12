#include <array>
#include <common.h>
#include <filesystem>
#include <fstream>
#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include <memory>

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
