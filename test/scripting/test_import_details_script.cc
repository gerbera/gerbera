/*GRB*

    Gerbera - https://gerbera.io/

    test_import_details_script.cc - this file is part of Gerbera.

    Copyright (C) 2025-2026 Gerbera Contributors

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

class ImportDetailsScriptTest : public CommonScriptTestFixture {
public:
    ImportDetailsScriptTest()
    {
        functionName = "importImageDetail";
    }
};

static duk_ret_t addContainerTree(duk_context* ctx)
{
    std::map<std::string, std::string> map {
        { "", "0" },
        { "/Video/All Video", "60" },
        { "/Video/Directories/home/gerbera", "61" },
        { "/Photos/All Photos", "70" },
        { "/Photos/Year/2025/01", "71" },
        { "/Photos/Year+Month/2025/01", "711" },
        { "/Photos/Year+Date/2025/2025-01-01", "712" },
        { "/Photos/Date/2025-01-01", "72" },
        { "/Photos/Directories/home/gerbera", "73" },
        { "/Photos/Model/Grb/2025/01", "74" },
        { "/Topics/TheTopic/hl 2025/2025-01", "75" },
        { "/Topics/thisTopic/hl 2025/2025-01", "752" },
        { "/Topics/gerbera/hl 2025/2025-01", "751" },
        { "/Topics/thisTopic/Rocks/2025-01", "76" },
        { "/Topics/gerbera/Rocks/2025-01", "761" },
    };
    std::vector<std::string> tree = ScriptTestFixture::addContainerTree(ctx, map);
    return ImportDetailsScriptTest::commonScriptMock->addContainerTree(tree);
}

static duk_ret_t mapModel(duk_context* ctx)
{
    std::map<std::string, std::string> map = {
        { "Mediatomb", "Mt" },
        { "Gerbera", "Grb" },
    };
    auto result = ScriptTestFixture::mapModel(ctx, map);
    return ImportDetailsScriptTest::commonScriptMock->mapModel(result);
}

static duk_ret_t addCdsObject(duk_context* ctx)
{
    std::vector<std::string> keys = {
        "title",
        "metaData['dc:title']",
        "metaData['upnp:artist']",
        "metaData['upnp:composer']",
        "metaData['dc:date']",
        "metaData['upnp:date']",
        "metaData['dc:description']"
    };
    addCdsObjectParams params = ScriptTestFixture::addCdsObject(ctx, keys);
    return ImportDetailsScriptTest::commonScriptMock->addCdsObject(params.objectValues, params.containerChain, params.rootPath);
}

static duk_ret_t getYear(duk_context* ctx)
{
    std::string date = ScriptTestFixture::getYear(ctx);
    return ImportDetailsScriptTest::commonScriptMock->getYear(date);
}

static duk_ret_t getRootPath(duk_context* ctx)
{
    getRootPathParams params = ScriptTestFixture::getRootPath(ctx);
    return ImportDetailsScriptTest::commonScriptMock->getRootPath(params.objScriptPath, params.origObjLocation);
}

// Mock the Duktape C methods
// that are called from the import.js script
// * These are static methods, which makes mocking difficult.
static duk_function_list_entry js_global_functions[] = {
    { "print", CommonScriptTestFixture::js_print, DUK_VARARGS },
    { "print2", CommonScriptTestFixture::js_print2, DUK_VARARGS },
    { "createContainerChain", CommonScriptTestFixture::js_createContainerChain, 1 },
    { "getLastPath", CommonScriptTestFixture::js_getLastPath, 1 },
    { "mapModel", mapModel, 1 },
    { "addCdsObject", addCdsObject, 3 },
    { "getYear", getYear, 1 },
    { "getRootPath", getRootPath, 2 },
    { "addContainerTree", addContainerTree, 1 },
    { nullptr, nullptr, 0 },
};
static const std::vector<boxConfig> detailsBox {
    { "Image/allDates", "Date", "object.container" },
    { "Image/allDirectories", "Directories", "object.container" },
    { "Image/allImages", "All Photos", "object.container" },
    { "Image/allYears", "Year", "object.container" },
    { "Image/imageRoot", "Photos", "object.container" },
    { "Image/unknown", "Unknown", "object.container" },
    { "Video/allDates", "Date", "object.container" },
    { "Video/allDirectories", "Directories", "object.container" },
    { "Video/allVideo", "All Video", "object.container" },
    { "Video/allYears", "Year", "object.container" },
    { "Video/unknown", "Unknown", "object.container" },
    { "Video/videoRoot", "Video", "object.container" },
    { "Video/allDates", "Date", "object.container" },
    { "ImageDetail/allModels", "Model", "object.container.album.photoAlbum" },
    { "ImageDetail/yearMonth", "Year+Month", "object.container.album.photoAlbum" },
    { "ImageDetail/yearDate", "Year+Date", "object.container.album.photoAlbum" },
    { "Topic/topicRoot", "Topics", "object.container" },
    { "Topic/topic", "TheTopic", "object.container.album.photoAlbum" },
    { "Topic/topicExtra", "Selection", "object.container.album.photoAlbum" },
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

TEST_F(ImportDetailsScriptTest, CreatesDukContextWithImportScript)
{
    EXPECT_NE(ctx, nullptr);
}

TEST_F(ImportDetailsScriptTest, AddsImageWithDetails)
{
    std::string title = "Image Title";
    std::string mimetype = "image/jpeg";
    std::string artist = "Photographer";
    std::string date = "2025-01-01";
    std::string year = "2025";
    std::string desc = "Headline 2025";
    std::string id = "2";
    std::string location = "/home/gerbera/home/gerbera/image.jpg";
    std::map<std::string, std::string> aux {
        { "EXIF_TAG_MODEL", "Gerbera" },
        { "EXIF_TAG_DATE_TIME_ORIGINAL", "2025:01:01 19:42:00" },
        { "EXIF_TAG_IMAGE_DESCRIPTION", desc },
        { "EXIF_TAG_ARTIST", artist },
        { "Xmp.photoshop.Headline", "" },
    };
    std::map<std::string, std::string> res;

    std::vector<std::pair<std::string, std::string>> meta {
        { "dc:title", title },
        { "upnp:artist", artist },
        { "dc:date", date },
        { "upnp:date", year },
        { "dc:description", desc },
    };

    // Represents the values passed to `addCdsObject`, extracted from keys defined there.
    std::map<std::string, std::string> asImageAllImage {
        { "title", title },
        { "metaData['dc:title']", title },
        { "metaData['upnp:artist']", artist },
        { "metaData['dc:date']", date },
        { "metaData['upnp:date']", year },
        { "metaData['dc:description']", desc },
    };

    std::map<std::string_view, std::map<std::string_view, std::string_view>> configDicts {
        { "/import/scripting/virtual-layout/script-options/script-option", { { "topicFromPath", "gerbera" } } },
    };

    std::map<std::string, std::string> asImageAllFullName;
    asImageAllFullName.insert(asImageAllImage.begin(), asImageAllImage.end());
    asImageAllFullName["title"] = "2025-01-01-19-42-00-Grb-Image Title";

    std::map<std::string, std::string> asImageAllIModel;
    asImageAllIModel.insert(asImageAllImage.begin(), asImageAllImage.end());
    asImageAllIModel["title"] = "2025-01-01-19-42-00-Image Title";

    // Expecting the common script calls
    // and will proxy through the mock objects
    // for verification.
    EXPECT_CALL(*commonScriptMock, mapModel(Eq("Grb"))).WillOnce(Return(1));
    EXPECT_CALL(*commonScriptMock, getRootPath(Eq("/home/gerbera"), Eq(location))).WillRepeatedly(Return(1));

    EXPECT_CALL(*commonScriptMock, addContainerTree(ElementsAre("Photos", "All Photos"))).WillOnce(Return(1));
    EXPECT_CALL(*commonScriptMock, addCdsObject(IsIdenticalMap(asImageAllImage), "70", "/home/gerbera")).WillOnce(Return(1));

    EXPECT_CALL(*commonScriptMock, addContainerTree(ElementsAre("Photos", "Directories", "home", "gerbera"))).WillOnce(Return(1));
    EXPECT_CALL(*commonScriptMock, addCdsObject(IsIdenticalMap(asImageAllImage), "73", "/home/gerbera")).WillOnce(Return(1));

    EXPECT_CALL(*commonScriptMock, addContainerTree(ElementsAre("Photos", "Year+Month", "2025", "01"))).WillOnce(Return(1));
    EXPECT_CALL(*commonScriptMock, addCdsObject(IsIdenticalMap(asImageAllFullName), "711", "/home/gerbera")).WillOnce(Return(1));

    EXPECT_CALL(*commonScriptMock, addContainerTree(ElementsAre("Photos", "Year+Date", "2025", "2025-01-01"))).WillOnce(Return(1));
    EXPECT_CALL(*commonScriptMock, addCdsObject(IsIdenticalMap(asImageAllFullName), "712", "/home/gerbera")).WillOnce(Return(1));

    // MODEL //
    EXPECT_CALL(*commonScriptMock, addContainerTree(ElementsAre("Photos", "Model", "Grb", "2025", "01"))).WillOnce(Return(1));
    EXPECT_CALL(*commonScriptMock, addCdsObject(IsIdenticalMap(asImageAllIModel), "74", "/home/gerbera")).WillOnce(Return(1));

    addGlobalFunctions(ctx, js_global_functions, {},
        detailsBox, configDicts);

    auto fnResult = callFunction(ctx, dukMockItem,
        { { "title", title },
            { "id", id },
            { "upnpclass", UPNP_CLASS_IMAGE_ITEM },
            { "location", location },
            { "mimetype", mimetype } },
        meta, aux, res,
        AutoscanDirectory::ContainerTypesDefaults.at(AutoscanMediaMode::Audio),
        "/home/gerbera");
    std::vector<int> items { 70000, 73000, 711000, 712000, 74000 };
    EXPECT_EQ(fnResult, items);
}

TEST_F(ImportDetailsScriptTest, AddsImageWithDetailsAndTopics)
{
    std::string title = "Image Title";
    std::string mimetype = "image/jpeg";
    std::string artist = "Photographer";
    std::string date = "2025-01-01";
    std::string year = "2025";
    std::string desc = "Headline 2025";
    std::string id = "2";
    std::string location = "/home/gerbera/home/gerbera/image.jpg";
    std::map<std::string, std::string> aux {
        { "EXIF_TAG_MODEL", "Gerbera" },
        { "EXIF_TAG_DATE_TIME_ORIGINAL", "2025:01:01 19:42:00" },
        { "EXIF_TAG_IMAGE_DESCRIPTION", desc },
        { "EXIF_TAG_ARTIST", artist },
        { "Xmp.photoshop.Headline", "" },
    };
    std::map<std::string, std::string> res;

    std::vector<std::pair<std::string, std::string>> meta {
        { "dc:title", title },
        { "upnp:artist", artist },
        { "dc:date", date },
        { "upnp:date", year },
        { "dc:description", desc },
    };

    // Represents the values passed to `addCdsObject`, extracted from keys defined there.
    std::map<std::string, std::string> asImageAllImage {
        { "title", title },
        { "metaData['dc:title']", title },
        { "metaData['upnp:artist']", artist },
        { "metaData['dc:date']", date },
        { "metaData['upnp:date']", year },
        { "metaData['dc:description']", desc },
    };

    std::map<std::string_view, std::map<std::string_view, std::string_view>> configDicts {};

    std::map<std::string_view, std::vector<std::vector<std::pair<std::string_view, std::string_view>>>> configVects {
        {
            "/import/scripting/virtual-layout/headline-map/headline",
            {
                { { "from", "Headline" }, { "to", "hl" } },
            },
        },
    };

    std::map<std::string, std::string> asImageAllFullName;
    asImageAllFullName.insert(asImageAllImage.begin(), asImageAllImage.end());
    asImageAllFullName["title"] = "2025-01-01-19-42-00-Grb-Image Title";

    std::map<std::string, std::string> asImageAllModel;
    asImageAllModel.insert(asImageAllImage.begin(), asImageAllImage.end());
    asImageAllModel["title"] = "2025-01-01-19-42-00-Image Title";

    // Expecting the common script calls
    // and will proxy through the mock objects
    // for verification.
    EXPECT_CALL(*commonScriptMock, mapModel(Eq("Grb"))).WillOnce(Return(1));
    EXPECT_CALL(*commonScriptMock, getRootPath(Eq("/home/gerbera"), Eq(location))).WillRepeatedly(Return(1));

    EXPECT_CALL(*commonScriptMock, addContainerTree(ElementsAre("Photos", "All Photos"))).WillOnce(Return(1));
    EXPECT_CALL(*commonScriptMock, addCdsObject(IsIdenticalMap(asImageAllImage), "70", "/home/gerbera")).WillOnce(Return(1));

    EXPECT_CALL(*commonScriptMock, addContainerTree(ElementsAre("Photos", "Directories", "home", "gerbera"))).WillOnce(Return(1));
    EXPECT_CALL(*commonScriptMock, addCdsObject(IsIdenticalMap(asImageAllImage), "73", "/home/gerbera")).WillOnce(Return(1));

    EXPECT_CALL(*commonScriptMock, addContainerTree(ElementsAre("Photos", "Year+Month", "2025", "01"))).WillOnce(Return(1));
    EXPECT_CALL(*commonScriptMock, addCdsObject(IsIdenticalMap(asImageAllFullName), "711", "/home/gerbera")).WillOnce(Return(1));

    EXPECT_CALL(*commonScriptMock, addContainerTree(ElementsAre("Photos", "Year+Date", "2025", "2025-01-01"))).WillOnce(Return(1));
    EXPECT_CALL(*commonScriptMock, addCdsObject(IsIdenticalMap(asImageAllFullName), "712", "/home/gerbera")).WillOnce(Return(1));

    // MODEL //
    EXPECT_CALL(*commonScriptMock, addContainerTree(ElementsAre("Photos", "Model", "Grb", "2025", "01"))).WillOnce(Return(1));
    EXPECT_CALL(*commonScriptMock, addCdsObject(IsIdenticalMap(asImageAllModel), "74", "/home/gerbera")).WillOnce(Return(1));

    // TOPIC //
    EXPECT_CALL(*commonScriptMock, addContainerTree(ElementsAre("Topics", "TheTopic", "hl 2025", "2025-01"))).WillOnce(Return(1));
    EXPECT_CALL(*commonScriptMock, addCdsObject(IsIdenticalMap(asImageAllFullName), "75", "/home/gerbera")).WillOnce(Return(1));

    addGlobalFunctions(ctx, js_global_functions, {},
        detailsBox, configDicts, configVects);

    auto fnResult = callFunction(ctx, dukMockItem,
        { { "title", title },
            { "id", id },
            { "upnpclass", UPNP_CLASS_IMAGE_ITEM },
            { "location", location },
            { "mimetype", mimetype } },
        meta, aux, res,
        AutoscanDirectory::ContainerTypesDefaults.at(AutoscanMediaMode::Audio),
        "/home/gerbera");
    std::vector<int> items { 70000, 73000, 711000, 75000, 712000, 74000 };
    EXPECT_EQ(fnResult, items);
}

TEST_F(ImportDetailsScriptTest, AddsImageWithDetailsAndTopicsMapped)
{
    std::string title = "Image Title";
    std::string mimetype = "image/jpeg";
    std::string artist = "Photographer";
    std::string date = "2025-01-01";
    std::string year = "2025";
    std::string desc = "Headline 2025";
    std::string id = "2";
    std::string location = "/home/gerbera/home/gerbera/image.jpg";
    std::map<std::string, std::string> aux {
        { "EXIF_TAG_MODEL", "Gerbera" },
        { "EXIF_TAG_DATE_TIME_ORIGINAL", "2025:01:01 19:42:00" },
        { "EXIF_TAG_IMAGE_DESCRIPTION", desc },
        { "EXIF_TAG_ARTIST", artist },
        { "Xmp.photoshop.Headline", "" },
    };
    std::map<std::string, std::string> res;

    std::vector<std::pair<std::string, std::string>> meta {
        { "dc:title", title },
        { "upnp:artist", artist },
        { "dc:date", date },
        { "upnp:date", year },
        { "dc:description", desc },
    };

    // Represents the values passed to `addCdsObject`, extracted from keys defined there.
    std::map<std::string, std::string> asImageAllImage {
        { "title", title },
        { "metaData['dc:title']", title },
        { "metaData['upnp:artist']", artist },
        { "metaData['dc:date']", date },
        { "metaData['upnp:date']", year },
        { "metaData['dc:description']", desc },
    };

    std::map<std::string_view, std::map<std::string_view, std::string_view>> configDicts {};

    std::map<std::string_view, std::vector<std::vector<std::pair<std::string_view, std::string_view>>>> configVects {
        {
            "/import/scripting/virtual-layout/headline-map/headline",
            {
                { { "from", "Headline" }, { "to", "hl" } },
                { { "from", "Headline.*" }, { "to", "thisTopic" }, { "type", "topic" } },
            },
        },
    };

    std::map<std::string, std::string> asImageAllFullName;
    asImageAllFullName.insert(asImageAllImage.begin(), asImageAllImage.end());
    asImageAllFullName["title"] = "2025-01-01-19-42-00-Grb-Image Title";

    std::map<std::string, std::string> asImageAllModel;
    asImageAllModel.insert(asImageAllImage.begin(), asImageAllImage.end());
    asImageAllModel["title"] = "2025-01-01-19-42-00-Image Title";

    // Expecting the common script calls
    // and will proxy through the mock objects
    // for verification.
    EXPECT_CALL(*commonScriptMock, mapModel(Eq("Grb"))).WillOnce(Return(1));
    EXPECT_CALL(*commonScriptMock, getRootPath(Eq("/home/gerbera"), Eq(location))).WillRepeatedly(Return(1));

    EXPECT_CALL(*commonScriptMock, addContainerTree(ElementsAre("Photos", "All Photos"))).WillOnce(Return(1));
    EXPECT_CALL(*commonScriptMock, addCdsObject(IsIdenticalMap(asImageAllImage), "70", "/home/gerbera")).WillOnce(Return(1));

    EXPECT_CALL(*commonScriptMock, addContainerTree(ElementsAre("Photos", "Directories", "home", "gerbera"))).WillOnce(Return(1));
    EXPECT_CALL(*commonScriptMock, addCdsObject(IsIdenticalMap(asImageAllImage), "73", "/home/gerbera")).WillOnce(Return(1));

    EXPECT_CALL(*commonScriptMock, addContainerTree(ElementsAre("Photos", "Year+Month", "2025", "01"))).WillOnce(Return(1));
    EXPECT_CALL(*commonScriptMock, addCdsObject(IsIdenticalMap(asImageAllFullName), "711", "/home/gerbera")).WillOnce(Return(1));

    EXPECT_CALL(*commonScriptMock, addContainerTree(ElementsAre("Photos", "Year+Date", "2025", "2025-01-01"))).WillOnce(Return(1));
    EXPECT_CALL(*commonScriptMock, addCdsObject(IsIdenticalMap(asImageAllFullName), "712", "/home/gerbera")).WillOnce(Return(1));

    // MODEL //
    EXPECT_CALL(*commonScriptMock, addContainerTree(ElementsAre("Photos", "Model", "Grb", "2025", "01"))).WillOnce(Return(1));
    EXPECT_CALL(*commonScriptMock, addCdsObject(IsIdenticalMap(asImageAllModel), "74", "/home/gerbera")).WillOnce(Return(1));

    // TOPIC //
    EXPECT_CALL(*commonScriptMock, addContainerTree(ElementsAre("Topics", "thisTopic", "hl 2025", "2025-01"))).WillOnce(Return(1));
    EXPECT_CALL(*commonScriptMock, addCdsObject(IsIdenticalMap(asImageAllFullName), "752", "/home/gerbera")).WillOnce(Return(1));

    addGlobalFunctions(ctx, js_global_functions, {},
        detailsBox, configDicts, configVects);

    auto fnResult = callFunction(ctx, dukMockItem,
        { { "title", title },
            { "id", id },
            { "upnpclass", UPNP_CLASS_IMAGE_ITEM },
            { "location", location },
            { "mimetype", mimetype } },
        meta, aux, res,
        AutoscanDirectory::ContainerTypesDefaults.at(AutoscanMediaMode::Audio),
        "/home/gerbera");
    std::vector<int> items { 70000, 73000, 711000, 752000, 712000, 74000 };
    EXPECT_EQ(fnResult, items);
}

TEST_F(ImportDetailsScriptTest, AddsImageWithDetailsAndTopicsFromPath)
{
    std::string title = "Image Title";
    std::string mimetype = "image/jpeg";
    std::string artist = "Photographer";
    std::string date = "2025-01-01";
    std::string year = "2025";
    std::string desc = "Headline 2025";
    std::string id = "2";
    std::string location = "/home/gerbera/home/gerbera/image.jpg";
    std::map<std::string, std::string> aux {
        { "EXIF_TAG_MODEL", "Gerbera" },
        { "EXIF_TAG_DATE_TIME_ORIGINAL", "2025:01:01 19:42:00" },
        { "EXIF_TAG_IMAGE_DESCRIPTION", desc },
        { "EXIF_TAG_ARTIST", artist },
        { "Xmp.photoshop.Headline", "" },
    };
    std::map<std::string, std::string> res;

    std::vector<std::pair<std::string, std::string>> meta {
        { "dc:title", title },
        { "upnp:artist", artist },
        { "dc:date", date },
        { "upnp:date", year },
        { "dc:description", desc },
    };

    // Represents the values passed to `addCdsObject`, extracted from keys defined there.
    std::map<std::string, std::string> asImageAllImage {
        { "title", title },
        { "metaData['dc:title']", title },
        { "metaData['upnp:artist']", artist },
        { "metaData['dc:date']", date },
        { "metaData['upnp:date']", year },
        { "metaData['dc:description']", desc },
    };

    std::map<std::string_view, std::map<std::string_view, std::string_view>> configDicts {
        { "/import/scripting/virtual-layout/script-options/script-option", { { "topicFromPath", "gerbera" } } },
    };

    std::map<std::string_view, std::vector<std::vector<std::pair<std::string_view, std::string_view>>>> configVects {
        {
            "/import/scripting/virtual-layout/headline-map/headline",
            {
                { { "from", "Headline" }, { "to", "hl" } },
            },
        },
    };

    std::map<std::string, std::string> asImageAllFullName;
    asImageAllFullName.insert(asImageAllImage.begin(), asImageAllImage.end());
    asImageAllFullName["title"] = "2025-01-01-19-42-00-Grb-Image Title";

    std::map<std::string, std::string> asImageAllModel;
    asImageAllModel.insert(asImageAllImage.begin(), asImageAllImage.end());
    asImageAllModel["title"] = "2025-01-01-19-42-00-Image Title";

    // Expecting the common script calls
    // and will proxy through the mock objects
    // for verification.
    EXPECT_CALL(*commonScriptMock, mapModel(Eq("Grb"))).WillOnce(Return(1));
    EXPECT_CALL(*commonScriptMock, getRootPath(Eq("/home/gerbera"), Eq(location))).WillRepeatedly(Return(1));

    EXPECT_CALL(*commonScriptMock, addContainerTree(ElementsAre("Photos", "All Photos"))).WillOnce(Return(1));
    EXPECT_CALL(*commonScriptMock, addCdsObject(IsIdenticalMap(asImageAllImage), "70", "/home/gerbera")).WillOnce(Return(1));

    EXPECT_CALL(*commonScriptMock, addContainerTree(ElementsAre("Photos", "Directories", "home", "gerbera"))).WillOnce(Return(1));
    EXPECT_CALL(*commonScriptMock, addCdsObject(IsIdenticalMap(asImageAllImage), "73", "/home/gerbera")).WillOnce(Return(1));

    EXPECT_CALL(*commonScriptMock, addContainerTree(ElementsAre("Photos", "Year+Month", "2025", "01"))).WillOnce(Return(1));
    EXPECT_CALL(*commonScriptMock, addCdsObject(IsIdenticalMap(asImageAllFullName), "711", "/home/gerbera")).WillOnce(Return(1));

    EXPECT_CALL(*commonScriptMock, addContainerTree(ElementsAre("Photos", "Year+Date", "2025", "2025-01-01"))).WillOnce(Return(1));
    EXPECT_CALL(*commonScriptMock, addCdsObject(IsIdenticalMap(asImageAllFullName), "712", "/home/gerbera")).WillOnce(Return(1));

    // MODEL //
    EXPECT_CALL(*commonScriptMock, addContainerTree(ElementsAre("Photos", "Model", "Grb", "2025", "01"))).WillOnce(Return(1));
    EXPECT_CALL(*commonScriptMock, addCdsObject(IsIdenticalMap(asImageAllModel), "74", "/home/gerbera")).WillOnce(Return(1));

    // TOPIC //
    EXPECT_CALL(*commonScriptMock, addContainerTree(ElementsAre("Topics", "gerbera", "hl 2025", "2025-01"))).WillOnce(Return(1));
    EXPECT_CALL(*commonScriptMock, addCdsObject(IsIdenticalMap(asImageAllFullName), "751", "/home/gerbera")).WillOnce(Return(1));

    addGlobalFunctions(ctx, js_global_functions, {},
        detailsBox, configDicts, configVects);

    auto fnResult = callFunction(ctx, dukMockItem,
        { { "title", title },
            { "id", id },
            { "upnpclass", UPNP_CLASS_IMAGE_ITEM },
            { "location", location },
            { "mimetype", mimetype } },
        meta, aux, res,
        AutoscanDirectory::ContainerTypesDefaults.at(AutoscanMediaMode::Audio),
        "/home/gerbera");
    std::vector<int> items { 70000, 73000, 711000, 751000, 712000, 74000 };
    EXPECT_EQ(fnResult, items);
}

TEST_F(ImportDetailsScriptTest, AddsVideoWithDetailsAndTopics)
{
    std::string title = "Video Title";
    std::string mimetype = "video/mp4";
    std::string artist = "Artist";
    std::string date = "2025-01-01";
    std::string year = "2025";
    std::string genre2 = "Mediatomb";
    std::string desc = "Video Plot";
    std::string id = "2";
    std::string location = "/home/gerbera/home/gerbera/video.mp4";
    std::map<std::string, std::string> res;

    std::vector<std::pair<std::string, std::string>> meta {
        { "dc:title", title },
        { "upnp:artist", artist },
        { "dc:date", date },
        { "upnp:date", year },
        { "upnp:genre", genre2 },
        { "dc:description", desc },
    };

    std::map<std::string, std::string> aux {
        { "NFO:model", "Gerbera" },
        { "NFO:topic", "supertop" },
        { "NFO:subtopic", "nosupertop" },
        { "NFO:createdate", "20250101094200" },
    };

    // Represents the values passed to `addCdsObject`, extracted from keys defined there.
    std::map<std::string, std::string> asVideoAllVideo {
        { "title", title },
        { "metaData['dc:title']", title },
        { "metaData['upnp:artist']", artist },
        { "metaData['dc:date']", date },
        { "metaData['upnp:date']", year },
        { "metaData['dc:description']", desc },
    };

    std::map<std::string_view, std::map<std::string_view, std::string_view>> configDicts {};

    std::map<std::string_view, std::vector<std::vector<std::pair<std::string_view, std::string_view>>>> configVects {
        {
            "/import/scripting/virtual-layout/headline-map/headline",
            {
                {
                    { { "from", "^supertop" }, { "to", "thisTopic" }, { "type", "topic" } },
                    { { "from", "^nosupertop" }, { "to", "Rocks" } },
                },
            },
        },
    };

    std::map<std::string, std::string> asVideoAllFullName;
    asVideoAllFullName.insert(asVideoAllVideo.begin(), asVideoAllVideo.end());
    asVideoAllFullName["title"] = "2025-01-01-09-42-00-Video Title";

    // Expecting the common script calls
    // and will proxy through the mock objects
    // for verification.
    EXPECT_CALL(*commonScriptMock, getRootPath(Eq("/home/gerbera"), Eq(location))).WillRepeatedly(Return(1));

    EXPECT_CALL(*commonScriptMock, addContainerTree(ElementsAre("Video", "All Video"))).WillOnce(Return(1));
    EXPECT_CALL(*commonScriptMock, addCdsObject(IsIdenticalMap(asVideoAllVideo), "60", "/home/gerbera")).WillOnce(Return(1));

    EXPECT_CALL(*commonScriptMock, addContainerTree(ElementsAre("Video", "Directories", "home", "gerbera"))).WillOnce(Return(1));
    EXPECT_CALL(*commonScriptMock, addCdsObject(IsIdenticalMap(asVideoAllVideo), "61", "/home/gerbera")).WillOnce(Return(1));

    EXPECT_CALL(*commonScriptMock, addContainerTree(ElementsAre("Topics", "thisTopic", "Rocks", "2025-01"))).WillOnce(Return(1));
    EXPECT_CALL(*commonScriptMock, addCdsObject(IsIdenticalMap(asVideoAllFullName), "76", "/home/gerbera")).WillOnce(Return(1));

    addGlobalFunctions(ctx, js_global_functions, {},
        detailsBox, configDicts, configVects);

    functionName = "importVideoDetail";
    auto fnResult = callFunction(ctx, dukMockItem,
        { { "title", title },
            { "id", id },
            { "upnpclass", UPNP_CLASS_VIDEO_ITEM },
            { "location", location },
            { "mimetype", mimetype } },
        meta, aux, res,
        AutoscanDirectory::ContainerTypesDefaults.at(AutoscanMediaMode::Audio),
        "/home/gerbera");
    std::vector<int> items {
        60000,
        61000,
        76000,
    };
    EXPECT_EQ(fnResult, items);
}

TEST_F(ImportDetailsScriptTest, AddsVideoWithDetailsAndTopicsFromPath)
{
    std::string title = "Video Title";
    std::string mimetype = "video/mp4";
    std::string artist = "Artist";
    std::string date = "2025-01-01";
    std::string year = "2025";
    std::string genre2 = "Mediatomb";
    std::string desc = "Video Plot";
    std::string id = "2";
    std::string location = "/home/gerbera/home/gerbera/video.mp4";
    std::map<std::string, std::string> res;

    std::vector<std::pair<std::string, std::string>> meta {
        { "dc:title", title },
        { "upnp:artist", artist },
        { "dc:date", date },
        { "upnp:date", year },
        { "upnp:genre", genre2 },
        { "dc:description", desc },
    };

    std::map<std::string, std::string> aux {
        { "NFO:model", "Gerbera" },
        { "NFO:topic", "supertop" },
        { "NFO:subtopic", "nosupertop" },
        { "NFO:createdate", "20250101094200" },
    };

    // Represents the values passed to `addCdsObject`, extracted from keys defined there.
    std::map<std::string, std::string> asVideoAllVideo {
        { "title", title },
        { "metaData['dc:title']", title },
        { "metaData['upnp:artist']", artist },
        { "metaData['dc:date']", date },
        { "metaData['upnp:date']", year },
        { "metaData['dc:description']", desc },
    };

    std::map<std::string_view, std::map<std::string_view, std::string_view>> configDicts {
        { "/import/scripting/virtual-layout/script-options/script-option", { { "topicFromPath", "gerbera" } } },
    };

    std::map<std::string_view, std::vector<std::vector<std::pair<std::string_view, std::string_view>>>> configVects {
        {
            "/import/scripting/virtual-layout/headline-map/headline",
            {
                {
                    { { "from", "^supertop" }, { "to", "thisTopic" }, { "type", "topic" } },
                    { { "from", "^nosupertop" }, { "to", "Rocks" } },
                },
            },
        },
    };

    std::map<std::string, std::string> asVideoAllFullName;
    asVideoAllFullName.insert(asVideoAllVideo.begin(), asVideoAllVideo.end());
    asVideoAllFullName["title"] = "2025-01-01-09-42-00-Video Title";

    // Expecting the common script calls
    // and will proxy through the mock objects
    // for verification.
    EXPECT_CALL(*commonScriptMock, getRootPath(Eq("/home/gerbera"), Eq(location))).WillRepeatedly(Return(1));

    EXPECT_CALL(*commonScriptMock, addContainerTree(ElementsAre("Video", "All Video"))).WillOnce(Return(1));
    EXPECT_CALL(*commonScriptMock, addCdsObject(IsIdenticalMap(asVideoAllVideo), "60", "/home/gerbera")).WillOnce(Return(1));

    EXPECT_CALL(*commonScriptMock, addContainerTree(ElementsAre("Video", "Directories", "home", "gerbera"))).WillOnce(Return(1));
    EXPECT_CALL(*commonScriptMock, addCdsObject(IsIdenticalMap(asVideoAllVideo), "61", "/home/gerbera")).WillOnce(Return(1));

    EXPECT_CALL(*commonScriptMock, addContainerTree(ElementsAre("Topics", "gerbera", "Rocks", "2025-01"))).WillOnce(Return(1));
    EXPECT_CALL(*commonScriptMock, addCdsObject(IsIdenticalMap(asVideoAllFullName), "761", "/home/gerbera")).WillOnce(Return(1));

    addGlobalFunctions(ctx, js_global_functions, {},
        detailsBox, configDicts, configVects);

    functionName = "importVideoDetail";
    auto fnResult = callFunction(ctx, dukMockItem,
        { { "title", title },
            { "id", id },
            { "upnpclass", UPNP_CLASS_VIDEO_ITEM },
            { "location", location },
            { "mimetype", mimetype } },
        meta, aux, res,
        AutoscanDirectory::ContainerTypesDefaults.at(AutoscanMediaMode::Audio),
        "/home/gerbera");
    std::vector<int> items {
        60000,
        61000,
        761000,
    };
    EXPECT_EQ(fnResult, items);
}
#endif // HAVE_JS
