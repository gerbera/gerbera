/*GRB*

    Gerbera - https://gerbera.io/

    test_import_struct_script.cc - this file is part of Gerbera.

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

#include "content/onlineservice/online_service.h"
#include "metadata/metadata_handler.h"
#include "upnp/upnp_common.h"

#include "mock/common_script_mock.h"
#include "mock/duk_helper.h"
#include "mock/script_test_fixture.h"

#include <duktape.h>
#include <fmt/format.h>
#include <memory>

class ImportStructuredScriptTest : public CommonScriptTestFixture {
public:
    ImportStructuredScriptTest()
    {
        scriptName = "import.js";
        audioLayout = "Structured";
    }
};

static duk_ret_t addContainerTree(duk_context* ctx)
{
    std::map<std::string, std::string> map {
        { "", "0" },
        { "/-Album-/-ABCD-/A/Album - Artist", "42" },
        { "/-Album-/-ABCD-/-all-/Album - Artist", "43" },
        { "/-Album-/--all--/Album - Artist", "44" },
        { "/-Artist-/--all--/Artist", "45" },
        { "/-Artist-/-ABCD-/-all-/Artist", "46" },
        { "/-Artist-/-ABCD-/A/Artist/-all-", "47" },
        { "/-Artist-/-ABCD-/A/Artist/Album (2018)", "48" },
        { "/-Genre-/Genre/--all--", "491" },
        { "/-Genre-/Genre2/--all--", "492" },
        { "/-Genre-/Genre/-A-/Album - Artist", "501" },
        { "/-Genre-/Genre2/-A-/Album - Artist", "502" },
        { "/-Track-/-ABCD-/A", "51" },
        { "/-Track-/--all--", "52" },
        { "/-Year-/2010 - 2019/-all-", "53" },
        { "/-Year-/2010 - 2019/2018/-all-", "54" },
        { "/-Year-/2010 - 2019/2018/Artist/Album", "55" },
    };
    std::vector<std::string> tree = ScriptTestFixture::addContainerTree(ctx, map);
    return ImportStructuredScriptTest::commonScriptMock->addContainerTree(tree);
}

static duk_ret_t addCdsObject(duk_context* ctx)
{
    std::vector<std::string> keys = {
        "title",
        "metaData['dc:title']",
        "metaData['upnp:artist']",
        "metaData['upnp:album']",
        "metaData['dc:date']",
        "metaData['upnp:date']",
        "metaData['upnp:genre']",
        "metaData['dc:description']"
    };
    addCdsObjectParams params = ScriptTestFixture::addCdsObject(ctx, keys);
    return ImportStructuredScriptTest::commonScriptMock->addCdsObject(params.objectValues, params.containerChain, params.objectType);
}

static duk_ret_t getYear(duk_context* ctx)
{
    std::string date = ScriptTestFixture::getYear(ctx);
    return ImportStructuredScriptTest::commonScriptMock->getYear(date);
}

static duk_ret_t getRootPath(duk_context* ctx)
{
    getRootPathParams params = ScriptTestFixture::getRootPath(ctx);
    return ImportStructuredScriptTest::commonScriptMock->getRootPath(params.objScriptPath, params.origObjLocation);
}

static duk_ret_t abcBox(duk_context* ctx)
{
    abcBoxParams params = ScriptTestFixture::abcBox(ctx);
    return ImportStructuredScriptTest::commonScriptMock->abcBox(params.inputValue, params.boxType, params.divChar);
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
    { "abcbox", abcBox, 3 },
    { "addContainerTree", addContainerTree, 1 },
    { nullptr, nullptr, 0 },
};
static const std::vector<boxConfig> audioBox {
    { "AudioStructured/allAlbums", "-Album-", "object.container", true, 6 },
    { "AudioStructured/allArtistTracks", "all", "object.container" },
    { "AudioStructured/allArtists", "-Artist-", "object.container", true, 9 },
    { "AudioStructured/allGenres", "-Genre-", "object.container", true, 26 },
    { "AudioStructured/allTracks", "-Track-", "object.container", true, 6 },
    { "AudioStructured/allYears", "-Year-", "object.container" },
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

TEST_F(ImportStructuredScriptTest, CreatesDukContextWithImportScript)
{
    EXPECT_NE(ctx, nullptr);
}

TEST_F(ImportStructuredScriptTest, AddsAudioItemWithABCBoxFormat)
{
    std::string title = "Audio Title";
    std::string mimetype = "audio/mpeg";
    std::string artist = "Artist";
    std::string album = "Album";
    std::string date = "2018-01-01";
    std::string year = "2018";
    std::string genre = "Genre";
    std::string genre2 = "Genre2";
    std::string desc = "Description";
    std::string id = "2";
    std::string location = "/home/gerbera/audio.mp3";
    int onlineService = 0;
    int theora = 0;
    std::map<std::string, std::string> aux;
    std::map<std::string, std::string> res;

    std::vector<std::pair<std::string, std::string>> meta {
        { "dc:title", title },
        { "upnp:artist", artist },
        { "upnp:album", album },
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
        { "metaData['dc:date']", date },
        { "metaData['upnp:date']", year },
        { "metaData['upnp:genre']", fmt::format("{},{}", genre, genre2) },
        { "metaData['dc:description']", desc },
    };

    std::map<std::string, std::string> asAudioAllFullName;
    asAudioAllFullName.insert(asAudioAllAudio.begin(), asAudioAllAudio.end());
    asAudioAllFullName["title"] = "Artist - Album - Audio Title";

    std::map<std::string, std::string> asAudioAllArtistTitle;
    asAudioAllArtistTitle.insert(asAudioAllAudio.begin(), asAudioAllAudio.end());
    asAudioAllArtistTitle["title"] = "Audio Title (Album, 2018)";

    std::map<std::string, std::string> asAudioAllAudioTitleArtist;
    asAudioAllAudioTitleArtist.insert(asAudioAllAudio.begin(), asAudioAllAudio.end());
    asAudioAllAudioTitleArtist["title"] = "Audio Title - Artist";

    std::map<std::string, std::string> asAudioTrackArtistTitle;
    asAudioTrackArtistTitle.insert(asAudioAllAudio.begin(), asAudioAllAudio.end());
    asAudioTrackArtistTitle["title"] = "Audio Title - Artist (Album, 2018)";

    // Expecting the common script calls
    // and will proxy through the mock objects
    // for verification.
    EXPECT_CALL(*commonScriptMock, getPlaylistType(Eq("audio/mpeg"))).WillOnce(Return(1));
    EXPECT_CALL(*commonScriptMock, getYear(Eq("2018-01-01"))).WillOnce(Return(1));

    EXPECT_CALL(*commonScriptMock, abcBox(Eq("Album"), Eq(6), Eq("-"))).WillOnce(Return(1));
    EXPECT_CALL(*commonScriptMock, addContainerTree(ElementsAre("-Album-", "-ABCD-", "A", "Album - Artist"))).WillOnce(Return(1));
    EXPECT_CALL(*commonScriptMock, addCdsObject(IsIdenticalMap(asAudioAllAudio), "42", UNDEFINED)).WillOnce(Return(0));

    EXPECT_CALL(*commonScriptMock, addContainerTree(ElementsAre("-Album-", "-ABCD-", "-all-", "Album - Artist"))).WillOnce(Return(1));
    EXPECT_CALL(*commonScriptMock, addCdsObject(IsIdenticalMap(asAudioAllAudio), "43", UNDEFINED)).WillOnce(Return(0));

    EXPECT_CALL(*commonScriptMock, addContainerTree(ElementsAre("-Album-", "--all--", "Album - Artist"))).WillOnce(Return(1));
    EXPECT_CALL(*commonScriptMock, addCdsObject(IsIdenticalMap(asAudioAllAudio), "44", UNDEFINED)).WillOnce(Return(0));

    // ARTIST //
    EXPECT_CALL(*commonScriptMock, addContainerTree(ElementsAre("-Artist-", "--all--", "Artist"))).WillOnce(Return(1));
    EXPECT_CALL(*commonScriptMock, addCdsObject(IsIdenticalMap(asAudioAllArtistTitle), "45", UNDEFINED)).WillOnce(Return(0));

    EXPECT_CALL(*commonScriptMock, addContainerTree(ElementsAre("-Artist-", "-ABCD-", "-all-", "Artist"))).WillOnce(Return(1));
    EXPECT_CALL(*commonScriptMock, addCdsObject(IsIdenticalMap(asAudioAllArtistTitle), "46", UNDEFINED)).WillOnce(Return(0));

    EXPECT_CALL(*commonScriptMock, abcBox(Eq("Artist"), Eq(9), Eq("-"))).WillOnce(Return(1));
    EXPECT_CALL(*commonScriptMock, addContainerTree(ElementsAre("-Artist-", "-ABCD-", "A", "Artist", "-all-"))).WillOnce(Return(1));
    EXPECT_CALL(*commonScriptMock, addCdsObject(IsIdenticalMap(asAudioAllArtistTitle), "47", UNDEFINED)).WillOnce(Return(0));

    EXPECT_CALL(*commonScriptMock, addContainerTree(ElementsAre("-Artist-", "-ABCD-", "A", "Artist", "Album (2018)"))).WillOnce(Return(1));
    EXPECT_CALL(*commonScriptMock, addCdsObject(IsIdenticalMap(asAudioAllAudio), "48", UNDEFINED)).WillOnce(Return(0));

    // GENRE //
    EXPECT_CALL(*commonScriptMock, addContainerTree(ElementsAre("-Genre-", "Genre", "--all--"))).WillOnce(Return(1));
    EXPECT_CALL(*commonScriptMock, addContainerTree(ElementsAre("-Genre-", "Genre2", "--all--"))).WillOnce(Return(1));
    EXPECT_CALL(*commonScriptMock, addCdsObject(IsIdenticalMap(asAudioAllAudioTitleArtist), "491", UNDEFINED)).WillOnce(Return(0));
    EXPECT_CALL(*commonScriptMock, addCdsObject(IsIdenticalMap(asAudioAllAudioTitleArtist), "492", UNDEFINED)).WillOnce(Return(0));

    EXPECT_CALL(*commonScriptMock, abcBox(Eq("Artist"), Eq(26), Eq("-"))).WillOnce(Return(1));
    EXPECT_CALL(*commonScriptMock, addContainerTree(ElementsAre("-Genre-", "Genre", "-A-", "Album - Artist"))).WillOnce(Return(1));
    EXPECT_CALL(*commonScriptMock, addContainerTree(ElementsAre("-Genre-", "Genre2", "-A-", "Album - Artist"))).WillOnce(Return(1));
    EXPECT_CALL(*commonScriptMock, addCdsObject(IsIdenticalMap(asAudioAllAudioTitleArtist), "501", UNDEFINED)).WillOnce(Return(0));
    EXPECT_CALL(*commonScriptMock, addCdsObject(IsIdenticalMap(asAudioAllAudioTitleArtist), "502", UNDEFINED)).WillOnce(Return(0));

    // TRACKS //
    EXPECT_CALL(*commonScriptMock, abcBox(Eq("Audio Title"), Eq(6), Eq("-"))).WillOnce(Return(1));
    EXPECT_CALL(*commonScriptMock, addContainerTree(ElementsAre("-Track-", "-ABCD-", "A"))).WillOnce(Return(1));
    EXPECT_CALL(*commonScriptMock, addCdsObject(IsIdenticalMap(asAudioTrackArtistTitle), "51", UNDEFINED)).WillOnce(Return(0));

    EXPECT_CALL(*commonScriptMock, addContainerTree(ElementsAre("-Track-", "--all--"))).WillOnce(Return(1));
    EXPECT_CALL(*commonScriptMock, addCdsObject(IsIdenticalMap(asAudioAllAudioTitleArtist), "52", UNDEFINED)).WillOnce(Return(0));

    // DECADES //
    EXPECT_CALL(*commonScriptMock, addContainerTree(ElementsAre("-Year-", "2010 - 2019", "-all-"))).WillOnce(Return(1));
    EXPECT_CALL(*commonScriptMock, addCdsObject(IsIdenticalMap(asAudioAllAudioTitleArtist), "53", UNDEFINED)).WillOnce(Return(0));

    EXPECT_CALL(*commonScriptMock, addContainerTree(ElementsAre("-Year-", "2010 - 2019", "2018", "-all-"))).WillOnce(Return(1));
    EXPECT_CALL(*commonScriptMock, addCdsObject(IsIdenticalMap(asAudioAllAudioTitleArtist), "54", UNDEFINED)).WillOnce(Return(0));

    EXPECT_CALL(*commonScriptMock, addContainerTree(ElementsAre("-Year-", "2010 - 2019", "2018", "Artist", "Album"))).WillOnce(Return(1));
    EXPECT_CALL(*commonScriptMock, addCdsObject(IsIdenticalMap(asAudioAllAudio), "55", UNDEFINED)).WillOnce(Return(0));

    addGlobalFunctions(ctx, js_global_functions, { { "/import/scripting/virtual-layout/attribute::audio-layout", audioLayout }, }, audioBox);

    dukMockItem(ctx, mimetype, UPNP_CLASS_AUDIO_ITEM, id, theora, title, meta, aux, res, location, onlineService);
    executeScript(ctx);
}

#endif //HAVE_JS
