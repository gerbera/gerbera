#ifdef HAVE_JS
#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include <duktape.h>
#include <memory>

#include "cds_objects.h"
#include "config/config_manager.h"
#include "util/string_converter.h"

#include "mock/common_script_mock.h"
#include "mock/duk_helper.h"
#include "mock/script_test_fixture.h"

using namespace ::testing;

// Extends ScriptTestFixture to allow
// for unique testing of the External URL Playlist
// processing
class ExternalUrlM3UPlaylistTest : public ScriptTestFixture {
public:
    // As Duktape requires static methods, so must the mock expectations be
    static unique_ptr<CommonScriptMock> commonScriptMock;

    // Used to iterate through `readln` content
    static int readLineCnt;

    ExternalUrlM3UPlaylistTest()
    {
        commonScriptMock.reset(new ::testing::NiceMock<CommonScriptMock>());
        scriptName = "playlists.js";
    };

    virtual ~ExternalUrlM3UPlaylistTest()
    {
        commonScriptMock.reset();
    };
};

unique_ptr<CommonScriptMock> ExternalUrlM3UPlaylistTest::commonScriptMock;
int ExternalUrlM3UPlaylistTest::readLineCnt = 0;

static duk_ret_t getPlaylistType(duk_context* ctx)
{
    string playlistMimeType = ScriptTestFixture::getPlaylistType(ctx);
    return ExternalUrlM3UPlaylistTest::commonScriptMock->getPlaylistType(playlistMimeType);
}

static duk_ret_t print(duk_context* ctx)
{
    string msg = ScriptTestFixture::print(ctx);
    return ExternalUrlM3UPlaylistTest::commonScriptMock->print(msg);
}

static duk_ret_t addContainerTree(duk_context* ctx)
{
    map<string,string> map = {
        { "", "0" },
        { "/Playlists/All Playlists/Playlist Title", "42" },
        { "/Playlists/Directories/of/Playlist Title", "43" },
    };
    vector<string> tree = ScriptTestFixture::addContainerTree(ctx, map);
    return ExternalUrlM3UPlaylistTest::commonScriptMock->addContainerTree(tree);
}

static duk_ret_t createContainerChain(duk_context* ctx)
{
    vector<string> array = ScriptTestFixture::createContainerChain(ctx);
    return ExternalUrlM3UPlaylistTest::commonScriptMock->createContainerChain(array);
}

static duk_ret_t getLastPath(duk_context* ctx)
{
    string inputPath = ScriptTestFixture::getLastPath(ctx);
    return ExternalUrlM3UPlaylistTest::commonScriptMock->getLastPath(inputPath);
}

// Proxy the Duktape script with `readln`
// Mimics reading the playlist file line by line
// Uses the `CommonScriptMock` to track expectations
static duk_ret_t readln(duk_context* ctx)
{
    vector<string> lines = {
        "#EXTM3U",
        "#EXTINF:-1,(#1 - 177/750) : Ibiza Global Radio :",
        "http://46.105.171.217:8024",
        "-EOF-" // used to stop processing
    };

    string line = lines.at(ExternalUrlM3UPlaylistTest::readLineCnt);

    duk_push_string(ctx, line.c_str());
    ExternalUrlM3UPlaylistTest::readLineCnt++;
    return ExternalUrlM3UPlaylistTest::commonScriptMock->readln(line);
}

// Proxy the Duktape script with `addCdsObject`
// global function.
// Translates the Duktape value stack to c++
// and uses the `CommonScriptMock` to track
// expectations.
static duk_ret_t addCdsObject(duk_context* ctx)
{
    vector<string> keys = { "mimetype", "objectType", "location",
        "title", "protocol", "upnpclass",
        "description", "playlistOrder" };
    addCdsObjectParams params = ScriptTestFixture::addCdsObject(ctx, keys);
    return ExternalUrlM3UPlaylistTest::commonScriptMock->addCdsObject(params.objectValues, params.containerChain, params.objectType);
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
    { "addContainerTree", addContainerTree, 1 },
    { nullptr, nullptr, 0 },
};

template <typename Map>
bool map_compare(Map const& lhs, Map const& rhs)
{
    return lhs.size() == rhs.size() && std::equal(lhs.begin(), lhs.end(), rhs.begin());
}

MATCHER_P(IsIdenticalMap, control, "Map to be identical")
{
    {
        return (map_compare(arg, control));
    }
}

TEST_F(ExternalUrlM3UPlaylistTest, CreatesDukContextWithPlaylistScript)
{
    EXPECT_NE(ctx, nullptr);
}

