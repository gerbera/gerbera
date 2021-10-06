#ifdef HAVE_JS

#include <duktape.h>
#include <filesystem>
#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include <memory>
#include <utility>
namespace fs = std::filesystem;

#include "cds_objects.h"
#include "config/config_manager.h"
#include "content/onlineservice/atrailers_content_handler.h"
#include "content/scripting/script_names.h"
#include "metadata/metadata_handler.h"
#include "util/string_converter.h"
#include "util/tools.h"

#include "common_script_mock.h"
#include "duk_helper.h"
#include "script_test_fixture.h"

using namespace ::testing;

void ScriptTestFixture::SetUp()
{
    ctx = duk_create_heap(nullptr, nullptr, nullptr, nullptr, nullptr);

    loadCommon(ctx);

    fs::path scriptFile = fs::path(SCRIPTS_DIR) / "js" / scriptName;
    std::string scriptContent = readTextFile(scriptFile.c_str());
    duk_push_thread_stash(ctx, ctx);
    duk_push_string(ctx, scriptFile.c_str());
    duk_pcompile_lstring_filename(ctx, 0, scriptContent.c_str(), scriptContent.length());
    duk_put_global_string(ctx, "script_under_test");
    duk_pop(ctx);
}

void ScriptTestFixture::TearDown()
{
    duk_destroy_heap(ctx);
}

void ScriptTestFixture::loadCommon(duk_context* ctx)
{
    if (scriptName != "common.js") {
        fs::path commonScript = fs::path(SCRIPTS_DIR) / "js" / "common.js";
        std::string script = readTextFile(commonScript.c_str());
        duk_push_string(ctx, commonScript.c_str());
        duk_pcompile_lstring_filename(ctx, 0, script.c_str(), script.length());

        if (duk_pcall(ctx, 0) != DUK_EXEC_SUCCESS) {
            std::cerr << "Failed to execute script: " << duk_safe_to_string(ctx, -1) << std::endl;
        }
        duk_pop(ctx); // commonScript
    }
}

duk_ret_t ScriptTestFixture::dukMockItem(duk_context* ctx, const std::string& mimetype, const std::string& id, int theora, const std::string& title,
    const std::map<std::string, std::string>& meta, const std::map<std::string, std::string>& aux, const std::map<std::string, std::string>& res,
    const std::string& location, int online_service)
{
    const std::string objectName = "orig";
    duk_idx_t orig_idx = duk_push_object(ctx);
    duk_push_string(ctx, mimetype.c_str());
    duk_put_prop_string(ctx, orig_idx, "mimetype");
    duk_push_string(ctx, id.c_str());
    duk_put_prop_string(ctx, orig_idx, "id");
    duk_push_string(ctx, title.c_str());
    duk_put_prop_string(ctx, orig_idx, "title");
    duk_push_string(ctx, location.c_str());
    duk_put_prop_string(ctx, orig_idx, "location");
    duk_push_int(ctx, online_service);
    duk_put_prop_string(ctx, orig_idx, "onlineservice");
    duk_push_int(ctx, theora);
    duk_put_prop_string(ctx, orig_idx, "theora");

    std::map<std::string, std::vector<std::string>> metaGroups;
    for (auto&& [mkey, mvalue] : meta) {
        if (metaGroups.find(mkey) == metaGroups.end()) {
            metaGroups[mkey] = std::vector<std::string>();
        }
        metaGroups[mkey].push_back(mvalue);
    }

    // obj.meta
    duk_idx_t meta_idx = duk_push_object(ctx);
    for (auto&& [key, array] : metaGroups) {
        duk_push_string(ctx, fmt::format("{}", fmt::join(array, "/")).c_str());
        duk_put_prop_string(ctx, meta_idx, key.c_str());
    }
    duk_put_prop_string(ctx, orig_idx, "meta");

    meta_idx = duk_push_object(ctx);
    // obj.metaData
    for (auto&& [key, array] : metaGroups) {
        auto duk_array = duk_push_array(ctx);
        for (std::size_t i = 0; i < array.size(); i++) {
            duk_push_string(ctx, array[i].c_str());
            duk_put_prop_index(ctx, duk_array, i);
        }
        duk_put_prop_string(ctx, meta_idx, key.c_str());
    }
    duk_put_prop_string(ctx, orig_idx, "metaData");

    // obj.res
    if (!res.empty()) {
        duk_idx_t res_idx = duk_push_object(ctx);
        for (auto const& val : res) {
            duk_push_string(ctx, val.second.c_str());
            duk_put_prop_string(ctx, res_idx, val.first.c_str());
        }
        duk_put_prop_string(ctx, orig_idx, "res");
    }

    // obj.aux
    if (!aux.empty()) {
        duk_idx_t aux_idx = duk_push_object(ctx);
        for (auto const& val : aux) {
            duk_push_string(ctx, val.second.c_str());
            duk_put_prop_string(ctx, aux_idx, val.first.c_str());
        }
        duk_put_prop_string(ctx, orig_idx, "aux");
    }
    duk_put_global_string(ctx, objectName.c_str());

    // TODO: parameterize?
    duk_push_string(ctx, "object/script/path");
    duk_put_global_string(ctx, "object_script_path");
    return 0;
}

