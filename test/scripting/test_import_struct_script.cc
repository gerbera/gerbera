#ifdef HAVE_JS

#include <duktape.h>
#include <gmock/gmock.h>
#include <gtest/gtest.h>
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

class ImportStructuredScriptTest : public ScriptTestFixture {
public:
    ImportStructuredScriptTest()
    {
        commonScriptMock = std::make_unique<::testing::NiceMock<CommonScriptMock>>();
        scriptName = "import.js";
        audioLayout = "Structured";
    }

    ~ImportStructuredScriptTest() override
    {
        commonScriptMock.reset();
    }

    // As Duktape requires static methods, so must the mock expectations be
    static std::unique_ptr<CommonScriptMock> commonScriptMock;
};

std::unique_ptr<CommonScriptMock> ImportStructuredScriptTest::commonScriptMock;

static duk_ret_t print(duk_context* ctx)
{
    std::string msg = ScriptTestFixture::print(ctx);
    return ImportStructuredScriptTest::commonScriptMock->print(msg);
}

static duk_ret_t getPlaylistType(duk_context* ctx)
{
    std::string playlistMimeType = ScriptTestFixture::getPlaylistType(ctx);
    return ImportStructuredScriptTest::commonScriptMock->getPlaylistType(playlistMimeType);
}

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
        { "/-Genre-/Genre/--all--", "49" },
        { "/-Genre-/Genre/-A-/Album - Artist", "50" },
        { "/-Track-/-ABCD-/A", "51" },
        { "/-Track-/--all--", "52" },
        { "/-Year-/2010 - 2019/-all-", "53" },
        { "/-Year-/2010 - 2019/2018/-all-", "54" },
        { "/-Year-/2010 - 2019/2018/Artist/Album", "55" },
    };
    std::vector<std::string> tree = ScriptTestFixture::addContainerTree(ctx, map);
    return ImportStructuredScriptTest::commonScriptMock->addContainerTree(tree);
}

static duk_ret_t createContainerChain(duk_context* ctx)
{
    std::vector<std::string> array = ScriptTestFixture::createContainerChain(ctx);
    return ImportStructuredScriptTest::commonScriptMock->createContainerChain(array);
}

static duk_ret_t getLastPath(duk_context* ctx)
{
    std::string inputPath = ScriptTestFixture::getLastPath(ctx);
    return ImportStructuredScriptTest::commonScriptMock->getLastPath(inputPath);
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
    { "print", print, DUK_VARARGS },
    { "getPlaylistType", getPlaylistType, 1 },
    { "createContainerChain", createContainerChain, 1 },
    { "getLastPath", getLastPath, 1 },
    { "addCdsObject", addCdsObject, 3 },
    { "getYear", getYear, 1 },
    { "getRootPath", getRootPath, 2 },
    { "abcbox", abcBox, 3 },
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
    std::string desc = "Description";
    std::string id = "2";
    std::string location = "/home/gerbera/audio.mp3";
    int online_service = 0;
    int theora = 0;
    std::map<std::string, std::string> aux;
    std::map<std::string, std::string> res;

    std::map<std::string, std::string> meta {
        { "dc:title", title },
        { "upnp:artist", artist },
        { "upnp:album", album },
        { "dc:date", date },
        { "upnp:date", year },
        { "upnp:genre", genre },
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
        { "metaData['upnp:genre']", genre },
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
    EXPECT_CALL(*commonScriptMock, addCdsObject(IsIdenticalMap(asAudioAllAudioTitleArtist), "49", UNDEFINED)).WillOnce(Return(0));

    EXPECT_CALL(*commonScriptMock, abcBox(Eq("Artist"), Eq(26), Eq("-"))).WillOnce(Return(1));
    EXPECT_CALL(*commonScriptMock, addContainerTree(ElementsAre("-Genre-", "Genre", "-A-", "Album - Artist"))).WillOnce(Return(1));
    EXPECT_CALL(*commonScriptMock, addCdsObject(IsIdenticalMap(asAudioAllAudioTitleArtist), "50", UNDEFINED)).WillOnce(Return(0));

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

    addGlobalFunctions(ctx, js_global_functions, { { "/import/scripting/virtual-layout/attribute::audio-layout", audioLayout }, { "/import/scripting/virtual-layout/structured-layout/attribute::genre-box", "26" } });

    dukMockItem(ctx, mimetype, id, theora, title, meta, aux, res, location, online_service);
    executeScript(ctx);
}

#endif //HAVE_JS
