#include <array>
#include <memory>
#include <fstream>
#include <common.h>
#include "gtest/gtest.h"
#include "gmock/gmock.h"

using namespace ::testing;


class ServerTest : public ::testing::Test {
 public:
  ServerTest() {};
  virtual ~ServerTest() {};

  virtual void SetUp() {
  }

  virtual void TearDown() {
  };

  std::string exec(const char* cmd) {
    std::array<char, 128> buffer;
    std::string result;
    std::shared_ptr<FILE> pipe(popen(cmd, "r"), pclose);
    if (!pipe) throw std::runtime_error("popen() failed!");
    while (!feof(pipe.get())) {
      if (fgets(buffer.data(), 128, pipe.get()) != nullptr)
        result += buffer.data();
    }
    return result;
  }

  std::string mockText(std::string mockFile) {
    std::ifstream t(mockFile);
    std::string str((std::istreambuf_iterator<char>(t)), std::istreambuf_iterator<char>());
    return str;
  }
};

TEST_F(ServerTest, ServerOutputsHelpInformation) {
  std::string expectedOutput = mockText("fixtures/mock-help.out");

  std::stringstream ss;
  ss << CMAKE_BINARY_DIR <<  DIR_SEPARATOR << "gerbera --help 2>&1";
  std::string cmd = ss.str();
  std::string output = exec(cmd.c_str());
  ASSERT_THAT(output.c_str(), HasSubstr(expectedOutput.c_str()));
}

TEST_F(ServerTest, ServerOutputsCompileInformationIncludingGit) {
  std::stringstream ss;
  ss << CMAKE_BINARY_DIR << DIR_SEPARATOR << "gerbera --compile-info 2>&1";
  std::string cmd = ss.str();
  std::string output = exec(cmd.c_str());
  ASSERT_THAT(output, HasSubstr("Compile info:\n-------------\nWITH_"));
  ASSERT_THAT(output, HasSubstr("Git info:\n-------------\n"));
  ASSERT_THAT(output, HasSubstr("Git Branch: "));
  ASSERT_THAT(output, HasSubstr("Git Commit: "));
}

TEST_F(ServerTest, GeneratesFullConfigFromServerCommand) {
  // simple check to ensure complete generation from server
  // rely on ConfigGeneratorTest for validation.
  std::string topOutput = "<config version=\"2\" xmlns=\"http://mediatomb.cc/config/2\" xmlns:xsi=\"http://www.w3.org/2001/XMLSchema-instance\" xsi:schemaLocation=\"http://mediatomb.cc/config/2 http://mediatomb.cc/config/2.xsd\">";
  std::string bottomOutput = "</profile>\n    </profiles>\n  </transcoding>\n</config>";

  std::stringstream ss;
  ss << CMAKE_BINARY_DIR << DIR_SEPARATOR << "gerbera --create-config 2>&1";
  std::string cmd = ss.str();
  std::string output = exec(cmd.c_str());

  ASSERT_THAT(output, HasSubstr(topOutput));
  ASSERT_THAT(output, HasSubstr(bottomOutput));
}