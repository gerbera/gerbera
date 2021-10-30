#ifdef HAVE_JS

#include <duktape.h>
#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include <memory>

#include "cds_objects.h"
#include "util/string_converter.h"

#include "mock/common_script_mock.h"
#include "mock/duk_helper.h"
#include "mock/script_test_fixture.h"

// Extends ScriptTestFixture to allow
// for unique testing of the External URL Playlist
// processing
class ExternalUrlPLSPlaylistTest : public ScriptTestFixture {
public:
    // As Duktape requires static methods, so must the mock expectations be
    static std::unique_ptr<CommonScriptMock> commonScriptMock;

    // Used to iterate through `readln` content
    static int readLineCnt;

    ExternalUrlPLSPlaylistTest()
    {
        commonScriptMock = std::make_unique<::testing::NiceMock<CommonScriptMock>>();
        scriptName = "playlists.js";
    }

    ~ExternalUrlPLSPlaylistTest() override
    {
        commonScriptMock.reset();
    }
};

std::unique_ptr<CommonScriptMock> ExternalUrlPLSPlaylistTest::commonScriptMock;
int ExternalUrlPLSPlaylistTest::readLineCnt = 0;

static duk_ret_t getPlaylistType(duk_context* ctx)
{
    std::string playlistMimeType = ScriptTestFixture::getPlaylistType(ctx);
    return ExternalUrlPLSPlaylistTest::commonScriptMock->getPlaylistType(playlistMimeType);
}

static duk_ret_t print(duk_context* ctx)
{
    std::string msg = ScriptTestFixture::print(ctx);
    return ExternalUrlPLSPlaylistTest::commonScriptMock->print(msg);
}

static duk_ret_t addContainerTree(duk_context* ctx)
{
    std::map<std::string, std::string> map {
        { "", "0" },
        { "/Playlists/All Playlists/Playlist Title", "42" },
        { "/Playlists/Directories/of/Playlist Title", "43" },
    };
    std::vector<std::string> tree = ScriptTestFixture::addContainerTree(ctx, map);
    return ExternalUrlPLSPlaylistTest::commonScriptMock->addContainerTree(tree);
}

static duk_ret_t createContainerChain(duk_context* ctx)
{
    std::vector<std::string> array = ScriptTestFixture::createContainerChain(ctx);
    return ExternalUrlPLSPlaylistTest::commonScriptMock->createContainerChain(array);
}

static duk_ret_t getLastPath(duk_context* ctx)
{
    std::string inputPath = ScriptTestFixture::getLastPath(ctx);
    return ExternalUrlPLSPlaylistTest::commonScriptMock->getLastPath(inputPath);
}

