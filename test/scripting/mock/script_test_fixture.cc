/*GRB*

    Gerbera - https://gerbera.io/

    script_test_fixture.cc - this file is part of Gerbera.

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

#include "cds/cds_objects.h"
#include "config/result/autoscan.h"
#include "content/scripting/script_names.h"
#include "metadata/metadata_enums.h"
#include "util/grb_fs.h"
#include "util/tools.h"

#include "common_script_mock.h"
#include "duk_helper.h"
#include "script_test_fixture.h"

#include <duktape.h>
#include <fstream>
#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include <sstream>

std::unique_ptr<CommonScriptMock> CommonScriptTestFixture::commonScriptMock;

void ScriptTestFixture::SetUp()
{
    ctx = duk_create_heap(nullptr, nullptr, nullptr, nullptr, nullptr);

    loadCommon(ctx);

    if (functionName.empty()) {
        fs::path scriptFile = fs::path(SCRIPTS_DIR) / "js" / scriptName;
        std::string scriptContent = GrbFile(scriptFile).readTextFile();
        duk_push_thread_stash(ctx, ctx);
        duk_push_string(ctx, scriptFile.c_str());
        if (duk_pcompile_lstring_filename(ctx, 0, scriptContent.c_str(), scriptContent.length()) != DUK_EXEC_SUCCESS) {
            DukTestHelper::printError(ctx, "Failed to load script ", scriptFile);
            return;
        }
        duk_put_global_string(ctx, "script_under_test");
        duk_pop(ctx);
    }
}

void ScriptTestFixture::TearDown()
{
    duk_destroy_heap(ctx);
}

void ScriptTestFixture::loadCommon(duk_context* ctx) const
{
    if (scriptName != "common.js" && functionName.empty()) {
        fs::path commonScript = fs::path(SCRIPTS_DIR) / "js" / "common.js";
        std::string script = GrbFile(commonScript).readTextFile();
        duk_push_string(ctx, commonScript.c_str());

        if (duk_pcompile_lstring_filename(ctx, 0, script.c_str(), script.length())) {
            DukTestHelper::printError(ctx, "Failed to load script ", commonScript);
            return;
        }
        if (duk_pcall(ctx, 0) != DUK_EXEC_SUCCESS) {
            DukTestHelper::printError(ctx, "Failed to execute script ", commonScript);
            return;
        }
        duk_pop(ctx); // commonScript
    } else if (scriptName != "common.js") {
        auto dirIterator = fs::directory_iterator(fs::path(SCRIPTS_DIR) / "js");
        for (auto&& dirEntry : dirIterator) {
            auto&& entryPath = dirEntry.path();
            if (entryPath.extension() == ".js") {
                try {
                    std::string script = GrbFile(entryPath).readTextFile();
                    duk_push_string(ctx, entryPath.c_str());
                    if (duk_pcompile_lstring_filename(ctx, 0, script.c_str(), script.length())) {
                        DukTestHelper::printError(ctx, "Failed to load script ", entryPath);
                        return;
                    }
                    if (duk_pcall(ctx, 0) != DUK_EXEC_SUCCESS) {
                        DukTestHelper::printError(ctx, "Failed to execute script ", entryPath);
                        return;
                    }
                    duk_pop(ctx); // entryPath
                } catch (const std::runtime_error& e) {
                    std::cerr << "Unable to load  " << entryPath << ": " << e.what() << std::endl;
                }
            }
        }
    }
}

void ScriptTestFixture::dukMockItem(
    duk_context* ctx,
    const std::map<std::string, std::string>& props,
    const std::vector<std::pair<std::string, std::string>>& meta,
    const std::map<std::string, std::string>& aux,
    const std::map<std::string, std::string>& res)
{
    duk_idx_t origIdx = duk_push_object(ctx);
    for (auto&& [name, value] : props) {
        duk_push_string(ctx, value.c_str());
        duk_put_prop_string(ctx, -2, name.c_str());
    }

    std::map<std::string, std::vector<std::string>> metaGroups;
    for (auto&& [mkey, mvalue] : meta) {
        if (metaGroups.find(mkey) == metaGroups.end()) {
            metaGroups[mkey] = std::vector<std::string>();
        }
        metaGroups[mkey].push_back(mvalue);
    }

    // obj.meta
    duk_idx_t metaIdx = duk_push_object(ctx);
    for (auto&& [key, array] : metaGroups) {
        duk_push_string(ctx, fmt::format("{}", fmt::join(array, "/")).c_str());
        duk_put_prop_string(ctx, metaIdx, key.c_str());
    }
    duk_put_prop_string(ctx, origIdx, "meta");

    metaIdx = duk_push_object(ctx);
    // obj.metaData
    for (auto&& [key, array] : metaGroups) {
        auto dukArray = duk_push_array(ctx);
        for (std::size_t i = 0; i < array.size(); i++) {
            duk_push_string(ctx, array[i].c_str());
            duk_put_prop_index(ctx, dukArray, i);
        }
        duk_put_prop_string(ctx, metaIdx, key.c_str());
    }
    duk_put_prop_string(ctx, origIdx, "metaData");

    // obj.res
    duk_idx_t resIdx = duk_push_object(ctx);
    duk_push_string(ctx, fmt::to_string(res.size()).c_str());
    duk_put_prop_string(ctx, resIdx, "count");
    for (auto const& val : res) {
        duk_push_string(ctx, val.second.c_str());
        duk_put_prop_string(ctx, resIdx, val.first.c_str());
    }
    duk_put_prop_string(ctx, origIdx, "res");

    // obj.aux
    if (!aux.empty()) {
        duk_idx_t auxIdx = duk_push_object(ctx);
        for (auto const& val : aux) {
            duk_push_string(ctx, val.second.c_str());
            duk_put_prop_string(ctx, auxIdx, val.first.c_str());
        }
        duk_put_prop_string(ctx, origIdx, "aux");
    }
}

duk_ret_t ScriptTestFixture::dukMockItem(
    duk_context* ctx,
    const std::string& mimetype,
    const std::string& upnpClass,
    const std::string& id,
    int theora,
    const std::string& title,
    const std::vector<std::pair<std::string, std::string>>& meta,
    const std::map<std::string, std::string>& aux,
    const std::map<std::string, std::string>& res,
    const std::string& location,
    int onlineService)
{
    duk_push_sprintf(ctx, "%s", AutoscanDirectory::ContainerTypesDefaults.at(AutoscanMediaMode::Audio).c_str());
    duk_put_global_string(ctx, "grb_container_type_audio");
    duk_push_sprintf(ctx, "%s", AutoscanDirectory::ContainerTypesDefaults.at(AutoscanMediaMode::Image).c_str());
    duk_put_global_string(ctx, "grb_container_type_image");
    duk_push_sprintf(ctx, "%s", AutoscanDirectory::ContainerTypesDefaults.at(AutoscanMediaMode::Video).c_str());
    duk_put_global_string(ctx, "grb_container_type_video");

    const std::string objectName = "orig";
    duk_idx_t origIdx = duk_push_object(ctx);
    duk_push_string(ctx, mimetype.c_str());
    duk_put_prop_string(ctx, origIdx, "mimetype");
    duk_push_string(ctx, upnpClass.c_str());
    duk_put_prop_string(ctx, origIdx, "upnpclass");
    duk_push_string(ctx, id.c_str());
    duk_put_prop_string(ctx, origIdx, "id");
    duk_push_string(ctx, title.c_str());
    duk_put_prop_string(ctx, origIdx, "title");
    duk_push_string(ctx, location.c_str());
    duk_put_prop_string(ctx, origIdx, "location");
    duk_push_int(ctx, onlineService);
    duk_put_prop_string(ctx, origIdx, "onlineservice");
    duk_push_int(ctx, theora);
    duk_put_prop_string(ctx, origIdx, "theora");

    std::map<std::string, std::vector<std::string>> metaGroups;
    for (auto&& [mkey, mvalue] : meta) {
        if (metaGroups.find(mkey) == metaGroups.end()) {
            metaGroups[mkey] = std::vector<std::string>();
        }
        metaGroups[mkey].push_back(mvalue);
    }

    // obj.meta
    duk_idx_t metaIdx = duk_push_object(ctx);
    for (auto&& [key, array] : metaGroups) {
        duk_push_string(ctx, fmt::format("{}", fmt::join(array, "/")).c_str());
        duk_put_prop_string(ctx, metaIdx, key.c_str());
    }
    duk_put_prop_string(ctx, origIdx, "meta");

    metaIdx = duk_push_object(ctx);
    // obj.metaData
    for (auto&& [key, array] : metaGroups) {
        auto dukArray = duk_push_array(ctx);
        for (std::size_t i = 0; i < array.size(); i++) {
            duk_push_string(ctx, array[i].c_str());
            duk_put_prop_index(ctx, dukArray, i);
        }
        duk_put_prop_string(ctx, metaIdx, key.c_str());
    }
    duk_put_prop_string(ctx, origIdx, "metaData");

    // obj.res
    duk_idx_t resIdx = duk_push_object(ctx);
    duk_push_string(ctx, fmt::to_string(res.size()).c_str());
    duk_put_prop_string(ctx, resIdx, "count");
    for (auto const& val : res) {
        duk_push_string(ctx, val.second.c_str());
        duk_put_prop_string(ctx, resIdx, val.first.c_str());
    }
    duk_put_prop_string(ctx, origIdx, "res");

    // obj.aux
    if (!aux.empty()) {
        duk_idx_t auxIdx = duk_push_object(ctx);
        for (auto const& val : aux) {
            duk_push_string(ctx, val.second.c_str());
            duk_put_prop_string(ctx, auxIdx, val.first.c_str());
        }
        duk_put_prop_string(ctx, origIdx, "aux");
    }
    duk_put_global_string(ctx, objectName.c_str());

    // TODO: parameterize?
    duk_push_string(ctx, "object/script/path");
    duk_put_global_string(ctx, "object_script_path");
    return 0;
}

int ScriptTestFixture::readLineCnt = 0;
std::vector<std::string> ScriptTestFixture::lines;
void ScriptTestFixture::mockPlaylistFile(const std::string& mockFile)
{
    std::ifstream t(mockFile);
    std::string str((std::istreambuf_iterator<char>(t)), std::istreambuf_iterator<char>());
    lines = splitString(str, '\n', true);
    lines.push_back("-EOF-"); // used to stop processing
    readLineCnt = 0;
}

void ScriptTestFixture::dukMockPlaylist(
    duk_context* ctx,
    const std::map<std::string, std::string>& props)
{
    duk_push_object(ctx);
    for (auto&& [name, value] : props) {
        duk_push_string(ctx, value.c_str());
        duk_put_prop_string(ctx, -2, name.c_str());
    }
}

duk_ret_t ScriptTestFixture::dukMockPlaylist(
    duk_context* ctx,
    const std::string& title,
    const std::string& location,
    const std::string& mimetype)
{
    duk_push_object(ctx);
    duk_push_string(ctx, location.c_str());
    duk_put_prop_string(ctx, -2, "location");
    duk_push_string(ctx, mimetype.c_str());
    duk_put_prop_string(ctx, -2, "mimetype");
    duk_push_string(ctx, title.c_str());
    duk_put_prop_string(ctx, -2, "title");
    duk_put_global_string(ctx, objectName.c_str());
    return 0;
}

duk_ret_t ScriptTestFixture::dukMockMetafile(
    duk_context* ctx,
    const std::string& location,
    const std::string& fileName)
{
    duk_push_object(ctx);
    duk_push_string(ctx, location.c_str());
    duk_put_prop_string(ctx, -2, "location");
    duk_push_string(ctx, fmt::to_string(10).c_str());
    duk_put_prop_string(ctx, -2, "trackNumber");
    duk_push_string(ctx, fmt::to_string(0).c_str());
    duk_put_prop_string(ctx, -2, "partNumber");
    // setting metadata
    {
        duk_push_object(ctx);
        auto dukArray = duk_push_array(ctx);
        duk_push_string(ctx, fmt::to_string(10).c_str());
        duk_put_prop_index(ctx, dukArray, 0);
        duk_put_prop_string(ctx, -2, MetaEnumMapper::getMetaFieldName(MetadataFields::M_TRACKNUMBER).c_str());
        dukArray = duk_push_array(ctx);
        duk_push_string(ctx, fmt::to_string(0).c_str());
        duk_put_prop_index(ctx, dukArray, 0);
        duk_put_prop_string(ctx, -2, MetaEnumMapper::getMetaFieldName(MetadataFields::M_PARTNUMBER).c_str());
        dukArray = duk_push_array(ctx);
        duk_push_string(ctx, "done");
        duk_put_prop_index(ctx, dukArray, 0);
        duk_put_prop_string(ctx, -2, "upnp:none");
        duk_put_prop_string(ctx, -2, "metaData");
    }
    duk_put_global_string(ctx, objectName.c_str());

    duk_push_string(ctx, fileName.c_str());
    duk_put_global_string(ctx, "object_script_path");
    return 0;
}

void ScriptTestFixture::dukMockMetafile(
    duk_context* ctx,
    const std::map<std::string, std::string>& props)
{
    duk_push_object(ctx);
    for (auto&& [name, value] : props) {
        duk_push_string(ctx, value.c_str());
        duk_put_prop_string(ctx, -2, name.c_str());
    }
    duk_push_string(ctx, fmt::to_string(10).c_str());
    duk_put_prop_string(ctx, -2, "trackNumber");
    duk_push_string(ctx, fmt::to_string(0).c_str());
    duk_put_prop_string(ctx, -2, "partNumber");
    // setting metadata
    {
        duk_push_object(ctx);
        auto dukArray = duk_push_array(ctx);
        duk_push_string(ctx, fmt::to_string(10).c_str());
        duk_put_prop_index(ctx, dukArray, 0);
        duk_put_prop_string(ctx, -2, MetaEnumMapper::getMetaFieldName(MetadataFields::M_TRACKNUMBER).c_str());
        dukArray = duk_push_array(ctx);
        duk_push_string(ctx, fmt::to_string(0).c_str());
        duk_put_prop_index(ctx, dukArray, 0);
        duk_put_prop_string(ctx, -2, MetaEnumMapper::getMetaFieldName(MetadataFields::M_PARTNUMBER).c_str());
        dukArray = duk_push_array(ctx);
        duk_push_string(ctx, "done");
        duk_put_prop_index(ctx, dukArray, 0);
        duk_put_prop_string(ctx, -2, "upnp:none");
        duk_put_prop_string(ctx, -2, "metaData");
    }
}

void ScriptTestFixture::addGlobalFunctions(
    duk_context* ctx,
    const duk_function_list_entry* funcs,
    const std::map<std::string_view, std::string_view>& configValues,
    const std::vector<boxConfig>& boxDefaults,
    const std::map<std::string_view, std::map<std::string_view, std::string_view>>& configDicts)
{

    for (auto&&[meta,str]: MetaEnumMapper::mt_keys) {
        duk_push_lstring(ctx, str.data(), str.size());
        auto sym = mt_names.at(meta);
        duk_put_global_lstring(ctx, sym.data(), sym.size());
    }

    for (auto&& entry : res_names) {
        duk_push_string(ctx, entry.second.data());
        auto sym = std::find_if(res_names.begin(), res_names.end(), [=](auto&& n) { return n.first == entry.first; });
        if (sym != res_names.end())
            duk_put_global_string(ctx, sym->second.data());
    }

    for (auto&& [field, sym] : ot_names) {
        duk_push_int(ctx, field);
        duk_put_global_lstring(ctx, sym.data(), sym.size());
    }

    for (auto&& [field, sym] : upnp_classes) {
        duk_push_lstring(ctx, field.data(), field.size());
        duk_put_global_lstring(ctx, sym.data(), sym.size());
    }

    for (auto&& [field, sym] : boxKeyNames) {
        duk_push_lstring(ctx, sym.data(), sym.length());
        duk_put_global_lstring(ctx, field.data(), field.length());
    }

    if (configValues.empty()) {
        addConfig(
            ctx,
            { { "/import/scripting/virtual-layout/attribute::audio-layout", audioLayout }, { "/import/scripting/virtual-layout/structured-layout/attribute::skip-chars", "" } },
            boxDefaults,
            configDicts);
    } else {
        addConfig(ctx, configValues, boxDefaults, configDicts);
    }

    duk_push_int(ctx, 0);
    duk_put_global_string(ctx, "ONLINE_SERVICE_NONE");

    duk_push_global_object(ctx);
    duk_put_function_list(ctx, -1, funcs);
    duk_pop(ctx);
}

void ScriptTestFixture::addConfig(
    duk_context* ctx,
    const std::map<std::string_view, std::string_view>& configValues,
    const std::vector<boxConfig>& boxDefaults,
    const std::map<std::string_view, std::map<std::string_view, std::string_view>>& configDicts)
{
    duk_push_object(ctx); // config
    for (auto&& [key, value] : configValues) {
        duk_push_string(ctx, value.data());
        duk_put_prop_string(ctx, -2, key.data());
    }

    for (auto&& [dictName, dict] : configDicts) {

        duk_push_object(ctx); // dict
        for (auto&& [key, value] : dict) {
            duk_push_string(ctx, value.data());
            duk_put_prop_string(ctx, -2, key.data());
        }
        duk_put_prop_string(ctx, -2, dictName.data()); // dict

    }

    duk_push_object(ctx); // box-layout
    for (auto&& boxLayout : boxDefaults) {
        duk_push_object(ctx);
        auto bl = std::map<std::string, std::string> {
            { "id", "-1" },
            { "size", fmt::to_string(boxLayout.size) },
            { "enabled", fmt::to_string(boxLayout.enabled) },
            { "title", boxLayout.title },
            { "class", boxLayout.upnpClass }
        };

        for (auto&& [key, value] : bl) {
            duk_push_string(ctx, value.c_str());
            duk_put_prop_string(ctx, -2, key.c_str());
        }
        duk_put_prop_string(ctx, -2, boxLayout.key.c_str());
    }
    duk_put_prop_string(ctx, -2, "/import/scripting/virtual-layout/boxlayout/box"); // box-layout

    duk_put_global_string(ctx, "config");
}

void ScriptTestFixture::executeScript(duk_context* ctx)
{
    duk_push_thread_stash(ctx, ctx);
    duk_get_global_string(ctx, "script_under_test");
    if (duk_is_function(ctx, -1)) {
        if (duk_pcall(ctx, 0) != DUK_EXEC_SUCCESS) {
            DukTestHelper::printError(ctx, "Failed to execute script ", scriptName);
        }
        duk_pop(ctx); // script_under_test
    }
}

void ScriptTestFixture::callFunction(
    duk_context* ctx,
    void(dukMockFunction)(duk_context* ctx, const std::map<std::string, std::string>& props),
    const std::map<std::string, std::string>& props,
    const std::string& rootPath)
{
    dukMockFunction(ctx, props);
    duk_put_global_string(ctx, objectName.c_str());
    // functionName(object, rootPath, autoScanId, containerType)
    std::string containerType;
    // Push function onto stack
    if (!duk_get_global_string(ctx, functionName.c_str()) || !duk_is_function(ctx, -1)) {
        std::cerr << "javascript function not found: " << functionName << '\n';
        duk_pop(ctx);
        return;
    }

    int narg = 0;

    // Push obj structure onto stack
    dukMockFunction(ctx, props);
    narg++;

    // Push obj structure onto stack (as container)
    dukMockFunction(ctx, props);
    narg++;

    // push rootPath onto stack
    duk_push_sprintf(ctx, "%s", rootPath.c_str());
    narg++;

    // Push autoScanId onto stack
    duk_push_sprintf(ctx, "%d", -1);
    narg++;

    // Container Type onto stack
    duk_push_sprintf(ctx, "%s", containerType.c_str());
    narg++;

    if (duk_pcall(ctx, static_cast<duk_idx_t>(narg)) != DUK_EXEC_SUCCESS) {
        // Note: The invoked function will be blamed for execution errors, not the actual offending line of code
        // https://github.com/svaarala/duktape/blob/master/doc/error-objects.rst
        DukTestHelper::printError(ctx, "JavaScript runtime error ", functionName);
        duk_pop(ctx);
        return;
    }
    duk_pop(ctx);
}

std::vector<std::string> ScriptTestFixture::createContainerChain(duk_context* ctx)
{
    std::string path;
    DukTestHelper dukHelper;
    std::vector<std::string> array = dukHelper.arrayToVector(ctx, -1);
    for (auto const& value : array) {
        path.append("\\/" + value);
    }
    duk_push_string(ctx, path.c_str());
    return array;
}

std::pair<std::string, int> ScriptTestFixture::getLastPath2(duk_context* ctx)
{
    std::string inputPath = duk_to_string(ctx, 0);
    int length = duk_to_int(ctx, 1);
    length = length <= 0 ? 1 : length;
    fs::path path = inputPath;
    path = path.remove_filename();

    int pos = 0;
    int idx = 0;
    int psize = std::distance(path.begin(), path.end()) - 1;
    auto dukArray = duk_push_array(ctx);
    for (auto&& i = path.begin(); i != path.end(); i++) {
        if (pos > 0 && psize - length <= pos && pos < psize) {
            duk_push_string(ctx, (*i).c_str());
            duk_put_prop_index(ctx, dukArray, idx);
	    idx++;
        }
	pos++;
    }
    return {inputPath, length};
}

std::string ScriptTestFixture::getLastPath(duk_context* ctx)
{
    std::string inputPath = duk_to_string(ctx, 0);
    std::string path = inputPath;
    std::string delimiter = "/";

    size_t pos;
    std::string token;
    std::vector<std::string> pathElements;
    while ((pos = path.find(delimiter)) != std::string::npos) {
        token = path.substr(0, pos);
        pathElements.push_back(token);
        path.erase(0, pos + delimiter.length());
    }
    std::string lastPath = pathElements.at(pathElements.size() - 1);
    duk_push_string(ctx, lastPath.c_str());
    return inputPath;
}

std::string ScriptTestFixture::getPlaylistType(duk_context* ctx)
{
    std::string playlistMimeType = duk_to_string(ctx, 0);
    std::string type;
    if (playlistMimeType == "audio/x-mpegurl") {
        type = "m3u";
    } else if (playlistMimeType == "audio/x-scpls") {
        type = "pls";
    } else if (playlistMimeType == MIME_TYPE_ASX_PLAYLIST) {
        type = "asx";
    } else {
        type = "";
    }
    duk_push_string(ctx, type.c_str());
    return playlistMimeType;
}

std::tuple<std::string, std::string> ScriptTestFixture::print2(duk_context* ctx)
{
    std::string mode = duk_to_string(ctx, 0);
    std::string result = duk_to_string(ctx, 1);
    return {mode, result};
}

std::string ScriptTestFixture::print(duk_context* ctx)
{
    std::string result = duk_to_string(ctx, 0);
    return result;
}

std::string ScriptTestFixture::getYear(duk_context* ctx)
{
    std::string date = duk_to_string(ctx, 0);
    std::string_view date_time_format { "%Y-%m-%dT%H:%M:%S" };
    std::istringstream ss { date };
    std::tm dt {};

    ss >> std::get_time(&dt, date_time_format.data());
    if (!ss.fail()) {
        auto t = std::mktime(&dt);
        duk_push_string(ctx, fmt::format("{}", dt.tm_year + 1900).c_str());
    } else
        duk_push_string(ctx, "0000");
    return date;
}

std::string ScriptTestFixture::mapGenre(
    duk_context* ctx,
    std::map<std::string, std::string> genMap)
{
    std::string genre = duk_to_string(ctx, 0);
    int mapped = genMap.find(genre) != genMap.end() ? 1 : 0;
    auto result = mapped == 1 ? genMap[genre] : genre;
    duk_push_object(ctx);

    duk_push_int(ctx, mapped);
    duk_put_prop_string(ctx, -2, "mapped");
    duk_push_string(ctx, result.c_str());
    duk_put_prop_string(ctx, -2, "value");

    return result;
}

std::vector<std::string> ScriptTestFixture::addContainerTree(
    duk_context* ctx,
    std::map<std::string, std::string> resMap)
{
    DukTestHelper dukHelper;
    std::vector<std::string> array = dukHelper.containerToPath(ctx, 0);

    std::string result;
    for (auto const& value : array) {
        result.append("/" + value);
    }

    duk_push_string(ctx, resMap[result].c_str());
    return array;
}

addCdsObjectParams ScriptTestFixture::addCdsObject(duk_context* ctx, const std::vector<std::string>& keys)
{
    DukTestHelper dukHelper;

    // parameter list
    std::map<std::string, std::string> dukObjValues = dukHelper.extractValues(ctx, keys, 0);
    std::string containerChain = duk_to_string(ctx, 1);
    std::string objContainer = duk_to_string(ctx, 2);

    addCdsObjectParams params;
    params.objectValues = dukObjValues;
    params.containerChain = containerChain;
    params.objectType = objContainer;

    return params;
}

copyObjectParams ScriptTestFixture::copyObject(
    duk_context* ctx,
    const std::map<std::string, std::string>& obj,
    const std::map<std::string, std::string>& meta)
{
    duk_bool_t isObjectParam = duk_is_object(ctx, 0);
    copyObjectParams params;
    params.isObject = isObjectParam;

    DukTestHelper dukHelper;
    dukHelper.createObject(ctx, obj, meta);

    return params;
}

getCdsObjectParams ScriptTestFixture::getCdsObject(
    duk_context* ctx,
    const std::map<std::string, std::string>& obj,
    const std::map<std::string, std::string>& meta)
{
    std::string location = duk_to_string(ctx, 0);
    getCdsObjectParams params;
    params.location = location;

    DukTestHelper dukHelper;
    dukHelper.createObject(ctx, obj, meta);

    return params;
}

abcBoxParams ScriptTestFixture::abcBox(duk_context* ctx)
{
    std::string inputValue = duk_to_string(ctx, 0);
    int boxType = duk_to_int32(ctx, 1);
    std::string divChar = duk_to_string(ctx, 2);

    abcBoxParams params;
    params.inputValue = inputValue;
    params.boxType = boxType;
    params.divChar = divChar;

    duk_push_string(ctx, boxType == 26 ? "-A-" : "-ABCD-");
    return params;
}

getRootPathParams ScriptTestFixture::getRootPath(duk_context* ctx)
{
    // parameter list
    std::string objScriptPath = duk_to_string(ctx, 0);
    std::string origObjLocation = duk_to_string(ctx, 1);
    std::string objLocation = origObjLocation;
    size_t pos;
    std::string delimiter = "/";
    std::vector<std::string> dirs;
    while ((pos = objLocation.find(delimiter)) != std::string::npos) {
        std::string token = objLocation.substr(0, pos);
        if (token.length() > 0)
            dirs.push_back(token);
        objLocation.erase(0, pos + delimiter.length());
    }

    duk_idx_t arrIdx = duk_push_array(ctx);
    for (size_t i = 0; i < dirs.size(); i++) {
        std::string dir = dirs.at(i);
        duk_push_string(ctx, dir.c_str());
        duk_put_prop_index(ctx, arrIdx, static_cast<int>(i));
    }

    getRootPathParams params;
    params.objScriptPath = objScriptPath;
    params.origObjLocation = origObjLocation;

    return params;
}

#endif
