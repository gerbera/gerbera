/*GRB*

    Gerbera - https://gerbera.io/

    test_nfo_metafile.cc - this file is part of Gerbera.

    Copyright (C) 2022-2025 Gerbera Contributors

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

#include "cds/cds_objects.h"
#include "exceptions.h"
#include "upnp/upnp_common.h"
#include "util/tools.h"

#include "mock/common_script_mock.h"
#include "mock/duk_helper.h"
#include "mock/script_test_fixture.h"

#include <duktape.h>
#include <map>
#include <memory>
#include <pugixml.hpp>

// Extends ScriptTestFixture to allow
// for unique testing of the External Metafile
// processing
class MetafileNfoTest : public CommonScriptTestFixture {
public:
    MetafileNfoTest()
    {
        scriptName = "metadata.js";
        functionName = "importMetadata";
        objectName = "obj";
    }
};

static pugi::xml_node nullNode;
static pugi::xml_document xmlDoc;
static pugi::xml_node root;

// Proxy the Duktape script with `readXml`
// Mimics reading the playlist file line by line
// Uses the `CommonScriptMock` to track expectations
static duk_ret_t readXml(duk_context* ctx)
{
    int direction = duk_to_int(ctx, 0);
    duk_pop(ctx);

    pugi::xml_node node;

    if (direction > 0 && root.first_child()) {
        root = root.first_child();
        node = root;
    } else if (direction < -1 && root.root()) {
        root = xmlDoc.document_element();
        node = root;
    } else if (direction < 0 && root.parent()) {
        root = root.parent();
        node = root;
    } else if (root.next_sibling()) {
        root = root.next_sibling();
        node = root;
    } else 
        node = nullNode;

    std::ostringstream buf;
    buf << "<" << toLower(node.name()) << " ";
    duk_push_object(ctx);
    for (auto&& attrib : node.attributes()) {
        duk_push_string(ctx, attrib.value());
        duk_put_prop_string(ctx, -2, toLower(attrib.name()).c_str());
        buf << toLower(attrib.name()) << "=" << attrib.value() << " ";
    }
    buf << ">" << node.text().as_string();
    buf << "</" << toLower(node.name()) << ">";

    duk_push_string(ctx, node.text().as_string());
    duk_put_prop_string(ctx, -2, "VALUE");
    duk_push_string(ctx, toLower(node.name()).c_str());
    duk_put_prop_string(ctx, -2, "NAME");

    return MetafileNfoTest::commonScriptMock->readXml(buf.str());
}

static std::vector<std::string> getPropertyNames(duk_context* ctx)
{
    std::vector<std::string> keys;
    duk_enum(ctx, -1, 0);
    while (duk_next(ctx, -1, 0 /*get_key*/)) {
        /* [ ... enum key ] */
        auto sym = std::string(duk_get_string(ctx, -1));
        keys.push_back(std::move(sym));
        duk_pop(ctx); /* pop_key */
    }
    duk_pop(ctx); // duk_enum
    return keys;
}

static std::vector<std::string> getArrayProperty(duk_context* ctx, const std::string& name)
{
    std::vector<std::string> result;
    if (!duk_is_object_coercible(ctx, -1))
        return result;
    duk_get_prop_string(ctx, -1, name.c_str());
    if (duk_is_null_or_undefined(ctx, -1) || !duk_is_array(ctx, -1)) {
        duk_pop(ctx);
        return result;
    }
    duk_enum(ctx, -1, 0);
    while (duk_next(ctx, -1, 1 /* get_value */)) {
        duk_get_string(ctx, -1);
        if (duk_is_null_or_undefined(ctx, -1)) {
            duk_pop_2(ctx);
            continue;
        }
        // [key = duk_to_string(ctx, -2)]
        auto val = std::string(duk_to_string(ctx, -1));
        result.push_back(std::move(val));
        duk_pop_2(ctx); /* pop_key */
    }
    duk_pop(ctx); // duk_enum
    duk_pop(ctx); // property
    return result;
}

