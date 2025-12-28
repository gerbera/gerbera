/*GRB*

    Gerbera - https://gerbera.io/

    test_import_community_scripts.cc - this file is part of Gerbera.

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

#include "config/result/autoscan.h"

#include "mock/common_script_mock.h"
#include "mock/duk_helper.h"
#include "mock/script_test_fixture.h"

#include <duktape.h>
#include <fmt/format.h>
#include <memory>

class ImportCommunityScriptsTest : public CommonScriptTestFixture {
public:
    ImportCommunityScriptsTest()
    {
        functionName = "importAudioClassical";
    }
};

static duk_ret_t addContainerTree(duk_context* ctx)
{
    std::map<std::string, std::string> map {
        { "", "0" },
        { "/-Classical-/-Artist-/T/TH/TheArtist/AnOldComposer/Recording", "42" },
        { "/-Classical-/-Album-/Recording/AnOldComposer", "43" },
        { "/-Classical-/-Genre-/Barock/AnOldComposer/Recording", "44" },
        { "/-Classical-/-Genre-/Barock2/AnOldComposer/Recording", "45" },
        { "/-Classical-/-Composer-/A/AnOldComposer/Barock2/Recording/TheArtist", "46" },
        { "/subdir/folder", "50" },
    };
    std::vector<std::string> tree = ScriptTestFixture::addContainerTree(ctx, map);
    return ImportCommunityScriptsTest::commonScriptMock->addContainerTree(tree);
}

static duk_ret_t addCdsObject(duk_context* ctx)
{
    std::vector<std::string> keys = {
        "title",
        "metaData['dc:title']",
        "metaData['upnp:artist']",
        "metaData['upnp:album']",
        "metaData['upnp:composer']",
        "metaData['dc:date']",
        "metaData['upnp:date']",
        "metaData['upnp:genre']",
        "metaData['dc:description']"
    };
    addCdsObjectParams params = ScriptTestFixture::addCdsObject(ctx, keys);
    return ImportCommunityScriptsTest::commonScriptMock->addCdsObject(params.objectValues, params.containerChain, params.rootPath);
}

static duk_ret_t getYear(duk_context* ctx)
{
    std::string date = ScriptTestFixture::getYear(ctx);
    return ImportCommunityScriptsTest::commonScriptMock->getYear(date);
}

static duk_ret_t getRootPath(duk_context* ctx)
{
    getRootPathParams params = ScriptTestFixture::getRootPath(ctx);
    return ImportCommunityScriptsTest::commonScriptMock->getRootPath(params.objScriptPath, params.origObjLocation);
}

static duk_ret_t abcBox(duk_context* ctx)
{
    abcBoxParams params = ScriptTestFixture::abcBox(ctx);
    return ImportCommunityScriptsTest::commonScriptMock->abcBox(params.inputValue, params.boxType, params.divChar);
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
    { "Audio/audioRoot", "-Classical-", "object.container", true, 6 },
    { "Audio/allAlbums", "-Album-", "object.container", true, 6 },
    { "Audio/allArtists", "-Artist-", "object.container", true, 9 },
    { "Audio/allGenres", "-Genre-", "object.container", true, 26 },
    { "Audio/allComposers", "-Composer-", "object.container", true, 6 },
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

TEST_F(ImportCommunityScriptsTest, CreatesDukContextWithImportScript)
{
    EXPECT_NE(ctx, nullptr);
}

TEST_F(ImportCommunityScriptsTest, AddsAudioItemClassical)
{
    std::string title = "Audio Title";
    std::string mimetype = "audio/mpeg";
    std::string artist = "TheArtist";
    std::string album = "Recording";
    std::string date = "2018-01-01";
    std::string year = "2018";
    std::string genre = "Barock";
    std::string genre2 = "Barock2";
    std::string composer = "AnOldComposer";
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
        { "upnp:composer", composer },
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
        { "metaData['upnp:composer']", composer },
        { "metaData['dc:description']", desc },
    };

    EXPECT_CALL(*commonScriptMock, getRootPath(Eq("/home/gerbera"), Eq("/home/gerbera/audio.mp3"))).WillOnce(Return(1));

    EXPECT_CALL(*commonScriptMock, getYear(Eq("2018-01-01"))).WillOnce(Return(1));

    EXPECT_CALL(*commonScriptMock, addContainerTree(ElementsAre("-Classical-", "-Artist-", "T", "TH", "TheArtist", "AnOldComposer", "Recording"))).WillOnce(Return(1));
    EXPECT_CALL(*commonScriptMock, addCdsObject(IsIdenticalMap(asAudioAllAudio), "42", "/home/gerbera")).WillOnce(Return(1));

    EXPECT_CALL(*commonScriptMock, addContainerTree(ElementsAre("-Classical-", "-Album-", "Recording", "AnOldComposer"))).WillOnce(Return(1));
    EXPECT_CALL(*commonScriptMock, addCdsObject(IsIdenticalMap(asAudioAllAudio), "43", "/home/gerbera")).WillOnce(Return(1));

    EXPECT_CALL(*commonScriptMock, addContainerTree(ElementsAre("-Classical-", "-Genre-", "Barock", "AnOldComposer", "Recording"))).WillOnce(Return(1));
    EXPECT_CALL(*commonScriptMock, addCdsObject(IsIdenticalMap(asAudioAllAudio), "44", "/home/gerbera")).WillOnce(Return(1));

    EXPECT_CALL(*commonScriptMock, addContainerTree(ElementsAre("-Classical-", "-Genre-", "Barock2", "AnOldComposer", "Recording"))).WillOnce(Return(1));
    EXPECT_CALL(*commonScriptMock, addCdsObject(IsIdenticalMap(asAudioAllAudio), "45", "/home/gerbera")).WillOnce(Return(1));

    EXPECT_CALL(*commonScriptMock, addContainerTree(ElementsAre("-Classical-", "-Composer-", "A", "AnOldComposer", "Barock2", "Recording", "TheArtist"))).WillOnce(Return(1));
    EXPECT_CALL(*commonScriptMock, addCdsObject(IsIdenticalMap(asAudioAllAudio), "46", "/home/gerbera")).WillOnce(Return(1));

    addGlobalFunctions(ctx, js_global_functions, {},
        audioBox);

    auto fnResult = callFunction(ctx, dukMockItem,
        { { "title", title },
            { "id", id },
            { "upnpclass", UPNP_CLASS_AUDIO_ITEM },
            { "location", location },
            { "onlineService", fmt::to_string(onlineService) },
            { "theora", fmt::to_string(theora) },
            { "mimetype", mimetype } },
        meta, aux, res,
        AutoscanDirectory::ContainerTypesDefaults.at(AutoscanMediaMode::Audio),
        "/home/gerbera");
    std::vector<int> items { 42000, 43000, 44000, 45000, 46000 };
    EXPECT_EQ(fnResult, items);
}

TEST_F(ImportCommunityScriptsTest, AddsAudioLikeFilesystem)
{
    std::string title = "Audio Title";
    std::string mimetype = "audio/mpeg";
    std::string artist = "TheArtist";
    std::string album = "Recording";
    std::string date = "2018-01-01";
    std::string year = "2018";
    std::string genre = "Genre";
    std::string genre2 = "Genre2";
    std::string desc = "Description";
    std::string id = "2";
    std::string location = "/home/gerbera/subdir/folder/audio.mp3";
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

    // Expecting the common script calls
    // and will proxy through the mock objects
    // for verification.
    EXPECT_CALL(*commonScriptMock, getRootPath(Eq("/home/gerbera"), Eq("/home/gerbera/subdir/folder/audio.mp3"))).WillOnce(Return(1));

    EXPECT_CALL(*commonScriptMock, addContainerTree(ElementsAre("subdir", "folder"))).WillOnce(Return(1));
    EXPECT_CALL(*commonScriptMock, addCdsObject(IsIdenticalMap(asAudioAllAudio), "50", "/home/gerbera")).WillOnce(Return(1));

    addGlobalFunctions(ctx, js_global_functions, {},
        audioBox);
    functionName = "importFsContainers";

    auto fnResult = callFunction(ctx, dukMockItem,
        { { "title", title },
            { "id", id },
            { "upnpclass", UPNP_CLASS_AUDIO_ITEM },
            { "location", location },
            { "onlineService", fmt::to_string(onlineService) },
            { "theora", fmt::to_string(theora) },
            { "mimetype", mimetype } },
        meta, aux, res,
        AutoscanDirectory::ContainerTypesDefaults.at(AutoscanMediaMode::Audio),
        "/home/gerbera");
    std::vector<int> items { 50000 };
    EXPECT_EQ(fnResult, items);
}

#endif // HAVE_JS
