#ifdef HAVE_JS

#include <duktape.h>
#include <filesystem>
#include <gmock/gmock.h>
#include <gtest/gtest.h>
namespace fs = std::filesystem;

#include "config/config_manager.h"
#include "util/string_converter.h"
#include "util/tools.h"

#include "mock/script_test_fixture.h"

using namespace ::testing;

class CommonScriptTest : public ::testing::Test {

public:
    CommonScriptTest() {};

    virtual ~CommonScriptTest() {};

    virtual void SetUp()
    {
        ctx = duk_create_heap(nullptr, nullptr, nullptr, nullptr, nullptr);

        fs::path ss = fs::path(SCRIPTS_DIR) / "js" / "common.js";
        std::string script = readTextFile(ss.c_str());
        duk_push_string(ctx, ss.c_str());
        duk_pcompile_lstring_filename(ctx, 0, script.c_str(), script.length());
        duk_call(ctx, 0);
    }

    virtual void TearDown()
    {
        duk_destroy_heap(ctx);
    }

    std::string invokeABCBOX(duk_context* ctx, std::string input, int boxType, std::string divChar)
    {
        ScriptTestFixture::addConfig(ctx, { { "/import/scripting/virtual-layout/abcBox/skipChars", "" } });

        duk_get_global_string(ctx, "abcbox");
        duk_push_string(ctx, input.c_str());
        duk_push_int(ctx, boxType);
        duk_push_string(ctx, divChar.c_str());

        duk_pcall(ctx, 3);
        return duk_to_string(ctx, -1);
    }

    duk_context* ctx;
};

TEST_F(CommonScriptTest, CreatesDukContextWithCommonScript)
{
    EXPECT_NE(ctx, nullptr);
}

TEST_F(CommonScriptTest, escapeSlash_AddsEscapeCharsForSlash)
{
    duk_get_global_string(ctx, "escapeSlash");
    duk_push_string(ctx, "some/path/to/escape");

    duk_pcall(ctx, 1);

    std::string result = duk_to_string(ctx, -1);

    EXPECT_STREQ(result.c_str(), "some\\/path\\/to\\/escape");
}

TEST_F(CommonScriptTest, mapInitial_Latin)
{
    duk_get_global_string(ctx, "mapInitial");
    duk_push_string(ctx, "a");

    duk_pcall(ctx, 1);

    std::string result = duk_to_string(ctx, -1);

    EXPECT_STREQ(result.c_str(), "A");
}

TEST_F(CommonScriptTest, mapInitial_Umlaut)
{
    duk_get_global_string(ctx, "mapInitial");
    duk_push_string(ctx, "ä");

    duk_pcall(ctx, 1);

    std::string result = duk_to_string(ctx, -1);

    EXPECT_STREQ(result.c_str(), "A");
}

TEST_F(CommonScriptTest, escapeSlash_AddsEscapeCharsForBackSlash)
{
    duk_get_global_string(ctx, "escapeSlash");
    duk_push_string(ctx, "some\\path\\to\\escape");

    duk_pcall(ctx, 1);

    std::string result = duk_to_string(ctx, -1);

    EXPECT_STREQ(result.c_str(), "some\\\\path\\\\to\\\\escape");
}

TEST_F(CommonScriptTest, createContainerChain_concatenatesContainerChainWithSlashBasedOnArray)
{
    duk_idx_t arr_idx;
    duk_get_global_string(ctx, "createContainerChain");
    arr_idx = duk_push_array(ctx);
    duk_push_string(ctx, "path");
    duk_put_prop_index(ctx, arr_idx, 0);
    duk_push_string(ctx, "to");
    duk_put_prop_index(ctx, arr_idx, 1);
    duk_push_string(ctx, "combine");
    duk_put_prop_index(ctx, arr_idx, 2);

    duk_pcall(ctx, 1);

    std::string result = duk_to_string(ctx, -1);

    EXPECT_STREQ(result.c_str(), "/path/to/combine");
}

TEST_F(CommonScriptTest, createContainerChain_returnsEmptyWhenArrayIsEmpty)
{
    duk_get_global_string(ctx, "createContainerChain");
    duk_push_array(ctx);

    duk_pcall(ctx, 1);

    std::string result = duk_to_string(ctx, -1);

    EXPECT_STREQ(result.c_str(), "");
}

