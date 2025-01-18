/*GRB*

    Gerbera - https://gerbera.io/

    test_common_script.cc - this file is part of Gerbera.

    Copyright (C) 2020-2025 Gerbera Contributors

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
#ifdef HAVE_JS

#include "util/grb_fs.h"
#include "util/tools.h"

#include "mock/duk_helper.h"
#include "mock/script_test_fixture.h"

#include <duktape.h>

class CommonScriptTest : public ::testing::Test {

public:
    CommonScriptTest() = default;
    ~CommonScriptTest() override = default;

    void SetUp() override
    {
        ctx = duk_create_heap(nullptr, nullptr, nullptr, nullptr, nullptr);

        fs::path scriptPath = fs::path(SCRIPTS_DIR) / "js" / "common.js";
        std::string script = GrbFile(scriptPath).readTextFile();
        duk_push_string(ctx, scriptPath.c_str());
        if (duk_pcompile_lstring_filename(ctx, 0, script.c_str(), script.length()) != 0) {
            DukTestHelper::printError(ctx, "Failed to load script ", scriptPath);
        } else {
            duk_call(ctx, 0);
        }
    }

    void TearDown() override
    {
        duk_destroy_heap(ctx);
    }

    static std::string invokeABCBOX(duk_context* ctx, const std::string& input, int boxType, const std::string& divChar)
    {
        ScriptTestFixture::addConfig(ctx, { { "/import/scripting/virtual-layout/abcBox/skipChars", "" } });

        duk_get_global_string(ctx, "abcbox");
        duk_push_string(ctx, input.c_str());
        duk_push_int(ctx, boxType);
        duk_push_string(ctx, divChar.c_str());

        duk_pcall(ctx, 3);
        return duk_to_string(ctx, -1);
    }

    static std::string invokeALLBOX(duk_context* ctx, const std::string& input, int boxType, const std::string& divChar)
    {
        duk_get_global_string(ctx, "allbox");
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
    duk_push_string(ctx, R"(some\path\to\escape)");

    duk_pcall(ctx, 1);

    std::string result = duk_to_string(ctx, -1);

    EXPECT_STREQ(result.c_str(), "some\\\\path\\\\to\\\\escape");
}

TEST_F(CommonScriptTest, createContainerChain_concatenatesContainerChainWithSlashBasedOnArray)
{
    duk_idx_t arrIdx;
    duk_get_global_string(ctx, "createContainerChain");
    arrIdx = duk_push_array(ctx);
    duk_push_string(ctx, "path");
    duk_put_prop_index(ctx, arrIdx, 0);
    duk_push_string(ctx, "to");
    duk_put_prop_index(ctx, arrIdx, 1);
    duk_push_string(ctx, "combine");
    duk_put_prop_index(ctx, arrIdx, 2);

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

TEST_F(CommonScriptTest, getLastPath2_ReturnsLastPathFromSlash)
{
    duk_get_global_string(ctx, "getLastPath2");
    duk_push_string(ctx, "/location/last/element/");
    duk_push_int(ctx, 1);

    duk_pcall(ctx, 2);
    duk_size_t arSize = duk_get_length(ctx, -1);

    EXPECT_EQ(1, arSize);
    EXPECT_TRUE(duk_is_array(ctx, -1));
    std::string result = duk_to_string(ctx, -1);
    EXPECT_STREQ(result.c_str(), "element");
}

TEST_F(CommonScriptTest, getLastPath2_ReturnsLast2PathFromSlash)
{
    duk_get_global_string(ctx, "getLastPath2");
    duk_push_string(ctx, "/location/last/element/file.ext");
    duk_push_int(ctx, 2);

    duk_pcall(ctx, 2);
    duk_size_t arSize = duk_get_length(ctx, -1);

    EXPECT_EQ(2, arSize);
    EXPECT_TRUE(duk_is_array(ctx, -1));
    std::string result = duk_to_string(ctx, -1);
    EXPECT_STREQ(result.c_str(), "last,element");
}

TEST_F(CommonScriptTest, getLastPath2_ReturnsLast2PathFromFolder)
{
    duk_get_global_string(ctx, "getLastPath2");
    duk_push_string(ctx, "/home/location/with/last/element/folder/");
    duk_push_int(ctx, -3);

    duk_pcall(ctx, 2);
    duk_size_t arSize = duk_get_length(ctx, -1);

    EXPECT_EQ(3, arSize);
    EXPECT_TRUE(duk_is_array(ctx, -1));
    std::string result = duk_to_string(ctx, -1);
    EXPECT_STREQ(result.c_str(), "with,last,element");
}

TEST_F(CommonScriptTest, getLastPath2_ReturnsEmptyWhenNotPath)
{
    duk_get_global_string(ctx, "getLastPath2");
    duk_push_string(ctx, "not-a-path");
    duk_push_int(ctx, 1);

    duk_pcall(ctx, 2);
    duk_size_t arSize = duk_get_length(ctx, -1);

    EXPECT_EQ(0, arSize);
    EXPECT_TRUE(duk_is_array(ctx, -1));
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

TEST_F(CommonScriptTest, allbox_BoxType1_Returns10)
{
    std::string divChar = "-";
    auto boxTest = std::map<int, std::string> {
        { 1, "------------all------------" },
        { 2, "------all------" },
        { 3, "---all---" },
        { 4, "--all--" },
        { 5, "--all--" },
        { 6, "--all--" },
        { 9, "-all-" },
        { 13, "-all-" },
        { 26, "-all-" },
    };

    for (auto&& [boxType, res] : boxTest) {
        std::string result = this->invokeALLBOX(ctx, "all", boxType, divChar);

        EXPECT_STREQ(result.c_str(), res.c_str());
    }
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
