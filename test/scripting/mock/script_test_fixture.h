/*GRB*

    Gerbera - https://gerbera.io/

    script_test_fixture.h - this file is part of Gerbera.

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
#ifndef GERBERA_SCRIPTTESTFIXTURE_H
#define GERBERA_SCRIPTTESTFIXTURE_H

#include "common_script_mock.h"

#include <duktape.h>
#include <gtest/gtest.h>
#include <memory>
#include <tuple>
#include <vector>

using namespace ::testing;

struct addCdsObjectParams {
    std::map<std::string, std::string> objectValues;
    std::string containerChain;
    std::string rootPath;
};

struct boxConfig {
    std::string key;
    std::string title;
    std::string upnpClass;
    bool enabled;
    int size;
};

struct abcBoxParams {
    std::string inputValue;
    int boxType;
    std::string divChar;
};

struct getRootPathParams {
    std::string objScriptPath;
    std::string origObjLocation;
};

struct copyObjectParams {
    bool isObject;
};

struct getCdsObjectParams {
    std::string location;
};

/// @brief The class provides a way to test the Duktape scripts
/// providing various c++ translations of script functions
/// useful for mocking said functions with expectations.
class ScriptTestFixture : public ::testing::Test {
    /// @brief Loads common.js if running test for another script
    void loadCommon(duk_context* ctx) const;

public:
    typedef void(DukMockFunction)(
        duk_context* ctx,
        const std::map<std::string, std::string>& props,
        const std::vector<std::pair<std::string, std::string>>& meta,
        const std::map<std::string, std::string>& aux,
        const std::map<std::string, std::string>& res);

    /// @brief Builds up the Duktape context and reads in the script under test
    void SetUp() override;

    /// @brief Destroys the Duktape context
    void TearDown() override;

    /// @brief Creates a mock item(orig) global object in Duktape context
    static void dukMockItem(
        duk_context* ctx,
        const std::map<std::string, std::string>& props,
        const std::vector<std::pair<std::string, std::string>>& meta,
        const std::map<std::string, std::string>& aux,
        const std::map<std::string, std::string>& res);

    /// @brief Load playlist file from fixtures
    static void mockPlaylistFile(const std::string& mockFile);

    /// @brief Creates a mock metafile global object in Duktape context
    static void dukMockMetafile(
        duk_context* ctx,
        const std::map<std::string, std::string>& props,
        const std::vector<std::pair<std::string, std::string>>& meta,
        const std::map<std::string, std::string>& aux,
        const std::map<std::string, std::string>& res);

    /// @brief Creates a mock playlist global object in Duktape context
    static void dukMockPlaylist(
        duk_context* ctx,
        const std::map<std::string, std::string>& props,
        const std::vector<std::pair<std::string, std::string>>& meta,
        const std::map<std::string, std::string>& aux,
        const std::map<std::string, std::string>& res);

    /// @brief Add global Duktape methods to proxy into c++ layer
    void addGlobalFunctions(
        duk_context* ctx,
        const duk_function_list_entry* funcs,
        const std::map<std::string_view, std::string_view>& config = {},
        const std::vector<boxConfig>& boxDefaults = {},
        const std::map<std::string_view, std::map<std::string_view, std::string_view>>& configDicts = {});

    /// @brief Add config entries to global context
    static void addConfig(
        duk_context* ctx,
        const std::map<std::string_view, std::string_view>& configValues,
        const std::vector<boxConfig>& boxDefaults = {},
        const std::map<std::string_view, std::map<std::string_view, std::string_view>>& configDicts = {});

    /// @brief Access the global object(script) by name, and execute
    std::vector<int> callFunction(
        duk_context* ctx,
        DukMockFunction dukMockFunction,
        const std::map<std::string, std::string>& props,
        const std::vector<std::pair<std::string, std::string>>& meta = {},
        const std::map<std::string, std::string>& aux = {},
        const std::map<std::string, std::string>& res = {},
        const std::string& containerType = "",
        const std::string& rootPath = "");

    /// @brief Proxy the common.js script with `createContainerChain`
    /// Mimics the creation of a directory chain
    /// Returns the chain as a vector of strings (for easy expectations)
    static std::vector<std::string> createContainerChain(duk_context* ctx);

    /// @brief Proxy the common.js script with `getLastPath`
    /// Mimics finding the last path of the item
    /// Pushes the last path value to the Duktape stack
    /// Returns the inputPath parameter sent by the script
    static std::pair<std::string, int> getLastPath2(duk_context* ctx);
    static std::string getLastPath(duk_context* ctx);

    /// @brief Proxy the common.js script with `getPlaylistType`
    /// Mimics determination of the playlist type
    /// Pushes the playlist type `m3u`, `pls` to the duktape stack
    /// Returns the input parameter matching the mimetype
    /// sent by the script
    static std::string getPlaylistType(duk_context* ctx);

    /// @brief Proxy the common.js script with `print`
    /// Mimics the print of text
    /// Returns the string sent by the script to print
    static std::string print(duk_context* ctx);
    static std::tuple<std::string, std::string> print2(duk_context* ctx);

    /// @brief Proxy the common.js script with `getYear`
    /// Mimics the parsing of YYYY
    /// Pushes the year in YYYY format to the duktape context
    /// Returns the full date sent by the script
    static std::string getYear(duk_context* ctx);

    /// @brief Proxy the Duktape script with `addCdsObject` global function.
    /// Translates the Duktape value stack to c++
    static addCdsObjectParams addCdsObject(
        duk_context* ctx,
        const std::vector<std::string>& objectKeys);

    /// @brief Proxy the Duktape script with `mapGenre` global function.
    /// Translates the Duktape value stack to c++
    static std::string mapGenre(
        duk_context* ctx,
        std::map<std::string, std::string> genMap);

    /// @brief roxy the Duktape script with `addContainerTree` C function.
    /// Translates the Duktape value stack to c++
    static std::vector<std::string> addContainerTree(
        duk_context* ctx,
        std::map<std::string, std::string> resMap);

    /// @brief Proxy the Duktape script with `abcbox` common.js function
    static abcBoxParams abcBox(duk_context* ctx);

    /// @brief Proxy the Duktape script with `getRootPath` common.js function
    static getRootPathParams getRootPath(duk_context* ctx);

    /// @brief roxy the Duktape script with `copyObject` global function
    static copyObjectParams copyObject(
        duk_context* ctx,
        const std::map<std::string, std::string>& obj,
        const std::map<std::string, std::string>& meta);

    /// @brief Proxy the Duktape script with `copyObject` global function
    static getCdsObjectParams getCdsObject(
        duk_context* ctx,
        const std::map<std::string, std::string>& obj,
        const std::map<std::string, std::string>& meta);

    /// @brief function name under test
    std::string functionName;

    // Used to iterate through `readln` content
    static int readLineCnt;
    static std::vector<std::string> lines;

    /// @brief The Duktape Context
    duk_context* ctx;
};

class CommonScriptTestFixture : public ScriptTestFixture {
public:
    /// @brief As Duktape requires static methods, so must the mock expectations be
    static std::unique_ptr<CommonScriptMock> commonScriptMock;

    CommonScriptTestFixture()
    {
        commonScriptMock = std::make_unique<::testing::NiceMock<CommonScriptMock>>();
    }

    ~CommonScriptTestFixture() override
    {
        commonScriptMock.reset();
    }

    static inline duk_ret_t js_print(duk_context* ctx)
    {
        std::string msg = ScriptTestFixture::print(ctx);
        return CommonScriptTestFixture::commonScriptMock->print(msg);
    }

    static inline duk_ret_t js_print2(duk_context* ctx)
    {
        auto [mode, msg] = ScriptTestFixture::print2(ctx);
        return CommonScriptTestFixture::commonScriptMock->print2(mode, msg);
    }

    static inline duk_ret_t js_getPlaylistType(duk_context* ctx)
    {
        std::string playlistMimeType = ScriptTestFixture::getPlaylistType(ctx);
        return CommonScriptTestFixture::commonScriptMock->getPlaylistType(playlistMimeType);
    }

    static inline duk_ret_t js_createContainerChain(duk_context* ctx)
    {
        std::vector<std::string> array = ScriptTestFixture::createContainerChain(ctx);
        return CommonScriptTestFixture::commonScriptMock->createContainerChain(array);
    }

    static inline duk_ret_t js_getLastPath(duk_context* ctx)
    {
        std::string inputPath = ScriptTestFixture::getLastPath(ctx);
        return CommonScriptTestFixture::commonScriptMock->getLastPath(inputPath);
    }

    static inline duk_ret_t js_getLastPath2(duk_context* ctx)
    {
        auto [inputPath, length] = ScriptTestFixture::getLastPath2(ctx);
        return CommonScriptTestFixture::commonScriptMock->getLastPath2(inputPath, length);
    }
};
#endif // GERBERA_SCRIPTTESTFIXTURE_H
#endif // HAVE_JS
