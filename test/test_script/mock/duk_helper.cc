#ifdef HAVE_JS

#include <string>
#include <map>
#include <duk_config.h>
#include <vector>
#include <duktape.h>
#include <regex>
#include "duk_helper.h"

using namespace std;

DukTestHelper::DukTestHelper() {};
DukTestHelper::~DukTestHelper() {};

vector<string> DukTestHelper::arrayToVector(duk_context *ctx, duk_idx_t idx) {
  duk_size_t i, n;
  string val;
  vector<string> vctr;

  duk_to_object(ctx, idx);
  n = duk_get_length(ctx, -1);
  for (i = 0; i < n; i++) {
    duk_get_prop_index(ctx, -1, i);
    val = duk_to_string(ctx, -1);
    vctr.push_back(val);
    duk_pop(ctx);
  }
  return vctr;
}

map<string, string> DukTestHelper::extractValues(duk_context *ctx, vector<string> keys, duk_idx_t idx) {
  map<string, string> objValues;
  string val;
  tuple<string, string> complexValue;
  for (auto key : keys) {
    if(this->isArrayNotation(key)) {
      complexValue = this->extractObjectValues(ctx, key, idx);
      val = get<1>(complexValue);
    } else {
      duk_get_prop_string(ctx, idx, key.c_str());
      if (duk_is_null_or_undefined(ctx, idx)) {
        duk_pop(ctx);
      } else {
        val = duk_to_string(ctx, -1);
        duk_pop(ctx);
      }
    }
    if(val.size() > 0) {
      objValues.insert(make_pair(key, val));
    }
  }
  return objValues;
}

bool DukTestHelper::isArrayNotation(string key) {
  regex associative_array_reg("^.*\\['.*'\\]$");
  return regex_match(key, associative_array_reg);
}

tuple<string, string> DukTestHelper::extractObjectValues(duk_context *ctx, string key, duk_idx_t idx) {
  regex associative_array_reg("^(.*)\\['(.*)'\\]$");
  std::cmatch pieces;

  string value;
  string objName;
  string objProp;

  if (std::regex_match(key.c_str(), pieces, associative_array_reg)) {
    if (pieces.size() == 3) {
      objName = pieces.str(1);
      objProp = pieces.str(2);
      duk_get_prop_string(ctx, idx, objName.c_str());
      if (duk_is_object(ctx, -1)) {
        duk_to_object(ctx, -1);
        duk_get_prop_string(ctx, -1, objProp.c_str());
        if (duk_is_null_or_undefined(ctx, -1)) {
          duk_pop(ctx);
        } else {
          value = duk_get_string(ctx, -1);
          duk_pop(ctx);
          return make_tuple(key, value);
        }
      }
    }
  }
  return make_tuple(key, string(""));
}

void DukTestHelper::createObject(duk_context *ctx, map<string, string> objectProps, map<string, string> metaProps) {
  // obj
  {
    duk_push_object(ctx);
    for (const auto &x : objectProps) {
      duk_push_string(ctx, x.second.c_str());
      duk_put_prop_string(ctx, -2, x.first.c_str());
    }
  }

  // obj.meta
  {
    duk_push_object(ctx);
    for (const auto &x : metaProps) {
      duk_push_string(ctx, x.second.c_str());
      duk_put_prop_string(ctx, -2, x.first.c_str());
    }
    duk_put_prop_string(ctx, -2, "meta");
  }
}
#endif //HAVE_JS