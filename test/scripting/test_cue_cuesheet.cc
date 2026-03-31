/*GRB*

    Gerbera - https://gerbera.io/

    test_cue_cuesheet.cc - this file is part of Gerbera.

    Copyright (C) 2026 Gerbera Contributors

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

#include "cds/cds_objects.h"
#include "config/result/autoscan.h"
#include "exceptions.h"
#include "util/tools.h"

#include "mock/common_script_mock.h"
#include "mock/duk_helper.h"
#include "mock/script_test_fixture.h"

#include <duktape.h>
#include <map>
#include <memory>

// Extends ScriptTestFixture to allow
// for unique testing of the External Metafile
// processing
class CuesheetCueTest : public CommonScriptTestFixture {
public:
    CuesheetCueTest()
    {
        functionName = "importCuesheet";
    }
};

static duk_ret_t addContainerTree(duk_context* ctx)
{
    std::map<std::string, std::string> map {
        { "", "0" },
        { "/Gerbera Rocks", "42" },
    };
    std::vector<std::string> tree = ScriptTestFixture::addContainerTree(ctx, map);
    return CuesheetCueTest::commonScriptMock->addContainerTree(tree);
}

// Proxy the Duktape script with `readln`
// Mimics reading the playlist file line by line
// Uses the `CommonScriptMock` to track expectations
static duk_ret_t readln(duk_context* ctx)
{
    std::string line = CuesheetCueTest::lines.at(CuesheetCueTest::readLineCnt);

    duk_push_string(ctx, line.c_str());
    CuesheetCueTest::readLineCnt++;
    return CuesheetCueTest::commonScriptMock->readln(line);
}

static duk_ret_t getCdsObject(duk_context* ctx)
{
    auto name = duk_get_string(ctx, -1);
    std::map<std::string, std::string> obj {
        { "id", "710" },
        { "title", "example.mp3" },
        { "objectType", "2" },
        { "location", fmt::format("/home/gerbera/{}", name) },
    };
    std::map<std::string, std::string> meta {
        { "dc:title", "example.mp3" },
        { "upnp:artist", "Artist" },
        { "upnp:album", "Album" },
        { "dc:date", "2018" },
    };
    getCdsObjectParams params = ScriptTestFixture::getCdsObject(ctx, obj, meta);
    return CuesheetCueTest::commonScriptMock->getCdsObject(params.location);
}

duk_ret_t addCueObject(duk_context* ctx)
{
    const std::string objectName = "entry";
    std::map<std::string, std::string> result;

    if (duk_is_undefined(ctx, 0)) {
        std::cerr << "Could not retrieve stack " << objectName << " object" << '\n';
        return 0;
    }

    duk_to_object(ctx, 0);
    if (!duk_is_object(ctx, 1)) {
        std::cerr << "No entry argument" << std::endl;
        return 0;
    }
    duk_to_object(ctx, 1);

    auto containerID = stoiString(duk_get_string(ctx, 2));

    for (auto&& val : std::array { "performer", "discArtist", "discTitle", "location", "format", "type", "trackTitle", "offset" }) {
        duk_get_prop_string(ctx, 1, val);
        if (duk_is_null_or_undefined(ctx, -1) || !duk_to_string(ctx, -1)) {
            std::cerr << "Could not retrieve stack " << objectName << "." << val << '\n';
            duk_pop(ctx);
            result[val] = "";
            continue;
        }
        result[val] = duk_get_string(ctx, -1);
        duk_pop(ctx);
    }
    for (auto&& val : std::array { "track", "frames" }) {
        duk_get_prop_string(ctx, 1, val);
        if (duk_is_null_or_undefined(ctx, -1)) {
            std::cerr << "Could not retrieve stack " << objectName << "." << val << '\n';
            duk_pop(ctx);
            result[val] = "0";
            continue;
        }
        result[val] = fmt::to_string(duk_get_int(ctx, -1));
        duk_pop(ctx);
    }

    duk_push_string(ctx, fmt::format("{}", containerID * 1000 + CuesheetCueTest::readLineCnt).c_str());
    return CuesheetCueTest::commonScriptMock->addCueObject(result);
}

// Mock the Duktape C methods
// that are called from the playlists.js script
// * These are static methods, which makes mocking difficult.
static duk_function_list_entry js_global_functions[] = {
    { "print", CommonScriptTestFixture::js_print, DUK_VARARGS },
    { "print2", CommonScriptTestFixture::js_print2, DUK_VARARGS },
    { "readln", readln, 1 },
    { "addCueObject", addCueObject, 4 },
    { "addContainerTree", addContainerTree, 1 },
    { "getCdsObject", getCdsObject, 1 },
    { nullptr, nullptr, 0 },
};

template <typename Map>
bool mapCompare(Map const& lhs, Map const& rhs)
{
    return lhs.size() == rhs.size() && std::equal(lhs.begin(), lhs.end(), rhs.begin());
}

MATCHER_P(IsIdenticalMap, control, "Map to be identical")
{
    {
        return (mapCompare(arg, control));
    }
}

TEST_F(CuesheetCueTest, CreatesDukContextWithCuesheet)
{
    EXPECT_NE(ctx, nullptr);
}

TEST_F(CuesheetCueTest, SetsEntriesFromFile)
{
    const std::string file = "fixtures/example.cue";
    ScriptTestFixture::mockPlaylistFile("fixtures/example.cue");
    std::map<std::string, std::string> track1 {
        { "discArtist", "Team Gerbera" },
        { "discTitle", "Gerbera Rocks" },
        { "format", "WAVE" },
        { "frames", "0" },
        { "location", "fixtures/example.cue/Side A.wv" },
        { "offset", "00:00" },
        { "performer", "Team Gerbera" },
        { "track", "1" },
        { "trackTitle", "UPnP Media Server" },
        { "type", "AUDIO" }
    };
    std::map<std::string, std::string> track2 {
        { "discArtist", "Team Gerbera" },
        { "discTitle", "Gerbera Rocks" },
        { "format", "WAVE" },
        { "frames", "0" },
        { "location", "fixtures/example.cue/Side A.wv" },
        { "offset", "06:35" },
        { "performer", "Mediatomb" },
        { "track", "2" },
        { "trackTitle", "Based on Mediatomb" },
        { "type", "AUDIO" }
    };
    std::map<std::string, std::string> track3 {
        { "discArtist", "Team Gerbera" },
        { "discTitle", "Gerbera Rocks" },
        { "format", "WAVE" },
        { "frames", "0" },
        { "location", "fixtures/example.cue/Side A.wv" },
        { "offset", "11:03" },
        { "performer", "Team Gerbera" },
        { "track", "3" },
        { "trackTitle", "For Lots of Media Types" },
        { "type", "AUDIO" }
    };
    std::map<std::string, std::string> track4 {
        { "discArtist", "Team Gerbera" },
        { "discTitle", "Gerbera Rocks" },
        { "format", "WAVE" },
        { "frames", "0" },
        { "location", "fixtures/example.cue/Side B.wv" },
        { "offset", "00:00" },
        { "performer", "feat. Javascript" },
        { "track", "5" },
        { "trackTitle", "Flexible Layout" },
        { "type", "AUDIO" }
    };
    std::map<std::string, std::string> track5 {
        { "discArtist", "Team Gerbera" },
        { "discTitle", "Gerbera Rocks" },
        { "format", "WAVE" },
        { "frames", "1" },
        { "location", "fixtures/example.cue/Side B.wv" },
        { "offset", "04:31" },
        { "performer", "Team Gerbera" },
        { "track", "6" },
        { "trackTitle", "From Metadata" },
        { "type", "AUDIO" }
    };

    // Expecting the common script calls..and will proxy through the mock objects for verification.
    EXPECT_CALL(*commonScriptMock, readln(Eq("PERFORMER \"Team Gerbera\""))).WillOnce(Return(1));
    EXPECT_CALL(*commonScriptMock, readln(Eq("TITLE \"Gerbera Rocks\""))).WillOnce(Return(1));
    EXPECT_CALL(*commonScriptMock, readln(Eq("FILE \"Side A.wv\" WAVE"))).WillOnce(Return(1));
    EXPECT_CALL(*commonScriptMock, readln(Eq("  TRACK 01 AUDIO"))).WillOnce(Return(1));
    EXPECT_CALL(*commonScriptMock, readln(Eq("    TITLE \"UPnP Media Server\""))).WillOnce(Return(1));
    EXPECT_CALL(*commonScriptMock, readln(Eq("    INDEX 01 00:00:00"))).WillRepeatedly(Return(1));
    EXPECT_CALL(*commonScriptMock, readln(Eq("  TRACK 02 AUDIO"))).WillOnce(Return(1));
    EXPECT_CALL(*commonScriptMock, readln(Eq("    TITLE \"Based on Mediatomb\""))).WillOnce(Return(1));
    EXPECT_CALL(*commonScriptMock, readln(Eq("    PERFORMER \"Mediatomb\""))).WillOnce(Return(1));
    EXPECT_CALL(*commonScriptMock, readln(Eq("    INDEX 01 06:35:00"))).WillOnce(Return(1));
    EXPECT_CALL(*commonScriptMock, readln(Eq("  TRACK 03 AUDIO"))).WillOnce(Return(1));
    EXPECT_CALL(*commonScriptMock, readln(Eq("    TITLE \"For Lots of Media Types\""))).WillOnce(Return(1));
    EXPECT_CALL(*commonScriptMock, readln(Eq("    INDEX 01 11:03:00"))).WillOnce(Return(1));
    EXPECT_CALL(*commonScriptMock, readln(Eq("FILE \"Side B.wv\" WAVE"))).WillOnce(Return(1));
    EXPECT_CALL(*commonScriptMock, readln(Eq("  TRACK 05 AUDIO"))).WillOnce(Return(1));
    EXPECT_CALL(*commonScriptMock, readln(Eq("    TITLE \"Flexible Layout\""))).WillOnce(Return(1));
    EXPECT_CALL(*commonScriptMock, readln(Eq("    PERFORMER \"feat. Javascript\""))).WillOnce(Return(1));
    EXPECT_CALL(*commonScriptMock, readln(Eq("  TRACK 06 AUDIO"))).WillOnce(Return(1));
    EXPECT_CALL(*commonScriptMock, readln(Eq("    TITLE \"From Metadata\""))).WillOnce(Return(1));
    EXPECT_CALL(*commonScriptMock, readln(Eq("    INDEX 01 04:31:01"))).WillOnce(Return(1));
    EXPECT_CALL(*commonScriptMock, readln(Eq(""))).WillRepeatedly(Return(1));
    EXPECT_CALL(*commonScriptMock, readln(Eq("-EOF-"))).WillOnce(Return(0));

    EXPECT_CALL(*commonScriptMock, getCdsObject(Eq("fixtures/example.cue/Side A.wv"))).WillRepeatedly(Return(1));
    EXPECT_CALL(*commonScriptMock, getCdsObject(Eq("fixtures/example.cue/Side B.wv"))).WillRepeatedly(Return(1));
    EXPECT_CALL(*commonScriptMock, addContainerTree(ElementsAre("Gerbera Rocks"))).WillOnce(Return(1));
    EXPECT_CALL(*commonScriptMock, print2(Eq("Info"), Eq(fmt::format("Processing cuesheet: {} as cue", file)))).WillOnce(Return(1));
    EXPECT_CALL(*commonScriptMock, addCueObject(IsIdenticalMap(track1))).WillOnce(Return(1));
    EXPECT_CALL(*commonScriptMock, addCueObject(IsIdenticalMap(track2))).WillOnce(Return(1));
    EXPECT_CALL(*commonScriptMock, addCueObject(IsIdenticalMap(track3))).WillOnce(Return(1));
    EXPECT_CALL(*commonScriptMock, addCueObject(IsIdenticalMap(track4))).WillOnce(Return(1));
    EXPECT_CALL(*commonScriptMock, addCueObject(IsIdenticalMap(track5))).WillOnce(Return(1));

    addGlobalFunctions(ctx, js_global_functions);
    auto fnResult = callFunction(ctx, dukMockMetafile,
        { { "title", "example.cue" }, { "location", file } }, {}, {}, {},
        AutoscanDirectory::ContainerTypesDefaults.at(AutoscanMediaMode::Audio),
        file);
    std::vector<int> items { 42, 42007, 42011, 42015, 42020, 42025 };
    EXPECT_EQ(fnResult, items);
}
#endif
