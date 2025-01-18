/*GRB*

    Gerbera - https://gerbera.io/

    test_internal_pls_playlist.cc - this file is part of Gerbera.

    Copyright (C) 2016-2025 Gerbera Contributors

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

#include "mock/common_script_mock.h"
#include "mock/duk_helper.h"
#include "mock/script_test_fixture.h"

#include <duktape.h>
#include <memory>

// Extends ScriptTestFixture to allow
// for unique testing of the Internal URL Playlist
// processing
class InternalUrlPLSPlaylistTest : public CommonScriptTestFixture {
public:
    InternalUrlPLSPlaylistTest()
    {
        scriptName = "playlists.js";
        functionName = "importPlaylist";
        objectName = "playlist";
    }
};

static duk_ret_t addContainerTree(duk_context* ctx)
{
    std::map<std::string, std::string> map {
        { "", "0" },
        { "/Playlists/All Playlists/Playlist Title", "42" },
        { "/Playlists/Directories/of/my/Playlist Title", "43" },
    };
    std::vector<std::string> tree = ScriptTestFixture::addContainerTree(ctx, map);
    return InternalUrlPLSPlaylistTest::commonScriptMock->addContainerTree(tree);
}

// Proxy the Duktape script with `readln`
// Mimics reading the playlist file line by line
// Uses the `CommonScriptMock` to track expectations
static duk_ret_t readln(duk_context* ctx)
{
    std::string line = InternalUrlPLSPlaylistTest::lines.at(InternalUrlPLSPlaylistTest::readLineCnt);

    duk_push_string(ctx, line.c_str());
    InternalUrlPLSPlaylistTest::readLineCnt++;
    return InternalUrlPLSPlaylistTest::commonScriptMock->readln(line);
}

// Proxy the Duktape script with `addCdsObject`
// global function.
// Translates the Duktape value stack to c++
// and uses the `CommonScriptMock` to track
// expectations.
static duk_ret_t addCdsObject(duk_context* ctx)
{
    std::vector<std::string> keys { "objectType", "location", "title", "playlistOrder" };
    addCdsObjectParams params = ScriptTestFixture::addCdsObject(ctx, keys);
    return InternalUrlPLSPlaylistTest::commonScriptMock->addCdsObject(params.objectValues, params.containerChain, params.objectType);
}

static duk_ret_t copyObject(duk_context* ctx)
{
    std::map<std::string, std::string> obj {
        { "title", "Song from Playlist Title" },
        { "objectType", "2" },
        { "location", "/home/gerbera/example.mp3" },
        { "playlistOrder", "1" },
    };
    std::map<std::string, std::string> meta {
        { "dc:title", "Song from Playlist Title" },
        { "upnp:artist", "Artist" },
        { "upnp:album", "Album" },
        { "dc:date", "2018" },
    };
    copyObjectParams params = ScriptTestFixture::copyObject(ctx, obj, meta);
    return InternalUrlPLSPlaylistTest::commonScriptMock->copyObject(params.isObject);
}

static duk_ret_t getCdsObject(duk_context* ctx)
{
    std::map<std::string, std::string> obj {
        { "title", "Song from Playlist Title" },
        { "objectType", "2" },
        { "location", "/home/gerbera/example.mp3" },
        { "playlistOrder", "1" },
    };
    std::map<std::string, std::string> meta {
        { "dc:title", "Song from Playlist Title" },
        { "upnp:artist", "Artist" },
        { "upnp:album", "Album" },
        { "dc:date", "2018" },
    };
    getCdsObjectParams params = ScriptTestFixture::getCdsObject(ctx, obj, meta);
    return InternalUrlPLSPlaylistTest::commonScriptMock->getCdsObject(params.location);
}

// Mock the Duktape C methods
// that are called from the playlists.js script
// * These are static methods, which makes mocking difficult.
static duk_function_list_entry js_global_functions[] = {
    { "print", CommonScriptTestFixture::js_print, DUK_VARARGS },
    { "print2", CommonScriptTestFixture::js_print2, DUK_VARARGS },
    { "getPlaylistType", CommonScriptTestFixture::js_getPlaylistType, 1 },
    { "createContainerChain", CommonScriptTestFixture::js_createContainerChain, 1 },
    { "getLastPath", CommonScriptTestFixture::js_getLastPath, 1 },
    { "getLastPath2", CommonScriptTestFixture::js_getLastPath2, 2 },
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
    { "Playlist/allDirectories", "Directories", "object.container", true, 2 },
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

TEST_F(InternalUrlPLSPlaylistTest, CreatesDukContextWithPlaylistScript)
{
    EXPECT_NE(ctx, nullptr);
}

TEST_F(InternalUrlPLSPlaylistTest, AddsCdsObjectFromPlaylistWithInternalUrlPlaylistAndDirChains)
{
    ScriptTestFixture::mockPlaylistFile("fixtures/example-internal.pls");
    std::map<std::string, std::string> asPlaylistChain {
        { "objectType", "2" },
        { "location", "/home/gerbera/example.mp3" },
        { "playlistOrder", "1" },
        { "title", "Song from Playlist Title" },
    };

    // Expecting the common script calls
    // and will proxy through the mock objects
    // for verification.
    EXPECT_CALL(*commonScriptMock, getPlaylistType(Eq("audio/x-scpls"))).WillOnce(Return(1));
    EXPECT_CALL(*commonScriptMock, print2(Eq("Info"), Eq("Processing playlist: /location/of/my/playlist.pls"))).WillOnce(Return(1));
    EXPECT_CALL(*commonScriptMock, print2(Eq("Debug"), Eq("Playlist 'Song from Playlist Title' Adding entry: 1 /home/gerbera/example.mp3"))).WillRepeatedly(Return(1));
    EXPECT_CALL(*commonScriptMock, addContainerTree(ElementsAre("Playlists", "All Playlists", "Playlist Title"))).WillOnce(Return(1));
    EXPECT_CALL(*commonScriptMock, getLastPath2(Eq("/location/of/my/playlist.pls"), Eq(2))).WillOnce(Return(1));
    EXPECT_CALL(*commonScriptMock, addContainerTree(ElementsAre("Playlists", "Directories", "of", "my", "Playlist Title"))).WillOnce(Return(1));
    EXPECT_CALL(*commonScriptMock, readln(Eq("[playlist]"))).WillOnce(Return(1));
    EXPECT_CALL(*commonScriptMock, readln(Eq(""))).Times(2).WillRepeatedly(Return(1));
    EXPECT_CALL(*commonScriptMock, readln(Eq("File1=/home/gerbera/example.mp3"))).WillOnce(Return(1));
    EXPECT_CALL(*commonScriptMock, readln(Eq("Title1=Song from Playlist Title"))).WillOnce(Return(1));
    EXPECT_CALL(*commonScriptMock, readln(Eq("Length1=120"))).WillOnce(Return(1));
    EXPECT_CALL(*commonScriptMock, readln(Eq("NumberOfEntries=1"))).WillOnce(Return(1));
    EXPECT_CALL(*commonScriptMock, readln(Eq("Version=2"))).WillOnce(Return(1));
    EXPECT_CALL(*commonScriptMock, readln(Eq("-EOF-"))).WillOnce(Return(0));
    EXPECT_CALL(*commonScriptMock, addCdsObject(IsIdenticalMap(asPlaylistChain), "42", UNDEFINED)).WillOnce(Return(0));
    EXPECT_CALL(*commonScriptMock, addCdsObject(IsIdenticalMap(asPlaylistChain), "43", UNDEFINED)).WillOnce(Return(0));
    EXPECT_CALL(*commonScriptMock, getCdsObject(Eq("/home/gerbera/example.mp3"))).WillRepeatedly(Return(1));
    EXPECT_CALL(*commonScriptMock, copyObject(Eq(true))).WillRepeatedly(Return(1));

    addGlobalFunctions(ctx, js_global_functions, {}, playlistBox);
    callFunction(ctx, dukMockPlaylist, {{"title", "Playlist Title"}, {"location", "/location/of/my/playlist.pls"}, {"mimetype", "audio/x-scpls"}});
}
#endif
