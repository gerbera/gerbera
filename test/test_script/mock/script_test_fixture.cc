#ifdef HAVE_JS

#include "gmock/gmock.h"
#include "gtest/gtest.h"
#include <duktape.h>
#include <string_converter.h>
#include <cds_objects.h>
#include <memory>
#include <metadata_handler.h>
#include <atrailers_content_handler.h>

#include "common_script_mock.h"
#include "script_test_fixture.h"
#include "duk_helper.h"

using namespace ::testing;
using namespace std;

void ScriptTestFixture::SetUp() {
  ctx = duk_create_heap(nullptr, nullptr, nullptr, nullptr, nullptr);

  stringstream scriptFile;
  scriptFile << SCRIPTS_DIR << DIR_SEPARATOR << "js" << DIR_SEPARATOR << scriptName;
  string scriptContent = read_text_file(scriptFile.str());
  duk_push_thread_stash(ctx, ctx);
  duk_push_string(ctx, scriptFile.str().c_str());
  duk_pcompile_lstring_filename(ctx, 0, scriptContent.c_str(), scriptContent.length());
  duk_put_prop_string(ctx, -2, "script_under_test");
  duk_pop(ctx);
}

void ScriptTestFixture::TearDown() {
  duk_destroy_heap(ctx);
}

duk_ret_t ScriptTestFixture::dukMockItem(duk_context *ctx, string mimetype, string id, int theora, string title,
map<string, string> meta, map<string, string> aux, map<string, string> res,
                                         string location, int online_service) {
  duk_idx_t orig_idx;
  duk_idx_t meta_idx;
  duk_idx_t aux_idx;
  duk_idx_t res_idx;
  const string OBJECT_NAME = "orig";
  orig_idx = duk_push_object(ctx);
  duk_push_string(ctx, mimetype.c_str()); duk_put_prop_string(ctx, orig_idx, "mimetype");
  duk_push_string(ctx, id.c_str()); duk_put_prop_string(ctx, orig_idx, "id");
  duk_push_string(ctx, title.c_str()); duk_put_prop_string(ctx, orig_idx, "title");
  duk_push_string(ctx, location.c_str()); duk_put_prop_string(ctx, orig_idx, "location");
  duk_push_int(ctx, online_service); duk_put_prop_string(ctx, orig_idx, "onlineservice");
  duk_push_int(ctx, theora); duk_put_prop_string(ctx, orig_idx, "theora");

  // obj.meta
  meta_idx = duk_push_object(ctx);
  for(auto const& val: meta) {
    duk_push_string(ctx, val.second.c_str());
    duk_put_prop_string(ctx, meta_idx, val.first.c_str());
  }
  duk_put_prop_string(ctx, orig_idx, "meta");

  // obj.res
  if(!res.empty()) {
    res_idx = duk_push_object(ctx);
    for (auto const &val: res) {
      duk_push_string(ctx, val.second.c_str());
      duk_put_prop_string(ctx, res_idx, val.first.c_str());
    }
    duk_put_prop_string(ctx, orig_idx, "res");
  }

  // obj.aux
  if(!aux.empty()) {
    aux_idx = duk_push_object(ctx);
    for (auto const &val: aux) {
      duk_push_string(ctx, val.second.c_str());
      duk_put_prop_string(ctx, aux_idx, val.first.c_str());
    }
    duk_put_prop_string(ctx, orig_idx, "aux");
  }
  duk_put_global_string(ctx, OBJECT_NAME.c_str());

  // TODO: parameterize?
  duk_push_string(ctx, "object/script/path");
  duk_put_global_string(ctx, "object_script_path");
  return 0;
}

duk_ret_t ScriptTestFixture::dukMockPlaylist(duk_context *ctx, string title, string location, string mimetype) {
  const string OBJECT_NAME = "playlist";
  duk_push_object(ctx);
  duk_push_string(ctx, location.c_str());
  duk_put_prop_string(ctx, -2, "location");
  duk_push_string(ctx, mimetype.c_str());
  duk_put_prop_string(ctx, -2, "mimetype");
  duk_push_string(ctx, title.c_str());
  duk_put_prop_string(ctx, -2, "title");
  duk_put_global_string(ctx, OBJECT_NAME.c_str());
  return 0;
}

