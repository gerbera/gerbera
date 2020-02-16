#include <gtest/gtest.h>

#include "util/headers.h"

using namespace ::testing;

class HeadersHelperTest : public ::testing::Test {

public:
    HeadersHelperTest()
    {
        subject = new Headers();
        info = UpnpFileInfo_new();
    };
    virtual ~HeadersHelperTest() override
    {
        UpnpFileInfo_delete(info);

        delete subject;
    };

    UpnpFileInfo* info;
    Headers* subject;
};

#ifdef UPNP_1_12_LIST
std::string headers_as_string(UpnpFileInfo* info)
{
    std::string out;

    UpnpExtraHeaders* extra;
    UpnpListIter pos;
    auto head = const_cast<UpnpListHead*>(UpnpFileInfo_get_ExtraHeadersList(info));
    for (pos = UpnpListBegin(head); pos != UpnpListEnd(head); pos = UpnpListNext(head, pos)) {
        extra = (UpnpExtraHeaders*)pos;
        out += UpnpExtraHeaders_get_resp(extra);
        out += "\r\n";
    }
    return out;
}
#define GET_HEADERS(x) headers_as_string(x).c_str()
#elif UPNP_HAS_EXTRA_HEADERS_LIST
std::string headers_as_string(UpnpFileInfo* info)
{
    std::string out;

    UpnpExtraHeaders* extra;
    list_head* pos;
    auto head = const_cast<list_head*>(UpnpFileInfo_get_ExtraHeadersList(info));
    list_for_each(pos, head)
    {
        extra = (UpnpExtraHeaders*)pos;
        out.insert(0, std::string { UpnpExtraHeaders_get_resp(extra) } + "\r\n");
    }
    return out;
}
#define GET_HEADERS(x) headers_as_string(x).c_str()
#else
#define GET_HEADERS UpnpFileInfo_get_ExtraHeaders_cstr
#endif

TEST_F(HeadersHelperTest, TerminatesTheHeaderWithCarriageNewLine)
{
    std::string header = "Content-Disposition";
    std::string value = "attachment; filename=\"file.mp3\"";

    subject->addHeader(header, value);
    subject->writeHeaders(info);

    EXPECT_STREQ(GET_HEADERS(info), "Content-Disposition: attachment; filename=\"file.mp3\"\r\n");
}

TEST_F(HeadersHelperTest, DoesNotAddTerminationCarriageNewLineWhenAlreadyExists)
{
    std::string header = "Content-Disposition";
    std::string value = "attachment; filename=\"file.mp3\"\r\n";

    subject->addHeader(header, value);
    subject->writeHeaders(info);

    EXPECT_STREQ(GET_HEADERS(info), "Content-Disposition: attachment; filename=\"file.mp3\"\r\n");
}

TEST_F(HeadersHelperTest, MultipleHeaders)
{
    std::string header = "Content-Disposition";
    std::string value = "attachment; filename=\"file.mp3\"";
    std::string header2 = "Accept-Ranges";
    std::string value2 = "bytes";

    subject->addHeader(header, value);
    subject->addHeader(header2, value2);
    subject->writeHeaders(info);

    EXPECT_STREQ(GET_HEADERS(info), "Content-Disposition: attachment; filename=\"file.mp3\"\r\nAccept-Ranges: bytes\r\n");
}

TEST_F(HeadersHelperTest, MultipleHeadersSingleCarriageNewLine)
{
    std::string header = "Content-Disposition";
    std::string value = "attachment; filename=\"file.mp3\"";
    std::string header2 = "Accept-Ranges";
    std::string value2 = "bytes\r\n";

    subject->addHeader(header, value);
    subject->addHeader(header2, value2);
    subject->writeHeaders(info);

    EXPECT_STREQ(GET_HEADERS(info), "Content-Disposition: attachment; filename=\"file.mp3\"\r\nAccept-Ranges: bytes\r\n");
}

TEST_F(HeadersHelperTest, MultiBothCarriageNewLine)
{
    std::string header = "Content-Disposition";
    std::string value = "attachment; filename=\"file.mp3\"\r\n";
    std::string header2 = "Accept-Ranges";
    std::string value2 = "bytes\r\n";

    subject->addHeader(header, value);
    subject->addHeader(header2, value2);
    subject->writeHeaders(info);

    EXPECT_STREQ(GET_HEADERS(info), "Content-Disposition: attachment; filename=\"file.mp3\"\r\nAccept-Ranges: bytes\r\n");
}

TEST_F(HeadersHelperTest, IgnoresDataAfterFirstCarriageNewLine)
{
    std::string header = "Content-Disposition";
    std::string value = "attachment; filename=\"file.mp3\"\r\nAccept-Ranges: bytes";

    subject->addHeader(header, value);
    subject->writeHeaders(info);

    EXPECT_STREQ(GET_HEADERS(info), "Content-Disposition: attachment; filename=\"file.mp3\"\r\n");
}

TEST_F(HeadersHelperTest, HeaderIsOnlyLinebreakReturnsEmpty)
{
    std::string header = "\r\n";

    subject->addHeader(header, header);
    subject->writeHeaders(info);

    EXPECT_STREQ(GET_HEADERS(info), "");
}

TEST_F(HeadersHelperTest, HeaderIsEmptyReturnsEmpty)
{
    std::string header = "";

    subject->addHeader(header, header);
    subject->writeHeaders(info);

    EXPECT_STREQ(GET_HEADERS(info), "");
}

TEST_F(HeadersHelperTest, HandlesSingleCarriageReturn)
{
    std::string header = "Content-Disposition";
    std::string value = "attachment; filename=\"file.mp3\"\r";

    subject->addHeader(header, value);
    subject->writeHeaders(info);

    EXPECT_STREQ(GET_HEADERS(info), "Content-Disposition: attachment; filename=\"file.mp3\"\r\n");
}

TEST_F(HeadersHelperTest, HandlesSingleNewLine)
{
    std::string header = "Content-Disposition";
    std::string value = "attachment; filename=\"file.mp3\"\n";

    subject->addHeader(header, value);
    subject->writeHeaders(info);

    EXPECT_STREQ(GET_HEADERS(info), "Content-Disposition: attachment; filename=\"file.mp3\"\r\n");
}

TEST_F(HeadersHelperTest, EmptyValueNotAdded)
{
    std::string header = "a";

    subject->addHeader(header, "");
    subject->writeHeaders(info);

    EXPECT_STREQ(GET_HEADERS(info), "");
}

TEST_F(HeadersHelperTest, HandlesEmptyString)
{
    std::string header;

    subject->addHeader(header, header);
    subject->writeHeaders(info);

    EXPECT_STREQ(GET_HEADERS(info), "");
}

TEST_F(HeadersHelperTest, HandlesExtraContent)
{
    std::string header = "foo";
    std::string value = "bar\r\nzoo: wombat\r\n";

    subject->addHeader(header, value);
    subject->writeHeaders(info);

    EXPECT_STREQ(GET_HEADERS(info), "foo: bar\r\n");
}

TEST_F(HeadersHelperTest, HandlesExtraContentTwo)
{
    std::string header = "foo";
    std::string value = "bar\r\nzoo: wombat\r\nwhere: somewhere";

    subject->addHeader(header, value);
    subject->writeHeaders(info);

    EXPECT_STREQ(GET_HEADERS(info), "foo: bar\r\n");
}
