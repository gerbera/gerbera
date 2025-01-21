/*GRB*
    Gerbera - https://gerbera.io/

    common_script_mock.h - this file is part of Gerbera.

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

#ifndef GERBERA_COMMON_SCRIPT_MOCK_H
#define GERBERA_COMMON_SCRIPT_MOCK_H

#ifdef HAVE_JS

#include <duk_config.h>
#include <gmock/gmock.h>

// The interface used to mock the `common.js` script functions and other global functions.
// When testing scripts this mock allows for tracking of script calls and inputs.
// Each method is a global function used in the script
// Expectations can be decided by each test for the given scenario.
class CommonScriptInterface {
public:
    virtual ~CommonScriptInterface() = default;
    virtual duk_ret_t getPlaylistType(std::string type) = 0;
    virtual duk_ret_t print(std::string text) = 0;
    virtual duk_ret_t print2(std::string mode, std::string text) = 0;
    virtual duk_ret_t addContainerTree(std::vector<std::string> tree) = 0;
    virtual duk_ret_t createContainerChain(std::vector<std::string> chain) = 0;
    virtual duk_ret_t mapGenre(std::string genre) = 0;
    virtual duk_ret_t getLastPath(std::string path) = 0;
    virtual duk_ret_t getLastPath2(std::string path, int length) = 0;
    virtual duk_ret_t readln(std::string line) = 0;
    virtual duk_ret_t readXml(std::string line) = 0;
    virtual duk_ret_t addCdsObject(std::map<std::string, std::string> item, std::string containerChain, std::string objectType) = 0;
    virtual duk_ret_t updateCdsObject(std::map<std::string, std::string> result) = 0;
    virtual duk_ret_t copyObject(bool isObject) = 0;
    virtual duk_ret_t getCdsObject(std::string location) = 0;
    virtual duk_ret_t getYear(std::string year) = 0;
    virtual duk_ret_t getRootPath(std::string objScriptPath, std::string location) = 0;
    virtual duk_ret_t abcBox(std::string inputValue, int boxType, std::string divChar) = 0;
};

class CommonScriptMock : public CommonScriptInterface {
public:
    MOCK_METHOD1(getPlaylistType, duk_ret_t(std::string type));
    MOCK_METHOD1(print, duk_ret_t(std::string text));
    MOCK_METHOD2(print2, duk_ret_t(std::string mode, std::string text));
    MOCK_METHOD1(addContainerTree, duk_ret_t(std::vector<std::string> tree));
    MOCK_METHOD1(createContainerChain, duk_ret_t(std::vector<std::string> chain));
    MOCK_METHOD1(mapGenre, duk_ret_t(std::string genre));
    MOCK_METHOD1(getLastPath, duk_ret_t(std::string path));
    MOCK_METHOD2(getLastPath2, duk_ret_t(std::string path, int length));
    MOCK_METHOD1(readln, duk_ret_t(std::string line));
    MOCK_METHOD1(updateCdsObject, duk_ret_t(std::map<std::string, std::string> result));
    MOCK_METHOD1(readXml, duk_ret_t(std::string line));
    MOCK_METHOD3(addCdsObject, duk_ret_t(std::map<std::string, std::string> item, std::string containerChain, std::string objectType));
    MOCK_METHOD1(copyObject, duk_ret_t(bool isObject));
    MOCK_METHOD1(getCdsObject, duk_ret_t(std::string location));
    MOCK_METHOD1(getYear, duk_ret_t(std::string year));
    MOCK_METHOD2(getRootPath, duk_ret_t(std::string objScriptPath, std::string location));
    MOCK_METHOD3(abcBox, duk_ret_t(std::string inputValue, int boxType, std::string divChar));
};
#endif //HAVE_JS
#endif //GERBERA_COMMON_SCRIPT_MOCK_H
