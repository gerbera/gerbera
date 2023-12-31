/*GRB*
    Gerbera - https://gerbera.io/

    test_internal_m3u_playlist.cc - this file is part of Gerbera.

    Copyright (C) 2016-2023 Gerbera Contributors

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

#include <duktape.h>
#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include <memory>

#include "cds/cds_objects.h"
#include "util/string_converter.h"

#include "mock/common_script_mock.h"
#include "mock/duk_helper.h"
#include "mock/script_test_fixture.h"

// Extends ScriptTestFixture to allow
// for unique testing of the Internal URL M3U Playlist
// processing
class InternalUrlM3UPlaylistTest : public ScriptTestFixture {
public:
    // As Duktape requires static methods, so must the mock expectations be
    static std::unique_ptr<CommonScriptMock> commonScriptMock;

    InternalUrlM3UPlaylistTest()
    {
        commonScriptMock = std::make_unique<::testing::NiceMock<CommonScriptMock>>();
        scriptName = "playlists.js";
        functionName = "importPlaylist";
        objectName = "playlist";
    }

    ~InternalUrlM3UPlaylistTest() override
    {
        commonScriptMock.reset();
    }
};

std::unique_ptr<CommonScriptMock> InternalUrlM3UPlaylistTest::commonScriptMock;

static duk_ret_t getPlaylistType(duk_context* ctx)
{
    std::string playlistMimeType = ScriptTestFixture::getPlaylistType(ctx);
    return InternalUrlM3UPlaylistTest::commonScriptMock->getPlaylistType(playlistMimeType);
}

static duk_ret_t print(duk_context* ctx)
{
    std::string msg = ScriptTestFixture::print(ctx);
    return InternalUrlM3UPlaylistTest::commonScriptMock->print(msg);
}

static duk_ret_t addContainerTree(duk_context* ctx)
{
    std::map<std::string, std::string> map {
        { "", "0" },
        { "/Playlists/All Playlists/Playlist Title", "42" },
        { "/Playlists/Directories/of/Playlist Title", "43" },
    };
    std::vector<std::string> tree = ScriptTestFixture::addContainerTree(ctx, map);
    return InternalUrlM3UPlaylistTest::commonScriptMock->addContainerTree(tree);
}

static duk_ret_t createContainerChain(duk_context* ctx)
{
    std::vector<std::string> array = ScriptTestFixture::createContainerChain(ctx);
    return InternalUrlM3UPlaylistTest::commonScriptMock->createContainerChain(array);
}

static duk_ret_t getLastPath(duk_context* ctx)
{
    std::string inputPath = ScriptTestFixture::getLastPath(ctx);
    return InternalUrlM3UPlaylistTest::commonScriptMock->getLastPath(inputPath);
}

// Proxy the Duktape script with `readln`
// Mimics reading the playlist file line by line
// Uses the `CommonScriptMock` to track expectations
static duk_ret_t readln(duk_context* ctx)
{
    std::string line = InternalUrlM3UPlaylistTest::lines.at(InternalUrlM3UPlaylistTest::readLineCnt);

    duk_push_string(ctx, line.c_str());
    InternalUrlM3UPlaylistTest::readLineCnt++;
    return InternalUrlM3UPlaylistTest::commonScriptMock->readln(line);
}

static duk_ret_t addCdsObject(duk_context* ctx)
{
    std::vector<std::string> keys { "objectType", "location", "title", "playlistOrder" };
    addCdsObjectParams params = ScriptTestFixture::addCdsObject(ctx, keys);
    return InternalUrlM3UPlaylistTest::commonScriptMock->addCdsObject(params.objectValues, params.containerChain, params.objectType);
}

static duk_ret_t copyObject(duk_context* ctx)
{
    std::map<std::string, std::string> obj {
        { "title", "example.mp3" },
        { "objectType", "2" },
        { "location", "/home/gerbera/example.mp3" },
        { "playlistOrder", "1" },
    };
    std::map<std::string, std::string> meta {
        { "dc:title", "example.mp3" },
        { "upnp:artist", "Artist" },
        { "upnp:album", "Album" },
        { "dc:date", "2018" },
    };
    copyObjectParams params = ScriptTestFixture::copyObject(ctx, obj, meta);
    return InternalUrlM3UPlaylistTest::commonScriptMock->copyObject(params.isObject);
}

static duk_ret_t getCdsObject(duk_context* ctx)
{
    std::map<std::string, std::string> obj {
        { "title", "example.mp3" },
        { "objectType", "2" },
        { "location", "/home/gerbera/example.mp3" },
        { "playlistOrder", "1" },
    };
    std::map<std::string, std::string> meta {
        { "dc:title", "example.mp3" },
        { "upnp:artist", "Artist" },
        { "upnp:album", "Album" },
        { "dc:date", "2018" },
    };
    getCdsObjectParams params = ScriptTestFixture::getCdsObject(ctx, obj, meta);
    return InternalUrlM3UPlaylistTest::commonScriptMock->getCdsObject(params.location);
}

// Mock the Duktape C methods
// that are called from the playlists.js script
// * These are static methods, which makes mocking difficult.
static duk_function_list_entry js_global_functions[] = {
    { "print", print, DUK_VARARGS },
    { "getPlaylistType", getPlaylistType, 1 },
    { "createContainerChain", createContainerChain, 1 },
    { "getLastPath", getLastPath, 1 },
    { "readln", readln, 1 },
    { "addCdsObject", addCdsObject, 3 },
    { "copyObject", copyObject, 1 },
    { "getCdsObject", getCdsObject, 1 },
    { "addContainerTree", addContainerTree, 1 },
    { nullptr, nullptr, 0 },
};

static const std::vector<boxConfig> playlistBox {
    { "Playlist/playlistRoot", "Playlists", "object.container" },
    { "Playlist/allPlaylists", "All Playlists", "object.container" },
    { "Playlist/allDirectories", "Directories", "object.container" },
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

TEST_F(InternalUrlM3UPlaylistTest, CreatesDukContextWithPlaylistScript)
{
    EXPECT_NE(ctx, nullptr);
}

TEST_F(InternalUrlM3UPlaylistTest, AddsCdsObjectFromPlaylistWithInternalUrlPlaylistAndDirChains)
{
    ScriptTestFixture::mockPlaylistFile("fixtures/example-internal.m3u");
    std::map<std::string, std::string> asPlaylistChain {
        { "objectType", "2" },
        { "location", "/home/gerbera/example.mp3" },
        { "playlistOrder", "1" },
        { "title", "example.mp3" },
    };

    std::map<std::string, std::string> asPlaylistDirChain {
        { "objectType", "2" },
        { "location", "/home/gerbera/example.mp3" },
        { "playlistOrder", "1" },
        { "title", "example.mp3" },
    };

    // Expecting the common script calls
    // and will proxy through the mock objects
    // for verification.
    EXPECT_CALL(*commonScriptMock, getPlaylistType(Eq("audio/x-mpegurl"))).WillOnce(Return(1));
    EXPECT_CALL(*commonScriptMock, print(Eq("Processing playlist: /location/of/playlist.m3u"))).WillOnce(Return(1));
    EXPECT_CALL(*commonScriptMock, addContainerTree(ElementsAre("Playlists", "All Playlists", "Playlist Title"))).WillOnce(Return(1));
    EXPECT_CALL(*commonScriptMock, getLastPath(Eq("/location/of/playlist.m3u"))).WillOnce(Return(1));
    EXPECT_CALL(*commonScriptMock, addContainerTree(ElementsAre("Playlists", "Directories", "of", "Playlist Title"))).WillOnce(Return(1));
    EXPECT_CALL(*commonScriptMock, readln(Eq("#EXTM3U"))).WillOnce(Return(1));
    EXPECT_CALL(*commonScriptMock, readln(Eq("#EXTINF:123, Example Artist, Example Title"))).WillOnce(Return(1));
    EXPECT_CALL(*commonScriptMock, readln(Eq("/home/gerbera/example.mp3"))).WillOnce(Return(1));
    EXPECT_CALL(*commonScriptMock, readln(Eq("-EOF-"))).WillOnce(Return(0));
    EXPECT_CALL(*commonScriptMock, addCdsObject(IsIdenticalMap(asPlaylistChain), "42", UNDEFINED)).WillOnce(Return(0));
    EXPECT_CALL(*commonScriptMock, addCdsObject(IsIdenticalMap(asPlaylistDirChain), "43", UNDEFINED)).WillOnce(Return(0));
    EXPECT_CALL(*commonScriptMock, getCdsObject(Eq("/home/gerbera/example.mp3"))).WillRepeatedly(Return(1));
    EXPECT_CALL(*commonScriptMock, copyObject(Eq(true))).WillRepeatedly(Return(1));

    addGlobalFunctions(ctx, js_global_functions, {}, playlistBox);
    callFunction(ctx, dukMockPlaylist, {{"title", "Playlist Title"}, {"location", "/location/of/playlist.m3u"}, {"mimetype", "audio/x-mpegurl"}});
}
#endif
