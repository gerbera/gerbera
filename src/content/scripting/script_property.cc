/*GRB*

    Gerbera - https://gerbera.io/

    script_property.cc - this file is part of Gerbera.

    Copyright (C) 2024-2026 Gerbera Contributors

    Gerbera is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License version 2
    as published by the Free Software Foundation.

    Gerbera is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with Gerbera.  If not, see <http://www.gnu.org/licenses/>.
*/

/// @file content/scripting/script_property.cc

#ifdef HAVE_JS
#define GRB_LOG_FAC GrbLogFacility::script

#include "script_property.h" // API

#include "util/tools.h"

ScriptProperty::~ScriptProperty() = default;

std::vector<std::string> ScriptProperty::getStringArrayValue() const
{
    std::vector<std::string> result;
    if (!isValid()) {
        return result;
    }

    if (duk_is_null_or_undefined(ctx, index) || !duk_is_array(ctx, index)) {
        return result;
    }
    duk_enum(ctx, index, 0);
    while (duk_next(ctx, index, 1 /* get_value */)) {
        duk_get_string(ctx, index);
        if (duk_is_null_or_undefined(ctx, index)) {
            duk_pop_2(ctx);
            continue;
        }
        // [key = duk_to_string(ctx, -2)]
        auto val = std::string(duk_to_string(ctx, index));
        result.push_back(std::move(val));
        duk_pop_2(ctx); /* pop_key */
    }

    duk_pop(ctx); // duk_enum
    return result;
}

std::vector<int> ScriptProperty::getIntArrayValue() const
{
    std::vector<int> result;
    if (!isValid()) {
        return result;
    }

    if (duk_is_null_or_undefined(ctx, index) || !duk_is_array(ctx, index)) {
        /* not an array */
        return result;
    }

    duk_enum(ctx, index, 0);
    while (duk_next(ctx, index, 1 /* get_value */)) {
        if (duk_is_string(ctx, index)) {
            duk_get_string(ctx, index);
            if (duk_is_null_or_undefined(ctx, index)) {
                duk_pop_2(ctx);
                continue;
            }
            result.push_back(stoiString(duk_to_string(ctx, index)));
        } else {
            if (duk_is_null_or_undefined(ctx, index)) {
                duk_pop_2(ctx);
                continue;
            }
            result.push_back(duk_to_int32(ctx, index));
        }
        duk_pop_2(ctx); /* pop_key */
    }
    duk_pop(ctx); // duk_enum

    return result;
}

std::string ScriptProperty::getStringValue() const
{
    if (!isValid()) {
        return "";
    }

    if (duk_is_null_or_undefined(ctx, -1) || !duk_to_string(ctx, -1)) {
        return "";
    }
    auto ret = duk_get_string(ctx, -1);
    return ret ? ret : "";
}

int ScriptProperty::getIntValue(int defValue) const
{
    if (!isValid()) {
        return defValue;
    }
    if (duk_is_null_or_undefined(ctx, index)) {
        return defValue;
    }
    return duk_to_int32(ctx, index);
}

int ScriptProperty::getBoolValue() const
{
    if (!isValid()) {
        return -1;
    }

    if (duk_is_null_or_undefined(ctx, -1)) {
        return -1;
    }
    return duk_to_boolean(ctx, -1);
}

std::vector<std::string> ScriptProperty::getPropertyNames() const
{
    std::vector<std::string> keys;
    duk_enum(ctx, -1, 0);
    while (duk_next(ctx, -1, 0 /*get_key*/)) {
        /* [ ... enum key ] */
        auto sym = std::string(duk_get_string(ctx, -1));
        keys.push_back(std::move(sym));
        duk_pop(ctx); /* pop_key */
    }
    duk_pop(ctx); // duk_enum
    return keys;
}

ScriptNamedProperty::ScriptNamedProperty(duk_context* ctx, std::string propertyName, int index)
    : ScriptProperty(ctx, index)
    , propertyName(std::move(propertyName))
{
    duk_get_prop_string(ctx, index, this->propertyName.c_str());
}

ScriptNamedProperty::~ScriptNamedProperty()
{
    duk_pop(ctx); // property
}
#endif
