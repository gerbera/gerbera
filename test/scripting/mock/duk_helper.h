/*GRB*

    Gerbera - https://gerbera.io/

    duk_helper.h - this file is part of Gerbera.

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

#ifndef GERBERA_DUK_HELPER_H
#define GERBERA_DUK_HELPER_H

#include <duk_config.h>
#include <map>
#include <string>
#include <vector>

#define UNDEFINED "undefined"

// Provides generic helper methods to convert Duktape Context
// data into c++ types
class DukTestHelper {
public:
    DukTestHelper() = default;
    ~DukTestHelper() = default;

    // Convert an array at the top of the stack
    // to a c++ vector of strings
    static std::vector<std::string> arrayToVector(duk_context* ctx, duk_idx_t idx);

    // Convert an array of objects containing title property at the top of the stack
    // to a c++ vector of strings
    static std::vector<std::string> containerToPath(duk_context* ctx, duk_idx_t idx);

    // Convert an object at the top of the stack
    // to a map of its properties using the property keys passed into
    // the method
    std::map<std::string, std::string> extractValues(duk_context* ctx, const std::vector<std::string>& keys, duk_idx_t idx);

    // Adds a new object to the duk_context with meta data
    static void createObject(duk_context* ctx, const std::map<std::string, std::string>& objectProps, const std::map<std::string, std::string>& metaProps);

    // Print error message based on duktape
    static void printError(duk_context* ctx, const std::string& message, const std::string& item);

private:
    // Extracts the property value given a complex associative array key
    // Example: key = `meta['dc:title']`
    // Finds *meta* object, then its child property *dc:title*
    static std::pair<std::string, std::string> extractObjectValues(duk_context* ctx, const std::string& key, duk_idx_t idx);

    // Returns true if the key is representing javascript array notation
    // true = meta['dc:title']
    // false = 'dc:title'
    // false = 'meta'
    // false = 'meta.title' // TODO: should be true...
    static bool isArrayNotation(const std::string& key);
};

#endif //GERBERA_DUK_HELPER_H
#endif //HAVE_JS
