#ifdef HAVE_JS

#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include <duktape.h>
#include <memory>

#include "cds_objects.h"
#include "config/config_manager.h"
#include "content/onlineservice/online_service.h"
#include "metadata/metadata_handler.h"
#include "util/string_converter.h"

#include "mock/common_script_mock.h"
#include "mock/duk_helper.h"
#include "mock/script_test_fixture.h"

using namespace ::testing;

class ImportScriptTest : public ScriptTestFixture {
public:
    ImportScriptTest()
    {
        commonScriptMock.reset(new ::testing::NiceMock<CommonScriptMock>());
        scriptName = "import.js";
    };

    virtual ~ImportScriptTest()
    {
        commonScriptMock.reset();
    };

    // As Duktape requires static methods, so must the mock expectations be
    static unique_ptr<CommonScriptMock> commonScriptMock;
};

unique_ptr<CommonScriptMock> ImportScriptTest::commonScriptMock;

static duk_ret_t print(duk_context* ctx)
{
    string msg = ScriptTestFixture::print(ctx);
    return ImportScriptTest::commonScriptMock->print(msg);
}

static duk_ret_t getPlaylistType(duk_context* ctx)
{
    string playlistMimeType = ScriptTestFixture::getPlaylistType(ctx);
    return ImportScriptTest::commonScriptMock->getPlaylistType(playlistMimeType);
}

static duk_ret_t addContainerTree(duk_context* ctx)
{
    map<string,string> map = {
        { "", "0" },
        { "/Audio/All Audio", "42" },
        { "/Audio/Artists/Artist/All Songs", "43" },
        { "/Audio/All - full name", "44" },
        { "/Audio/Artists/Artist/All - full name", "45" },
        { "/Audio/Artists/Artist/Album", "46" },
        { "/Audio/Albums/Album", "47" },
        { "/Audio/Genres/Genre", "48" },
        { "/Audio/Composers/Composer", "49" },
        { "/Audio/Year/2018", "50" },
        { "/Video/All Video", "60" },
        { "/Video/Directories/home/gerbera", "61" },
        { "/Photos/All Photos", "70" },
        { "/Photos/Year/2018/01", "71" },
        { "/Photos/Date/2018-01-01", "72" },
        { "/Photos/Directories/home/gerbera", "73" },
        { "/Online Services/Apple Trailers/All Trailers", "80" },
        { "/Online Services/Apple Trailers/Genres/Genre", "81" },
        { "/Online Services/Apple Trailers/Release Date/2018-01", "82" },
        { "/Online Services/Apple Trailers/Post Date/2018-01", "83" },
    };
    vector<string> tree = ScriptTestFixture::addContainerTree(ctx, map);
    return ImportScriptTest::commonScriptMock->addContainerTree(tree);
}

static duk_ret_t createContainerChain(duk_context* ctx)
{
    vector<string> array = ScriptTestFixture::createContainerChain(ctx);
    return ImportScriptTest::commonScriptMock->createContainerChain(array);
}

static duk_ret_t getLastPath(duk_context* ctx)
{
    string inputPath = ScriptTestFixture::getLastPath(ctx);
    return ImportScriptTest::commonScriptMock->getLastPath(inputPath);
}

static duk_ret_t addCdsObject(duk_context* ctx)
{
    vector<string> keys = {
        "title",
        "meta['dc:title']",
        "meta['upnp:artist']",
        "meta['upnp:album']",
        "meta['dc:date']",
        "meta['upnp:date']",
        "meta['upnp:genre']",
        "meta['dc:description']",
        "meta['upnp:composer']",
        "meta['upnp:conductor']",
        "meta['upnp:orchestra']"
    };
    addCdsObjectParams params = ScriptTestFixture::addCdsObject(ctx, keys);
    return ImportScriptTest::commonScriptMock->addCdsObject(params.objectValues, params.containerChain, params.objectType);
}

static duk_ret_t getYear(duk_context* ctx)
{
    string date = ScriptTestFixture::getYear(ctx);
    return ImportScriptTest::commonScriptMock->getYear(date);
}

