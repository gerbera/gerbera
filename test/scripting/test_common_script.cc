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

#include "metadata/metadata_enums.h"
#include "util/grb_fs.h"
#include "util/logger.h"
#include "util/tools.h"

#include "mock/duk_helper.h"
#include "mock/script_test_fixture.h"

#include <algorithm>
#include <duktape.h>
#include <map>
#include <memory>
#include <spdlog/sinks/ostream_sink.h>
#include <sstream>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

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
using AudioTitleRules = std::vector<std::vector<std::pair<std::string_view, std::string_view>>>;

enum class AudioTitleChange { None,
    NoTitle,
    NoArtist,
    NoAlbumArtist,
    OnlyTitle,
    LongTrack,
    ZeroNumbers,
    WindowsFilename,
    NoDate };

struct AudioTitleCase {
    const char* name;
    const char* pattern;
    const char* expected;
    AudioTitleRules rules;
    AudioTitleChange change;
};

class AudioTitleScriptTest : public CommonScriptTest {
protected:
    std::ostringstream warnings;
    std::shared_ptr<spdlog::logger> previousLogger;

    void SetUp() override
    {
        CommonScriptTest::SetUp();
        previousLogger = spdlog::default_logger();
        auto sink = std::make_shared<spdlog::sinks::ostream_sink_mt>(warnings);
        auto logger = std::make_shared<spdlog::logger>("audio-title-script-test", sink);
        logger->set_pattern("%v");
        logger->set_level(spdlog::level::warn);
        spdlog::set_default_logger(std::move(logger));
        const duk_function_list_entry functions[] { { nullptr, nullptr, 0 } };
        addGlobalFunctions(ctx, functions);
    }

    void TearDown() override
    {
        spdlog::set_default_logger(std::move(previousLogger));
        CommonScriptTest::TearDown();
    }

    void configure(const char* pattern, const AudioTitleRules& rules = {})
    {
        std::map<std::string_view, std::map<std::string_view, std::string_view>> dictionaries;
        if (pattern)
            dictionaries["/import/scripting/virtual-layout/script-options/script-option"]["audioTitleFormat"] = pattern;
        addConfig(ctx, {}, {}, dictionaries,
            { { "/import/scripting/virtual-layout/title-replacements/replace", rules } });
    }

    struct TitleResult {
        std::string title;
        std::string date;
        std::string decade;
        std::string details;
        std::string source;
    };

    TitleResult invoke(AudioTitleChange change = AudioTitleChange::None)
    {
        const auto albumArtistKey = MetaEnumMapper::getMetaFieldName(MetadataFields::M_ALBUMARTIST);
        std::map<std::string, std::string> props { { "title", "Artist - File" }, { "location", "/music/Artist - File.flac" } };
        std::vector<std::pair<std::string, std::string>> metadata {
            { "dc:title", " Track_\"音楽\" " }, { "upnp:artist", "Artist" }, { "upnp:artist", "Guest" },
            { albumArtistKey, "Various Artists" }, { "upnp:album", "Album" },
            { "upnp:originalTrackNumber", "02" }, { "dc:date", "2026-01-01" },
            { "upnp:genre", "Techno" }, { "upnp:genre", "Trance" }, { "upnp:composer", "Composer" }
        };
        std::map<std::string, std::string> aux { { "DISCNUMBER", "2/3" } };
        const char* removed = nullptr;
        switch (change) {
        case AudioTitleChange::NoTitle:
            removed = "dc:title";
            break;
        case AudioTitleChange::NoArtist:
            removed = "upnp:artist";
            break;
        case AudioTitleChange::NoAlbumArtist:
            removed = albumArtistKey.c_str();
            break;
        case AudioTitleChange::NoDate:
            removed = "dc:date";
            break;
        case AudioTitleChange::OnlyTitle:
            metadata = { { "dc:title", "Track_\"音楽\"" } };
            aux.clear();
            break;
        case AudioTitleChange::LongTrack:
        case AudioTitleChange::ZeroNumbers:
            for (auto& [key, value] : metadata) {
                if (key == "upnp:originalTrackNumber")
                    value = change == AudioTitleChange::LongTrack ? "123" : "0";
            }
            if (change == AudioTitleChange::ZeroNumbers)
                aux["DISCNUMBER"] = "0";
            break;
        case AudioTitleChange::WindowsFilename:
            props["location"] = "C:\\music\\Title.mp3";
            break;
        case AudioTitleChange::None:
            break;
        }
        if (removed) {
            metadata.erase(std::remove_if(metadata.begin(), metadata.end(), [&](const auto& field) {
                return field.first == removed;
            }),
                metadata.end());
        }
        dukMockItem(ctx, props, metadata, aux, {});
        duk_put_global_string(ctx, "titleSource");
        duk_get_global_string(ctx, "getAudioDetails");
        duk_get_global_string(ctx, "titleSource");
        duk_push_string(ctx, "/music");
        if (duk_pcall(ctx, 2) != DUK_EXEC_SUCCESS) {
            ADD_FAILURE() << duk_safe_to_string(ctx, -1);
            duk_pop(ctx);
            return {};
        }
        TitleResult result;
        duk_get_prop_string(ctx, -1, "title");
        result.title = duk_safe_to_string(ctx, -1);
        duk_pop(ctx);
        duk_get_prop_string(ctx, -1, "date");
        result.date = duk_safe_to_string(ctx, -1);
        duk_pop(ctx);
        duk_get_prop_string(ctx, -1, "decade");
        result.decade = duk_safe_to_string(ctx, -1);
        duk_pop(ctx);
        duk_del_prop_string(ctx, -1, "title");
        result.details = duk_json_encode(ctx, -1);
        duk_pop(ctx);
        duk_get_global_string(ctx, "titleSource");
        result.source = duk_json_encode(ctx, -1);
        duk_pop(ctx);
        return result;
    }

