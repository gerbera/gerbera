#ifdef HAVE_JS

#include "duk_helper.h"
#include <duk_config.h>
#include <duktape.h>
#include <fmt/core.h>
#include <fmt/format.h>
#include <regex>

std::vector<std::string> DukTestHelper::arrayToVector(duk_context* ctx, duk_idx_t idx)
{
    duk_size_t i, n;
    std::string val;
    std::vector<std::string> vctr;

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

std::vector<std::string> DukTestHelper::containerToPath(duk_context* ctx, duk_idx_t idx)
{
    std::vector<std::string> vctr;

    if (!duk_is_array(ctx, idx)) {
        return vctr;
    }

    duk_to_object(ctx, idx);
    duk_size_t n = duk_get_length(ctx, idx);
    for (duk_size_t i = 0; i < n; i++) {
        if (duk_get_prop_index(ctx, idx, i)) {
            if (!duk_is_object(ctx, -1)) {
                duk_pop(ctx); // item
                break;
            }
            //duk_to_object(ctx, -1);
            duk_get_prop_string(ctx, -1, "title");
            if (!duk_is_null_or_undefined(ctx, -1)) {
                std::string val = duk_to_string(ctx, -1);
                vctr.push_back(val);
            }
            duk_pop(ctx); // title
        }
        duk_pop(ctx); // item
    }

    return vctr;
}

std::map<std::string, std::string> DukTestHelper::extractValues(duk_context* ctx, const std::vector<std::string>& keys, duk_idx_t idx)
{
    std::map<std::string, std::string> objValues;
    std::string val;
    std::pair<std::string, std::string> complexValue;
    for (const auto& key : keys) {
        if (this->isArrayNotation(key)) {
            complexValue = this->extractObjectValues(ctx, key, idx);
            val = std::get<1>(complexValue);
        } else {
            duk_get_prop_string(ctx, idx, key.c_str());
            if (!duk_is_null_or_undefined(ctx, idx)) {
                val = duk_to_string(ctx, -1);
            }
            duk_pop(ctx);
        }
        if (!val.empty()) {
            objValues.emplace(key, val);
        }
    }
    return objValues;
}

bool DukTestHelper::isArrayNotation(const std::string& key)
{
    std::regex associative_array_reg("^.*\\['.*'\\]$");
    return regex_match(key, associative_array_reg);
}

std::pair<std::string, std::string> DukTestHelper::extractObjectValues(duk_context* ctx, const std::string& key, duk_idx_t idx)
{
    std::regex associative_array_reg("^(.*)\\['(.*)'\\]$");
    std::cmatch pieces;

    if (std::regex_match(key.c_str(), pieces, associative_array_reg)) {
        if (pieces.size() == 3) {
            std::string objName = pieces.str(1);
            std::string objProp = pieces.str(2);

            duk_get_prop_string(ctx, idx, objName.c_str());
            if (duk_is_object(ctx, -1)) {
                duk_to_object(ctx, -1);
                duk_get_prop_string(ctx, -1, objProp.c_str());
                if (duk_is_null_or_undefined(ctx, -1)) {
                    duk_pop(ctx); // objProp
                    duk_pop(ctx); // objName
                } else if (duk_to_string(ctx, -1)) {
                    std::string value = duk_get_string(ctx, -1);
                    duk_pop(ctx); // objProp
                    duk_pop(ctx); // objName
                    return { key, value };
                } else {
                    duk_pop(ctx); // objProp not string
                    std::vector<std::string> result;
                    duk_enum(ctx, -1, 0);
                    while (duk_next(ctx, -1, 1 /* get_value */)) {
                        auto val = std::string(duk_get_string(ctx, -1));
                        result.push_back(std::move(val));
                        duk_pop_2(ctx); /* pop_key */
                    }
                    duk_pop(ctx); // duk_enum
                    duk_pop(ctx); // objProp
                    duk_pop(ctx); // objName
                    return { key, fmt::format("{}", fmt::join(result, "/")) };
                }
            }
        }
    }
    return { key, "" };
}

void DukTestHelper::createObject(duk_context* ctx, const std::map<std::string, std::string>& objectProps, const std::map<std::string, std::string>& metaProps)
{
    // obj
    {
        duk_push_object(ctx);
        for (auto&& x : objectProps) {
            duk_push_string(ctx, x.second.c_str());
            duk_put_prop_string(ctx, -2, x.first.c_str());
        }
    }

    std::map<std::string, std::vector<std::string>> metaGroups;
    for (auto&& [mkey, mvalue] : metaProps) {
        if (metaGroups.find(mkey) == metaGroups.end()) {
            metaGroups[mkey] = std::vector<std::string>();
        }
        metaGroups[mkey].push_back(mvalue);
    }

    // obj.meta
    {
        duk_push_object(ctx);
        for (auto&& [key, attr] : metaGroups) {
            duk_push_string(ctx, fmt::format("{}", fmt::join(attr, "/")).c_str());
            duk_put_prop_string(ctx, -2, key.c_str());
        }
        duk_put_prop_string(ctx, -2, "meta");
    }

    // obj.metaData
    {
        duk_push_object(ctx);
        for (auto&& [key, array] : metaGroups) {
            auto duk_array = duk_push_array(ctx);
            for (std::size_t i = 0; i < array.size(); i++) {
                duk_push_string(ctx, array[i].c_str());
                duk_put_prop_index(ctx, duk_array, i);
            }
            duk_put_prop_string(ctx, -2, key.c_str());
        }
        duk_put_prop_string(ctx, -2, "metaData");
    }
}
#endif //HAVE_JS
