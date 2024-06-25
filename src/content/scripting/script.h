/*MT*

    MediaTomb - http://www.mediatomb.cc/

    script.h - this file is part of MediaTomb.

    Copyright (C) 2005 Gena Batyan <bgeradz@mediatomb.cc>,
                       Sergey 'Jin' Bostandzhyan <jin@mediatomb.cc>

    Copyright (C) 2006-2010 Gena Batyan <bgeradz@mediatomb.cc>,
                            Sergey 'Jin' Bostandzhyan <jin@mediatomb.cc>,
                            Leonhard Wimmer <leo@mediatomb.cc>

    Copyright (C) 2016-2024 Gerbera Contributors

    MediaTomb is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License version 2
    as published by the Free Software Foundation.

    MediaTomb is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    version 2 along with MediaTomb; if not, write to the Free Software
    Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301, USA.

    $Id$
*/

/// \file script.h

#ifndef __SCRIPTING_SCRIPT_H__
#define __SCRIPTING_SCRIPT_H__

#include "util/grb_fs.h"

#include <duktape.h>
#include <map>
#include <mutex>
#include <vector>

// forward declaration
enum class AutoscanMediaMode;
class CdsContainer;
class CdsItem;
class CdsObject;
class Config;
class Content;
class Database;
class ScriptingRuntime;
class StringConverter;

// perform garbage collection after script has been run for x times
#define JS_CALL_GC_AFTER_NUM (1000)

enum charset_convert_t {
    M2I,
    F2I,
    J2I,
    P2I,
    I2I,
};

class Script {
public:
    virtual ~Script();

    Script(const Script&) = delete;
    Script& operator=(const Script&) = delete;

    void setProperty(const std::string& name, const std::string& value);
    void setIntProperty(const std::string& name, int value);

    void defineFunction(const std::string& name, duk_c_function function, std::uint32_t numParams);
    void defineFunctions(const duk_function_list_entry* functions);
    void load(const fs::path& scriptPath);
    void loadFolder(const fs::path& scriptFolder);

    std::shared_ptr<CdsObject> dukObject2cdsObject(const std::shared_ptr<CdsObject>& pcd);
    void cdsObject2dukObject(const std::shared_ptr<CdsObject>& obj);

    std::string convertToCharset(const std::string& str, charset_convert_t chr);
    virtual std::pair<std::shared_ptr<CdsObject>, int> createObject2cdsObject(const std::shared_ptr<CdsObject>& origObject, const std::string& rootPath) = 0;
    virtual bool setRefId(const std::shared_ptr<CdsObject>& cdsObj, const std::shared_ptr<CdsObject>& origObject, int pcdId) = 0;
    static Script* getContextScript(duk_context* ctx);

    std::shared_ptr<Database> getDatabase() const { return database; }
    std::shared_ptr<Content> getContent() const { return content; }
    std::string getOrigName() const { return objectName; }
    std::shared_ptr<CdsObject> getProcessedObject() const { return processed; }

protected:
    Script(const std::shared_ptr<Content>& content, const std::string& parent,
        const std::string& name, std::string objName, std::unique_ptr<StringConverter> sc);

    std::vector<int> execute(const std::shared_ptr<CdsObject>& obj, const std::string& rootPath);
    std::vector<int> call(const std::shared_ptr<CdsObject>& obj, const std::shared_ptr<CdsContainer>& cont, const std::string& functionName, const fs::path& rootPath, const std::string& containerType);
    void cleanup();
    int gc_counter {};
    void setMetaData(const std::shared_ptr<CdsObject>& obj, const std::shared_ptr<CdsItem>& item, const std::string& sym, const std::string& val) const;

    virtual void handleObject2cdsItem(duk_context* ctx, const std::shared_ptr<CdsObject>& pcd, const std::shared_ptr<CdsItem>& item) { }
    virtual void handleObject2cdsContainer(duk_context* ctx, const std::shared_ptr<CdsObject>& pcd, const std::shared_ptr<CdsContainer>& cont) { }
    virtual std::shared_ptr<CdsObject> createObject(const std::shared_ptr<CdsObject>& pcd);

    // object that is currently being processed by the script (set in import script)
    std::shared_ptr<CdsObject> processed;

    duk_context* ctx;

    std::shared_ptr<Config> config;
    std::shared_ptr<Database> database;
    std::shared_ptr<Content> content;
    std::shared_ptr<ScriptingRuntime> runtime;
    std::unique_ptr<StringConverter> sc;

private:
    bool hasCaseSensitiveNames;
    std::string entrySeparator;
    std::string contextName;
    std::string objectName;
    std::string scriptPath;
    void _load(const fs::path& scriptPath);
    void _execute();
    std::unique_ptr<StringConverter> _p2i;
    std::unique_ptr<StringConverter> _j2i;
    std::unique_ptr<StringConverter> _f2i;
    std::unique_ptr<StringConverter> _m2i;
    std::unique_ptr<StringConverter> _i2i;
};

#endif // __SCRIPTING_SCRIPT_H__