// Proxy the Duktape script with `readln`
// Mimics reading the playlist file line by line
// Uses the `CommonScriptMock` to track expectations
static duk_ret_t readln(duk_context* ctx)
{
    std::vector<std::string> lines = {
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

    std::string line = lines.at(ExternalUrlPLSPlaylistTest::readLineCnt);

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
    std::vector<std::string> keys = { "mimetype", "objectType", "location",
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
    { "addContainerTree", addContainerTree, 1 },
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

TEST_F(ExternalUrlPLSPlaylistTest, CreatesDukContextWithPlaylistScript)
{
    EXPECT_NE(ctx, nullptr);
}

TEST_F(ExternalUrlPLSPlaylistTest, PrintsWarningWhenPlaylistTypeIsNotFound)
{
    // Expecting the common script calls..and will proxy through the mock objects
    EXPECT_CALL(*commonScriptMock, getPlaylistType(Eq("no/type"))).WillOnce(Return(1));
    EXPECT_CALL(*commonScriptMock, print(Eq("Processing playlist: /location/of/playlist.pls"))).WillOnce(Return(1));
    EXPECT_CALL(*commonScriptMock, addContainerTree(ElementsAre("Playlists", "All Playlists", "Playlist Title"))).WillOnce(Return(1));
    EXPECT_CALL(*commonScriptMock, getLastPath(Eq("/location/of/playlist.pls"))).WillOnce(Return(1));
    EXPECT_CALL(*commonScriptMock, addContainerTree(ElementsAre("Playlists", "Directories", "of", "Playlist Title"))).WillOnce(Return(1));
    EXPECT_CALL(*commonScriptMock, print(Eq("Unknown playlist mimetype: 'no/type' of playlist '/location/of/playlist.pls'"))).WillOnce(Return(1));

    addGlobalFunctions(ctx, js_global_functions);
    dukMockPlaylist(ctx, "Playlist Title", "/location/of/playlist.pls", "no/type");

    executeScript(ctx);
}

TEST_F(ExternalUrlPLSPlaylistTest, AddsCdsObjectFromM3UPlaylistWithExternalUrlPlaylistAndDirChains)
{
    const std::string mimeTypeAudioMpeg = "audio/mpeg";
    const std::string objtypeItemExternalUrl = "8";

    std::map<std::string, std::string> asPlaylistChain {
        { "title", "Song from Playlist Title" },
        { "location", "http://46.105.171.217:8024" },
        { "mimetype", mimeTypeAudioMpeg },
        { "objectType", objtypeItemExternalUrl },
        { "playlistOrder", "1" },
        { "protocol", "http-get" },
        { "description", "Song from Playlist Title" },
        { "upnpclass", UPNP_CLASS_MUSIC_TRACK },
    };

    // Expecting the common script calls..and will proxy through the mock objects for verification.
    EXPECT_CALL(*commonScriptMock, getPlaylistType(Eq("audio/x-scpls"))).WillOnce(Return(1));
    EXPECT_CALL(*commonScriptMock, print(Eq("Processing playlist: /location/of/playlist.pls"))).WillOnce(Return(1));
    EXPECT_CALL(*commonScriptMock, addContainerTree(ElementsAre("Playlists", "All Playlists", "Playlist Title"))).WillOnce(Return(1));
    EXPECT_CALL(*commonScriptMock, getLastPath(Eq("/location/of/playlist.pls"))).WillOnce(Return(1));
    EXPECT_CALL(*commonScriptMock, addContainerTree(ElementsAre("Playlists", "Directories", "of", "Playlist Title"))).WillOnce(Return(1));
    EXPECT_CALL(*commonScriptMock, readln(Eq("[playlist]"))).WillOnce(Return(1));
    EXPECT_CALL(*commonScriptMock, readln(Eq("\n"))).Times(2).WillRepeatedly(Return(1));
    EXPECT_CALL(*commonScriptMock, readln(Eq("File1=http://46.105.171.217:8024"))).WillOnce(Return(1));
    EXPECT_CALL(*commonScriptMock, readln(Eq("Title1=Song from Playlist Title"))).WillOnce(Return(1));
    EXPECT_CALL(*commonScriptMock, readln(Eq("Length1=-1"))).WillOnce(Return(1));
    EXPECT_CALL(*commonScriptMock, readln(Eq("NumberOfEntries=1"))).WillOnce(Return(1));
    EXPECT_CALL(*commonScriptMock, readln(Eq("Version=2"))).WillOnce(Return(1));
    EXPECT_CALL(*commonScriptMock, readln(Eq("-EOF-"))).WillOnce(Return(0));
    EXPECT_CALL(*commonScriptMock, addCdsObject(IsIdenticalMap(asPlaylistChain), "42", UNDEFINED)).WillOnce(Return(0));
    EXPECT_CALL(*commonScriptMock, addCdsObject(IsIdenticalMap(asPlaylistChain), "43", UNDEFINED)).WillOnce(Return(0));

    addGlobalFunctions(ctx, js_global_functions);
    dukMockPlaylist(ctx, "Playlist Title", "/location/of/playlist.pls", "audio/x-scpls");

    executeScript(ctx);
}
#endif