TEST_F(CommonScriptTest, getYear_returnsFourDigitYearNextToDash)
{
    duk_get_global_string(ctx, "getYear");
    duk_push_string(ctx, "2018-01-01");

    duk_pcall(ctx, 1);

    std::string result = duk_to_string(ctx, -1);

    EXPECT_STREQ(result.c_str(), "2018");
}

TEST_F(CommonScriptTest, getYear_returnsAsIsIfNoYearFound)
{
    duk_get_global_string(ctx, "getYear");
    duk_push_string(ctx, "January 1, 2018");

    duk_pcall(ctx, 1);

    std::string result = duk_to_string(ctx, -1);

    EXPECT_STREQ(result.c_str(), "January 1, 2018");
}

TEST_F(CommonScriptTest, getPlaylistType_AsM3U)
{
    duk_get_global_string(ctx, "getPlaylistType");
    duk_push_string(ctx, "audio/x-mpegurl");

    duk_pcall(ctx, 1);

    std::string result = duk_to_string(ctx, -1);

    EXPECT_STREQ(result.c_str(), "m3u");
}

TEST_F(CommonScriptTest, getPlaylistType_AsPLS)
{
    duk_get_global_string(ctx, "getPlaylistType");
    duk_push_string(ctx, "audio/x-scpls");

    duk_pcall(ctx, 1);

    std::string result = duk_to_string(ctx, -1);

    EXPECT_STREQ(result.c_str(), "pls");
}

TEST_F(CommonScriptTest, getLastPath_ReturnsLastPathFromSlash)
{
    duk_get_global_string(ctx, "getLastPath");
    duk_push_string(ctx, "/location/last/element/");

    duk_pcall(ctx, 1);

    std::string result = duk_to_string(ctx, -1);

    EXPECT_STREQ(result.c_str(), "element");
}

TEST_F(CommonScriptTest, getLastPath_ReturnsEmptyWhenNotPath)
{
    duk_get_global_string(ctx, "getLastPath");
    duk_push_string(ctx, "not-a-path");

    duk_pcall(ctx, 1);

    std::string result = duk_to_string(ctx, -1);

    EXPECT_STREQ(result.c_str(), "");
}

TEST_F(CommonScriptTest, getRootPath_ReturnsArrayOfRootPath)
{
    duk_get_global_string(ctx, "getRootPath");
    duk_push_string(ctx, "/parent/child/");
    duk_push_string(ctx, "/parent/child/folder/path/");

    duk_pcall(ctx, 2);

    duk_size_t arSize = duk_get_length(ctx, -1);

    EXPECT_EQ(2, arSize);
    EXPECT_TRUE(duk_is_array(ctx, -1));
    std::string result = duk_to_string(ctx, -1);
    EXPECT_STREQ(result.c_str(), "folder,path");
}

TEST_F(CommonScriptTest, getRootPath_ReturnsArrayOfLastPathWhenRootIsEmpty)
{
    duk_get_global_string(ctx, "getRootPath");
    duk_push_string(ctx, "");
    duk_push_string(ctx, "/parent/child/folder/path/");

    duk_pcall(ctx, 2);
    duk_size_t arSize = duk_get_length(ctx, -1);

    EXPECT_EQ(1, arSize);
    EXPECT_TRUE(duk_is_array(ctx, -1));
    std::string result = duk_to_string(ctx, -1);
    EXPECT_STREQ(result.c_str(), "path");
}

TEST_F(CommonScriptTest, abcbox_BoxType1_ReturnsASingleBox)
{
    int boxType = 1;
    std::string divChar = "-";

    std::string result = this->invokeABCBOX(ctx, "Alpha", boxType, divChar);

    EXPECT_STREQ(result.c_str(), "-ABCDEFGHIJKLMNOPQRSTUVWXYZ-");
}

TEST_F(CommonScriptTest, abcbox_BoxType2_ReturnsCorrectBoxBasedOnCharacter)
{
    int boxType = 2;
    std::string divChar = "-";

    std::string result = this->invokeABCBOX(ctx, "Alpha", boxType, divChar);
    EXPECT_STREQ(result.c_str(), "-ABCDEFGHIJKLM-");

    result = this->invokeABCBOX(ctx, "Omega", boxType, divChar);
    EXPECT_STREQ(result.c_str(), "-NOPQRSTUVWXYZ-");
}

