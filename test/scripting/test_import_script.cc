/*GRB*

    Gerbera - https://gerbera.io/

    test_import_script.cc - this file is part of Gerbera.

    Copyright (C) 2020-2025 Gerbera Contributors

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

#include "content/onlineservice/online_service.h"
#include "metadata/metadata_handler.h"
#include "upnp/upnp_common.h"

#include "mock/common_script_mock.h"
#include "mock/duk_helper.h"
#include "mock/script_test_fixture.h"

#include <duktape.h>
#include <fmt/format.h>
#include <memory>

class ImportScriptTest : public CommonScriptTestFixture {
public:
    ImportScriptTest()
    {
        scriptName = "import.js";
    }
};

static duk_ret_t addContainerTree(duk_context* ctx)
{
    std::map<std::string, std::string> map {
        { "", "0" },
        { "/Audio/All Audio", "42" },
        { "/Audio/Artists/Artist/All Songs", "43" },
        { "/Audio/All - full name", "44" },
        { "/Audio/Artists/Artist/All - full name", "45" },
        { "/Audio/Artists/Artist/Album", "46" },
        { "/Audio/Artists/Artist/Album Chronology/2018 - Album", "462" },
        { "/Audio/Albums/Album", "47" },
        { "/Audio/Genres/Genre", "481" },
        { "/Audio/Genres/Genre2", "482" },
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
    std::vector<std::string> tree = ScriptTestFixture::addContainerTree(ctx, map);
    return ImportScriptTest::commonScriptMock->addContainerTree(tree);
}

static duk_ret_t addCdsObject(duk_context* ctx)
{
    std::vector<std::string> keys {
        "title",
        "metaData['dc:title']",
        "metaData['upnp:artist']",
        "metaData['upnp:album']",
        "metaData['upnp:originalTrackNumber']",
        "metaData['dc:date']",
        "metaData['upnp:date']",
        "metaData['upnp:genre']",
        "metaData['dc:description']",
        "metaData['upnp:composer']",
        "metaData['upnp:conductor']",
        "metaData['upnp:orchestra']",
    };
    addCdsObjectParams params = ScriptTestFixture::addCdsObject(ctx, keys);
    return ImportScriptTest::commonScriptMock->addCdsObject(params.objectValues, params.containerChain, params.objectType);
}

static duk_ret_t getYear(duk_context* ctx)
{
    std::string date = ScriptTestFixture::getYear(ctx);
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
    { "print", CommonScriptTestFixture::js_print, DUK_VARARGS },
    { "print2", CommonScriptTestFixture::js_print2, DUK_VARARGS },
    { "getPlaylistType", CommonScriptTestFixture::js_getPlaylistType, 1 },
    { "createContainerChain", CommonScriptTestFixture::js_createContainerChain, 1 },
    { "getLastPath", CommonScriptTestFixture::js_getLastPath, 1 },
    { "addCdsObject", addCdsObject, 3 },
    { "getYear", getYear, 1 },
    { "getRootPath", getRootPath, 2 },
    { "addContainerTree", addContainerTree, 1 },
    { nullptr, nullptr, 0 },
};

static const std::vector<boxConfig> audioBox {
    { "Audio/allAlbums", "Albums", "object.container" },
    { "Audio/allArtists", "Artists", "object.container" },
    { "Audio/allAudio", "All Audio", "object.container" },
    { "Audio/allComposers", "Composers", "object.container" },
    { "Audio/allDirectories", "Directories", "object.container" },
    { "Audio/allGenres", "Genres", "object.container" },
    { "Audio/allSongs", "All Songs", "object.container" },
    { "Audio/allTracks", "All - full name", "object.container" },
    { "Audio/allYears", "Year", "object.container" },
    { "Audio/audioRoot", "Audio", "object.container" },
    { "Audio/artistChronology", "Album Chronology", "object.container" },
};
static const std::vector<boxConfig> videoBox {
    { "Video/allDates", "Date", "object.container" },
    { "Video/allDirectories", "Directories", "object.container" },
    { "Video/allVideo", "All Video", "object.container" },
    { "Video/allYears", "Year", "object.container" },
    { "Video/unknown", "Unknown", "object.container" },
    { "Video/videoRoot", "Video", "object.container" },
    { "Video/allDates", "Date", "object.container" },
};
static const std::vector<boxConfig> imageBox {
    { "Image/allDates", "Date", "object.container" },
    { "Image/allDirectories", "Directories", "object.container" },
    { "Image/allImages", "All Photos", "object.container" },
    { "Image/allYears", "Year", "object.container" },
    { "Image/imageRoot", "Photos", "object.container" },
    { "Image/unknown", "Unknown", "object.container" },
};
static const std::vector<boxConfig> trailerBox {
    { "Trailer/trailerRoot", "Online Services", "object.container" },
    { "Trailer/appleTrailers", "Apple Trailers", "object.container" },
    { "Trailer/allTrailers", "All Trailers", "object.container" },
    { "Trailer/allGenres", "Genres", "object.container" },
    { "Trailer/relDate", "Release Date", "object.container" },
    { "Trailer/postDate", "Post Date", "object.container" },
    { "Trailer/unknown", "Unknown", "object.container" },
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

TEST_F(ImportScriptTest, CreatesDukContextWithImportScript)
{
    EXPECT_NE(ctx, nullptr);
}

TEST_F(ImportScriptTest, AddsAudioItemToVariousCdsContainerChains)
{
    std::string title = "Audio Title";
    std::string mimetype = "audio/mpeg";
    std::string artist = "Artist";
    std::string album = "Album";
    std::string composer = "Composer";
    std::string conductor = "Conductor";
    std::string orchestra = "Orchestra";
    std::string date = "2018-01-01";
    std::string year = "2018";
    std::string genre = "Genre";
    std::string genre2 = "Genre2";
    std::string desc = "Description";
    std::string id = "2";
    std::string location = "/home/gerbera/audio.mp3";
    std::string channels = "2";
    std::string track = "2";
    int onlineService = 0;
    int theora = 0;
    std::map<std::string, std::string> aux;

    std::vector<std::pair<std::string, std::string>> meta {
        { "dc:date", date },
        { "dc:description", desc },
        { "dc:title", title },
        { "upnp:album", album },
        { "upnp:originalTrackNumber", track },
        { "upnp:artist", artist },
        { "upnp:composer", composer },
        { "upnp:conductor", conductor },
        { "upnp:date", year },
        { "upnp:genre", genre },
        { "upnp:genre", genre2 },
        { "upnp:orchestra", orchestra },
    };

    std::map<std::string, std::string> res {
        { "res['nrAudioChannels']", channels },
    };

    // Represents the values passed to `addCdsObject`, extracted from keys defined there.
    std::map<std::string, std::string> asAudioAllAudio {
        { "metaData['dc:date']", date },
        { "metaData['dc:description']", desc },
        { "metaData['dc:title']", title },
        { "metaData['upnp:album']", album },
        { "metaData['upnp:originalTrackNumber']", fmt::format("0{}", track) },
        { "metaData['upnp:artist']", artist },
        { "metaData['upnp:composer']", composer },
        { "metaData['upnp:conductor']", conductor },
        { "metaData['upnp:date']", year },
        { "metaData['upnp:genre']", fmt::format("{},{}", genre, genre2) },
        { "metaData['upnp:orchestra']", orchestra },
        { "title", title },
    };
    std::map<std::string_view, std::map<std::string_view, std::string_view>> configDicts {
        { "/import/scripting/virtual-layout/script-options/script-option", { { "trackNumbers", "show" } } },
    };

    std::map<std::string, std::string> asAudioAllFullName;
    asAudioAllFullName.insert(asAudioAllAudio.begin(), asAudioAllAudio.end());
    asAudioAllFullName["title"] = "Artist - Album - Audio Title";


    std::map<std::string, std::string> asAudioTrackNumbered;
    asAudioTrackNumbered.insert(asAudioAllAudio.begin(), asAudioAllAudio.end());
    asAudioTrackNumbered["title"] = fmt::format("0{} {}", track, title);

    // Expecting the common script calls
    // and will proxy through the mock objects
    // for verification.
    EXPECT_CALL(*commonScriptMock, getPlaylistType(Eq("audio/mpeg"))).WillOnce(Return(1));
    EXPECT_CALL(*commonScriptMock, getYear(Eq("2018-01-01"))).WillOnce(Return(1));
    EXPECT_CALL(*commonScriptMock, getRootPath("object/script/path", location)).WillOnce(Return(1));

    EXPECT_CALL(*commonScriptMock, addContainerTree(ElementsAre("Audio", "All Audio"))).WillOnce(Return(1));
    EXPECT_CALL(*commonScriptMock, addCdsObject(IsIdenticalMap(asAudioAllAudio), "42", UNDEFINED)).WillOnce(Return(0));

    EXPECT_CALL(*commonScriptMock, addContainerTree(ElementsAre("Audio", "Artists", "Artist", "All Songs"))).WillOnce(Return(1));
    EXPECT_CALL(*commonScriptMock, addCdsObject(IsIdenticalMap(asAudioAllAudio), "43", UNDEFINED)).WillOnce(Return(0));

    EXPECT_CALL(*commonScriptMock, addContainerTree(ElementsAre("Audio", "All - full name"))).WillOnce(Return(1));
    EXPECT_CALL(*commonScriptMock, addCdsObject(IsIdenticalMap(asAudioAllFullName), "44", UNDEFINED)).WillOnce(Return(0));

    EXPECT_CALL(*commonScriptMock, addContainerTree(ElementsAre("Audio", "Artists", "Artist", "All - full name"))).WillOnce(Return(1));
    EXPECT_CALL(*commonScriptMock, addCdsObject(IsIdenticalMap(asAudioAllFullName), "45", UNDEFINED)).WillOnce(Return(0));

    EXPECT_CALL(*commonScriptMock, addContainerTree(ElementsAre("Audio", "Artists", "Artist", "Album"))).WillOnce(Return(1));
    EXPECT_CALL(*commonScriptMock, addCdsObject(IsIdenticalMap(asAudioTrackNumbered), "46", UNDEFINED)).WillOnce(Return(0));

    EXPECT_CALL(*commonScriptMock, addContainerTree(ElementsAre("Audio", "Albums", "Album"))).WillOnce(Return(1));
    EXPECT_CALL(*commonScriptMock, addCdsObject(IsIdenticalMap(asAudioTrackNumbered), "47", UNDEFINED)).WillOnce(Return(0));

    EXPECT_CALL(*commonScriptMock, addContainerTree(ElementsAre("Audio", "Genres", "Genre"))).WillOnce(Return(1));
    EXPECT_CALL(*commonScriptMock, addContainerTree(ElementsAre("Audio", "Genres", "Genre2"))).WillOnce(Return(1));
    EXPECT_CALL(*commonScriptMock, addCdsObject(IsIdenticalMap(asAudioAllAudio), "481", UNDEFINED)).WillOnce(Return(0));
    EXPECT_CALL(*commonScriptMock, addCdsObject(IsIdenticalMap(asAudioAllAudio), "482", UNDEFINED)).WillOnce(Return(0));

    EXPECT_CALL(*commonScriptMock, addContainerTree(ElementsAre("Audio", "Composers", "Composer"))).WillOnce(Return(1));
    EXPECT_CALL(*commonScriptMock, addCdsObject(IsIdenticalMap(asAudioAllAudio), "49", UNDEFINED)).WillOnce(Return(0));

    EXPECT_CALL(*commonScriptMock, addContainerTree(ElementsAre("Audio", "Year", "2018"))).WillOnce(Return(1));
    EXPECT_CALL(*commonScriptMock, addCdsObject(IsIdenticalMap(asAudioAllAudio), "50", UNDEFINED)).WillOnce(Return(0));

    EXPECT_CALL(*commonScriptMock, addContainerTree(ElementsAre("Audio", "Artists", artist, "Album Chronology", "2018 - Album"))).WillOnce(Return(1));
    EXPECT_CALL(*commonScriptMock, addCdsObject(IsIdenticalMap(asAudioAllAudio), "462", UNDEFINED)).WillOnce(Return(0));

    addGlobalFunctions(ctx, js_global_functions, {}, audioBox, configDicts);
    dukMockItem(ctx, mimetype, UPNP_CLASS_AUDIO_ITEM, id, theora, title, meta, aux, res, location, onlineService);
    executeScript(ctx);
}

TEST_F(ImportScriptTest, AddsVideoItemToCdsContainerChainWithDirs)
{
    std::string title = "Video Title";
    std::string mimetype = "video/mpeg";
    std::string id = "2";
    std::string location = "/home/gerbera/video.mp4";
#ifdef ONLINE_SERVICES
    auto onlineService = to_underlying(OnlineServiceType::None);
#else
    auto onlineService = 0;
#endif
    int theora = 0;
    std::map<std::string, std::string> aux;
    std::vector<std::pair<std::string, std::string>> meta;
    std::map<std::string, std::string> res;

    // Represents the values passed to `addCdsObject`, extracted from keys defined there.
    std::map<std::string, std::string> asVideoAllVideo {
        { "title", title },
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

    addGlobalFunctions(ctx, js_global_functions, {}, videoBox);
    dukMockItem(ctx, mimetype, UPNP_CLASS_VIDEO_ITEM, id, theora, title, meta, aux, res, location, onlineService);
    executeScript(ctx);
}

TEST_F(ImportScriptTest, AddsImageItemToCdsContainerChains)
{
    std::string title = "Image Title";
    std::string mimetype = "image/jpeg";
    std::string date = "2018-01-01";
    std::string id = "2";
    std::string location = "/home/gerbera/image.jpg";
#ifdef ONLINE_SERVICES
    auto onlineService = to_underlying(OnlineServiceType::None);
#else
    auto onlineService = 0;
#endif
    int theora = 0;

    std::vector<std::pair<std::string, std::string>> meta {
        { "dc:date", date },
    };

    std::map<std::string, std::string> aux;
    std::map<std::string, std::string> res;

    // Represents the values passed to `addCdsObject`, extracted from keys defined there.
    std::map<std::string, std::string> asImagePhotos {
        { "title", title },
        { "metaData['dc:date']", date },
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

    addGlobalFunctions(ctx, js_global_functions, {}, imageBox);
    dukMockItem(ctx, mimetype, UPNP_CLASS_IMAGE_ITEM, id, theora, title, meta, aux, res, location, onlineService);
    executeScript(ctx);
}

TEST_F(ImportScriptTest, AddsOggTheoraVideoItemToCdsContainerChainWithDirs)
{
    std::string title = "Video Title";
    std::string mimetype = "application/ogg";
    std::string id = "2";
    std::string location = "/home/gerbera/video.ogg";
#ifdef ONLINE_SERVICES
    auto onlineService = to_underlying(OnlineServiceType::None);
#else
    auto onlineService = 0;
#endif
    int theora = 1;
    std::map<std::string, std::string> aux;
    std::vector<std::pair<std::string, std::string>> meta;
    std::map<std::string, std::string> res;

    // Represents the values passed to `addCdsObject`, extracted from keys defined there.
    std::map<std::string, std::string> asVideoAllVideo {
        { "title", title },
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

    addGlobalFunctions(ctx, js_global_functions, {}, videoBox);
    dukMockItem(ctx, mimetype, UPNP_CLASS_VIDEO_ITEM, id, theora, title, meta, aux, res, location, onlineService);
    executeScript(ctx);
}

TEST_F(ImportScriptTest, AddsOggTheoraAudioItemToVariousCdsContainerChains)
{
    std::string title = "Audio Title";
    std::string mimetype = "application/ogg";
    std::string artist = "Artist";
    std::string album = "Album";
    std::string date = "2018-01-01";
    std::string year = "2018";
    std::string genre = "Genre";
    std::string desc = "Description";
    std::string composer = "Composer";
    std::string conductor = "Conductor";
    std::string orchestra = "Orchestra";
    std::string id = "2";
    std::string location = "/home/gerbera/audio.mp3";
    std::string channels = "2";
    int onlineService = 0;
    int theora = 0;
    std::map<std::string, std::string> aux;

    std::vector<std::pair<std::string, std::string>> meta {
        { "dc:title", title },
        { "upnp:artist", artist },
        { "upnp:album", album },
        { "dc:date", date },
        { "upnp:date", year },
        { "upnp:genre", genre },
        { "dc:description", desc },
        { "upnp:composer", composer },
        { "upnp:conductor", conductor },
        { "upnp:orchestra", orchestra },
    };

    std::map<std::string, std::string> res {
        { "res['nrAudioChannels']", channels },
    };

    // Represents the values passed to `addCdsObject`, extracted from keys defined there.
    std::map<std::string, std::string> asAudioAllAudio {
        { "metaData['dc:date']", date },
        { "metaData['dc:description']", desc },
        { "metaData['dc:title']", title },
        { "metaData['upnp:album']", album },
        { "metaData['upnp:artist']", artist },
        { "metaData['upnp:composer']", composer },
        { "metaData['upnp:conductor']", conductor },
        { "metaData['upnp:date']", year },
        { "metaData['upnp:genre']", genre },
        { "metaData['upnp:orchestra']", orchestra },
        { "title", title },
    };

    std::map<std::string_view, std::map<std::string_view, std::string_view>> configDicts {
        { "/import/scripting/virtual-layout/script-options/script-option", { { "trackNumbers", "hide" } } },
    };

    std::map<std::string, std::string> asAudioAllFullName;
    asAudioAllFullName.insert(asAudioAllAudio.begin(), asAudioAllAudio.end());
    asAudioAllFullName["title"] = "Artist - Album - Audio Title";

    // Expecting the common script calls
    // and will proxy through the mock objects
    // for verification.
    EXPECT_CALL(*commonScriptMock, getPlaylistType(Eq("application/ogg"))).WillOnce(Return(1));
    EXPECT_CALL(*commonScriptMock, getYear(Eq("2018-01-01"))).WillOnce(Return(1));
    EXPECT_CALL(*commonScriptMock, getRootPath("object/script/path", location)).WillOnce(Return(1));

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
    EXPECT_CALL(*commonScriptMock, addCdsObject(IsIdenticalMap(asAudioAllAudio), "481", UNDEFINED)).WillOnce(Return(0));

    EXPECT_CALL(*commonScriptMock, addContainerTree(ElementsAre("Audio", "Composers", "Composer"))).WillOnce(Return(1));
    EXPECT_CALL(*commonScriptMock, addCdsObject(IsIdenticalMap(asAudioAllAudio), "49", UNDEFINED)).WillOnce(Return(0));

    EXPECT_CALL(*commonScriptMock, addContainerTree(ElementsAre("Audio", "Year", "2018"))).WillOnce(Return(1));
    EXPECT_CALL(*commonScriptMock, addCdsObject(IsIdenticalMap(asAudioAllAudio), "50", UNDEFINED)).WillOnce(Return(0));

    EXPECT_CALL(*commonScriptMock, addContainerTree(ElementsAre("Audio", "Artists", artist, "Album Chronology", "2018 - Album"))).WillOnce(Return(1));
    EXPECT_CALL(*commonScriptMock, addCdsObject(IsIdenticalMap(asAudioAllAudio), "462", UNDEFINED)).WillOnce(Return(0));

    addGlobalFunctions(ctx, js_global_functions, {}, audioBox, configDicts);
    dukMockItem(ctx, mimetype, UPNP_CLASS_AUDIO_ITEM, id, theora, title, meta, aux, res, location, onlineService);
    executeScript(ctx);
}

#endif //HAVE_JS
