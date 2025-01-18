/*GRB*

    Gerbera - https://gerbera.io/

    test_external_asx_playlist.cc - this file is part of Gerbera.

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
        { "/Playlists/Directories/of/Playlist Title", "43" },
    };
    std::vector<std::string> tree = ScriptTestFixture::addContainerTree(ctx, map);
    return ExternalUrlAsxPlaylistTest::commonScriptMock->addContainerTree(tree);
}

static pugi::xml_node nullNode;
static pugi::xml_document xmlDoc;
static pugi::xml_node root;

// Proxy the Duktape script with `readXml`
// Mimics reading the playlist file line by line
// Uses the `CommonScriptMock` to track expectations
static duk_ret_t readXml(duk_context* ctx)
{
    int direction = duk_to_int(ctx, 0);
    duk_pop(ctx);

    pugi::xml_node node;

    if (direction > 0 && root.first_child()) {
        root = root.first_child();
        node = root;
    } else if (direction < -1 && root.root()) {
        root = xmlDoc.document_element();
        node = root;
    } else if (direction < 0 && root.parent()) {
        root = root.parent();
        node = root;
    } else if (root.next_sibling()) {
        root = root.next_sibling();
        node = root;
    } else 
        node = nullNode;

    std::ostringstream buf;
    buf << "<" << toLower(node.name()) << " ";
    duk_push_object(ctx);
    for (auto&& attrib : node.attributes()) {
        duk_push_string(ctx, attrib.value());
        duk_put_prop_string(ctx, -2, toLower(attrib.name()).c_str());
        buf << toLower(attrib.name()) << "=" << attrib.value() << " ";
    }
    buf << ">" << node.text().as_string();
    buf << "</" << toLower(node.name()) << ">";

    duk_push_string(ctx, node.text().as_string());
    duk_put_prop_string(ctx, -2, "VALUE");
    duk_push_string(ctx, toLower(node.name()).c_str());
    duk_put_prop_string(ctx, -2, "NAME");

    return ExternalUrlAsxPlaylistTest::commonScriptMock->readXml(buf.str());
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
    return ExternalUrlAsxPlaylistTest::commonScriptMock->addCdsObject(params.objectValues, params.containerChain, params.objectType);
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
    { "readXml", readXml, 1 },
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
    root = xmlDoc.document_element();
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
    EXPECT_CALL(*commonScriptMock, readXml(Eq("<asx version=3.0 ></asx>"))).Times(2).WillRepeatedly(Return(1));
    EXPECT_CALL(*commonScriptMock, readXml(Eq("<title >ASX Playlist</title>"))).WillOnce(Return(1));
    EXPECT_CALL(*commonScriptMock, readXml(Eq("<entry ></entry>"))).Times(2).WillRepeatedly(Return(1));
    EXPECT_CALL(*commonScriptMock, readXml(Eq("<title >ASX Playlist Entry</title>"))).WillOnce(Return(1));
    EXPECT_CALL(*commonScriptMock, readXml(Eq("<author >Gerbera Team</author>"))).WillOnce(Return(1));
    EXPECT_CALL(*commonScriptMock, readXml(Eq("<copyright >GPL</copyright>"))).WillOnce(Return(1));
    EXPECT_CALL(*commonScriptMock, readXml(Eq("<ref href=https://10.0.0.10/pl/stream/12110/1641070200 ></ref>"))).WillOnce(Return(1));
    EXPECT_CALL(*commonScriptMock, readXml(Eq("<param name=mimetype value=video/mp4 ></param>"))).WillOnce(Return(1));
    EXPECT_CALL(*commonScriptMock, readXml(Eq("< ></>"))).Times(3).WillRepeatedly(Return(0));
    EXPECT_CALL(*commonScriptMock, addCdsObject(IsIdenticalMap(asPlaylistChain), "42", UNDEFINED)).WillOnce(Return(0));
    EXPECT_CALL(*commonScriptMock, addCdsObject(IsIdenticalMap(asPlaylistChain), "43", UNDEFINED)).WillOnce(Return(0));

    addGlobalFunctions(ctx, js_global_functions, {}, playlistBox);
    callFunction(ctx, dukMockPlaylist, {{"title", "Playlist Title"}, {"location", "/location/of/playlist.asx"}, {"mimetype", MIME_TYPE_ASX_PLAYLIST}});
}
#endif
