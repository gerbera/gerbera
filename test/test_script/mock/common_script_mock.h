#ifdef HAVE_JS

#ifndef GERBERA_COMMON_SCRIPT_MOCK_H
#define GERBERA_COMMON_SCRIPT_MOCK_H
#include <duk_config.h>

using namespace std;

// The interface used to mock the `common.js` script functions and other global functions.
// When testing scripts this mock allows for tracking of script calls and inputs.
// Each method is a global function used in the script
// Expectations can be decided by each test for the given scenario.
class CommonScriptInterface {
 public:
  virtual ~CommonScriptInterface(){}
  virtual duk_ret_t getPlaylistType(string type) = 0;
  virtual duk_ret_t print(string text) = 0;
  virtual duk_ret_t createContainerChain(vector<string> chain) = 0;
  virtual duk_ret_t getLastPath(string path) = 0;
  virtual duk_ret_t readln(string line) = 0;
  virtual duk_ret_t addCdsObject(map<string, string> item, string playlistContainer, string objectType) = 0;
  virtual duk_ret_t copyObject(bool isObject) = 0;
  virtual duk_ret_t getCdsObject(string location) = 0;
  virtual duk_ret_t getYear(string year) = 0;
  virtual duk_ret_t getRootPath(string objScriptPath, string location) = 0;
  virtual duk_ret_t abcBox(string inputValue, int boxType, string divChar) = 0;
};

class CommonScriptMock : public CommonScriptInterface {
 public:
  MOCK_METHOD1(getPlaylistType, duk_ret_t(string type));
  MOCK_METHOD1(print, duk_ret_t(string text));
  MOCK_METHOD1(createContainerChain, duk_ret_t(vector<string> chain));
  MOCK_METHOD1(getLastPath, duk_ret_t(string path));
  MOCK_METHOD1(readln, duk_ret_t(string line));
  MOCK_METHOD3(addCdsObject, duk_ret_t(map<string, string> item, string playlistContainer, string objectType));
  MOCK_METHOD1(copyObject, duk_ret_t(bool isObject));
  MOCK_METHOD1(getCdsObject, duk_ret_t(string location));
  MOCK_METHOD1(getYear, duk_ret_t(string year));
  MOCK_METHOD2(getRootPath, duk_ret_t(string objScriptPath, string location));
  MOCK_METHOD3(abcBox, duk_ret_t(string inputValue, int boxType, string divChar));
};
#endif //GERBERA_COMMON_SCRIPT_MOCK_H
#endif //HAVE_JS