    void* preparedRules()
    {
        duk_get_global_string(ctx, "config");
        duk_get_prop_string(ctx, -1, "audioTitleReplacements");
        EXPECT_TRUE(duk_is_array(ctx, -1));
        auto result = duk_get_heapptr(ctx, -1);
        duk_pop_2(ctx);
        return result;
    }
};

class AudioTitleFormatTest : public AudioTitleScriptTest,
                             public ::testing::WithParamInterface<AudioTitleCase> { };

TEST_P(AudioTitleFormatTest, UsesScriptOptionsWithoutChangingMetadataOrOrdering)
{
    const auto& test = GetParam();
    configure(nullptr);
    const auto baseline = invoke(test.change);
    configure(test.pattern, test.rules);
    const auto actual = invoke(test.change);
    EXPECT_EQ(actual.title, test.expected);
    EXPECT_EQ(actual.details, baseline.details);
    EXPECT_EQ(actual.source, baseline.source);
}

INSTANTIATE_TEST_SUITE_P(ConfiguredFormats, AudioTitleFormatTest,
    ::testing::ValuesIn(std::vector<AudioTitleCase> {
        { "LegacyDefault", nullptr, "Artist - Guest -   Track_\"音楽\" ", {}, AudioTitleChange::None },
        { "Title", "%title%", "Track_\"音楽\"", {}, AudioTitleChange::None },
        { "Artist", "%artist% - %title%", "Artist / Guest - Track_\"音楽\"", {}, AudioTitleChange::None },
        { "Metadata", "%albumartist% | %albumtitle% | %year% | %genre% | %composer%", "Various Artists | Album | 2026 | Techno / Trance | Composer", {}, AudioTitleChange::None },
        { "Numbers", "%cdnumber%.%tracknr2%/%tracknr1%/%tracknr3%", "2.02/2/002", {}, AudioTitleChange::None },
        { "Filename", "%filename%", "Artist - File", {}, AudioTitleChange::None },
        { "UnknownTokens", "%% %unknown% %TITLE%", "% %unknown% %TITLE%", {}, AudioTitleChange::None },
        { "MissingTitle", "%title%", "Artist - File", {}, AudioTitleChange::NoTitle },
        { "MissingArtist", "%albumartist%", "Various Artists", {}, AudioTitleChange::NoArtist },
        { "MissingAlbumArtist", "%albumartist%", "Artist", {}, AudioTitleChange::NoAlbumArtist },
        { "OnlyTitle", "%artist%%albumartist%%albumtitle%%genre%%composer%%year%%tracknr2%%cdnumber%", "Track_\"音楽\"", {}, AudioTitleChange::OnlyTitle },
        { "LongTrack", "%tracknr2%/%tracknr3%", "123/123", {}, AudioTitleChange::LongTrack },
        { "ZeroNumbers", "%tracknr2%%cdnumber%", "Track_\"音楽\"", {}, AudioTitleChange::ZeroNumbers },
        { "LiteralUnderscore", "%title%", "Track \"音楽\"", { { { "from", "_" }, { "to", " " } } }, AudioTitleChange::None },
        { "LiteralQuote", "%title%", "Track_音楽", { { { "from", "\"" }, { "to", "" } } }, AudioTitleChange::None },
        { "Sequential", "ab a", "Xb X", { { { "from", "a" }, { "to", "ab" } }, { { "from", "ab" }, { "to", "X" } } }, AudioTitleChange::None },
        { "OrderedRules", "a", "first", { { { "from", "a" }, { "to", "first" } }, { { "from", "a" }, { "to", "second" } } }, AudioTitleChange::None },
        { "LiteralUnicodeAndDollar", "A $1 [x]", "A literal 音楽", { { { "from", "a" }, { "to", "z" } }, { { "from", "$1" }, { "to", "literal" } }, { { "from", "[x]" }, { "to", "音楽" } } }, AudioTitleChange::None },
        { "EmptyResultFallback", "REMOVE", "Track_\"音楽\"", { { { "from", "REMOVE" }, { "to", "" } } }, AudioTitleChange::None },
        { "WindowsFilename", "%filename%", "Title", {}, AudioTitleChange::WindowsFilename },
        { "EmptyFrom", "%title%", "Track_\"音楽\"", { { { "from", "" }, { "to", "ignored" } } }, AudioTitleChange::None },
        { "RegexYes", "A  B\tC", "A B C", { { { "from", "\\s+" }, { "to", " " }, { "regex", "yes" } } }, AudioTitleChange::None },
        { "RegexTrue", "A  B", "A B", { { { "from", "\\s+" }, { "to", " " }, { "regex", "true" } } }, AudioTitleChange::None },
        { "LiteralRegexNo", "A.*B", "literal", { { { "from", "A.*B" }, { "to", "literal" }, { "regex", "no" } } }, AudioTitleChange::None },
        { "LiteralRegexFalse", "A.*B", "literal", { { { "from", "A.*B" }, { "to", "literal" }, { "regex", "false" } } }, AudioTitleChange::None },
        { "RegexCaptures", "A B A B", "B/A B/A", { { { "from", "(A) (B)" }, { "to", "$2/$1" }, { "regex", "yes" } } }, AudioTitleChange::None },
        { "LiteralCaptureSyntax", "A", "$1", { { { "from", "A" }, { "to", "$1" } } }, AudioTitleChange::None },
        { "AlbumMatches", "%title%", "Track \"音楽\"", { { { "from", "_" }, { "to", " " }, { "if-albumtitle", "Album" } } }, AudioTitleChange::None },
        { "AlbumDiffers", "%title%", "Track_\"音楽\"", { { { "from", "_" }, { "to", " " }, { "if-albumtitle", "Other" } } }, AudioTitleChange::None },
        { "AnyArtistMatches", "%title%", "Track \"音楽\"", { { { "from", "_" }, { "to", " " }, { "if-artist", "Guest" } } }, AudioTitleChange::None },
        { "ArtistCaseDiffers", "%title%", "Track_\"音楽\"", { { { "from", "_" }, { "to", " " }, { "if-artist", "guest" } } }, AudioTitleChange::None },
        { "BothConditionsMatch", "%title%", "Track \"音楽\"", { { { "from", "_" }, { "to", " " }, { "if-artist", "Artist" }, { "if-albumtitle", "Album" } } }, AudioTitleChange::None },
        { "OneConditionDiffers", "%title%", "Track_\"音楽\"", { { { "from", "_" }, { "to", " " }, { "if-artist", "Artist" }, { "if-albumtitle", "Other" } } }, AudioTitleChange::None },
        { "MissingArtistCondition", "%title%", "Track_\"音楽\"", { { { "from", "_" }, { "to", " " }, { "if-artist", "Artist" } } }, AudioTitleChange::NoArtist },
        { "MissingAlbumArtistCondition", "%title%", "Track_\"音楽\"", { { { "from", "_" }, { "to", " " }, { "if-albumartist", "Artist" } } }, AudioTitleChange::NoAlbumArtist },
        { "AllMetadataConditions", "%title%", "Track \"音楽\"", { { { "from", "_" }, { "to", " " }, { "if-albumartist", "Various Artists" }, { "if-genre", "Trance" }, { "if-composer", "Composer" }, { "if-year", "2026" } } }, AudioTitleChange::None },
        { "YearDiffers", "%title%", "Track_\"音楽\"", { { { "from", "_" }, { "to", " " }, { "if-year", "2025" } } }, AudioTitleChange::None },
        { "MissingYear", "%title%", "Track_\"音楽\"", { { { "from", "_" }, { "to", " " }, { "if-year", "Unknown" } } }, AudioTitleChange::NoDate },
        { "RawTitleCondition", "%title%", "Track \"音楽\"", { { { "from", "_" }, { "to", " " }, { "if-title", " Track_\"音楽\" " } } }, AudioTitleChange::None },
        { "FallbackNotMetadata", "%title%", "Artist - File", { { { "from", "Artist" }, { "to", "Changed" }, { "if-title", "Artist - File" } } }, AudioTitleChange::NoTitle },
        { "EmptyCondition", "%title%", "Track \"音楽\"", { { { "from", "_" }, { "to", " " }, { "if-artist", "" } } }, AudioTitleChange::None },
        { "RegexConditionMatches", "A  B", "A B", { { { "from", "\\s+" }, { "to", " " }, { "regex", "yes" }, { "if-artist", "Guest" } } }, AudioTitleChange::None },
        { "RegexConditionDiffers", "A  B", "A  B", { { { "from", "\\s+" }, { "to", " " }, { "regex", "yes" }, { "if-artist", "Other" } } }, AudioTitleChange::None },
        { "UntrimmedTitleCondition", "%title%", "Track_\"音楽\"", { { { "from", "Track" }, { "to", "Changed" }, { "if-title", "Track_\"音楽\"" } } }, AudioTitleChange::None },
        { "RegexZeroCaptureLiteral", "A", "$0", { { { "from", "A" }, { "to", "$0" }, { "regex", "yes" } } }, AudioTitleChange::None },
        { "RegexDigitFallback", "A", "A0", { { { "from", "(A)" }, { "to", "$10" }, { "regex", "yes" } } }, AudioTitleChange::None },
        { "RegexMissingCaptureLiteral", "A", "$9", { { { "from", "(A)" }, { "to", "$9" }, { "regex", "yes" } } }, AudioTitleChange::None },
        { "UnicodeRegexCharacter", "Äther", "ther", { { { "from", "^." }, { "to", "" }, { "regex", "yes" } } }, AudioTitleChange::None } }),
    [](const ::testing::TestParamInfo<AudioTitleCase>& info) { return info.param.name; });