duk_ret_t ScriptTestFixture::dukMockPlaylist(duk_context* ctx, const std::string& title, const std::string& location, const std::string& mimetype)
{
    const std::string objectName = "playlist";
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

void ScriptTestFixture::addGlobalFunctions(duk_context* ctx, const duk_function_list_entry* funcs, const std::map<std::string_view, std::string_view>& config)
{
    for (auto&& entry : mt_keys) {
        duk_push_string(ctx, entry.second);
        auto sym = std::find_if(mt_names.begin(), mt_names.end(), [=](auto&& n) { return n.first == entry.first; });
        if (sym != mt_names.end())
            duk_put_global_string(ctx, sym->second);
    }

    for (auto&& entry : res_keys) {
        duk_push_string(ctx, entry.second);
        auto sym = std::find_if(res_names.begin(), res_names.end(), [=](auto&& n) { return n.first == entry.first; });
        if (sym != res_names.end())
            duk_put_global_string(ctx, sym->second);
    }

    for (auto&& [field, sym] : ot_names) {
        duk_push_int(ctx, field);
        duk_put_global_string(ctx, sym);
    }

    for (auto&& [field, sym] : upnp_classes) {
        duk_push_string(ctx, field);
        duk_put_global_string(ctx, sym);
    }

    if (config.empty()) {
        addConfig(ctx, { { "/import/scripting/virtual-layout/attribute::audio-layout", audioLayout }, { "/import/scripting/virtual-layout/structured-layout/attribute::skip-chars", "" } });
    } else {
        addConfig(ctx, config);
    }

    duk_push_int(ctx, 0);
    duk_put_global_string(ctx, "ONLINE_SERVICE_NONE");
    duk_push_int(ctx, 1);
    duk_put_global_string(ctx, "ONLINE_SERVICE_YOUTUBE");
    duk_push_int(ctx, 4);
    duk_put_global_string(ctx, "ONLINE_SERVICE_APPLE_TRAILERS");
    duk_push_string(ctx, ATRAILERS_AUXDATA_POST_DATE);
    duk_put_global_string(ctx, "APPLE_TRAILERS_AUXDATA_POST_DATE");
    duk_push_int(ctx, 2);
    duk_put_global_string(ctx, "ONLINE_SERVICE_SOPCAST");

    duk_push_global_object(ctx);
    duk_put_function_list(ctx, -1, funcs);
    duk_pop(ctx);
}

void ScriptTestFixture::addConfig(duk_context* ctx, const std::map<std::string_view, std::string_view>& config)
{
    duk_push_object(ctx); // config
    for (auto&& [key, value] : config) {
        duk_push_string(ctx, value.data());
        duk_put_prop_string(ctx, -2, key.data());
    }
    duk_put_global_string(ctx, "config");
}

void ScriptTestFixture::executeScript(duk_context* ctx)
{
    duk_push_thread_stash(ctx, ctx);
    duk_get_global_string(ctx, "script_under_test");
    if (duk_is_function(ctx, -1)) {
        if (duk_pcall(ctx, 0) != DUK_EXEC_SUCCESS) {
            std::cerr << "Failed to execute script: " << duk_safe_to_string(ctx, -1) << std::endl;
        }
        duk_pop(ctx); // script_under_test
    }
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

std::string ScriptTestFixture::getLastPath(duk_context* ctx)
{
    std::string inputPath = duk_to_string(ctx, 0);
    std::string path = inputPath;
    std::string delimiter = "/";

    size_t pos = 0;
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
    } else {
        type = "";
    }
    duk_push_string(ctx, type.c_str());
    return playlistMimeType;
}

std::string ScriptTestFixture::print(duk_context* ctx)
{
    std::string result = duk_to_string(ctx, 0);
    return result;
}

std::string ScriptTestFixture::getYear(duk_context* ctx)
{
    std::string date = duk_to_string(ctx, 0);
    // TODO: parse YYYY...
    duk_push_string(ctx, "2018");
    return date;
}

std::vector<std::string> ScriptTestFixture::addContainerTree(duk_context* ctx, std::map<std::string, std::string> resMap)
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
    std::string containerChain;
    std::string objContainer;
    std::map<std::string, std::string> dukObjValues;
    DukTestHelper dukHelper;

    // parameter list
    dukObjValues = dukHelper.extractValues(ctx, keys, 0);
    containerChain = duk_to_string(ctx, 1);
    objContainer = duk_to_string(ctx, 2);

    addCdsObjectParams params;
    params.objectValues = dukObjValues;
    params.containerChain = containerChain;
    params.objectType = objContainer;

    return params;
}

