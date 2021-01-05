#ifdef HAVE_JS

#include "cds_objects.h"
#include "config/config_manager.h"
#include "util/string_converter.h"
#include "gmock/gmock.h"
#include "gtest/gtest.h"
#include <duktape.h>
#include <memory>

#include "mock/common_script_mock.h"
#include "mock/duk_helper.h"
#include "mock/script_test_fixture.h"

using namespace ::testing;

class ExternalUrlPLSPlaylistTest : public ScriptTestFixture {
public:
    ExternalUrlPLSPlaylistTest()
    {
        commonScriptMock.reset(new ::testing::NiceMock<CommonScriptMock>());
        scriptName = "playlists.js";
    };

    virtual ~ExternalUrlPLSPlaylistTest()
    {
        commonScriptMock.reset();
    };

    // As Duktape requires static methods, so must the mock expectations be
    static unique_ptr<CommonScriptMock> commonScriptMock;

    // Used to iterate through `readln` content
    static int readLineCnt;
};

unique_ptr<CommonScriptMock> ExternalUrlPLSPlaylistTest::commonScriptMock;
int ExternalUrlPLSPlaylistTest::readLineCnt = 0;

static duk_ret_t getPlaylistType(duk_context* ctx)
{
    string playlistMimeType = ScriptTestFixture::getPlaylistType(ctx);
    return ExternalUrlPLSPlaylistTest::commonScriptMock->getPlaylistType(playlistMimeType);
}

static duk_ret_t print(duk_context* ctx)
{
    string msg = ScriptTestFixture::print(ctx);
    return ExternalUrlPLSPlaylistTest::commonScriptMock->print(msg);
}

static duk_ret_t createContainerChain(duk_context* ctx)
{
    vector<string> array = ScriptTestFixture::createContainerChain(ctx);
    return ExternalUrlPLSPlaylistTest::commonScriptMock->createContainerChain(array);
}

static duk_ret_t getLastPath(duk_context* ctx)
{
    string inputPath = ScriptTestFixture::getLastPath(ctx);
    return ExternalUrlPLSPlaylistTest::commonScriptMock->getLastPath(inputPath);
}

