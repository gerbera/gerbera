/*GRB*

    Gerbera - https://gerbera.io/

    test_import_initials_script.cc - this file is part of Gerbera.

    Copyright (C) 2025 Gerbera Contributors

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

class ImportInitialsScriptTest : public CommonScriptTestFixture {
public:
    ImportInitialsScriptTest()
    {
        scriptName = "audio.js";
        audioLayout = "Initial";
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
        { "/Audio/Albums/Artist/000 All", "47" },
        { "/Audio/Albums/Artist/Album", "48" },
        { "/Audio/Albums/000 All/Album", "49" },
        { "/Audio/ABC/A/Artist/Album", "50" },
        { "/Audio/ABC/A/Artist/000 All", "51" },
        { "/Audio/Genres/Map/000 All", "491" },
        { "/Audio/Genres/Map/Artist/Album", "492" },
        { "/Audio/Genres/Map2/000 All", "493" },
        { "/Audio/Genres/Map2/Artist/Album", "494" },
        { "/Audio/Genres/000 All/Genre", "496" },
        { "/Audio/Genres/000 All/Genre2", "497" },
        { "/Audio/Year/2018", "530" },
        { "/Audio/Year/2025", "531" },
        { "/Audio/Composers/Composer", "540" },
        { "/Audio/Artists/Artist/Album Chronology/2018 - Album", "550" },
        { "/Audio/Artists/Artist/Album Chronology/2025 - Album", "551" },
    };
    std::vector<std::string> tree = ScriptTestFixture::addContainerTree(ctx, map);
    return ImportInitialsScriptTest::commonScriptMock->addContainerTree(tree);
}

static duk_ret_t mapGenre(duk_context* ctx)
{
    std::map<std::string, std::string> map = {
        { "Genre", "Map" },
        { "Genre2", "Map2" },
    };
    auto result = ScriptTestFixture::mapGenre(ctx, map);
    return ImportInitialsScriptTest::commonScriptMock->mapGenre(result);
}

static duk_ret_t addCdsObject(duk_context* ctx)
{
    std::vector<std::string> keys = {
        "title",
        "metaData['dc:title']",
        "metaData['upnp:artist']",
        "metaData['upnp:composer']",
        "metaData['upnp:album']",
        "metaData['upnp:originalTrackNumber']",
        "metaData['dc:date']",
        "metaData['upnp:date']",
        "metaData['upnp:genre']",
        "metaData['dc:description']"
    };
    addCdsObjectParams params = ScriptTestFixture::addCdsObject(ctx, keys);
    return ImportInitialsScriptTest::commonScriptMock->addCdsObject(params.objectValues, params.containerChain, params.objectType);
}

static duk_ret_t getYear(duk_context* ctx)
{
    std::string date = ScriptTestFixture::getYear(ctx);
    return ImportInitialsScriptTest::commonScriptMock->getYear(date);
}

static duk_ret_t getRootPath(duk_context* ctx)
{
    getRootPathParams params = ScriptTestFixture::getRootPath(ctx);
    return ImportInitialsScriptTest::commonScriptMock->getRootPath(params.objScriptPath, params.origObjLocation);
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
    { "mapGenre", mapGenre, 1 },
    { "addCdsObject", addCdsObject, 3 },
    { "getYear", getYear, 1 },
    { "getRootPath", getRootPath, 2 },
    { "addContainerTree", addContainerTree, 1 },
    { nullptr, nullptr, 0 },
};
static const std::vector<boxConfig> audioIniital {
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
    { "AudioInitial/abc", "ABC", "object.container"},
    { "AudioInitial/allArtistTracks", "000 All", "object.container" },
    { "AudioInitial/allBooks", "Audiobooks", "object.container" },
    { "AudioInitial/audioBookRoot", "Audiobooks", "object.container"},
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

TEST_F(ImportInitialsScriptTest, CreatesDukContextWithImportScript)
{
    EXPECT_NE(ctx, nullptr);
}

TEST_F(ImportInitialsScriptTest, AddsAudioItemWithInitialFormat)
{
    std::string title = "Audio Title";
    std::string mimetype = "audio/mpeg";
    std::string artist = "Artist";
    std::string composer = "Composer";
    std::string album = "Album";
    std::string date = "2018-01-01";
    std::string year = "2018";
    std::string genre = "Genre";
    std::string genre2 = "Genre2";
    std::string desc = "Description";
    std::string id = "2";
    std::string location = "/home/gerbera/audio.mp3";
    std::string track = "2";
    int onlineService = 0;
    int theora = 0;
    std::map<std::string, std::string> aux;
    std::map<std::string, std::string> res;

    std::vector<std::pair<std::string, std::string>> meta {
        { "dc:title", title },
        { "upnp:artist", artist },
        { "upnp:composer", composer },
        { "upnp:album", album },
        { "upnp:originalTrackNumber", track },
        { "dc:date", date },
        { "upnp:date", year },
        { "upnp:genre", genre },
        { "upnp:genre", genre2 },
        { "dc:description", desc },
    };

    // Represents the values passed to `addCdsObject`, extracted from keys defined there.
    std::map<std::string, std::string> asAudioAllAudio {
        { "title", title },
        { "metaData['dc:title']", title },
        { "metaData['upnp:artist']", artist },
        { "metaData['upnp:album']", album },
        { "metaData['upnp:originalTrackNumber']", fmt::format("0{}", track) },
        { "metaData['upnp:composer']", composer },
        { "metaData['dc:date']", date },
        { "metaData['upnp:date']", year },
        { "metaData['upnp:genre']", fmt::format("{},{}", genre, genre2) },
        { "metaData['dc:description']", desc },
    };

    std::map<std::string, std::string> asAudioAllFullName;
    asAudioAllFullName.insert(asAudioAllAudio.begin(), asAudioAllAudio.end());
    asAudioAllFullName["title"] = "Artist - Album - Audio Title";

    std::map<std::string, std::string> asAudioAllFullNameNumbered;
    asAudioAllFullNameNumbered.insert(asAudioAllAudio.begin(), asAudioAllAudio.end());
    asAudioAllFullNameNumbered["title"] = fmt::format("Artist - Album - 0{} Audio Title", track);

    std::map<std::string, std::string> asAudioNumbered;
    asAudioNumbered.insert(asAudioAllAudio.begin(), asAudioAllAudio.end());
    asAudioNumbered["title"] = fmt::format("0{} {}", track, title);

    std::map<std::string, std::string> asAudioAllArtistTitle;
    asAudioAllArtistTitle.insert(asAudioAllAudio.begin(), asAudioAllAudio.end());
    asAudioAllArtistTitle["title"] = fmt::format("Album - 0{} Audio Title", track);

    std::map<std::string, std::string> asAudioAllAudioTitleArtist;
    asAudioAllAudioTitleArtist.insert(asAudioAllAudio.begin(), asAudioAllAudio.end());
    asAudioAllAudioTitleArtist["title"] = "Audio Title - Artist";

    std::map<std::string, std::string> asAudioTrackArtistTitle;
    asAudioTrackArtistTitle.insert(asAudioAllAudio.begin(), asAudioAllAudio.end());
    asAudioTrackArtistTitle["title"] = fmt::format("2018 - Album - 0{} Audio Title", track);

    // Expecting the common script calls
    // and will proxy through the mock objects
    // for verification.
    EXPECT_CALL(*commonScriptMock, mapGenre(Eq("Map"))).WillOnce(Return(1));
    EXPECT_CALL(*commonScriptMock, mapGenre(Eq("Map2"))).WillOnce(Return(1));
    EXPECT_CALL(*commonScriptMock, getYear(Eq("2018-01-01"))).WillOnce(Return(1));
    EXPECT_CALL(*commonScriptMock, getRootPath(Eq(UNDEFINED), Eq(location))).WillRepeatedly(Return(1));

    EXPECT_CALL(*commonScriptMock, addContainerTree(ElementsAre("Audio", "All Audio"))).WillOnce(Return(1));
    EXPECT_CALL(*commonScriptMock, addCdsObject(IsIdenticalMap(asAudioAllAudio), "42", UNDEFINED)).WillOnce(Return(0));

    EXPECT_CALL(*commonScriptMock, addContainerTree(ElementsAre("Audio", "Artists", "Artist", "All Songs"))).WillOnce(Return(1));
    EXPECT_CALL(*commonScriptMock, addCdsObject(IsIdenticalMap(asAudioAllAudio), "43", UNDEFINED)).WillOnce(Return(0));

    EXPECT_CALL(*commonScriptMock, addContainerTree(ElementsAre("Audio", "All - full name"))).WillOnce(Return(1));
    EXPECT_CALL(*commonScriptMock, addCdsObject(IsIdenticalMap(asAudioAllFullName), "44", UNDEFINED)).WillOnce(Return(0));

    // ARTIST //
    EXPECT_CALL(*commonScriptMock, addContainerTree(ElementsAre( "Audio", "Artists", "Artist", "All - full name"))).WillOnce(Return(1));
    EXPECT_CALL(*commonScriptMock, addCdsObject(IsIdenticalMap(asAudioAllFullName), "45", UNDEFINED)).WillOnce(Return(0));

    EXPECT_CALL(*commonScriptMock, addContainerTree(ElementsAre("Audio", "Artists", "Artist", "Album"))).WillOnce(Return(1));
    EXPECT_CALL(*commonScriptMock, addCdsObject(IsIdenticalMap(asAudioNumbered), "46", UNDEFINED)).WillOnce(Return(0));

    EXPECT_CALL(*commonScriptMock, addContainerTree(ElementsAre("Audio", "Albums", "Artist", "000 All"))).WillOnce(Return(1));
    EXPECT_CALL(*commonScriptMock, addCdsObject(IsIdenticalMap(asAudioAllArtistTitle), "47", UNDEFINED)).WillOnce(Return(0));

    EXPECT_CALL(*commonScriptMock, addContainerTree(ElementsAre("Audio", "Albums", "Artist", "Album"))).WillOnce(Return(1));
    EXPECT_CALL(*commonScriptMock, addCdsObject(IsIdenticalMap(asAudioNumbered), "48", UNDEFINED)).WillOnce(Return(0));

    EXPECT_CALL(*commonScriptMock, addContainerTree(ElementsAre("Audio", "Albums", "000 All", "Album"))).WillOnce(Return(1));
    EXPECT_CALL(*commonScriptMock, addCdsObject(IsIdenticalMap(asAudioNumbered), "49", UNDEFINED)).WillOnce(Return(0));

    EXPECT_CALL(*commonScriptMock, addContainerTree(ElementsAre("Audio", "ABC", "A", "Artist", "Album"))).WillOnce(Return(1));
    EXPECT_CALL(*commonScriptMock, addCdsObject(IsIdenticalMap(asAudioNumbered), "50", UNDEFINED)).WillOnce(Return(0));

    EXPECT_CALL(*commonScriptMock, addContainerTree(ElementsAre("Audio", "ABC", "A", "Artist", "000 All"))).WillOnce(Return(1));
    EXPECT_CALL(*commonScriptMock, addCdsObject(IsIdenticalMap(asAudioTrackArtistTitle), "51", UNDEFINED)).WillOnce(Return(0));

    // GENRE //

    EXPECT_CALL(*commonScriptMock, addContainerTree(ElementsAre("Audio", "Genres", "Map", "000 All"))).WillOnce(Return(1));
    EXPECT_CALL(*commonScriptMock, addCdsObject(IsIdenticalMap(asAudioAllFullNameNumbered), "491", UNDEFINED)).WillOnce(Return(0));

    EXPECT_CALL(*commonScriptMock, addContainerTree(ElementsAre("Audio", "Genres", "Map", "Artist", "Album"))).WillOnce(Return(1));
    EXPECT_CALL(*commonScriptMock, addCdsObject(IsIdenticalMap(asAudioAllFullNameNumbered), "492", UNDEFINED)).WillOnce(Return(0));

    EXPECT_CALL(*commonScriptMock, addContainerTree(ElementsAre("Audio", "Genres", "Map2", "000 All"))).WillOnce(Return(1));
    EXPECT_CALL(*commonScriptMock, addCdsObject(IsIdenticalMap(asAudioAllFullNameNumbered), "493", UNDEFINED)).WillOnce(Return(0));

    EXPECT_CALL(*commonScriptMock, addContainerTree(ElementsAre("Audio", "Genres", "Map2", "Artist", "Album"))).WillOnce(Return(1));
    EXPECT_CALL(*commonScriptMock, addCdsObject(IsIdenticalMap(asAudioAllFullNameNumbered), "494", UNDEFINED)).WillOnce(Return(0));

    EXPECT_CALL(*commonScriptMock, addContainerTree(ElementsAre("Audio", "Genres", "000 All", "Genre"))).WillOnce(Return(1));
    EXPECT_CALL(*commonScriptMock, addCdsObject(IsIdenticalMap(asAudioAllFullName), "496", UNDEFINED)).WillOnce(Return(0));

    // MISC //
    EXPECT_CALL(*commonScriptMock, addContainerTree(ElementsAre("Audio", "Year", "2018"))).WillOnce(Return(1));
    EXPECT_CALL(*commonScriptMock, addCdsObject(IsIdenticalMap(asAudioAllFullName), "530", UNDEFINED)).WillOnce(Return(0));

    EXPECT_CALL(*commonScriptMock, addContainerTree(ElementsAre("Audio", "Composers", "Composer"))).WillOnce(Return(1));
    EXPECT_CALL(*commonScriptMock, addCdsObject(IsIdenticalMap(asAudioAllFullName), "540", UNDEFINED)).WillOnce(Return(0));

    EXPECT_CALL(*commonScriptMock, addContainerTree(ElementsAre("Audio", "Artists", "Artist", "Album Chronology", "2018 - Album"))).WillOnce(Return(1));
    EXPECT_CALL(*commonScriptMock, addCdsObject(IsIdenticalMap(asAudioAllFullName), "550", UNDEFINED)).WillOnce(Return(0));

    addGlobalFunctions(ctx, js_global_functions, { { "/import/scripting/virtual-layout/attribute::audio-layout", audioLayout }, }, audioIniital);

    dukMockItem(ctx, mimetype, UPNP_CLASS_AUDIO_ITEM, id, theora, title, meta, aux, res, location, onlineService);
    executeScript(ctx);
}

TEST_F(ImportInitialsScriptTest, AddsSpokenAudioItemWithInitialFormat)
{
    std::string title = "Audio Title";
    std::string mimetype = "audio/mpeg";
    std::string artist = "Artist";
    std::string composer = "Composer";
    std::string album = "Album";
    std::string date = "2025-01-01";
    std::string year = "2025";
    std::string genre2 = "Genre2";
    std::string desc = "Description";
    std::string id = "2";
    std::string location = "/home/gerbera/audio.mp3";
    std::string track = "2";
    std::string disk = "42";
    int onlineService = 0;
    int theora = 0;
    std::map<std::string, std::string> res;

    std::vector<std::pair<std::string, std::string>> meta {
        { "dc:title", title },
        { "upnp:artist", artist },
        { "upnp:composer", composer },
        { "upnp:album", album },
        { "upnp:originalTrackNumber", track },
        { "dc:date", date },
        { "upnp:date", year },
        { "upnp:genre", genre2 },
        { "dc:description", desc },
    };

    std::map<std::string, std::string> aux {
        { "DISCNUMBER", disk },
    };

    // Represents the values passed to `addCdsObject`, extracted from keys defined there.
    std::map<std::string, std::string> asAudioAllAudio {
        { "title", title },
        { "metaData['dc:title']", title },
        { "metaData['upnp:artist']", artist },
        { "metaData['upnp:album']", album },
        { "metaData['upnp:originalTrackNumber']", fmt::format("{}00{}", disk, track) },
        { "metaData['upnp:composer']", composer },
        { "metaData['dc:date']", date },
        { "metaData['upnp:date']", year },
        { "metaData['upnp:genre']", genre2 },
        { "metaData['dc:description']", desc },
    };

    std::map<std::string_view, std::map<std::string_view, std::string_view>> configDicts {
        { "/import/scripting/virtual-layout/script-options/script-option", { { "spokenGenre", "Audio Book|Genre2" } } },
    };

    std::map<std::string, std::string> asAudioAllFullName;
    asAudioAllFullName.insert(asAudioAllAudio.begin(), asAudioAllAudio.end());
    asAudioAllFullName["title"] = "Artist - Album - Audio Title";

    std::map<std::string, std::string> asAudioAllFullNameNumbered;
    asAudioAllFullNameNumbered.insert(asAudioAllAudio.begin(), asAudioAllAudio.end());
    asAudioAllFullNameNumbered["title"] = fmt::format("Artist - Album - {} 00{} Audio Title", disk, track);

    std::map<std::string, std::string> asAudioNumbered;
    asAudioNumbered.insert(asAudioAllAudio.begin(), asAudioAllAudio.end());
    asAudioNumbered["title"] = fmt::format("{} 00{} {}", disk, track, title);

    std::map<std::string, std::string> asAudioAllArtistTitle;
    asAudioAllArtistTitle.insert(asAudioAllAudio.begin(), asAudioAllAudio.end());
    asAudioAllArtistTitle["title"] = fmt::format("Album - {} 00{} Audio Title", disk, track);

    std::map<std::string, std::string> asAudioAllAudioTitleArtist;
    asAudioAllAudioTitleArtist.insert(asAudioAllAudio.begin(), asAudioAllAudio.end());
    asAudioAllAudioTitleArtist["title"] = "Audio Title - Artist";

    std::map<std::string, std::string> asAudioTrackArtistTitle;
    asAudioTrackArtistTitle.insert(asAudioAllAudio.begin(), asAudioAllAudio.end());
    asAudioTrackArtistTitle["title"] = fmt::format("{} - Album - {} 00{} Audio Title", year, disk, track);

    // Expecting the common script calls
    // and will proxy through the mock objects
    // for verification.
    EXPECT_CALL(*commonScriptMock, mapGenre(Eq("Map2"))).WillOnce(Return(1));
    EXPECT_CALL(*commonScriptMock, getYear(Eq("2025-01-01"))).WillOnce(Return(1));
    EXPECT_CALL(*commonScriptMock, getRootPath(Eq(UNDEFINED), Eq(location))).WillRepeatedly(Return(1));

    EXPECT_CALL(*commonScriptMock, addContainerTree(ElementsAre("Audio", "All Audio"))).WillOnce(Return(1));
    EXPECT_CALL(*commonScriptMock, addCdsObject(IsIdenticalMap(asAudioAllAudio), "42", UNDEFINED)).WillOnce(Return(0));

    EXPECT_CALL(*commonScriptMock, addContainerTree(ElementsAre("Audio", "Artists", "Artist", "All Songs"))).WillOnce(Return(1));
    EXPECT_CALL(*commonScriptMock, addCdsObject(IsIdenticalMap(asAudioAllAudio), "43", UNDEFINED)).WillOnce(Return(0));

    EXPECT_CALL(*commonScriptMock, addContainerTree(ElementsAre("Audio", "All - full name"))).WillOnce(Return(1));
    EXPECT_CALL(*commonScriptMock, addCdsObject(IsIdenticalMap(asAudioAllFullName), "44", UNDEFINED)).WillOnce(Return(0));

    // ARTIST //
    EXPECT_CALL(*commonScriptMock, addContainerTree(ElementsAre( "Audio", "Artists", "Artist", "All - full name"))).WillOnce(Return(1));
    EXPECT_CALL(*commonScriptMock, addCdsObject(IsIdenticalMap(asAudioAllFullName), "45", UNDEFINED)).WillOnce(Return(0));

    EXPECT_CALL(*commonScriptMock, addContainerTree(ElementsAre("Audio", "Artists", "Artist", "Album"))).WillOnce(Return(1));
    EXPECT_CALL(*commonScriptMock, addCdsObject(IsIdenticalMap(asAudioNumbered), "46", UNDEFINED)).WillOnce(Return(0));

    EXPECT_CALL(*commonScriptMock, addContainerTree(ElementsAre("Audio", "Albums", "Artist", "000 All"))).WillOnce(Return(1));
    EXPECT_CALL(*commonScriptMock, addCdsObject(IsIdenticalMap(asAudioAllArtistTitle), "47", UNDEFINED)).WillOnce(Return(0));

    EXPECT_CALL(*commonScriptMock, addContainerTree(ElementsAre("Audio", "Albums", "Artist", "Album"))).WillOnce(Return(1));
    EXPECT_CALL(*commonScriptMock, addCdsObject(IsIdenticalMap(asAudioNumbered), "48", UNDEFINED)).WillOnce(Return(0));

    EXPECT_CALL(*commonScriptMock, addContainerTree(ElementsAre("Audio", "Albums", "000 All", "Album"))).WillOnce(Return(1));
    EXPECT_CALL(*commonScriptMock, addCdsObject(IsIdenticalMap(asAudioNumbered), "49", UNDEFINED)).WillOnce(Return(0));

    EXPECT_CALL(*commonScriptMock, addContainerTree(ElementsAre("Audio", "ABC", "A", "Artist", "Album"))).WillOnce(Return(1));
    EXPECT_CALL(*commonScriptMock, addCdsObject(IsIdenticalMap(asAudioNumbered), "50", UNDEFINED)).WillOnce(Return(0));

    // GENRE //

    EXPECT_CALL(*commonScriptMock, addContainerTree(ElementsAre("Audio", "Genres", "Map2", "Artist", "Album"))).WillOnce(Return(1));
    EXPECT_CALL(*commonScriptMock, addCdsObject(IsIdenticalMap(asAudioAllFullNameNumbered), "494", UNDEFINED)).WillOnce(Return(0));

    EXPECT_CALL(*commonScriptMock, addContainerTree(ElementsAre("Audio", "Genres", "000 All", "Genre2"))).WillOnce(Return(1));
    EXPECT_CALL(*commonScriptMock, addCdsObject(IsIdenticalMap(asAudioAllFullName), "497", UNDEFINED)).WillOnce(Return(0));

    // MISC //
    EXPECT_CALL(*commonScriptMock, addContainerTree(ElementsAre("Audio", "Year", "2025"))).WillOnce(Return(1));
    EXPECT_CALL(*commonScriptMock, addCdsObject(IsIdenticalMap(asAudioAllFullName), "531", UNDEFINED)).WillOnce(Return(0));

    EXPECT_CALL(*commonScriptMock, addContainerTree(ElementsAre("Audio", "Composers", "Composer"))).WillOnce(Return(1));
    EXPECT_CALL(*commonScriptMock, addCdsObject(IsIdenticalMap(asAudioAllFullName), "540", UNDEFINED)).WillOnce(Return(0));

    EXPECT_CALL(*commonScriptMock, addContainerTree(ElementsAre("Audio", "Artists", "Artist", "Album Chronology", "2025 - Album"))).WillOnce(Return(1));
    EXPECT_CALL(*commonScriptMock, addCdsObject(IsIdenticalMap(asAudioAllFullName), "551", UNDEFINED)).WillOnce(Return(0));

    addGlobalFunctions(ctx, js_global_functions, { { "/import/scripting/virtual-layout/attribute::audio-layout", audioLayout }, }, audioIniital, configDicts);

    dukMockItem(ctx, mimetype, UPNP_CLASS_AUDIO_ITEM, id, theora, title, meta, aux, res, location, onlineService);
    executeScript(ctx);
}

TEST_F(ImportInitialsScriptTest, AddsUnnumberedAudioItemWithInitialFormat)
{
    std::string title = "Audio Title";
    std::string mimetype = "audio/mpeg";
    std::string artist = "Artist";
    std::string composer = "Composer";
    std::string album = "Album";
    std::string date = "2018-01-01";
    std::string year = "2018";
    std::string genre = "Genre";
    std::string genre2 = "Genre2";
    std::string desc = "Description";
    std::string id = "2";
    std::string location = "/home/gerbera/audio.mp3";
    std::string track = "2";
    int onlineService = 0;
    int theora = 0;
    std::map<std::string, std::string> aux;
    std::map<std::string, std::string> res;

    std::vector<std::pair<std::string, std::string>> meta {
        { "dc:title", title },
        { "upnp:artist", artist },
        { "upnp:composer", composer },
        { "upnp:album", album },
        { "upnp:originalTrackNumber", track },
        { "dc:date", date },
        { "upnp:date", year },
        { "upnp:genre", genre },
        { "upnp:genre", genre2 },
        { "dc:description", desc },
    };

    // Represents the values passed to `addCdsObject`, extracted from keys defined there.
    std::map<std::string, std::string> asAudioAllAudio {
        { "title", title },
        { "metaData['dc:title']", title },
        { "metaData['upnp:artist']", artist },
        { "metaData['upnp:album']", album },
        { "metaData['upnp:originalTrackNumber']", fmt::format("0{}", track) },
        { "metaData['upnp:composer']", composer },
        { "metaData['dc:date']", date },
        { "metaData['upnp:date']", year },
        { "metaData['upnp:genre']", fmt::format("{},{}", genre, genre2) },
        { "metaData['dc:description']", desc },
    };

    std::map<std::string_view, std::map<std::string_view, std::string_view>> configDicts {
        { "/import/scripting/virtual-layout/script-options/script-option", { { "trackNumbers", "hide" } } },
    };

    std::map<std::string, std::string> asAudioAllFullName;
    asAudioAllFullName.insert(asAudioAllAudio.begin(), asAudioAllAudio.end());
    asAudioAllFullName["title"] = "Artist - Album - Audio Title";

    std::map<std::string, std::string> asAudioAllArtistTitle;
    asAudioAllArtistTitle.insert(asAudioAllAudio.begin(), asAudioAllAudio.end());
    asAudioAllArtistTitle["title"] = "Album - Audio Title";

    std::map<std::string, std::string> asAudioAllAudioTitleArtist;
    asAudioAllAudioTitleArtist.insert(asAudioAllAudio.begin(), asAudioAllAudio.end());
    asAudioAllAudioTitleArtist["title"] = "Audio Title - Artist";

    std::map<std::string, std::string> asAudioTrackArtistTitle;
    asAudioTrackArtistTitle.insert(asAudioAllAudio.begin(), asAudioAllAudio.end());
    asAudioTrackArtistTitle["title"] = "2018 - Album - Audio Title";

    // Expecting the common script calls
    // and will proxy through the mock objects
    // for verification.
    EXPECT_CALL(*commonScriptMock, mapGenre(Eq("Map"))).WillOnce(Return(1));
    EXPECT_CALL(*commonScriptMock, mapGenre(Eq("Map2"))).WillOnce(Return(1));
    EXPECT_CALL(*commonScriptMock, getYear(Eq("2018-01-01"))).WillOnce(Return(1));
    EXPECT_CALL(*commonScriptMock, getRootPath(Eq(UNDEFINED), Eq(location))).WillRepeatedly(Return(1));

    EXPECT_CALL(*commonScriptMock, addContainerTree(ElementsAre("Audio", "All Audio"))).WillOnce(Return(1));
    EXPECT_CALL(*commonScriptMock, addCdsObject(IsIdenticalMap(asAudioAllAudio), "42", UNDEFINED)).WillOnce(Return(0));

    EXPECT_CALL(*commonScriptMock, addContainerTree(ElementsAre("Audio", "Artists", "Artist", "All Songs"))).WillOnce(Return(1));
    EXPECT_CALL(*commonScriptMock, addCdsObject(IsIdenticalMap(asAudioAllAudio), "43", UNDEFINED)).WillOnce(Return(0));

    EXPECT_CALL(*commonScriptMock, addContainerTree(ElementsAre("Audio", "All - full name"))).WillOnce(Return(1));
    EXPECT_CALL(*commonScriptMock, addCdsObject(IsIdenticalMap(asAudioAllFullName), "44", UNDEFINED)).WillOnce(Return(0));

    // ARTIST //
    EXPECT_CALL(*commonScriptMock, addContainerTree(ElementsAre( "Audio", "Artists", "Artist", "All - full name"))).WillOnce(Return(1));
    EXPECT_CALL(*commonScriptMock, addCdsObject(IsIdenticalMap(asAudioAllFullName), "45", UNDEFINED)).WillOnce(Return(0));

    EXPECT_CALL(*commonScriptMock, addContainerTree(ElementsAre("Audio", "Artists", "Artist", "Album"))).WillOnce(Return(1));
    EXPECT_CALL(*commonScriptMock, addCdsObject(IsIdenticalMap(asAudioAllAudio), "46", UNDEFINED)).WillOnce(Return(0));

    EXPECT_CALL(*commonScriptMock, addContainerTree(ElementsAre("Audio", "Albums", "Artist", "000 All"))).WillOnce(Return(1));
    EXPECT_CALL(*commonScriptMock, addCdsObject(IsIdenticalMap(asAudioAllArtistTitle), "47", UNDEFINED)).WillOnce(Return(0));

    EXPECT_CALL(*commonScriptMock, addContainerTree(ElementsAre("Audio", "Albums", "Artist", "Album"))).WillOnce(Return(1));
    EXPECT_CALL(*commonScriptMock, addCdsObject(IsIdenticalMap(asAudioAllAudio), "48", UNDEFINED)).WillOnce(Return(0));

    EXPECT_CALL(*commonScriptMock, addContainerTree(ElementsAre("Audio", "Albums", "000 All", "Album"))).WillOnce(Return(1));
    EXPECT_CALL(*commonScriptMock, addCdsObject(IsIdenticalMap(asAudioAllAudio), "49", UNDEFINED)).WillOnce(Return(0));

    EXPECT_CALL(*commonScriptMock, addContainerTree(ElementsAre("Audio", "ABC", "A", "Artist", "Album"))).WillOnce(Return(1));
    EXPECT_CALL(*commonScriptMock, addCdsObject(IsIdenticalMap(asAudioAllAudio), "50", UNDEFINED)).WillOnce(Return(0));

    EXPECT_CALL(*commonScriptMock, addContainerTree(ElementsAre("Audio", "ABC", "A", "Artist", "000 All"))).WillOnce(Return(1));
    EXPECT_CALL(*commonScriptMock, addCdsObject(IsIdenticalMap(asAudioTrackArtistTitle), "51", UNDEFINED)).WillOnce(Return(0));

    // GENRE //

    EXPECT_CALL(*commonScriptMock, addContainerTree(ElementsAre("Audio", "Genres", "Map", "000 All"))).WillOnce(Return(1));
    EXPECT_CALL(*commonScriptMock, addCdsObject(IsIdenticalMap(asAudioAllFullName), "491", UNDEFINED)).WillOnce(Return(0));

    EXPECT_CALL(*commonScriptMock, addContainerTree(ElementsAre("Audio", "Genres", "Map", "Artist", "Album"))).WillOnce(Return(1));
    EXPECT_CALL(*commonScriptMock, addCdsObject(IsIdenticalMap(asAudioAllFullName), "492", UNDEFINED)).WillOnce(Return(0));

    EXPECT_CALL(*commonScriptMock, addContainerTree(ElementsAre("Audio", "Genres", "Map2", "000 All"))).WillOnce(Return(1));
    EXPECT_CALL(*commonScriptMock, addCdsObject(IsIdenticalMap(asAudioAllFullName), "493", UNDEFINED)).WillOnce(Return(0));

    EXPECT_CALL(*commonScriptMock, addContainerTree(ElementsAre("Audio", "Genres", "Map2", "Artist", "Album"))).WillOnce(Return(1));
    EXPECT_CALL(*commonScriptMock, addCdsObject(IsIdenticalMap(asAudioAllFullName), "494", UNDEFINED)).WillOnce(Return(0));

    EXPECT_CALL(*commonScriptMock, addContainerTree(ElementsAre("Audio", "Genres", "000 All", "Genre"))).WillOnce(Return(1));
    EXPECT_CALL(*commonScriptMock, addCdsObject(IsIdenticalMap(asAudioAllFullName), "496", UNDEFINED)).WillOnce(Return(0));

    // MISC //
    EXPECT_CALL(*commonScriptMock, addContainerTree(ElementsAre("Audio", "Year", "2018"))).WillOnce(Return(1));
    EXPECT_CALL(*commonScriptMock, addCdsObject(IsIdenticalMap(asAudioAllFullName), "530", UNDEFINED)).WillOnce(Return(0));

    EXPECT_CALL(*commonScriptMock, addContainerTree(ElementsAre("Audio", "Composers", "Composer"))).WillOnce(Return(1));
    EXPECT_CALL(*commonScriptMock, addCdsObject(IsIdenticalMap(asAudioAllFullName), "540", UNDEFINED)).WillOnce(Return(0));

    EXPECT_CALL(*commonScriptMock, addContainerTree(ElementsAre("Audio", "Artists", "Artist", "Album Chronology", "2018 - Album"))).WillOnce(Return(1));
    EXPECT_CALL(*commonScriptMock, addCdsObject(IsIdenticalMap(asAudioAllFullName), "550", UNDEFINED)).WillOnce(Return(0));

    addGlobalFunctions(ctx, js_global_functions, { { "/import/scripting/virtual-layout/attribute::audio-layout", audioLayout }, }, audioIniital, configDicts);

    dukMockItem(ctx, mimetype, UPNP_CLASS_AUDIO_ITEM, id, theora, title, meta, aux, res, location, onlineService);
    executeScript(ctx);
}

#endif //HAVE_JS
