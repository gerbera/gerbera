/*GRB*

    Gerbera - https://gerbera.io/

    test_external_asx_playlist.cc - this file is part of Gerbera.

    Copyright (C) 2016-2026 Gerbera Contributors

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
#include "content/scripting/playlist_parser_script.h"
#include "exceptions.h"
#include "upnp/upnp_common.h"
#include "util/tools.h"

#include "mock/common_script_mock.h"
#include "mock/duk_helper.h"
#include "mock/script_test_fixture.h"

#include <duktape.h>
#include <memory>
#include <pugixml.hpp>

// Extends ScriptTestFixture to allow
// for unique testing of the External URL Playlist
// processing
class ExternalUrlAsxPlaylistTest : public CommonScriptTestFixture {
public:
    ExternalUrlAsxPlaylistTest()
    {
        functionName = "importPlaylist";
    }
};

static duk_ret_t addContainerTree(duk_context* ctx)
{
    std::map<std::string, std::string> map {
        { "", "0" },
        { "/Playlists/All Playlists/Playlist Title", "42" },
        { "/Playlists/Directories/of/Playlist Title", "43" },
    };
    std::vector<std::string> tree = ScriptTestFixture::addContainerTree(ctx, map);
    return ExternalUrlAsxPlaylistTest::commonScriptMock->addContainerTree(tree);
}

static pugi::xml_document xmlDoc;
static AsxPlaylistPlan asxPlan;
static std::size_t asxPosition;

// Feed the native structural plan to the actual JavaScript importer.
static duk_ret_t readAsxPlaylistEntry(duk_context* ctx)
{
    if (asxPosition >= asxPlan.entries.size())
        return 0;
    const auto& entry = asxPlan.entries[asxPosition++];
    PlaylistParserScript::pushAsxPlaylistEntry(ctx, entry);
    return 1;
}

// Proxy the Duktape script with `addCdsObject`
// global function.
// Translates the Duktape value stack to c++
// and uses the `CommonScriptMock` to track
// expectations.
static duk_ret_t addCdsObject(duk_context* ctx)
{
    std::vector<std::string> keys = { "mimetype", "objectType", "location",
        "title", "protocol", "upnpclass",
        "description", "playlistOrder" };
    addCdsObjectParams params = ScriptTestFixture::addCdsObject(ctx, keys);
    return ExternalUrlAsxPlaylistTest::commonScriptMock->addCdsObject(params.objectValues, params.containerChain, params.rootPath);
}

// External references in this script test already use absolute HTTP URLs.
static duk_ret_t resolvePlaylistReference(duk_context* ctx)
{
    duk_dup(ctx, 0);
    return 1;
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
    { "readAsxPlaylistEntry", readAsxPlaylistEntry, 0 },
    { "resolvePlaylistReference", resolvePlaylistReference, 2 },
    { "addCdsObject", addCdsObject, 3 },
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

TEST_F(ExternalUrlAsxPlaylistTest, CreatesDukContextWithPlaylistScript)
{
    EXPECT_NE(ctx, nullptr);
}

TEST_F(ExternalUrlAsxPlaylistTest, AddsVideoFromPlaylistWithExternalUrlPlaylistAndDirChains)
{
    pugi::xml_parse_result result = xmlDoc.load_file("fixtures/example.asx");
    if (result.status != pugi::xml_parse_status::status_ok) {
        throw ConfigParseException(result.description());
    }
    asxPlan = readAsxPlaylistPlan(xmlDoc.document_element(), "/location/of/playlist.asx", {});
    asxPosition = 0;
    const std::string mimeType = "video/mp4";
    const std::string objtypeItemExternalUrl = "8";

    std::map<std::string, std::string> asPlaylistChain {
        { "title", "ASX Playlist Entry" },
        { "location", "https://10.0.0.10/pl/stream/12110/1641070200" },
        { "mimetype", mimeType },
        { "objectType", objtypeItemExternalUrl },
        { "playlistOrder", "1" },
        { "protocol", "http-get" },
        { "description", "Entry from Playlist Title" },
        { "upnpclass", UPNP_CLASS_VIDEO_ITEM },
    };

    // Expecting the common script calls..and will proxy through the mock objects for verification.
    EXPECT_CALL(*commonScriptMock, getPlaylistType(Eq(MIME_TYPE_ASX_PLAYLIST))).WillOnce(Return(1));
    EXPECT_CALL(*commonScriptMock, print2(Eq("Info"), Eq("Processing playlist: /location/of/playlist.asx"))).WillOnce(Return(1));
    EXPECT_CALL(*commonScriptMock, addContainerTree(ElementsAre("Playlists", "All Playlists", "Playlist Title"))).WillOnce(Return(1));
    EXPECT_CALL(*commonScriptMock, getLastPath2(Eq("/location/of/playlist.asx"), Eq(1))).WillOnce(Return(1));
    EXPECT_CALL(*commonScriptMock, addContainerTree(ElementsAre("Playlists", "Directories", "of", "Playlist Title"))).WillOnce(Return(1));
    EXPECT_CALL(*commonScriptMock, addCdsObject(IsIdenticalMap(asPlaylistChain), "42", "/home/gerbera")).WillOnce(Return(1));
    EXPECT_CALL(*commonScriptMock, addCdsObject(IsIdenticalMap(asPlaylistChain), "43", "/home/gerbera")).WillOnce(Return(1));

    addGlobalFunctions(ctx, js_global_functions, {}, playlistBox);
    auto fnResult = callFunction(ctx, dukMockPlaylist,
        { { "title", "Playlist Title" },
            { "location", "/location/of/playlist.asx" },
            { "mimetype", MIME_TYPE_ASX_PLAYLIST } },
        {}, {}, {},
        AutoscanDirectory::ContainerTypesDefaults.at(AutoscanMediaMode::Audio),
        "/home/gerbera");
    std::vector<int> items { 42000, 43000 };
    EXPECT_EQ(fnResult, items);
}

TEST_F(ExternalUrlAsxPlaylistTest, TriesAlternativesUntilOnePositionCanBeAdded)
{
    ASSERT_TRUE(xmlDoc.load_string("<ASX><ENTRY><REF HREF='https://example.invalid/a.mp3'/><REF HREF='https://example.invalid/b.mp3'/></ENTRY><ENTRY><REF HREF='https://example.invalid/c.mp3'/></ENTRY></ASX>"));
    asxPlan = readAsxPlaylistPlan(xmlDoc.document_element(), "/location/of/playlist.asx", {});
    asxPosition = 0;
    addGlobalFunctions(ctx, js_global_functions, {}, playlistBox);
    std::vector<std::pair<std::string, std::string>> inserted;
    EXPECT_CALL(*commonScriptMock, addCdsObject(_, _, _)).WillRepeatedly(Invoke([&](auto item, auto chain, auto) {
        inserted.emplace_back(item.at("location"), chain);
        EXPECT_EQ(item.at("playlistOrder"), item.at("location").find("/c.mp3") != std::string::npos ? "2" : "1");
        return item.at("location").find("/a.mp3") != std::string::npos ? 0 : 1;
    }));
    ASSERT_EQ(duk_peval_string(ctx, "readAsxPlaylist('Playlist Title', '/location/of/', '42', '43', '/home/gerbera').length"), DUK_EXEC_SUCCESS);
    EXPECT_EQ(duk_get_int(ctx, -1), 4);
    EXPECT_EQ(inserted, (std::vector<std::pair<std::string, std::string>> { { "https://example.invalid/a.mp3", "42" }, { "https://example.invalid/b.mp3", "42" }, { "https://example.invalid/b.mp3", "43" }, { "https://example.invalid/c.mp3", "42" }, { "https://example.invalid/c.mp3", "43" } }));
}

TEST_F(ExternalUrlAsxPlaylistTest, PreservesEmbeddedMetadataAndWritesThroughOnlyInFirstView)
{
    ASSERT_TRUE(xmlDoc.load_string("<ASX><AUTHOR>Root author</AUTHOR><COPYRIGHT>Root rights</COPYRIGHT><ENTRY><TITLE>Override</TITLE><REF HREF='/media/a.wav' WRITETHROUGH='1'/></ENTRY></ASX>"));
    asxPlan = readAsxPlaylistPlan(xmlDoc.document_element(), "/location/of/playlist.asx", {});
    asxPosition = 0;
    addGlobalFunctions(ctx, js_global_functions, {}, playlistBox);
    const char* script = R"JS(
        var asxSeen = [];
        var asxWrites = 0;
        var asxSource = { title: 'Embedded title', metaData: { 'dc:title': ['Embedded title'], 'upnp:author': ['Embedded author'] } };
        getCdsObject = function() {
          return JSON.parse(JSON.stringify(asxSource));
        };
        copyObject = function(obj) { return JSON.parse(JSON.stringify(obj)); };
        addCdsObject = function(item) {
          if (item.writeThrough > 0) {
            asxWrites++;
            asxSource.metaData['dc:title'] = [item.title];
          }
          asxSeen.push(item);
          return asxSeen.length;
        };
        readAsxPlaylist('Playlist Title', '/location/of/', '42', '43', '/home/gerbera');
        if (asxWrites !== 1) throw new Error('writeThrough repeated across views');
        JSON.stringify(asxSeen.map(function(item) {
          return [item.title, item.metaData['upnp:author'][0], item.metaData['dc:rights'][0], item.writeThrough];
        }));
    )JS";
    ASSERT_EQ(duk_peval_string(ctx, script), DUK_EXEC_SUCCESS);
    EXPECT_STREQ(duk_get_string(ctx, -1), "[[\"Override\",\"Embedded author\",\"Root rights\",1],[\"Override\",\"Embedded author\",\"Root rights\",0]]");
}
#endif
