#include "gtest/gtest.h"
#include "gmock/gmock.h"

#include <handler/http_protocol_helper.h>

using namespace ::testing;

class HttpProtocolHelperTest : public ::testing::Test {

 public:
  HttpProtocolHelperTest() {};
  virtual ~HttpProtocolHelperTest() {};

  virtual void SetUp() {
    subject = new HttpProtocolHelper();
  }

  virtual void TearDown() {
    delete subject;
  };

  HttpProtocolHelper *subject;
};

TEST_F(HttpProtocolHelperTest, TerminatesTheHeaderWithCarriageNewLine) {
  std::string header = "Content-Disposition: attachment; filename=\"file.mp3\"";

  std::string result = subject->finalizeHttpHeader(header.c_str());

  EXPECT_STREQ(result.c_str(), "Content-Disposition: attachment; filename=\"file.mp3\"\r\n");
}

TEST_F(HttpProtocolHelperTest, DoesNotAddTerminationCarriageNewLineWhenAlreadyExists) {
  std::string header = "Content-Disposition: attachment; filename=\"file.mp3\"\r\n";

  std::string result = subject->finalizeHttpHeader(header.c_str());

  EXPECT_STREQ(result.c_str(), "Content-Disposition: attachment; filename=\"file.mp3\"\r\n");
}

TEST_F(HttpProtocolHelperTest, AddsEndingTerminationCarriageNewLineIfNotFoundAtEnd) {
  std::string header = "Content-Disposition: attachment; filename=\"file.mp3\"\r\nAccept-Ranges: bytes";

  std::string result = subject->finalizeHttpHeader(header.c_str());

  EXPECT_STREQ(result.c_str(), "Content-Disposition: attachment; filename=\"file.mp3\"\r\nAccept-Ranges: bytes\r\n");
}

TEST_F(HttpProtocolHelperTest, HeaderIsOnlyLinebreak) {
  std::string header = "\r\n";

  std::string result = subject->finalizeHttpHeader(header.c_str());

  EXPECT_STREQ(result.c_str(), "\r\n");
}

TEST_F(HttpProtocolHelperTest, HeaderIsEmptyReturnsEmpty) {
  std::string header = "";

  std::string result = subject->finalizeHttpHeader(header.c_str());

  EXPECT_STREQ(result.c_str(), "");
}

TEST_F(HttpProtocolHelperTest, AddsCarriageLineBreakWithSingleChar) {
  std::string header = "a";

  std::string result = subject->finalizeHttpHeader(header.c_str());

  EXPECT_STREQ(result.c_str(), "a\r\n");
}