string ScriptTestFixture::read_text_file(string path) {
  FILE *f = fopen(path.c_str(), "r");
  ostringstream buf;
  char buffer[1024];
  size_t bytesRead;
  while ((bytesRead = fread(buffer, 1, sizeof(buffer), f)) > 0) {
    buf << string(buffer, bytesRead);
  }
  fclose(f);
  return buf.str();
}

void ScriptTestFixture::addGlobalFunctions(duk_context *ctx, const duk_function_list_entry *funcs) {
  for (int i = 0; i < M_MAX; i++)
  {
    duk_push_string(ctx, MT_KEYS[i].upnp); duk_put_global_string(ctx, MT_KEYS[i].sym);
  }

  for (int i = 0; i < R_MAX; i++)
  {
    duk_push_string(ctx, RES_KEYS[i].upnp); duk_put_global_string(ctx, RES_KEYS[i].sym);
  }

  duk_push_int(ctx, OBJECT_TYPE_ITEM_EXTERNAL_URL); duk_put_global_string(ctx, "OBJECT_TYPE_ITEM_EXTERNAL_URL");
  duk_push_int(ctx, OBJECT_TYPE_ITEM_INTERNAL_URL); duk_put_global_string(ctx, "OBJECT_TYPE_ITEM_INTERNAL_URL");
  duk_push_int(ctx, OBJECT_TYPE_ITEM); duk_put_global_string(ctx, "OBJECT_TYPE_ITEM");
  duk_push_string(ctx, UPNP_DEFAULT_CLASS_MUSIC_TRACK); duk_put_global_string(ctx, "UPNP_CLASS_ITEM_MUSIC_TRACK");
  duk_push_string(ctx, UPNP_DEFAULT_CLASS_PLAYLIST_CONTAINER); duk_put_global_string(ctx, "UPNP_CLASS_PLAYLIST_CONTAINER");
  duk_push_string(ctx, UPNP_DEFAULT_CLASS_MUSIC_ALBUM); duk_put_global_string(ctx, "UPNP_CLASS_CONTAINER_MUSIC_ALBUM");
  duk_push_string(ctx, UPNP_DEFAULT_CLASS_MUSIC_ARTIST); duk_put_global_string(ctx, "UPNP_CLASS_CONTAINER_MUSIC_ARTIST");
  duk_push_string(ctx, UPNP_DEFAULT_CLASS_MUSIC_GENRE); duk_put_global_string(ctx, "UPNP_CLASS_CONTAINER_MUSIC_GENRE");
  duk_push_string(ctx, UPNP_DEFAULT_CLASS_MUSIC_COMPOSER); duk_put_global_string(ctx, "UPNP_CLASS_CONTAINER_MUSIC_COMPOSER");
  duk_push_string(ctx, UPNP_DEFAULT_CLASS_MUSIC_CONDUCTOR); duk_put_global_string(ctx, "UPNP_CLASS_CONTAINER_MUSIC_CONDUCTOR");
  duk_push_string(ctx, UPNP_DEFAULT_CLASS_MUSIC_ORCHESTRA); duk_put_global_string(ctx, "UPNP_CLASS_CONTAINER_MUSIC_ORCHESTRA");
  duk_push_string(ctx, UPNP_DEFAULT_CLASS_CONTAINER); duk_put_global_string(ctx, "UPNP_CLASS_CONTAINER");

  duk_push_int(ctx, 0); duk_put_global_string(ctx, "ONLINE_SERVICE_NONE");
  duk_push_int(ctx, 1); duk_put_global_string(ctx, "ONLINE_SERVICE_YOUTUBE");
  duk_push_int(ctx, 4); duk_put_global_string(ctx, "ONLINE_SERVICE_APPLE_TRAILERS");
  duk_push_string(ctx, ATRAILERS_AUXDATA_POST_DATE); duk_put_global_string(ctx, "APPLE_TRAILERS_AUXDATA_POST_DATE");
  duk_push_int(ctx, 2); duk_put_global_string(ctx, "ONLINE_SERVICE_SOPCAST");

  duk_push_global_object(ctx);
  duk_put_function_list(ctx, -1, funcs);
  duk_pop(ctx);
}

void ScriptTestFixture::executeScript(duk_context *ctx) {
  duk_push_thread_stash(ctx, ctx);
  duk_get_prop_string(ctx, -1, "script_under_test");
  if (duk_is_function(ctx, -1)) {
    duk_pcall(ctx, 0);
  };
}