copyObjectParams ScriptTestFixture::copyObject(duk_context* ctx, const std::map<std::string, std::string>& obj, const std::map<std::string, std::string>& meta)
{
    duk_bool_t isObjectParam = duk_is_object(ctx, 0);
    copyObjectParams params;
    params.isObject = isObjectParam;

    DukTestHelper dukHelper;
    dukHelper.createObject(ctx, obj, meta);

    return params;
}

getCdsObjectParams ScriptTestFixture::getCdsObject(duk_context* ctx, const std::map<std::string, std::string>& obj, const std::map<std::string, std::string>& meta)
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
    std::string inputValue;
    int boxType;
    std::string divChar;

    inputValue = duk_to_string(ctx, 0);
    boxType = duk_to_int32(ctx, 1);
    divChar = duk_to_string(ctx, 2);

    abcBoxParams params;
    params.inputValue = inputValue;
    params.boxType = boxType;
    params.divChar = divChar;

    duk_push_string(ctx, boxType == 26 ? "-A-" : "-ABCD-");
    return params;
}

getRootPathParams ScriptTestFixture::getRootPath(duk_context* ctx)
{
    duk_idx_t arr_idx;
    std::string objScriptPath;
    std::string objLocation;
    std::string origObjLocation;

    // parameter list
    objScriptPath = duk_to_string(ctx, 0);
    origObjLocation = duk_to_string(ctx, 1);
    objLocation = origObjLocation;

    size_t pos = 0;
    std::string token;
    std::string delimiter = "/";
    std::vector<std::string> dirs;
    while ((pos = objLocation.find(delimiter)) != std::string::npos) {
        token = objLocation.substr(0, pos);
        if (token.length() > 0)
            dirs.push_back(token);
        objLocation.erase(0, pos + delimiter.length());
    }

    arr_idx = duk_push_array(ctx);
    for (size_t i = 0; i < dirs.size(); i++) {
        std::string dir = dirs.at(i);
        duk_push_string(ctx, dir.c_str());
        duk_put_prop_index(ctx, arr_idx, static_cast<int>(i));
    }

    getRootPathParams params;
    params.objScriptPath = objScriptPath;
    params.origObjLocation = origObjLocation;

    return params;
}

#endif