TEST_F(AudioTitleScriptTest, MissingDatesDoNotInheritPreviousTrackDecade)
{
    configure("%title%");
    const auto onlyTitle = invoke(AudioTitleChange::OnlyTitle);
    EXPECT_EQ(onlyTitle.date, "Unknown");
    EXPECT_EQ(onlyTitle.decade, "Unknown");

    const auto dated = invoke();
    EXPECT_EQ(dated.date, "2026");
    EXPECT_EQ(dated.decade, "2020 - 2029");

    const auto noDate = invoke(AudioTitleChange::NoDate);
    EXPECT_EQ(noDate.date, "Unknown");
    EXPECT_EQ(noDate.decade, "Unknown");

    const auto onlyTitleAgain = invoke(AudioTitleChange::OnlyTitle);
    EXPECT_EQ(onlyTitleAgain.date, onlyTitle.date);
    EXPECT_EQ(onlyTitleAgain.decade, onlyTitle.decade);
    EXPECT_EQ(onlyTitleAgain.details, onlyTitle.details);
    EXPECT_EQ(onlyTitleAgain.source, onlyTitle.source);

    const auto noDateAgain = invoke(AudioTitleChange::NoDate);
    EXPECT_EQ(noDateAgain.details, noDate.details);
    EXPECT_EQ(noDateAgain.source, noDate.source);
}