TEST_F(CommonScriptTest, abcbox_BoxType3_ReturnsCorrectBoxBasedOnCharacter)
{
    int boxType = 3;
    std::string divChar = "-";

    std::string result = this->invokeABCBOX(ctx, "Alpha", boxType, divChar);
    EXPECT_STREQ(result.c_str(), "-ABCDEFGH-");

    result = this->invokeABCBOX(ctx, "Omega", boxType, divChar);
    EXPECT_STREQ(result.c_str(), "-IJKLMNOPQ-");

    result = this->invokeABCBOX(ctx, "Xi", boxType, divChar);
    EXPECT_STREQ(result.c_str(), "-RSTUVWXYZ-");
}

TEST_F(CommonScriptTest, abcbox_BoxType4_ReturnsCorrectBoxBasedOnCharacter)
{
    int boxType = 4;
    std::string divChar = "-";

    std::string result = this->invokeABCBOX(ctx, "Alpha", boxType, divChar);
    EXPECT_STREQ(result.c_str(), "-ABCDEFG-");

    result = this->invokeABCBOX(ctx, "Kappa", boxType, divChar);
    EXPECT_STREQ(result.c_str(), "-HIJKLM-");

    result = this->invokeABCBOX(ctx, "Omega", boxType, divChar);
    EXPECT_STREQ(result.c_str(), "-NOPQRST-");

    result = this->invokeABCBOX(ctx, "Xi", boxType, divChar);
    EXPECT_STREQ(result.c_str(), "-UVWXYZ-");
}

TEST_F(CommonScriptTest, abcbox_BoxType5_ReturnsCorrectBoxBasedOnCharacter)
{
    int boxType = 5;
    std::string divChar = "-";

    std::string result = this->invokeABCBOX(ctx, "Alpha", boxType, divChar);
    EXPECT_STREQ(result.c_str(), "-ABCDE-");

    result = this->invokeABCBOX(ctx, "Gamma", boxType, divChar);
    EXPECT_STREQ(result.c_str(), "-FGHIJ-");

    result = this->invokeABCBOX(ctx, "Kappa", boxType, divChar);
    EXPECT_STREQ(result.c_str(), "-KLMNO-");

    result = this->invokeABCBOX(ctx, "Phi", boxType, divChar);
    EXPECT_STREQ(result.c_str(), "-PQRSTU-");

    result = this->invokeABCBOX(ctx, "Xi", boxType, divChar);
    EXPECT_STREQ(result.c_str(), "-VWXYZ-");
}

TEST_F(CommonScriptTest, abcbox_BoxType6_ReturnsCorrectBoxBasedOnCharacter)
{
    int boxType = 6;
    std::string divChar = "-";

    std::string result = this->invokeABCBOX(ctx, "Alpha", boxType, divChar);
    EXPECT_STREQ(result.c_str(), "-ABCD-");

    result = this->invokeABCBOX(ctx, "Gamma", boxType, divChar);
    EXPECT_STREQ(result.c_str(), "-EFGHI-");

    result = this->invokeABCBOX(ctx, "Kappa", boxType, divChar);
    EXPECT_STREQ(result.c_str(), "-JKLM-");

    result = this->invokeABCBOX(ctx, "Phi", boxType, divChar);
    EXPECT_STREQ(result.c_str(), "-NOPQ-");

    result = this->invokeABCBOX(ctx, "Rho", boxType, divChar);
    EXPECT_STREQ(result.c_str(), "-RSTUV-");

    result = this->invokeABCBOX(ctx, "Xi", boxType, divChar);
    EXPECT_STREQ(result.c_str(), "-WXYZ-");
}

TEST_F(CommonScriptTest, abcbox_BoxType7_ReturnsCorrectBoxBasedOnCharacter)
{
    int boxType = 7;
    std::string divChar = "-";

    std::string result = this->invokeABCBOX(ctx, "Alpha", boxType, divChar);
    EXPECT_STREQ(result.c_str(), "-ABCD-");

    result = this->invokeABCBOX(ctx, "Gamma", boxType, divChar);
    EXPECT_STREQ(result.c_str(), "-EFG-");

    result = this->invokeABCBOX(ctx, "Kappa", boxType, divChar);
    EXPECT_STREQ(result.c_str(), "-HIJK-");

    result = this->invokeABCBOX(ctx, "Lambda", boxType, divChar);
    EXPECT_STREQ(result.c_str(), "-LMNO-");

    result = this->invokeABCBOX(ctx, "Phi", boxType, divChar);
    EXPECT_STREQ(result.c_str(), "-PQRS-");

    result = this->invokeABCBOX(ctx, "Tau", boxType, divChar);
    EXPECT_STREQ(result.c_str(), "-TUV-");

    result = this->invokeABCBOX(ctx, "Xi", boxType, divChar);
    EXPECT_STREQ(result.c_str(), "-WXYZ-");
}

