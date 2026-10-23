/*GRB*

    Gerbera - https://gerbera.io/

    test_common_script.cc - this file is part of Gerbera.

    Copyright (C) 2020-2026 Gerbera Contributors

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

class CommonScriptTest : public ScriptTestFixture {

public:
    CommonScriptTest() { }

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

TEST_F(CommonScriptTest, weekOf0)
{
    duk_get_global_string(ctx, "weekOf");
    duk_push_int(ctx, 2024);
    duk_push_int(ctx, 0); // month starts at 0
    duk_push_int(ctx, 1);

    duk_pcall(ctx, 3);

    std::string result = duk_to_string(ctx, -1);

    EXPECT_STREQ(result.c_str(), "2024-01") << "2024-01-01";

    duk_get_global_string(ctx, "weekOf");
    duk_push_int(ctx, 2023);
    duk_push_int(ctx, 11); // month starts at 0
    duk_push_int(ctx, 31);

    duk_pcall(ctx, 3);

    result = duk_to_string(ctx, -1);

    EXPECT_STREQ(result.c_str(), "2023-52") << "2023-12-31";

    duk_get_global_string(ctx, "weekOf");
    duk_push_int(ctx, 2024);
    duk_push_int(ctx, 0); // month starts at 0
    duk_push_int(ctx, 7);

    duk_pcall(ctx, 3);

    result = duk_to_string(ctx, -1);

    EXPECT_STREQ(result.c_str(), "2024-01") << "2024-01-07";
}

TEST_F(CommonScriptTest, weekOf1)
{
    duk_get_global_string(ctx, "weekOf");
    duk_push_int(ctx, 2025);
    duk_push_int(ctx, 0); // month starts at 0
    duk_push_int(ctx, 1);

    duk_pcall(ctx, 3);

    std::string result = duk_to_string(ctx, -1);

    EXPECT_STREQ(result.c_str(), "2025-01") << "2025-01-01";

    duk_get_global_string(ctx, "weekOf");
    duk_push_int(ctx, 2024);
    duk_push_int(ctx, 11); // month starts at 0
    duk_push_int(ctx, 30);

    duk_pcall(ctx, 3);

    result = duk_to_string(ctx, -1);

    EXPECT_STREQ(result.c_str(), "2025-01") << "2024-12-30";

    duk_get_global_string(ctx, "weekOf");
    duk_push_int(ctx, 2025);
    duk_push_int(ctx, 0); // month starts at 0
    duk_push_int(ctx, 5);

    duk_pcall(ctx, 3);

    result = duk_to_string(ctx, -1);

    EXPECT_STREQ(result.c_str(), "2025-01") << "2025-01-05";
}

TEST_F(CommonScriptTest, weekOf53)
{
    duk_get_global_string(ctx, "weekOf");
    duk_push_int(ctx, 2027);
    duk_push_int(ctx, 0); // month starts at 0
    duk_push_int(ctx, 1);

    duk_pcall(ctx, 3);

    std::string result = duk_to_string(ctx, -1);

    EXPECT_STREQ(result.c_str(), "2026-53") << "2027-01-01";

    duk_get_global_string(ctx, "weekOf");
    duk_push_int(ctx, 2026);
    duk_push_int(ctx, 11); // month starts at 0
    duk_push_int(ctx, 27);

    duk_pcall(ctx, 3);

    result = duk_to_string(ctx, -1);

    EXPECT_STREQ(result.c_str(), "2026-52") << "2026-12-27";

    duk_get_global_string(ctx, "weekOf");
    duk_push_int(ctx, 2026);
    duk_push_int(ctx, 11); // month starts at 0
    duk_push_int(ctx, 28);

    duk_pcall(ctx, 3);

    result = duk_to_string(ctx, -1);

    EXPECT_STREQ(result.c_str(), "2026-53") << "2026-12-28";

    duk_get_global_string(ctx, "weekOf");
    duk_push_int(ctx, 2027);
    duk_push_int(ctx, 0); // month starts at 0
    duk_push_int(ctx, 3);

    duk_pcall(ctx, 3);

    result = duk_to_string(ctx, -1);

    EXPECT_STREQ(result.c_str(), "2026-53") << "2027-01-03";

    duk_get_global_string(ctx, "weekOf");
    duk_push_int(ctx, 2027);
    duk_push_int(ctx, 0); // month starts at 0
    duk_push_int(ctx, 4);

    duk_pcall(ctx, 3);

    result = duk_to_string(ctx, -1);

    EXPECT_STREQ(result.c_str(), "2027-01") << "2027-01-04";
}

TEST_F(CommonScriptTest, weekOf45)
{
    duk_get_global_string(ctx, "weekOf");
    duk_push_int(ctx, 2025);
    duk_push_int(ctx, 10); // month starts at 0
    duk_push_int(ctx, 3);

    duk_pcall(ctx, 3);

    std::string result = duk_to_string(ctx, -1);

    EXPECT_STREQ(result.c_str(), "2025-45");

    duk_get_global_string(ctx, "weekOf");
    duk_push_int(ctx, 2025);
    duk_push_int(ctx, 10); // month starts at 0
    duk_push_int(ctx, 2);

    duk_pcall(ctx, 3);

    result = duk_to_string(ctx, -1);

    EXPECT_STREQ(result.c_str(), "2025-44");

    duk_get_global_string(ctx, "weekOf");
    duk_push_int(ctx, 2025);
    duk_push_int(ctx, 10); // month starts at 0
    duk_push_int(ctx, 8);

    duk_pcall(ctx, 3);

    result = duk_to_string(ctx, -1);

    EXPECT_STREQ(result.c_str(), "2025-45");
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
TEST_F(CommonScriptTest, AudioTitleFormatsPreserveMetadataAndOrdering)
{
    const duk_function_list_entry functions[] = { { nullptr, nullptr, 0 } };
    addGlobalFunctions(ctx, functions);
    ASSERT_EQ(duk_peval_string(ctx, R"JS(
        function getRootPath() { return []; }
        var formatKey = '/import/scripting/virtual-layout/attribute::audio-title-format';
        var rulesKey = '/import/scripting/virtual-layout/title-replacements/replace';
        var sourceItem = {
            title: 'Artist - File', location: '/music/Artist - File.flac',
            aux: { DISCNUMBER: '2/3' }, res: {},
            metaData: {
                'dc:title': [' Track_"音楽" '],
                'upnp:artist': ['Artist', 'Guest'],
                'upnp:albumArtist': ['Various Artists'], 'upnp:album': ['Album'],
                'upnp:originalTrackNumber': ['02'], 'dc:date': ['2026-01-01'],
                'upnp:genre': ['Techno', 'Trance'], 'upnp:composer': ['Composer']
            }
        };
        sourceItem.metaData[M_ALBUMARTIST] = ['Various Artists'];
        var titleCaseNumber = 0;
        function titleCase(pattern, expected, rules, change) {
            titleCaseNumber++;
            var item = JSON.parse(JSON.stringify(sourceItem));
            if (change) change(item);
            delete config[formatKey];
            delete config[rulesKey];
            var baseline = getAudioDetails(JSON.parse(JSON.stringify(item)), '/music');
            var baselineItem = JSON.parse(JSON.stringify(item));
            getAudioDetails(baselineItem, '/music');
            config[formatKey] = pattern;
            config[rulesKey] = rules || [];
            var result = getAudioDetails(item, '/music');
            if (result.title !== expected)
                throw new Error('Audio title case ' + titleCaseNumber + ' (' + pattern + '): expected ' +
                    JSON.stringify(expected) + ', got ' + JSON.stringify(result.title));
            result.title = baseline.title;
            if (JSON.stringify(result) !== JSON.stringify(baseline) ||
                JSON.stringify(item) !== JSON.stringify(baselineItem))
                throw new Error('Audio title case ' + titleCaseNumber + ': formatting changed source metadata or ordering');
        }
        var defaultItem = JSON.parse(JSON.stringify(sourceItem));
        var defaultTitle = getAudioDetails(defaultItem, '/music').title;
        if (defaultTitle !== 'Artist - Guest -   Track_"音楽" ')
            throw new Error('Default artist prefix changed: ' + defaultTitle);
        titleCase('%title%', 'Track_"音楽"');
        titleCase('%artist% - %title%', 'Artist / Guest - Track_"音楽"');
        titleCase('%albumartist% | %albumtitle% | %year% | %genre% | %composer%',
            'Various Artists | Album | 2026 | Techno / Trance | Composer');
        titleCase('%cdnumber%.%tracknr2%/%tracknr1%/%tracknr3%', '2.02/2/002');
        titleCase('%filename%', 'Artist - File');
        titleCase('%% %unknown% %TITLE%', '% %unknown% %TITLE%');
        titleCase('%title%', 'Artist - File', [], function(i) { delete i.metaData['dc:title']; });
        titleCase('%albumartist%', 'Various Artists', [], function(i) { delete i.metaData['upnp:artist']; });
        titleCase('%albumartist%', 'Artist', [], function(i) { delete i.metaData[M_ALBUMARTIST]; });
        titleCase('%artist%%albumartist%%albumtitle%%genre%%composer%%year%%tracknr2%%cdnumber%',
            'Track_"音楽"', [], function(i) { i.metaData = { 'dc:title': ['Track_"音楽"'] }; i.aux = {}; });
        titleCase('%tracknr2%/%tracknr3%', '123/123', [], function(i) { i.metaData['upnp:originalTrackNumber'] = ['123']; });
        titleCase('%tracknr2%%cdnumber%', 'Track_"音楽"', [], function(i) { i.metaData['upnp:originalTrackNumber'] = ['0']; i.aux.DISCNUMBER = '0'; });
        titleCase('%title%', 'Track "音楽"', [{from:'_',to:' '}]);
        titleCase('%title%', 'Track_音楽', [{from:'"',to:''}]);
        titleCase('ab a', 'Xb X', [{from:'a',to:'ab'},{from:'ab',to:'X'}]);
        titleCase('a', 'first', [{from:'a',to:'first'},{from:'a',to:'second'}]);
        titleCase('A $1 [x]', 'A literal 音楽', [{from:'a',to:'z'},{from:'$1',to:'literal'},{from:'[x]',to:'音楽'}]);
        titleCase('REMOVE', 'Track_"音楽"', [{from:'REMOVE',to:''}]);
        titleCase('%filename%', 'Title', [], function(i) { i.location = 'C:\\music\\Title.mp3'; });
        titleCase('%title%', 'Track_"音楽"', [{from:'',to:'ignored'}]);
        titleCase('A  B\tC', 'A B C', [{from:'\\s+',to:' ',regex:'yes'}]);
        titleCase('A  B', 'A B', [{from:'\\s+',to:' ',regex:'true'}]);
        titleCase('A.*B', 'literal', [{from:'A.*B',to:'literal',regex:'no'}]);
        titleCase('A.*B', 'literal', [{from:'A.*B',to:'literal',regex:'false'}]);
        titleCase('A B A B', 'B/A B/A', [{from:'(A) (B)',to:'$2/$1',regex:'yes'}]);
        titleCase('A', '$1', [{from:'A',to:'$1'}]);
        titleCase('%title%', 'Track "音楽"', [{from:'_',to:' ','if-albumtitle':'Album'}]);
        titleCase('%title%', 'Track_"音楽"', [{from:'_',to:' ','if-albumtitle':'Other'}]);
        titleCase('%title%', 'Track "音楽"', [{from:'_',to:' ','if-artist':'Guest'}]);
        titleCase('%title%', 'Track_"音楽"', [{from:'_',to:' ','if-artist':'guest'}]);
        titleCase('%title%', 'Track "音楽"', [{from:'_',to:' ','if-artist':'Artist','if-albumtitle':'Album'}]);
        titleCase('%title%', 'Track_"音楽"', [{from:'_',to:' ','if-artist':'Artist','if-albumtitle':'Other'}]);
        titleCase('%title%', 'Track_"音楽"', [{from:'_',to:' ','if-artist':'Artist'}], function(i) { delete i.metaData[M_ARTIST]; });
        titleCase('%title%', 'Track_"音楽"', [{from:'_',to:' ','if-albumartist':'Artist'}], function(i) { delete i.metaData[M_ALBUMARTIST]; });
        titleCase('%title%', 'Track "音楽"', [{from:'_',to:' ','if-albumartist':'Various Artists','if-genre':'Trance','if-composer':'Composer','if-year':'2026'}]);
        titleCase('%title%', 'Track_"音楽"', [{from:'_',to:' ','if-year':'2025'}]);
        titleCase('%title%', 'Track_"音楽"', [{from:'_',to:' ','if-year':'Unknown'}], function(i) { delete i.metaData[M_DATE]; });
        titleCase('%title%', 'Track "音楽"', [{from:'_',to:' ','if-title':' Track_"音楽" '}]);
        titleCase('%title%', 'Artist - File', [{from:'Artist',to:'Changed','if-title':'Artist - File'}], function(i) { delete i.metaData[M_TITLE]; });
        titleCase('%title%', 'Track "音楽"', [{from:'_',to:' ','if-artist':''}]);
        titleCase('A  B', 'A B', [{from:'\\s+',to:' ',regex:'yes','if-artist':'Guest'}]);
        titleCase('A  B', 'A  B', [{from:'\\s+',to:' ',regex:'yes','if-artist':'Other'}]);
        titleCase('%title%', 'Track_"音楽"', [{from:'Track',to:'Changed','if-title':'Track_"音楽"'}]);
        var warnings = [];
        function print(message) { warnings.push(message); }
        config[rulesKey] = [{from:'(',to:'bad',regex:'yes'}, {from:'_',to:' '}];
        var prepared = getAudioTitleReplacements();
        if (warnings.length !== 1 || prepared.length !== 1 || getAudioTitleReplacements() !== prepared || warnings.length !== 1)
            throw new Error('Rules must be prepared once; invalid regex skipped and logged once');
        config[formatKey] = '%title%';
        if (getAudioDetails(JSON.parse(JSON.stringify(sourceItem)), '/music').title !== 'Track "音楽"')
            throw new Error('Valid rules following an invalid regex must still apply');
        config[rulesKey] = [{from:'\\s+',to:' ',regex:'yes'}];
        config[formatKey] = 'A  B';
        var regexRules = getAudioTitleReplacements();
        for (var repeat = 0; repeat < 3; repeat++) {
            if (getAudioDetails(JSON.parse(JSON.stringify(sourceItem)), '/music').title !== 'A B' ||
                getAudioTitleReplacements() !== regexRules)
                throw new Error('Cached global regex state leaked between titles');
        }
        config[rulesKey] = [{from:'A',to:'C'}];
        if (getAudioTitleReplacements() === regexRules || getAudioDetails(JSON.parse(JSON.stringify(sourceItem)), '/music').title !== 'C  B')
            throw new Error('Replacing configuration must invalidate the prepared rules');

        true;
    )JS"),
        DUK_EXEC_SUCCESS)
        << duk_safe_to_string(ctx, -1);
    EXPECT_TRUE(duk_get_boolean(ctx, -1));
    duk_pop(ctx);
}

#endif
