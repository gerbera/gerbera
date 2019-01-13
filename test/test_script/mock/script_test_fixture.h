#ifdef HAVE_JS
#ifndef GERBERA_SCRIPTTESTFIXTURE_H
#define GERBERA_SCRIPTTESTFIXTURE_H

#include "gmock/gmock.h"
#include "gtest/gtest.h"
#include <duktape.h>
#include <string_converter.h>
#include <memory>

#include "common_script_mock.h"

using namespace ::testing;
using namespace std;

struct _addCdsObjectParams {
    map<string, string> objectValues;
    string containerChain;
    string objectType;
};
typedef struct _addCdsObjectParams addCdsObjectParams;

struct _abcBoxParams {
    string inputValue;
    int boxType;
    string divChar;
};
typedef struct _abcBoxParams abcBoxParams;

struct _getRootPathParams {
    string objScriptPath;
    string origObjLocation;
};
typedef struct _getRootPathParams getRootPathParams;

struct _copyObjectParams {
    bool isObject;
};
typedef struct _copyObjectParams copyObjectParams;

struct _getCdsObjectParams {
    string location;
};
typedef struct _getCdsObjectParams getCdsObjectParams;

// The class provides a way to test the Duktape scripts
// providing various c++ translations of script functions
// useful for mocking said functions with expectations.
class ScriptTestFixture : public ::testing::Test {

 public:

  // Builds up the Duktape context
  // Reads in the script under test
  virtual void SetUp();

  // Destroys the Duktape context
  virtual void TearDown();

  // Creates a mock item(orig) global object in Duktape context
  duk_ret_t dukMockItem(duk_context *ctx, string mimetype, string id, int theora, string title,
                        map<string, string> meta, map<string, string> aux, map<string, string> res,
                        string location, int online_service);

  // Creates a mock playlist global object in Duktape context
  duk_ret_t dukMockPlaylist(duk_context* ctx, string title, string location, string mimetype);

  // Reads the Duktape script under test
  string read_text_file(string path);

  // Add global Duktape methods to proxy into c++ layer
  void addGlobalFunctions(duk_context *ctx, const duk_function_list_entry *funcs);

  // Access the global object(script) by name, and execute
  void executeScript(duk_context *ctx);

  // Proxy the common.js script with `createContainerChain`
  // Mimics the creation of a directory chain
  // Returns the chain as a vector of strings (for easy expectations)
  static vector<string> createContainerChain(duk_context *ctx);

  // Proxy the common.js script with `getLastPath`
  // Mimics finding the last path of the item
  // Pushes the last path value to the Duktape stack
  // Returns the inputPath parameter sent by the script
  static string getLastPath(duk_context *ctx);

  // Proxy the common.js script with `getPlaylistType`
  // Mimics determination of the playlist type
  // Pushes the playlist type `m3u`, `pls` to the duktape stack
  // Returns the input parameter matching the mimetype
  // sent by the script
  static string getPlaylistType(duk_context *ctx);

  // Proxy the common.js script with `print`
  // Mimics the print of text
  // Returns the string sent by the script to print
  static string print(duk_context *ctx);

  // Proxy the common.js script with `getYear`
  // Mimics the parsing of YYYY
  // Pushes the year in YYYY format to the duktape context
  // Returns the full date sent by the script
  static string getYear(duk_context *ctx);

  // Proxy the Duktape script with `addCdsObject` global function.
  // Translates the Duktape value stack to c++
  static addCdsObjectParams addCdsObject(duk_context *ctx, vector<string> objectKeys);

  // Proxy the Duktape script with `abcbox` common.js function
  static abcBoxParams abcBox(duk_context *ctx);

  // Proxy the Duktape script with `getRootPath` common.js function
  static getRootPathParams getRootPath(duk_context *ctx);

  // Proxy the Duktape script with `copyObject` global function
  static copyObjectParams copyObject(duk_context *ctx, map<string, string> obj, map<string, string> meta);

  // Proxy the Duktape script with `copyObject` global function
  static getCdsObjectParams getCdsObject(duk_context *ctx, map<string, string> obj, map<string, string> meta);

  // Script file name under test
  // System defaults to known project path `/scripts/js/<scriptName>`
  string scriptName;

  // The Duktape Context
  duk_context *ctx;
};

#endif //GERBERA_SCRIPTTESTFIXTURE_H
#endif //HAVE_JS
