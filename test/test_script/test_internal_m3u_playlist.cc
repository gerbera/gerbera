#ifdef HAVE_JS

#include "gmock/gmock.h"
#include "gtest/gtest.h"
#include <duktape.h>
#include <string_converter.h>
#include <cds_objects.h>
#include <memory>

#include "mock/common_script_mock.h"
#include "mock/duk_helper.h"
#include "mock/script_test_fixture.h"

using namespace ::testing;
using namespace std;

class InternalUrlM3UPlaylistTest : public ScriptTestFixture {
 public:
  InternalUrlM3UPlaylistTest() {
    commonScriptMock.reset(new ::testing::NiceMock<CommonScriptMock>());
    scriptName = "playlists.js";
  };

  virtual ~InternalUrlM3UPlaylistTest() {
    commonScriptMock.reset();
  };

  // As Duktape requires static methods, so must the mock expectations be
  static unique_ptr<CommonScriptMock> commonScriptMock;

  // Used to iterate through `readln` content
  static int readLineCnt;
};

unique_ptr<CommonScriptMock> InternalUrlM3UPlaylistTest::commonScriptMock;
int InternalUrlM3UPlaylistTest::readLineCnt = 0;

static duk_ret_t getPlaylistType(duk_context *ctx) {
  string playlistMimeType = ScriptTestFixture::getPlaylistType(ctx);
  return InternalUrlM3UPlaylistTest::commonScriptMock->getPlaylistType(playlistMimeType);
}

static duk_ret_t print(duk_context *ctx) {
  string msg = ScriptTestFixture::print(ctx);
  return InternalUrlM3UPlaylistTest::commonScriptMock->print(msg);
}

static duk_ret_t createContainerChain(duk_context *ctx) {
  vector<string> array = ScriptTestFixture::createContainerChain(ctx);
  return InternalUrlM3UPlaylistTest::commonScriptMock->createContainerChain(array);
}

static duk_ret_t getLastPath(duk_context* ctx) {
  string inputPath = ScriptTestFixture::getLastPath(ctx);
  return InternalUrlM3UPlaylistTest::commonScriptMock->getLastPath(inputPath);
}

// Proxy the Duktape script with `readln`
// Mimics reading the playlist file line by line
// Uses the `CommonScriptMock` to track expectations
static duk_ret_t readln(duk_context *ctx) {
  vector<string> lines = {
      "#EXTM3U",
      "#EXTINF:123, Example Artist, Example Title",
      "/home/gerbera/example.mp3",
      "-EOF-"  // used to stop processing :/
  };

  string line = lines.at(InternalUrlM3UPlaylistTest::readLineCnt);

  duk_push_string(ctx, line.c_str());
  InternalUrlM3UPlaylistTest::readLineCnt++;
  return InternalUrlM3UPlaylistTest::commonScriptMock->readln(line);
}

static duk_ret_t addCdsObject(duk_context *ctx) {
  vector<string> keys = {"objectType", "location", "title", "playlistOrder" };
  addCdsObjectParams params = ScriptTestFixture::addCdsObject(ctx, keys);
  return InternalUrlM3UPlaylistTest::commonScriptMock->addCdsObject(params.objectValues, params.containerChain, params.objectType);
}

static duk_ret_t copyObject(duk_context *ctx) {
  map<string, string> obj = {
    make_pair("title", "example.mp3"),
    make_pair("objectType", "2"),
    make_pair("location", "/home/gerbera/example.mp3"),
    make_pair("playlistOrder", "1")
  };
  map<string, string> meta = {
    make_pair("dc:title", "example.mp3"),
    make_pair("upnp:artist", "Artist"),
    make_pair("upnp:album", "Album"),
    make_pair("dc:date", "2018")
  };
  copyObjectParams params = ScriptTestFixture::copyObject(ctx, obj, meta);
  return InternalUrlM3UPlaylistTest::commonScriptMock->copyObject(params.isObject);
}

static duk_ret_t getCdsObject(duk_context *ctx) {
  map<string, string> obj = {
          make_pair("title", "example.mp3"),
          make_pair("objectType", "2"),
          make_pair("location", "/home/gerbera/example.mp3"),
          make_pair("playlistOrder", "1")
  };
  map<string, string> meta = {
          make_pair("dc:title", "example.mp3"),
          make_pair("upnp:artist", "Artist"),
          make_pair("upnp:album", "Album"),
          make_pair("dc:date", "2018")
  };
  getCdsObjectParams params = ScriptTestFixture::getCdsObject(ctx, obj, meta);
  return InternalUrlM3UPlaylistTest::commonScriptMock->getCdsObject(params.location);
}