duk_ret_t updateCdsObject(duk_context* ctx)
{
    const std::string objectName = "obj";
    std::map<std::string, std::string> result;

    if (duk_is_undefined(ctx, -1)) {
        std::cerr << "Could not retrieve stack " << objectName << " object" << '\n';
        return 0;
    }

    duk_get_prop_string(ctx, -1, "title");
    if (duk_is_null_or_undefined(ctx, -1) || !duk_to_string(ctx, -1)) {
        std::cerr << "Could not retrieve stack " << objectName << ".title" << '\n';
        duk_pop(ctx);
        return 0;
    }
    result["title"] = duk_get_string(ctx, -1);
    duk_pop(ctx);

    duk_get_prop_string(ctx, -1, "upnpclass");
    if (duk_is_null_or_undefined(ctx, -1) || !duk_to_string(ctx, -1)) {
        std::cerr << "Could not retrieve stack " << objectName << ".upnpclass" << '\n';
        duk_pop(ctx);
        return 0;
    }
    result["upnpclass"] = duk_get_string(ctx, -1);
    duk_pop(ctx);

    duk_get_prop_string(ctx, -1, "description");
    if (duk_is_null_or_undefined(ctx, -1) || !duk_to_string(ctx, -1)) {
        std::cerr << "Could not retrieve stack " << objectName << ".description" << '\n';
        duk_pop(ctx);
        return 0;
    }
    result["description"] = duk_get_string(ctx, -1);
    duk_pop(ctx);

    duk_get_prop_string(ctx, -1, "metaData");
    int idx = 0;
    if (!duk_is_null_or_undefined(ctx, -1) && duk_is_object(ctx, -1)) {
        duk_to_object(ctx, -1);
        auto keys = getPropertyNames(ctx);
        for (auto&& sym : keys) {
            auto arrayVal = getArrayProperty(ctx, sym);
            for (auto&& val : arrayVal) {
                if (!val.empty()) {
                    result[fmt::format("{}-{}", idx, sym)] = val;
                    idx++;
                }
            }
        }
    }
    duk_pop(ctx); // metaData
    return MetafileNfoTest::commonScriptMock->updateCdsObject(result);
}