TEST_F(ExternalUrlM3UPlaylistTest, PrintsWarningWhenPlaylistTypeIsNotFound)
{
    // Expecting the common script calls..and will proxy through the mock objects
    EXPECT_CALL(*commonScriptMock, getPlaylistType(Eq("no/type"))).WillOnce(Return(1));
    EXPECT_CALL(*commonScriptMock, print(Eq("Processing playlist: /location/of/playlist.m3u"))).WillOnce(Return(1));
    EXPECT_CALL(*commonScriptMock, addContainerTree(ElementsAre("Playlists", "All Playlists", "Playlist Title"))).WillOnce(Return(1));
    EXPECT_CALL(*commonScriptMock, getLastPath(Eq("/location/of/playlist.m3u"))).WillOnce(Return(1));
    EXPECT_CALL(*commonScriptMock, addContainerTree(ElementsAre("Playlists", "Directories", "of", "Playlist Title"))).WillOnce(Return(1));
    EXPECT_CALL(*commonScriptMock, print(Eq("Unknown playlist mimetype: 'no/type' of playlist '/location/of/playlist.m3u'"))).WillOnce(Return(1));

    addGlobalFunctions(ctx, js_global_functions);
    dukMockPlaylist(ctx, "Playlist Title", "/location/of/playlist.m3u", "no/type");

    executeScript(ctx);
}

TEST_F(ExternalUrlM3UPlaylistTest, AddsCdsObjectFromM3UPlaylistWithExternalUrlPlaylistAndDirChains)
{
    const string MIME_TYPE_AUDIO_MPEG = "audio/mpeg";
    const string OBJTYPE_ITEM_EXTERNAL_URL = "8";

    map<string, string> asPlaylistChain = {
        make_pair("title", "(#1 - 177/750) : Ibiza Global Radio :"),
        make_pair("location", "http://46.105.171.217:8024"),
        make_pair("mimetype", MIME_TYPE_AUDIO_MPEG),
        make_pair("objectType", OBJTYPE_ITEM_EXTERNAL_URL),
        make_pair("playlistOrder", "1"),
        make_pair("protocol", "http-get"),
        make_pair("description", "Song from Playlist Title"),
        make_pair("upnpclass", UPNP_CLASS_MUSIC_TRACK)
    };

    map<string, string> asPlaylistDirChain = {
        make_pair("title", "(#1 - 177/750) : Ibiza Global Radio :"),
        make_pair("location", "http://46.105.171.217:8024"),
        make_pair("mimetype", MIME_TYPE_AUDIO_MPEG),
        make_pair("objectType", OBJTYPE_ITEM_EXTERNAL_URL),
        make_pair("playlistOrder", "2"),
        make_pair("protocol", "http-get"),
        make_pair("description", "Song from Playlist Title"),
        make_pair("upnpclass", UPNP_CLASS_MUSIC_TRACK)
    };

    // Expecting the common script calls..and will proxy through the mock objects for verification.
    EXPECT_CALL(*commonScriptMock, getPlaylistType(Eq("audio/x-mpegurl"))).WillOnce(Return(1));
    EXPECT_CALL(*commonScriptMock, print(Eq("Processing playlist: /location/of/playlist.m3u"))).WillOnce(Return(1));
    EXPECT_CALL(*commonScriptMock, addContainerTree(ElementsAre("Playlists", "All Playlists", "Playlist Title"))).WillOnce(Return(1));
    EXPECT_CALL(*commonScriptMock, getLastPath(Eq("/location/of/playlist.m3u"))).WillOnce(Return(1));
    EXPECT_CALL(*commonScriptMock, addContainerTree(ElementsAre("Playlists", "Directories", "of", "Playlist Title"))).WillOnce(Return(1));
    EXPECT_CALL(*commonScriptMock, readln(Eq("#EXTM3U"))).WillOnce(Return(1));
    EXPECT_CALL(*commonScriptMock, readln(Eq("#EXTINF:-1,(#1 - 177/750) : Ibiza Global Radio :"))).WillOnce(Return(1));
    EXPECT_CALL(*commonScriptMock, readln(Eq("http://46.105.171.217:8024"))).WillOnce(Return(1));
    EXPECT_CALL(*commonScriptMock, readln(Eq("-EOF-"))).WillOnce(Return(0));
    EXPECT_CALL(*commonScriptMock, addCdsObject(IsIdenticalMap(asPlaylistChain), "42", UNDEFINED)).WillOnce(Return(0));
    EXPECT_CALL(*commonScriptMock, addCdsObject(IsIdenticalMap(asPlaylistDirChain), "43", UNDEFINED)).WillOnce(Return(0));

    addGlobalFunctions(ctx, js_global_functions);
    dukMockPlaylist(ctx, "Playlist Title", "/location/of/playlist.m3u", "audio/x-mpegurl");

    executeScript(ctx);
}
#endif