vector<string> ScriptTestFixture::createContainerChain(duk_context *ctx) {
  string path;
  DukTestHelper dukHelper;
  vector<string> array = dukHelper.arrayToVector(ctx, -1);
  for(auto const& value: array) {
    path.append("\\/" + value);
  }
  duk_push_lstring(ctx, path.c_str(), path.length());
  return array;
}

string ScriptTestFixture::getLastPath(duk_context *ctx) {
  string inputPath = duk_get_string(ctx, 0);
  string path = inputPath;
  string delimiter = "/";

  size_t pos = 0;
  string token;
  vector<string> pathElements;
  while ((pos = path.find(delimiter)) != string::npos) {
    token = path.substr(0, pos);
    pathElements.push_back(token);
    path.erase(0, pos + delimiter.length());
  }
  string lastPath = pathElements.at(pathElements.size() -1);
  duk_push_string(ctx, lastPath.c_str());
  return inputPath;
}

string ScriptTestFixture::getPlaylistType(duk_context *ctx) {
  string playlistMimeType = duk_get_string(ctx, 0);
  string type;
  if(playlistMimeType == "audio/x-mpegurl") {
    type = "m3u";
  } else if(playlistMimeType == "audio/x-scpls") {
    type = "pls";
  } else {
    type = "";
  }
  duk_push_lstring(ctx, type.c_str(), type.length());
  return playlistMimeType;
}

string ScriptTestFixture::print(duk_context *ctx) {
  string result = duk_get_string(ctx, 0);
  return result;
}

string ScriptTestFixture::getYear(duk_context *ctx) {
  string date = duk_get_string(ctx, 0);
  // TODO: parse YYYY...
  duk_push_string(ctx, "2018");
  return date;
}

addCdsObjectParams ScriptTestFixture::addCdsObject(duk_context *ctx, vector<string> keys) {
  string containerChain;
  string objContainer;
  map<string, string> dukObjValues;
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

copyObjectParams ScriptTestFixture::copyObject(duk_context *ctx, map<string, string> obj, map<string, string> meta) {
  duk_bool_t isObjectParam = duk_is_object(ctx, 0);
  copyObjectParams params;
  params.isObject = isObjectParam;

  DukTestHelper dukHelper;
  dukHelper.createObject(ctx, std::move(obj), std::move(meta));

  return params;
}

getCdsObjectParams ScriptTestFixture::getCdsObject(duk_context *ctx, map<string, string> obj, map<string, string> meta) {
  string location = duk_get_string(ctx, 0);
  getCdsObjectParams params;
  params.location = location;

  DukTestHelper dukHelper;
  dukHelper.createObject(ctx, std::move(obj), std::move(meta));

  return params;
}

abcBoxParams ScriptTestFixture::abcBox(duk_context *ctx) {
  string inputValue;
  int boxType;
  string divChar;

  inputValue = duk_to_string(ctx, 0);
  boxType = duk_to_int32(ctx, 1);
  divChar = duk_to_string(ctx, 2);

  abcBoxParams params;
  params.inputValue = inputValue;
  params.boxType = boxType;
  params.divChar = divChar;

  duk_push_string(ctx, "-ABCD-");
  return params;
}

getRootPathParams ScriptTestFixture::getRootPath(duk_context *ctx) {
  duk_idx_t arr_idx;
  string objScriptPath;
  string objLocation;
  string origObjLocation;

  // parameter list
  objScriptPath = duk_to_string(ctx, 0);
  origObjLocation = duk_to_string(ctx, 1);
  objLocation = origObjLocation;

  size_t pos = 0;
  string token;
  string delimiter = "/";
  vector<string> dirs;
  while ((pos = objLocation.find(delimiter)) != string::npos) {
    token = objLocation.substr(0, pos);
    if(token.length() > 0 ) dirs.push_back(token);
    objLocation.erase(0, pos + delimiter.length());
  }

  arr_idx = duk_push_array(ctx);
  for (size_t i = 0; i < dirs.size(); i++) {
    string dir = dirs.at(i);
    duk_push_string(ctx, dir.c_str());
    duk_put_prop_index(ctx, arr_idx, (int)i);
  }

  getRootPathParams params;
  params.objScriptPath = objScriptPath;
  params.origObjLocation = origObjLocation;

  return params;
}


#endif