TEST_F(CommonScriptTest, abcbox_BoxType9_ReturnsCorrectBoxBasedOnCharacter)
{
    int boxType = 9;
    std::string divChar = "-";

    std::string result = this->invokeABCBOX(ctx, "Alpha", boxType, divChar);
    EXPECT_STREQ(result.c_str(), "-ABCDE-");

    result = this->invokeABCBOX(ctx, "Gamma", boxType, divChar);
    EXPECT_STREQ(result.c_str(), "-FGHIJ-");

    result = this->invokeABCBOX(ctx, "Kappa", boxType, divChar);
    EXPECT_STREQ(result.c_str(), "-KLMNO-");

    result = this->invokeABCBOX(ctx, "Phi", boxType, divChar);
    EXPECT_STREQ(result.c_str(), "-PQRS-");

    result = this->invokeABCBOX(ctx, "Tau", boxType, divChar);
    EXPECT_STREQ(result.c_str(), "-T-");

    result = this->invokeABCBOX(ctx, "Xi", boxType, divChar);
    EXPECT_STREQ(result.c_str(), "-UVWXYZ-");
}

TEST_F(CommonScriptTest, abcbox_BoxType26_ReturnsCorrectBoxBasedOnCharacter)
{
    int boxType = 26;
    std::string divChar = "-";

    std::string result = this->invokeABCBOX(ctx, "Alpha", boxType, divChar);
    EXPECT_STREQ(result.c_str(), "-A-");

    result = this->invokeABCBOX(ctx, "Gamma", boxType, divChar);
    EXPECT_STREQ(result.c_str(), "-G-");

    result = this->invokeABCBOX(ctx, "Kappa", boxType, divChar);
    EXPECT_STREQ(result.c_str(), "-K-");

    result = this->invokeABCBOX(ctx, "Phi", boxType, divChar);
    EXPECT_STREQ(result.c_str(), "-P-");

    result = this->invokeABCBOX(ctx, "Tau", boxType, divChar);
    EXPECT_STREQ(result.c_str(), "-T-");

    result = this->invokeABCBOX(ctx, "Xi", boxType, divChar);
    EXPECT_STREQ(result.c_str(), "-X-");

    result = this->invokeABCBOX(ctx, "2L", boxType, divChar);
    EXPECT_STREQ(result.c_str(), "-0-9-");

    result = this->invokeABCBOX(ctx, "#x", boxType, divChar);
    EXPECT_STREQ(result.c_str(), "-^&#'-");
}

TEST_F(CommonScriptTest, abcbox_BoxTypeDefault_ReturnsCorrectBoxBasedOnCharacter)
{
    int boxType = 9999;
    std::string divChar = "-";

    std::string result = this->invokeABCBOX(ctx, "Alpha", boxType, divChar);
    EXPECT_STREQ(result.c_str(), "-ABCDE-");

    result = this->invokeABCBOX(ctx, "Gamma", boxType, divChar);
    EXPECT_STREQ(result.c_str(), "-FGHIJ-");

    result = this->invokeABCBOX(ctx, "Kappa", boxType, divChar);
    EXPECT_STREQ(result.c_str(), "-KLMNO-");

    result = this->invokeABCBOX(ctx, "Phi", boxType, divChar);
    EXPECT_STREQ(result.c_str(), "-PQRSTU-");

    result = this->invokeABCBOX(ctx, "Xi", boxType, divChar);
    EXPECT_STREQ(result.c_str(), "-VWXYZ-");
}

TEST_F(CommonScriptTest, abcbox_BoxTypeSpecialChars_ReturnsCorrectBoxBasedOnCharacter)
{
    int boxType = 6;
    std::string divChar = "-";

    std::string result = this->invokeABCBOX(ctx, "111", boxType, divChar);
    EXPECT_STREQ(result.c_str(), "-0-9-");

    result = this->invokeABCBOX(ctx, "#1", boxType, divChar);
    EXPECT_STREQ(result.c_str(), "-^&#'-");
}
#endif
