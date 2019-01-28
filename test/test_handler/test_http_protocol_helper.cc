#include "gtest/gtest.h"
#include "gmock/gmock.h"

#include <handler/headers.h>

using namespace ::testing;

class HeadersHelperTest : public ::testing::Test {

 public:
  HeadersHelperTest() {
    subject = new Headers();
    info = UpnpFileInfo_new();
  };
  virtual ~HeadersHelperTest() override {
    UpnpFileInfo_delete(info);
    delete subject;
  };

  UpnpFileInfo* info;
  Headers* subject;
};

TEST_F(HeadersHelperTest, TerminatesTheHeaderWithCarriageNewLine) {
  std::string header = "Content-Disposition: attachment; filename=\"file.mp3\"";

  subject->addHeader(header);
  subject->writeHeaders(info);

  EXPECT_STREQ(UpnpFileInfo_get_ExtraHeaders_cstr(info), "Content-Disposition: attachment; filename=\"file.mp3\"\r\n");
}

TEST_F(HeadersHelperTest, DoesNotAddTerminationCarriageNewLineWhenAlreadyExists) {
  std::string header = "Content-Disposition: attachment; filename=\"file.mp3\"\r\n";

  subject->addHeader(header);
  subject->writeHeaders(info);

  EXPECT_STREQ(UpnpFileInfo_get_ExtraHeaders_cstr(info), "Content-Disposition: attachment; filename=\"file.mp3\"\r\n");
}

TEST_F(HeadersHelperTest, MultipleHeaders) {
  std::string header = "Content-Disposition: attachment; filename=\"file.mp3\"";
  std::string header2 = "Accept-Ranges: bytes";

  subject->addHeader(header);
  subject->addHeader(header2);
  subject->writeHeaders(info);

  EXPECT_STREQ(UpnpFileInfo_get_ExtraHeaders_cstr(info), "Content-Disposition: attachment; filename=\"file.mp3\"\r\nAccept-Ranges: bytes\r\n");
}

TEST_F(HeadersHelperTest, MultipleHeadersSingleCarriageNewLine) {
  std::string header = "Content-Disposition: attachment; filename=\"file.mp3\"";
  std::string header2 = "Accept-Ranges: bytes\r\n";

  subject->addHeader(header);
  subject->addHeader(header2);
  subject->writeHeaders(info);

  EXPECT_STREQ(UpnpFileInfo_get_ExtraHeaders_cstr(info), "Content-Disposition: attachment; filename=\"file.mp3\"\r\nAccept-Ranges: bytes\r\n");
}

TEST_F(HeadersHelperTest, MultiBothCarriageNewLine) {
  std::string header = "Content-Disposition: attachment; filename=\"file.mp3\"\r\n";
  std::string header2 = "Accept-Ranges: bytes\r\n";

  subject->addHeader(header);
  subject->addHeader(header2);
  subject->writeHeaders(info);

  EXPECT_STREQ(UpnpFileInfo_get_ExtraHeaders_cstr(info), "Content-Disposition: attachment; filename=\"file.mp3\"\r\nAccept-Ranges: bytes\r\n");
}

TEST_F(HeadersHelperTest, IgnoresDataAfterFirstCarriageNewLine) {
  std::string header = "Content-Disposition: attachment; filename=\"file.mp3\"\r\nAccept-Ranges: bytes";

  subject->addHeader(header);
  subject->writeHeaders(info);

  EXPECT_STREQ(UpnpFileInfo_get_ExtraHeaders_cstr(info), "Content-Disposition: attachment; filename=\"file.mp3\"\r\n");
}

TEST_F(HeadersHelperTest, HeaderIsOnlyLinebreakReturnsEmpty) {
  std::string header = "\r\n";

  subject->addHeader(header);
  subject->writeHeaders(info);

  EXPECT_STREQ(UpnpFileInfo_get_ExtraHeaders_cstr(info), "");
}

TEST_F(HeadersHelperTest, HeaderIsEmptyReturnsEmpty) {
  std::string header = "";

  subject->addHeader(header);
  subject->writeHeaders(info);

  EXPECT_STREQ(UpnpFileInfo_get_ExtraHeaders_cstr(info), "");
}

TEST_F(HeadersHelperTest, HandlesSingleCarriageReturn) {
  std::string header = "Content-Disposition: attachment; filename=\"file.mp3\"\r";

  subject->addHeader(header);
  subject->writeHeaders(info);

  EXPECT_STREQ(UpnpFileInfo_get_ExtraHeaders_cstr(info), "Content-Disposition: attachment; filename=\"file.mp3\"\r\n");
}

TEST_F(HeadersHelperTest, HandlesSingleNewLine) {
  std::string header = "Content-Disposition: attachment; filename=\"file.mp3\"\n";

  subject->addHeader(header);
  subject->writeHeaders(info);

  EXPECT_STREQ(UpnpFileInfo_get_ExtraHeaders_cstr(info), "Content-Disposition: attachment; filename=\"file.mp3\"\r\n");
}

TEST_F(HeadersHelperTest, AddsCarriageLineBreakWithSingleChar) {
  std::string header = "a";

  subject->addHeader(header);
  subject->writeHeaders(info);

  EXPECT_STREQ(UpnpFileInfo_get_ExtraHeaders_cstr(info), "a\r\n");
}

TEST_F(HeadersHelperTest, HandlesNullZmmString) {
  zmm::String header;

  subject->addHeader(header);
  subject->writeHeaders(info);

  EXPECT_STREQ(UpnpFileInfo_get_ExtraHeaders_cstr(info), "");
}

TEST_F(HeadersHelperTest, HandlesExtraContent) {
  std::string header = "foo: bar\r\nzoo: wombat\r\n";

  subject->addHeader(header);
  subject->writeHeaders(info);

  EXPECT_STREQ(UpnpFileInfo_get_ExtraHeaders_cstr(info), "foo: bar\r\n");
}

TEST_F(HeadersHelperTest, HandlesExtraContentTwo) {
  std::string header = "foo: bar\r\nzoo: wombat\r\nwhere: somewhere";

  subject->addHeader(header);
  subject->writeHeaders(info);

  EXPECT_STREQ(UpnpFileInfo_get_ExtraHeaders_cstr(info), "foo: bar\r\n");
}