static duk_ret_t getRootPath(duk_context* ctx)
{
    getRootPathParams params = ScriptTestFixture::getRootPath(ctx);
    return ImportScriptTest::commonScriptMock->getRootPath(params.objScriptPath, params.origObjLocation);
}

// Mock the Duktape C methods
// that are called from the import.js script
// * These are static methods, which makes mocking difficult.
static duk_function_list_entry js_global_functions[] = {
    { "print", print, DUK_VARARGS },
    { "getPlaylistType", getPlaylistType, 1 },
    { "createContainerChain", createContainerChain, 1 },
    { "getLastPath", getLastPath, 1 },
    { "addCdsObject", addCdsObject, 3 },
    { "getYear", getYear, 1 },
    { "getRootPath", getRootPath, 2 },
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

TEST_F(ImportScriptTest, CreatesDukContextWithImportScript)
{
    EXPECT_NE(ctx, nullptr);
}

TEST_F(ImportScriptTest, AddsAudioItemToVariousCdsContainerChains)
{
    string title = "Audio Title";
    string mimetype = "audio/mpeg";
    string artist = "Artist";
    string album = "Album";
    string composer = "Composer";
    string conductor = "Conductor";
    string orchestra = "Orchestra";
    string date = "2018-01-01";
    string year = "2018";
    string genre = "Genre";
    string desc = "Description";
    string id = "2";
    string location = "/home/gerbera/audio.mp3";
    string channels = "2";
    int online_service = 0;
    int theora = 0;
    map<string, string> aux;

    map<string, string> meta = {
        make_pair("dc:title", title),
        make_pair("upnp:artist", artist),
        make_pair("upnp:album", album),
        make_pair("dc:date", date),
        make_pair("upnp:date", year),
        make_pair("upnp:genre", genre),
        make_pair("dc:description", desc),
        make_pair("upnp:composer", composer),
        make_pair("upnp:conductor", conductor),
        make_pair("upnp:orchestra", orchestra)
    };

    map<string, string> res = {
        make_pair("res['nrAudioChannels']", channels)
    };

    // Represents the values passed to `addCdsObject`, extracted from keys defined there.
    map<string, string> asAudioAllAudio = {
        make_pair("title", title),
        make_pair("meta['dc:title']", title),
        make_pair("meta['upnp:artist']", artist),
        make_pair("meta['upnp:album']", album),
        make_pair("meta['dc:date']", date),
        make_pair("meta['upnp:date']", year),
        make_pair("meta['upnp:genre']", genre),
        make_pair("meta['dc:description']", desc),
        make_pair("meta['upnp:composer']", composer),
        make_pair("meta['upnp:conductor']", conductor),
        make_pair("meta['upnp:orchestra']", orchestra)
    };

    map<string, string> asAudioAllFullName;
    asAudioAllFullName.insert(asAudioAllAudio.begin(), asAudioAllAudio.end());
    asAudioAllFullName["title"] = "Artist - Album - Audio Title";

    // Expecting the common script calls
    // and will proxy through the mock objects
    // for verification.
    EXPECT_CALL(*commonScriptMock, getPlaylistType(Eq("audio/mpeg"))).WillOnce(Return(1));
    EXPECT_CALL(*commonScriptMock, getYear(Eq("2018-01-01"))).WillOnce(Return(1));

    EXPECT_CALL(*commonScriptMock, addContainerTree(ElementsAre("Audio", "All Audio"))).WillOnce(Return(1));
    EXPECT_CALL(*commonScriptMock, addCdsObject(IsIdenticalMap(asAudioAllAudio), "42", UNDEFINED)).WillOnce(Return(0));

    EXPECT_CALL(*commonScriptMock, addContainerTree(ElementsAre("Audio", "Artists", "Artist", "All Songs"))).WillOnce(Return(1));
    EXPECT_CALL(*commonScriptMock, addCdsObject(IsIdenticalMap(asAudioAllAudio), "43", UNDEFINED)).WillOnce(Return(0));

    EXPECT_CALL(*commonScriptMock, addContainerTree(ElementsAre("Audio", "All - full name"))).WillOnce(Return(1));
    EXPECT_CALL(*commonScriptMock, addCdsObject(IsIdenticalMap(asAudioAllFullName), "44", UNDEFINED)).WillOnce(Return(0));

    EXPECT_CALL(*commonScriptMock, addContainerTree(ElementsAre("Audio", "Artists", "Artist", "All - full name"))).WillOnce(Return(1));
    EXPECT_CALL(*commonScriptMock, addCdsObject(IsIdenticalMap(asAudioAllFullName), "45", UNDEFINED)).WillOnce(Return(0));

    EXPECT_CALL(*commonScriptMock, addContainerTree(ElementsAre("Audio", "Artists", "Artist", "Album"))).WillOnce(Return(1));
    EXPECT_CALL(*commonScriptMock, addCdsObject(IsIdenticalMap(asAudioAllAudio), "46", UNDEFINED)).WillOnce(Return(0));

    EXPECT_CALL(*commonScriptMock, addContainerTree(ElementsAre("Audio", "Albums", "Album"))).WillOnce(Return(1));
    EXPECT_CALL(*commonScriptMock, addCdsObject(IsIdenticalMap(asAudioAllAudio), "47", UNDEFINED)).WillOnce(Return(0));

    EXPECT_CALL(*commonScriptMock, addContainerTree(ElementsAre("Audio", "Genres", "Genre"))).WillOnce(Return(1));
    EXPECT_CALL(*commonScriptMock, addCdsObject(IsIdenticalMap(asAudioAllAudio), "48", UNDEFINED)).WillOnce(Return(0));

    EXPECT_CALL(*commonScriptMock, addContainerTree(ElementsAre("Audio", "Composers", "Composer"))).WillOnce(Return(1));
    EXPECT_CALL(*commonScriptMock, addCdsObject(IsIdenticalMap(asAudioAllAudio), "49", UNDEFINED)).WillOnce(Return(0));

    EXPECT_CALL(*commonScriptMock, addContainerTree(ElementsAre("Audio", "Year", "2018"))).WillOnce(Return(1));
    EXPECT_CALL(*commonScriptMock, addCdsObject(IsIdenticalMap(asAudioAllAudio), "50", UNDEFINED)).WillOnce(Return(0));

    addGlobalFunctions(ctx, js_global_functions);
    dukMockItem(ctx, mimetype, id, theora, title, meta, aux, res, location, online_service);
    executeScript(ctx);
}

TEST_F(ImportScriptTest, AddsVideoItemToCdsContainerChainWithDirs)
{
    string title = "Video Title";
    string mimetype = "video/mpeg";
    string id = "2";
    string location = "/home/gerbera/video.mp4";
    int online_service = (int)OS_None;
    int theora = 0;
    map<string, string> aux;
    map<string, string> meta;
    map<string, string> res;

    // Represents the values passed to `addCdsObject`, extracted from keys defined there.
    map<string, string> asVideoAllVideo = {
        make_pair("title", title)
    };

    // Expecting the common script calls
    // and will proxy through the mock objects
    // for verification.
    EXPECT_CALL(*commonScriptMock, getPlaylistType(Eq("video/mpeg"))).WillOnce(Return(1));

    EXPECT_CALL(*commonScriptMock, addContainerTree(ElementsAre("Video", "All Video"))).WillOnce(Return(1));
    EXPECT_CALL(*commonScriptMock, addCdsObject(IsIdenticalMap(asVideoAllVideo), "60", UNDEFINED)).WillOnce(Return(0));

    EXPECT_CALL(*commonScriptMock, getRootPath("object/script/path", location)).WillOnce(Return(1));

    EXPECT_CALL(*commonScriptMock, addContainerTree(ElementsAre("Video", "Directories", "home", "gerbera"))).WillOnce(Return(1));
    EXPECT_CALL(*commonScriptMock, addCdsObject(IsIdenticalMap(asVideoAllVideo), "61", UNDEFINED)).WillOnce(Return(0));

    addGlobalFunctions(ctx, js_global_functions);
    dukMockItem(ctx, mimetype, id, theora, title, meta, aux, res, location, online_service);
    executeScript(ctx);
}

TEST_F(ImportScriptTest, AddsAppleTrailerVideoItemToCdsContainerChains)
{
    string title = "Video Title";
    string mimetype = "video/mpeg";
    string date = "2018-01-01";
    string genre = "Genre";
    string id = "2";
    string location = "/home/gerbera/video.mp4";
    int online_service = (int)OS_ATrailers;
    int theora = 0;
    map<string, string> res;

    map<string, string> meta = {
        make_pair("dc:date", date),
        make_pair("upnp:genre", genre)
    };

    map<string, string> aux = {
        make_pair("T0", date)
    };

    // Represents the values passed to `addCdsObject`, extracted from keys defined there.
    map<string, string> asVideoAllVideo = {
        make_pair("title", title),
        make_pair("meta['dc:date']", date),
        make_pair("meta['upnp:genre']", genre)
    };

    // Expecting the common script calls
    // and will proxy through the mock objects
    // for verification.
    EXPECT_CALL(*commonScriptMock, getPlaylistType(Eq("video/mpeg"))).WillOnce(Return(1));

    EXPECT_CALL(*commonScriptMock, addContainerTree(ElementsAre("Online Services", "Apple Trailers", "All Trailers"))).WillOnce(Return(1));
    EXPECT_CALL(*commonScriptMock, addCdsObject(IsIdenticalMap(asVideoAllVideo), "80", UNDEFINED)).WillOnce(Return(0));

    EXPECT_CALL(*commonScriptMock, addContainerTree(ElementsAre("Online Services", "Apple Trailers", "Genres", "Genre"))).WillOnce(Return(1));
    EXPECT_CALL(*commonScriptMock, addCdsObject(IsIdenticalMap(asVideoAllVideo), "81", UNDEFINED)).WillOnce(Return(0));

    EXPECT_CALL(*commonScriptMock, addContainerTree(ElementsAre("Online Services", "Apple Trailers", "Release Date", "2018-01"))).WillOnce(Return(1));
    EXPECT_CALL(*commonScriptMock, addCdsObject(IsIdenticalMap(asVideoAllVideo), "82", UNDEFINED)).WillOnce(Return(0));

    EXPECT_CALL(*commonScriptMock, addContainerTree(ElementsAre("Online Services", "Apple Trailers", "Post Date", "2018-01"))).WillOnce(Return(1));
    EXPECT_CALL(*commonScriptMock, addCdsObject(IsIdenticalMap(asVideoAllVideo), "83", UNDEFINED)).WillOnce(Return(0));

    addGlobalFunctions(ctx, js_global_functions);
    dukMockItem(ctx, mimetype, id, theora, title, meta, aux, res, location, online_service);
    executeScript(ctx);
}

TEST_F(ImportScriptTest, AddsImageItemToCdsContainerChains)
{
    string title = "Image Title";
    string mimetype = "image/jpeg";
    string date = "2018-01-01";
    string id = "2";
    string location = "/home/gerbera/image.jpg";
    int online_service = (int)OS_ATrailers;
    int theora = 0;

    map<string, string> meta = {
        make_pair("dc:date", date)
    };

    map<string, string> aux;
    map<string, string> res;

    // Represents the values passed to `addCdsObject`, extracted from keys defined there.
    map<string, string> asImagePhotos = {
        make_pair("title", title),
        make_pair("meta['dc:date']", date)
    };

    // Expecting the common script calls
    // and will proxy through the mock objects
    // for verification.
    EXPECT_CALL(*commonScriptMock, getPlaylistType(Eq("image/jpeg"))).WillOnce(Return(1));

    EXPECT_CALL(*commonScriptMock, addContainerTree(ElementsAre("Photos", "All Photos"))).WillOnce(Return(1));
    EXPECT_CALL(*commonScriptMock, addCdsObject(IsIdenticalMap(asImagePhotos), "70", UNDEFINED)).WillOnce(Return(0));

    EXPECT_CALL(*commonScriptMock, addContainerTree(ElementsAre("Photos", "Year", "2018", "01"))).WillOnce(Return(1));
    EXPECT_CALL(*commonScriptMock, addCdsObject(IsIdenticalMap(asImagePhotos), "71", UNDEFINED)).WillOnce(Return(0));

    EXPECT_CALL(*commonScriptMock, addContainerTree(ElementsAre("Photos", "Date", "2018-01-01"))).WillOnce(Return(1));
    EXPECT_CALL(*commonScriptMock, addCdsObject(IsIdenticalMap(asImagePhotos), "72", UNDEFINED)).WillOnce(Return(0));

    EXPECT_CALL(*commonScriptMock, getRootPath("object/script/path", location)).WillOnce(Return(1));

    EXPECT_CALL(*commonScriptMock, addContainerTree(ElementsAre("Photos", "Directories", "home", "gerbera"))).WillOnce(Return(1));
    EXPECT_CALL(*commonScriptMock, addCdsObject(IsIdenticalMap(asImagePhotos), "73", UNDEFINED)).WillOnce(Return(0));

    addGlobalFunctions(ctx, js_global_functions);
    dukMockItem(ctx, mimetype, id, theora, title, meta, aux, res, location, online_service);
    executeScript(ctx);
}

TEST_F(ImportScriptTest, AddsOggTheoraVideoItemToCdsContainerChainWithDirs)
{
    string title = "Video Title";
    string mimetype = "application/ogg";
    string id = "2";
    string location = "/home/gerbera/video.ogg";
    int online_service = (int)OS_None;
    int theora = 1;
    map<string, string> aux;
    map<string, string> meta;
    map<string, string> res;

    // Represents the values passed to `addCdsObject`, extracted from keys defined there.
    map<string, string> asVideoAllVideo = {
        make_pair("title", title)
    };

    // Expecting the common script calls
    // and will proxy through the mock objects
    // for verification.
    EXPECT_CALL(*commonScriptMock, getPlaylistType(Eq("application/ogg"))).WillOnce(Return(1));

    EXPECT_CALL(*commonScriptMock, addContainerTree(ElementsAre("Video", "All Video"))).WillOnce(Return(1));
    EXPECT_CALL(*commonScriptMock, addCdsObject(IsIdenticalMap(asVideoAllVideo), "60", UNDEFINED)).WillOnce(Return(0));

    EXPECT_CALL(*commonScriptMock, getRootPath("object/script/path", location)).WillOnce(Return(1));

    EXPECT_CALL(*commonScriptMock, addContainerTree(ElementsAre("Video", "Directories", "home", "gerbera"))).WillOnce(Return(1));
    EXPECT_CALL(*commonScriptMock, addCdsObject(IsIdenticalMap(asVideoAllVideo), "61", UNDEFINED)).WillOnce(Return(0));

    addGlobalFunctions(ctx, js_global_functions);
    dukMockItem(ctx, mimetype, id, theora, title, meta, aux, res, location, online_service);
    executeScript(ctx);
}

TEST_F(ImportScriptTest, AddsOggTheoraAudioItemToVariousCdsContainerChains)
{
    string title = "Audio Title";
    string mimetype = "application/ogg";
    string artist = "Artist";
    string album = "Album";
    string date = "2018-01-01";
    string year = "2018";
    string genre = "Genre";
    string desc = "Description";
    string composer = "Composer";
    string conductor = "Conductor";
    string orchestra = "Orchestra";
    string id = "2";
    string location = "/home/gerbera/audio.mp3";
    string channels = "2";
    int online_service = 0;
    int theora = 0;
    map<string, string> aux;

    map<string, string> meta = {
        make_pair("dc:title", title),
        make_pair("upnp:artist", artist),
        make_pair("upnp:album", album),
        make_pair("dc:date", date),
        make_pair("upnp:date", year),
        make_pair("upnp:genre", genre),
        make_pair("dc:description", desc),
        make_pair("upnp:composer", composer),
        make_pair("upnp:conductor", conductor),
        make_pair("upnp:orchestra", orchestra)
    };

    map<string, string> res = {
        make_pair("res['nrAudioChannels']", channels)
    };

    // Represents the values passed to `addCdsObject`, extracted from keys defined there.
    map<string, string> asAudioAllAudio = {
        make_pair("title", title),
        make_pair("meta['dc:title']", title),
        make_pair("meta['upnp:artist']", artist),
        make_pair("meta['upnp:album']", album),
        make_pair("meta['dc:date']", date),
        make_pair("meta['upnp:date']", year),
        make_pair("meta['upnp:genre']", genre),
        make_pair("meta['dc:description']", desc),
        make_pair("meta['upnp:composer']", composer),
        make_pair("meta['upnp:conductor']", conductor),
        make_pair("meta['upnp:orchestra']", orchestra)
    };

    map<string, string> asAudioAllFullName;
    asAudioAllFullName.insert(asAudioAllAudio.begin(), asAudioAllAudio.end());
    asAudioAllFullName["title"] = "Artist - Album - Audio Title";

    // Expecting the common script calls
    // and will proxy through the mock objects
    // for verification.
    EXPECT_CALL(*commonScriptMock, getPlaylistType(Eq("application/ogg"))).WillOnce(Return(1));
    EXPECT_CALL(*commonScriptMock, getYear(Eq("2018-01-01"))).WillOnce(Return(1));

    EXPECT_CALL(*commonScriptMock, addContainerTree(ElementsAre("Audio", "All Audio"))).WillOnce(Return(1));
    EXPECT_CALL(*commonScriptMock, addCdsObject(IsIdenticalMap(asAudioAllAudio), "42", UNDEFINED)).WillOnce(Return(0));

    EXPECT_CALL(*commonScriptMock, addContainerTree(ElementsAre("Audio", "Artists", "Artist", "All Songs"))).WillOnce(Return(1));
    EXPECT_CALL(*commonScriptMock, addCdsObject(IsIdenticalMap(asAudioAllAudio), "43", UNDEFINED)).WillOnce(Return(0));

    EXPECT_CALL(*commonScriptMock, addContainerTree(ElementsAre("Audio", "All - full name"))).WillOnce(Return(1));
    EXPECT_CALL(*commonScriptMock, addCdsObject(IsIdenticalMap(asAudioAllFullName), "44", UNDEFINED)).WillOnce(Return(0));

    EXPECT_CALL(*commonScriptMock, addContainerTree(ElementsAre("Audio", "Artists", "Artist", "All - full name"))).WillOnce(Return(1));
    EXPECT_CALL(*commonScriptMock, addCdsObject(IsIdenticalMap(asAudioAllFullName), "45", UNDEFINED)).WillOnce(Return(0));

    EXPECT_CALL(*commonScriptMock, addContainerTree(ElementsAre("Audio", "Artists", "Artist", "Album"))).WillOnce(Return(1));
    EXPECT_CALL(*commonScriptMock, addCdsObject(IsIdenticalMap(asAudioAllAudio), "46", UNDEFINED)).WillOnce(Return(0));

    EXPECT_CALL(*commonScriptMock, addContainerTree(ElementsAre("Audio", "Albums", "Album"))).WillOnce(Return(1));
    EXPECT_CALL(*commonScriptMock, addCdsObject(IsIdenticalMap(asAudioAllAudio), "47", UNDEFINED)).WillOnce(Return(0));

    EXPECT_CALL(*commonScriptMock, addContainerTree(ElementsAre("Audio", "Genres", "Genre"))).WillOnce(Return(1));
    EXPECT_CALL(*commonScriptMock, addCdsObject(IsIdenticalMap(asAudioAllAudio), "48", UNDEFINED)).WillOnce(Return(0));

    EXPECT_CALL(*commonScriptMock, addContainerTree(ElementsAre("Audio", "Composers", "Composer"))).WillOnce(Return(1));
    EXPECT_CALL(*commonScriptMock, addCdsObject(IsIdenticalMap(asAudioAllAudio), "49", UNDEFINED)).WillOnce(Return(0));

    EXPECT_CALL(*commonScriptMock, addContainerTree(ElementsAre("Audio", "Year", "2018"))).WillOnce(Return(1));
    EXPECT_CALL(*commonScriptMock, addCdsObject(IsIdenticalMap(asAudioAllAudio), "50", UNDEFINED)).WillOnce(Return(0));

    addGlobalFunctions(ctx, js_global_functions);
    dukMockItem(ctx, mimetype, id, theora, title, meta, aux, res, location, online_service);
    executeScript(ctx);
}

#endif //HAVE_JS