// Proxy the Duktape script with `readln`
// Mimics reading the playlist file line by line
// Uses the `CommonScriptMock` to track expectations
static duk_ret_t readln(duk_context* ctx)
{
    vector<string> lines = {
        "[playlist]",
        "\n",
        "File1=http://46.105.171.217:8024",
        "Title1=Song from Playlist Title",
        "Length1=-1",
        "\n",
        "NumberOfEntries=1",
        "Version=2",
        "-EOF-" // used to stop processing
    };

    string line = lines.at(ExternalUrlPLSPlaylistTest::readLineCnt);

    duk_push_string(ctx, line.c_str());
    ExternalUrlPLSPlaylistTest::readLineCnt++;
    return ExternalUrlPLSPlaylistTest::commonScriptMock->readln(line);
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
    return ExternalUrlPLSPlaylistTest::commonScriptMock->addCdsObject(params.objectValues, params.containerChain, params.objectType);
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

TEST_F(ExternalUrlPLSPlaylistTest, CreatesDukContextWithPlaylistScript)
{
    EXPECT_NE(ctx, nullptr);
}

TEST_F(ExternalUrlPLSPlaylistTest, PrintsWarningWhenPlaylistTypeIsNotFound)
{
    // Expecting the common script calls..and will proxy through the mock objects
    EXPECT_CALL(*commonScriptMock, getPlaylistType(Eq("no/type"))).WillOnce(Return(1));
    EXPECT_CALL(*commonScriptMock, print(Eq("Processing playlist: /location/of/playlist.pls"))).WillOnce(Return(1));
    EXPECT_CALL(*commonScriptMock, createContainerChain(ElementsAre("Playlists", "All Playlists", "Playlist Title"))).WillOnce(Return(1));
    EXPECT_CALL(*commonScriptMock, getLastPath(Eq("/location/of/playlist.pls"))).WillOnce(Return(1));
    EXPECT_CALL(*commonScriptMock, createContainerChain(ElementsAre("Playlists", "Directories", "of", "Playlist Title"))).WillOnce(Return(1));
    EXPECT_CALL(*commonScriptMock, print(Eq("Unknown playlist mimetype: 'no/type' of playlist '/location/of/playlist.pls'"))).WillOnce(Return(1));

    addGlobalFunctions(ctx, js_global_functions);
    dukMockPlaylist(ctx, "Playlist Title", "/location/of/playlist.pls", "no/type");

    executeScript(ctx);
}

TEST_F(ExternalUrlPLSPlaylistTest, AddsCdsObjectFromM3UPlaylistWithExternalUrlPlaylistAndDirChains)
{
    const string MIME_TYPE_AUDIO_MPEG = "audio/mpeg";
    const string OBJTYPE_ITEM_EXTERNAL_URL = "8";

    map<string, string> asPlaylistChain = {
        make_pair("title", "Song from Playlist Title"),
        make_pair("location", "http://46.105.171.217:8024"),
        make_pair("mimetype", MIME_TYPE_AUDIO_MPEG),
        make_pair("objectType", OBJTYPE_ITEM_EXTERNAL_URL),
        make_pair("playlistOrder", "1"),
        make_pair("protocol", "http-get"),
        make_pair("description", "Song from Playlist Title"),
        make_pair("upnpclass", UPNP_CLASS_MUSIC_TRACK)
    };

    // Expecting the common script calls..and will proxy through the mock objects for verification.
    EXPECT_CALL(*commonScriptMock, getPlaylistType(Eq("audio/x-scpls"))).WillOnce(Return(1));
    EXPECT_CALL(*commonScriptMock, print(Eq("Processing playlist: /location/of/playlist.pls"))).WillOnce(Return(1));
    EXPECT_CALL(*commonScriptMock, createContainerChain(ElementsAre("Playlists", "All Playlists", "Playlist Title"))).WillOnce(Return(1));
    EXPECT_CALL(*commonScriptMock, getLastPath(Eq("/location/of/playlist.pls"))).WillOnce(Return(1));
    EXPECT_CALL(*commonScriptMock, createContainerChain(ElementsAre("Playlists", "Directories", "of", "Playlist Title"))).WillOnce(Return(1));
    EXPECT_CALL(*commonScriptMock, readln(Eq("[playlist]"))).WillOnce(Return(1));
    EXPECT_CALL(*commonScriptMock, readln(Eq("\n"))).Times(2).WillRepeatedly(Return(1));
    EXPECT_CALL(*commonScriptMock, readln(Eq("File1=http://46.105.171.217:8024"))).WillOnce(Return(1));
    EXPECT_CALL(*commonScriptMock, readln(Eq("Title1=Song from Playlist Title"))).WillOnce(Return(1));
    EXPECT_CALL(*commonScriptMock, readln(Eq("Length1=-1"))).WillOnce(Return(1));
    EXPECT_CALL(*commonScriptMock, readln(Eq("NumberOfEntries=1"))).WillOnce(Return(1));
    EXPECT_CALL(*commonScriptMock, readln(Eq("Version=2"))).WillOnce(Return(1));
    EXPECT_CALL(*commonScriptMock, readln(Eq("-EOF-"))).WillOnce(Return(0));
    EXPECT_CALL(*commonScriptMock, addCdsObject(IsIdenticalMap(asPlaylistChain), "\\/Playlists\\/All Playlists\\/Playlist Title", "object.container.playlistContainer")).WillOnce(Return(0));
    EXPECT_CALL(*commonScriptMock, addCdsObject(IsIdenticalMap(asPlaylistChain), "\\/Playlists\\/Directories\\/of\\/Playlist Title", "object.container.playlistContainer")).WillOnce(Return(0));

    addGlobalFunctions(ctx, js_global_functions);
    dukMockPlaylist(ctx, "Playlist Title", "/location/of/playlist.pls", "audio/x-scpls");

    executeScript(ctx);
}
#endif
