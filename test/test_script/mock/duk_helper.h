#ifdef HAVE_JS

#ifndef GERBERA_DUK_HELPER_H
#define GERBERA_DUK_HELPER_H

#include <duk_config.h>
#include <string>
#include <map>

using namespace std;

// Provides generic helper methods to convert Duktape Context
// data into c++ types
class DukTestHelper {
 public:
  DukTestHelper();
  ~DukTestHelper();

  // Convert an array at the top of the stack
  // to a c++ vector of strings
  vector<string> arrayToVector(duk_context *ctx, duk_idx_t idx);

  // Convert an object at the top of the stack
  // to a map of its properties using the property keys passed into
  // the method
  map<string, string> extractValues(duk_context *ctx, vector<string> keys, duk_idx_t idx);

  // Adds a new object to the duk_context with meta data
  void createObject(duk_context *ctx, map<string, string> objectProps, map<string, string> metaProps);
 private:

  // Extracts the property value given a complex associative array key
  // Example: key = `meta['dc:title']`
  // Finds *meta* object, then its child property *dc:title*
  tuple<string, string> extractObjectValues(duk_context *ctx, string key, duk_idx_t idx);

  // Returns true if the key is representing javascript array notation
  // true = meta['dc:title']
  // false = 'dc:title'
  // false = 'meta'
  // false = 'meta.title' // TODO: should be true...
  bool isArrayNotation(string key);
};

#endif //GERBERA_DUK_HELPER_H
#endif //HAVE_JS