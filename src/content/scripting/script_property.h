/*GRB*

    Gerbera - https://gerbera.io/

    script_property.h - this file is part of Gerbera.

    Copyright (C) 2024-2025 Gerbera Contributors

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

/// \file script_property.h

#ifndef __SCRIPTING_SCRIPT_PROPERTY_H__
#define __SCRIPTING_SCRIPT_PROPERTY_H__

#ifdef HAVE_JS

#include <duktape.h>
#include <functional>
#include <string>
#include <vector>

class ScriptProperty {
public:
    ScriptProperty(duk_context* ctx, int index = -1, bool global = false)
        : ctx(ctx)
        , index(index)
    {
        valid = global || duk_is_object_coercible(ctx, index);
    }

    virtual ~ScriptProperty();

    std::vector<std::string> getStringArrayValue() const;
    std::vector<int> getIntArrayValue() const;
    std::string getStringValue() const;
    int getIntValue(int defValue) const;
    int getBoolValue() const;
    std::vector<std::string> getPropertyNames() const;
    void getObject(const std::function<void()>& objectHandler) const
    {
        if (!duk_is_null_or_undefined(ctx, -1) && duk_is_object(ctx, -1)) {
            duk_to_object(ctx, -1);
            objectHandler();
        }
    }

protected:
    duk_context* ctx;
    int index;
    bool valid;
    bool isValid() const { return valid; };
};

class ScriptNamedProperty : public ScriptProperty {
public:
    ScriptNamedProperty(duk_context* ctx, std::string propertyName, int index = -1);
    ~ScriptNamedProperty() override;

private:
    std::string propertyName;
};

class ScriptGlobalProperty : public ScriptProperty {
public:
    ScriptGlobalProperty(duk_context* ctx, const std::string& propertyName, int index = -1)
        : ScriptProperty(ctx, index, true)
        , propertyName(propertyName)
    {
        duk_get_global_string(ctx, propertyName.c_str());
    }
    ~ScriptGlobalProperty() override
    {
        duk_pop(ctx); // property
    }

private:
    std::string propertyName;
};

class ScriptResultProperty : public ScriptProperty {
public:
    ScriptResultProperty(duk_context* ctx, int index = -1)
        : ScriptProperty(ctx, index, true)
    {
    }

    ~ScriptResultProperty() override = default;

private:
    std::string propertyName;
};

#endif // HAVE_JS

#endif // __SCRIPTING_SCRIPT_PROPERTY_H__
