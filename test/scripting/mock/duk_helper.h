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
    DukTestHelper();
    ~DukTestHelper();

    // Convert an array at the top of the stack
    // to a c++ vector of strings
    std::vector<std::string> arrayToVector(duk_context* ctx, duk_idx_t idx);

    // Convert an array of objects containing title property at the top of the stack
    // to a c++ vector of strings
    std::vector<std::string> containerToPath(duk_context* ctx, duk_idx_t idx);

    // Convert an object at the top of the stack
    // to a map of its properties using the property keys passed into
    // the method
    std::map<std::string, std::string> extractValues(duk_context* ctx, std::vector<std::string> keys, duk_idx_t idx);

    // Adds a new object to the duk_context with meta data
    void createObject(duk_context* ctx, std::map<std::string, std::string> objectProps, std::map<std::string, std::string> metaProps);

private:
    // Extracts the property value given a complex associative array key
    // Example: key = `meta['dc:title']`
    // Finds *meta* object, then its child property *dc:title*
    std::tuple<std::string, std::string> extractObjectValues(duk_context* ctx, std::string key, duk_idx_t idx);

    // Returns true if the key is representing javascript array notation
    // true = meta['dc:title']
    // false = 'dc:title'
    // false = 'meta'
    // false = 'meta.title' // TODO: should be true...
    bool isArrayNotation(std::string key);
};

#endif //GERBERA_DUK_HELPER_H
#endif //HAVE_JS