// Mock the Duktape C methods
// that are called from the playlists.js script
// * These are static methods, which makes mocking difficult.
static duk_function_list_entry js_global_functions[] = {
    { "print", CommonScriptTestFixture::js_print, DUK_VARARGS },
    { "print2", CommonScriptTestFixture::js_print2, DUK_VARARGS },
    { "readXml", readXml, 1 },
    { "updateCdsObject", updateCdsObject, 1 },
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

TEST_F(MetafileNfoTest, CreatesDukContextWithNfoMetafile)
{
    EXPECT_NE(ctx, nullptr);
}

TEST_F(MetafileNfoTest, SetsPropertiesFromFile)
{
    pugi::xml_parse_result result = xmlDoc.load_file("fixtures/example.nfo");
    if (result.status != pugi::xml_parse_status::status_ok) {
        throw ConfigParseException(result.description());
    }
    root = xmlDoc.document_element();
    const std::string location = "/location/of/movie.mpg";
    const std::string fileName = "/location/of/movie.nfo";
    const std::string plot = "UPnP Media Server for 2022";
    const std::string title = "This is Gerbera";
    std::map<std::string, std::string> asDictionary {
        { "upnpclass", UPNP_CLASS_VIDEO_MOVIE },
        { "title", title },
        { "description", plot },
        { "0-upnp:originalTrackNumber", "10" },
        { "1-upnp:episodeSeason", "0" },
        { "2-upnp:none", "done" },
        { "3-upnp:rating", "NONE" },
        { "4-upnp:rating@type", "MPAA.ORG" },
        { "5-upnp:rating@equivalentAge", "NONE" },
        { "6-upnp:genre", "Mediaplayer" },
        { "7-upnp:region", "Github" },
        { "8-upnp:director", "Gerbera Team" },
        { "9-dc:date", "2005-01-04" },
        { "10-dc:publisher", "Gerbera Home" },
        { "11-upnp:actor", "Ian Whyman" },
        { "12-upnp:actor", "Karl Straussberger" },
    };

    // Expecting the common script calls..and will proxy through the mock objects for verification.
    EXPECT_CALL(*commonScriptMock, print2(Eq("Info"), Eq(fmt::format("Processing metafile: {} for {} nfo", fileName, location)))).WillOnce(Return(1));
    EXPECT_CALL(*commonScriptMock, readXml(Eq("<movie ></movie>"))).Times(2).WillRepeatedly(Return(1));
    EXPECT_CALL(*commonScriptMock, readXml(Eq(fmt::format("<title >{}</title>", title)))).WillOnce(Return(1));
    EXPECT_CALL(*commonScriptMock, readXml(Eq("<originaltitle >It was MediaTomb</originaltitle>"))).WillOnce(Return(1));
    EXPECT_CALL(*commonScriptMock, readXml(Eq("<sorttitle >Gerbera</sorttitle>"))).WillOnce(Return(1));
    EXPECT_CALL(*commonScriptMock, readXml(Eq("<userrating >0</userrating>"))).WillOnce(Return(1));
    EXPECT_CALL(*commonScriptMock, readXml(Eq(fmt::format("<plot >{}</plot>", plot)))).Times(1).WillOnce(Return(1));
    EXPECT_CALL(*commonScriptMock, readXml(Eq("<mpaa >NONE</mpaa>"))).WillOnce(Return(1));
    EXPECT_CALL(*commonScriptMock, readXml(Eq("<director >Gerbera Team</director>"))).WillOnce(Return(1));
    EXPECT_CALL(*commonScriptMock, readXml(Eq("<credits >Mediatomb team</credits>"))).WillOnce(Return(1));
    EXPECT_CALL(*commonScriptMock, readXml(Eq("<genre >Mediaplayer</genre>"))).WillOnce(Return(1));
    EXPECT_CALL(*commonScriptMock, readXml(Eq("<country >Github</country>"))).WillOnce(Return(1));
    EXPECT_CALL(*commonScriptMock, readXml(Eq("<premiered >2005-01-04</premiered>"))).WillOnce(Return(1));
    EXPECT_CALL(*commonScriptMock, readXml(Eq("<year >2005</year>"))).WillOnce(Return(1));
    EXPECT_CALL(*commonScriptMock, readXml(Eq("<studio >Gerbera Home</studio>"))).WillOnce(Return(1));
    EXPECT_CALL(*commonScriptMock, readXml(Eq("<fileinfo ></fileinfo>"))).WillOnce(Return(1));
    EXPECT_CALL(*commonScriptMock, readXml(Eq("<actor ></actor>"))).Times(4).WillRepeatedly(Return(1));
    EXPECT_CALL(*commonScriptMock, readXml(Eq("<name >Ian Whyman</name>"))).WillOnce(Return(1));
    EXPECT_CALL(*commonScriptMock, readXml(Eq("<name >Karl Straussberger</name>"))).WillOnce(Return(1));
    EXPECT_CALL(*commonScriptMock, readXml(Eq("<role >Developer</role>"))).Times(2).WillRepeatedly(Return(1));
    EXPECT_CALL(*commonScriptMock, readXml(Eq("< ></>"))).Times(4).WillRepeatedly(Return(0));
    EXPECT_CALL(*commonScriptMock, updateCdsObject(IsIdenticalMap(asDictionary))).WillOnce(Return(1));

    addGlobalFunctions(ctx, js_global_functions);
    callFunction(ctx, dukMockMetafile, {{"location", location}}, fileName);
}
#endif