TEST_F(AudioTitleScriptTest, InvalidRegexIsLoggedOnceAndDoesNotHideLaterRules)
{
    configure("%title%", { { { "from", "(" }, { "to", "bad" }, { "regex", "yes" } }, { { "from", "_" }, { "to", " " } } });
    auto* first = preparedRules();
    ASSERT_NE(first, nullptr);
    const auto warning = warnings.str();
    EXPECT_NE(warning.find("Ignoring invalid audio title replacement:"), std::string::npos);
    EXPECT_EQ(std::count(warning.begin(), warning.end(), '\n'), 1);
    duk_push_heapptr(ctx, first);
    EXPECT_EQ(duk_get_length(ctx, -1), 1U);
    duk_pop(ctx);
    EXPECT_EQ(preparedRules(), first);
    EXPECT_EQ(warnings.str(), warning);
    EXPECT_EQ(invoke().title, "Track \"音楽\"");
    EXPECT_EQ(warnings.str(), warning);
}

TEST_F(AudioTitleScriptTest, CachedRegexResetsStateAndNewConfigurationReplacesRules)
{
    configure("A  B", { { { "from", "\\s+" }, { "to", " " }, { "regex", "yes" } } });
    auto* first = preparedRules();
    ASSERT_NE(first, nullptr);
    for (const auto* name : { "audioTitleReplacementSource", "audioTitleReplacementRules" }) {
        EXPECT_FALSE(duk_get_global_string(ctx, name));
        duk_pop(ctx);
    }
    duk_push_heapptr(ctx, first);
    duk_put_global_string(ctx, "savedTitleRules");
    for (int repeat = 0; repeat < 3; ++repeat) {
        EXPECT_EQ(invoke().title, "A B");
        EXPECT_EQ(preparedRules(), first);
    }
    configure("A  B", { { { "from", "A" }, { "to", "C" } } });
    EXPECT_NE(preparedRules(), first);
    EXPECT_EQ(invoke().title, "C  B");
}

#endif