// Mock the Duktape C methods
// that are called from the playlists.js script
// * These are static methods, which makes mocking difficult.
static  duk_function_list_entry js_global_functions[] =
{
  { "print",                 print,                DUK_VARARGS },
  { "getPlaylistType",       getPlaylistType,      1 },
  { "createContainerChain",  createContainerChain, 1 },
  { "getLastPath",           getLastPath,          1 },
  { "readln",                readln,               1 },
  { "addCdsObject",          addCdsObject,         3 },
  { "copyObject",            copyObject,           1 },
  { "getCdsObject",          getCdsObject,         1 },
  { nullptr,                 nullptr,              0 },
};

template <typename Map>
bool map_compare (Map const &lhs, Map const &rhs) {
  return lhs.size() == rhs.size() && std::equal(lhs.begin(), lhs.end(), rhs.begin());
}
MATCHER_P(IsIdenticalMap, control, "Map to be identical") {
  {
    return (map_compare(arg, control));
  }
}

TEST_F(InternalUrlM3UPlaylistTest, CreatesDukContextWithPlaylistScript) {
  EXPECT_NE(ctx, nullptr);
}

TEST_F(InternalUrlM3UPlaylistTest, AddsCdsObjectFromM3UPlaylistWithInternalUrlPlaylistAndDirChains) {
  map<string, string> asPlaylistChain = {
      make_pair("objectType", "2"),
      make_pair("location", "/home/gerbera/example.mp3"),
      make_pair("playlistOrder", "1"),
      make_pair("title", "example.mp3")
  };

  map<string, string> asPlaylistDirChain = {
      make_pair("objectType", "2"),
      make_pair("location", "/home/gerbera/example.mp3"),
      make_pair("playlistOrder", "2"),
      make_pair("title", "example.mp3")
  };

  // Expecting the common script calls
  // and will proxy through the mock objects
  // for verification.
  EXPECT_CALL(*commonScriptMock, getPlaylistType(Eq("audio/x-mpegurl"))).WillOnce(Return(1));
  EXPECT_CALL(*commonScriptMock, print(Eq("Processing playlist: /location/of/playlist.m3u"))).WillOnce(Return(1));
  EXPECT_CALL(*commonScriptMock, createContainerChain(ElementsAre("Playlists", "All Playlists", "Playlist Title"))).WillOnce(Return(1));
  EXPECT_CALL(*commonScriptMock, getLastPath(Eq("/location/of/playlist.m3u"))).WillOnce(Return(1));
  EXPECT_CALL(*commonScriptMock, createContainerChain(ElementsAre("Playlists", "Directories", "of", "Playlist Title"))).WillOnce(Return(1));
  EXPECT_CALL(*commonScriptMock, readln(Eq("#EXTM3U"))).WillOnce(Return(1));
  EXPECT_CALL(*commonScriptMock, readln(Eq("#EXTINF:123, Example Artist, Example Title"))).WillOnce(Return(1));
  EXPECT_CALL(*commonScriptMock, readln(Eq("/home/gerbera/example.mp3"))).WillOnce(Return(1));
  EXPECT_CALL(*commonScriptMock, readln(Eq("-EOF-"))).WillOnce(Return(0));
  EXPECT_CALL(*commonScriptMock, addCdsObject(IsIdenticalMap(asPlaylistChain),
      "\\/Playlists\\/All Playlists\\/Playlist Title",
      "object.container.playlistContainer")).WillOnce(Return(0));
  EXPECT_CALL(*commonScriptMock, addCdsObject(IsIdenticalMap(asPlaylistDirChain),
      "\\/Playlists\\/Directories\\/of\\/Playlist Title",
      "object.container.playlistContainer")).WillOnce(Return(0));
  EXPECT_CALL(*commonScriptMock, getCdsObject(Eq("/home/gerbera/example.mp3"))).WillRepeatedly(Return(1));
  EXPECT_CALL(*commonScriptMock, copyObject(Eq(true))).WillRepeatedly(Return(1));

  addGlobalFunctions(ctx, js_global_functions);
  dukMockPlaylist(ctx, "Playlist Title", "/location/of/playlist.m3u", "audio/x-mpegurl");

  executeScript(ctx);
}
#endif
