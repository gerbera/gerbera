/*MT*

    MediaTomb - http://www.mediatomb.cc/

    script.cc - this file is part of MediaTomb.

    Copyright (C) 2005 Gena Batyan <bgeradz@mediatomb.cc>,
                       Sergey 'Jin' Bostandzhyan <jin@mediatomb.cc>

    Copyright (C) 2006-2010 Gena Batyan <bgeradz@mediatomb.cc>,
                            Sergey 'Jin' Bostandzhyan <jin@mediatomb.cc>,
                            Leonhard Wimmer <leo@mediatomb.cc>

    Copyright (C) 2016-2025 Gerbera Contributors

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

/// @file content/scripting/script.cc

#ifdef HAVE_JS
#define GRB_LOG_FAC GrbLogFacility::script
#include "script.h" // API

#include "cds/cds_container.h"
#include "cds/cds_item.h"
#include "config/config_definition.h"
#include "config/result/autoscan.h"
#include "config/result/box_layout.h"
#include "config/setup/config_setup_array.h"
#include "config/setup/config_setup_autoscan.h"
#include "config/setup/config_setup_boxlayout.h"
#include "config/setup/config_setup_dictionary.h"
#include "config/setup/config_setup_enum.h"
#include "content/autoscan_setting.h"
#include "content/content.h"
#include "context.h"
#include "database/database.h"
#include "duk_compat.h"
#include "js_functions.h"
#include "script_names.h"
#include "script_property.h"
#include "scripting_runtime.h"
#include "util/string_converter.h"
#include "util/tools.h"

#ifdef ONLINE_SERVICES
#include "content/onlineservice/online_service.h"
#endif

#include <array>
#include <fmt/chrono.h>

static constexpr std::array jsGlobalFunctions {
    duk_function_list_entry { "print2", js_print2, DUK_VARARGS },
    duk_function_list_entry { "print", js_print, DUK_VARARGS },
    duk_function_list_entry { "addCdsObject", js_addCdsObject, 3 },
    duk_function_list_entry { "addContainerTree", js_addContainerTree, 1 },
    duk_function_list_entry { "copyObject", js_copyObject, 1 },
    duk_function_list_entry { "f2i", js_f2i, 1 },
    duk_function_list_entry { "m2i", js_m2i, 1 },
    duk_function_list_entry { "p2i", js_p2i, 1 },
    duk_function_list_entry { "j2i", js_j2i, 1 },
    duk_function_list_entry { nullptr, nullptr, 0 },
};

void Script::setProperty(const std::string& name, const std::string& value, bool doEmpty)
{
    if (doEmpty || !value.empty()) {
        duk_push_string(ctx, value.c_str());
        duk_put_prop_string(ctx, -2, camelCaseString(name).c_str());
    }
}

void Script::setIntProperty(const std::string& name, int value)
{
    duk_push_int(ctx, value);
    duk_put_prop_string(ctx, -2, camelCaseString(name).c_str());
}

void Script::setIntProperty(const std::string& name, int value, int checkValue)
{
    if (value != checkValue) {
        duk_push_int(ctx, value);
        duk_put_prop_string(ctx, -2, camelCaseString(name).c_str());
    }
}

void Script::setBoolProperty(const std::string& name, bool value)
{
    duk_push_int(ctx, value ? 1 : 0);
    duk_put_prop_string(ctx, -2, camelCaseString(name).c_str());
}

/* **************** */

Script::Script(
    const std::shared_ptr<Content>& content,
    const std::string& parent,
    const std::string& name,
    std::string objName,
    bool needResult,
    std::shared_ptr<StringConverter> sc)
    : config(content->getContext()->getConfig())
    , database(content->getContext()->getDatabase())
    , converterManager(content->getContext()->getConverterManager())
    , definition(content->getContext()->getDefinition())
    , content(content)
    , runtime(content->getScriptingRuntime())
    , sc(std::move(sc))
    , contextName(fmt::format("{}_{}", name, parent))
    , objectName(std::move(objName))
{
    hasCaseSensitiveNames = config->getBoolOption(ConfigVal::IMPORT_CASE_SENSITIVE_TAGS);
    entrySeparator = config->getOption(ConfigVal::IMPORT_LIBOPTS_ENTRY_SEP);
    /* create a context and associate it with the JS run time */
    ScriptingRuntime::AutoLock lock(runtime->getMutex());
    replaceAllString(contextName, "/", "_");
    ctx = runtime->createContext(contextName);
    if (!ctx)
        throw_std_runtime_error("Scripting: could not initialize js context");

    _p2i = converterManager->p2i();
    _j2i = converterManager->j2i();
    _m2i = converterManager->m2i(ConfigVal::IMPORT_METADATA_CHARSET, "");
    _f2i = converterManager->f2i();
    _i2i = converterManager->i2i();
}

void Script::init()
{
    duk_push_thread_stash(ctx, ctx);
    duk_push_pointer(ctx, this);
    duk_put_prop_string(ctx, -2, "this");
    duk_pop(ctx);

    /* initialize contstants */
    for (auto&& [field, sym] : ot_names) {
        duk_push_int(ctx, field);
        duk_put_global_lstring(ctx, sym.data(), sym.size());
    }
#ifdef ONLINE_SERVICES
    duk_push_int(ctx, to_underlying(OnlineServiceType::None));
    duk_put_global_string(ctx, "ONLINE_SERVICE_NONE");
    duk_push_int(ctx, -1);
    duk_put_global_string(ctx, "ONLINE_SERVICE_YOUTUBE");
    duk_push_int(ctx, -1);
    duk_put_global_string(ctx, "ONLINE_SERVICE_APPLE_TRAILERS");
    duk_push_int(ctx, -1);
    duk_put_global_string(ctx, "ONLINE_SERVICE_SOPCAST");
#else // ONLINE SERVICES
    duk_push_int(ctx, 0);
    duk_put_global_string(ctx, "ONLINE_SERVICE_NONE");
    duk_push_int(ctx, -1);
    duk_put_global_string(ctx, "ONLINE_SERVICE_YOUTUBE");
    duk_push_int(ctx, -1);
    duk_put_global_string(ctx, "ONLINE_SERVICE_APPLE_TRAILERS");
    duk_push_int(ctx, -1);
    duk_put_global_string(ctx, "ONLINE_SERVICE_SOPCAST");
#endif // ONLINE_SERVICES

    // M_TITLE = "dc:title"
    for (auto&& [field, sym] : MetaEnumMapper::mt_keys) {
        auto s = getValueOrDefault(mt_names, field, { "" });
        if (!s.empty()) {
            duk_push_lstring(ctx, sym.data(), sym.size());
            duk_put_global_lstring(ctx, s.data(), s.length());
        }
    }

    // R_SIZE = "size"
    for (auto&& [field, sym] : res_names) {
        duk_push_string(ctx, EnumMapper::getAttributeName(field).c_str());
        duk_put_global_lstring(ctx, sym.data(), sym.length());
    }

    // UPNP_CLASS_MUSIC_ALBUM = "object.container.album.musicAlbum"
    for (auto&& [field, sym] : upnp_classes) {
        duk_push_lstring(ctx, field.data(), field.length());
        duk_put_global_lstring(ctx, sym.data(), sym.size());
    }

    for (auto&& [field, sym] : boxKeyNames) {
        duk_push_lstring(ctx, sym.data(), sym.length());
        duk_put_global_lstring(ctx, field.data(), field.length());
    }

    duk_push_object(ctx); // config
    for (auto&& i : ConfigOptionIterator()) {
        auto scs = definition->findConfigSetup(i, true);
        if (!scs)
            continue;
        setProperty(scs->getItemPathRoot(), scs->getCurrentValue(), false);
    }

    for (auto&& dcs : definition->getConfigSetupList<ConfigDictionarySetup>()) {
        duk_push_object(ctx);
        auto dictionary = dcs->getValue()->getDictionaryOption(true);
        for (auto&& [key, val] : dictionary) {
            setProperty(key.substr(5), val);
        }
        duk_put_prop_string(ctx, -2, dcs->getItemPathRoot().c_str());
    }

    for (auto&& acs : definition->getConfigSetupList<ConfigArraySetup>()) {
        auto array = acs->getValue()->getArrayOption(true);
        auto dukArray = duk_push_array(ctx);
        for (duk_uarridx_t i = 0; i < array.size(); i++) {
            auto&& entry = array[i];
            duk_push_string(ctx, entry.c_str());
            duk_put_prop_index(ctx, dukArray, i);
        }
        duk_put_prop_string(ctx, -2, acs->getItemPathRoot().c_str());
    }

    for (auto&& ascs : definition->getConfigSetupList<ConfigAutoscanSetup>()) {
        duk_push_object(ctx); // autoscan

        std::size_t idx = 0;
        std::string autoscanItemPath = ascs->getItemPathRoot(true); // prefix
        for (const auto& adir : content->getAutoscanDirectories()) {
            if (adir->getScanMode() == ascs->getScanMode()) {
                duk_push_object(ctx);

                setProperty(definition->removeAttribute(ConfigVal::A_AUTOSCAN_DIRECTORY_LOCATION), adir->getLocation());
                setProperty(definition->removeAttribute(ConfigVal::A_AUTOSCAN_DIRECTORY_MODE), AutoscanDirectory::mapScanmode(adir->getScanMode()));
                setIntProperty(definition->removeAttribute(ConfigVal::A_AUTOSCAN_DIRECTORY_INTERVAL), adir->getInterval().count());
                setBoolProperty(definition->removeAttribute(ConfigVal::A_AUTOSCAN_DIRECTORY_RECURSIVE), adir->getRecursive());
                setIntProperty(definition->removeAttribute(ConfigVal::A_AUTOSCAN_DIRECTORY_MEDIATYPE), adir->getMediaType());
                setBoolProperty(definition->removeAttribute(ConfigVal::A_AUTOSCAN_DIRECTORY_HIDDENFILES), adir->getHidden());
                setBoolProperty(definition->removeAttribute(ConfigVal::A_AUTOSCAN_DIRECTORY_DIRTYPES), adir->hasDirTypes());
                setBoolProperty(definition->removeAttribute(ConfigVal::A_AUTOSCAN_DIRECTORY_FORCE_REREAD_UNKNOWN), adir->getForceRescan());
                setIntProperty(definition->removeAttribute(ConfigVal::A_AUTOSCAN_DIRECTORY_SCANCOUNT), adir->getActiveScanCount());
                setIntProperty(definition->removeAttribute(ConfigVal::A_AUTOSCAN_DIRECTORY_TASKCOUNT), adir->getTaskCount());
                setIntProperty(definition->removeAttribute(ConfigVal::A_AUTOSCAN_DIRECTORY_RETRYCOUNT), adir->getRetryCount());
                setProperty(definition->removeAttribute(ConfigVal::A_AUTOSCAN_DIRECTORY_LMT), grbLocaltime("{:%Y-%m-%d %H:%M:%S}", adir->getPreviousLMT()));

                duk_put_prop_string(ctx, -2, fmt::to_string(adir->getScanID()).c_str());
                log_debug("Adding config[{}][{}] {}", autoscanItemPath, adir->getScanID(), adir->getLocation().string());
                idx++;
            }
        }
        duk_put_prop_string(ctx, -2, autoscanItemPath.c_str()); // autoscan
        log_debug("Adding config[{}] {}", autoscanItemPath, idx);
    }

    auto&& boxSetup = definition->findConfigSetup<ConfigSetup>(ConfigVal::A_BOXLAYOUT_BOX);
    auto&& chainSetup = definition->findConfigSetup<ConfigSetup>(ConfigVal::A_BOXLAYOUT_CHAIN);
    auto&& typeSetup = definition->findConfigSetup<ConfigEnumSetup<AutoscanMediaMode>>(ConfigVal::A_BOXLAYOUT_CHAIN_TYPE);
    for (auto&& bcs : definition->getConfigSetupList<ConfigBoxLayoutSetup>()) {
        auto boxLayoutList = bcs->getValue()->getBoxLayoutListOption();

        duk_push_object(ctx); // box-layout
        for (std::size_t i = 0; i < EDIT_CAST(EditHelperBoxLayout, boxLayoutList)->size(); i++) {
            duk_push_object(ctx);
            auto boxLayout = EDIT_CAST(EditHelperBoxLayout, boxLayoutList)->get(i);
            setIntProperty("id", boxLayout->getId());
            setIntProperty(definition->removeAttribute(ConfigVal::A_BOXLAYOUT_BOX_SIZE), boxLayout->getSize());
            setBoolProperty(definition->removeAttribute(ConfigVal::A_BOXLAYOUT_BOX_ENABLED), boxLayout->getEnabled());
            setProperty(definition->removeAttribute(ConfigVal::A_BOXLAYOUT_BOX_TITLE), boxLayout->getTitle());
            setProperty(definition->removeAttribute(ConfigVal::A_BOXLAYOUT_BOX_CLASS), boxLayout->getClass());
            setProperty(definition->removeAttribute(ConfigVal::A_BOXLAYOUT_BOX_UPNP_SHORTCUT), boxLayout->getUpnpShortcut());
            setProperty(definition->removeAttribute(ConfigVal::A_BOXLAYOUT_BOX_SORT_KEY), boxLayout->getSortKey());
            duk_put_prop_string(ctx, -2, boxLayout->getKey().c_str());
        }
        std::string boxLayoutItemPath = fmt::format("{}/{}", bcs->getItemPathRoot(true), boxSetup->getItemPathRoot(true));
        log_debug("Adding box config[{}] {}", boxLayoutItemPath, EDIT_CAST(EditHelperBoxLayout, boxLayoutList)->size());
        duk_put_prop_string(ctx, -2, boxLayoutItemPath.c_str()); // box-layout

        for (auto&& type : { AutoscanMediaMode::Audio, AutoscanMediaMode::Image, AutoscanMediaMode::Video }) {
            duk_push_object(ctx); // chain-layout
            std::size_t size = 0;
            for (std::size_t i = 0; i < EDIT_CAST(EditHelperBoxChain, boxLayoutList)->size(); i++) {
                auto&& chain = EDIT_CAST(EditHelperBoxChain, boxLayoutList)->get(i);
                if (chain->getType() == type) {
                    duk_push_object(ctx);
                    setIntProperty("index", chain->getIndex());
                    setIntProperty("size", chain->getLinksSize());
                    auto&& links = chain->getLinks();
                    for (std::size_t j = 0; j < links.size(); j++) {
                        duk_push_object(ctx);
                        auto&& link = links.at(j);
                        for (auto&& [key, value] : link) {
                            setProperty(key, value);
                        }
                        duk_put_prop_string(ctx, -2, fmt::to_string(j).c_str());
                    }
                    duk_put_prop_string(ctx, -2, fmt::to_string(chain->getIndex()).c_str());
                    ++size;
                }
            }
            setIntProperty("size", size);
            std::string chainLayoutItemPath = fmt::format("{}/{}/{}", bcs->getItemPathRoot(true), chainSetup->getItemPathRoot(true), typeSetup->mapEnumValue(type));
            log_debug("Adding chain config[{}] {}", chainLayoutItemPath, EDIT_CAST(EditHelperBoxChain, boxLayoutList)->size());
            duk_put_prop_string(ctx, -2, chainLayoutItemPath.c_str()); // chain-layout
        }
    }

    duk_put_global_string(ctx, "config");

    defineFunctions(jsGlobalFunctions.data());
    loadContent();
    runtime->addScript(shared_from_this());
}

void Script::loadContent()
{
    std::string commonFdrPath = config->getOption(ConfigVal::IMPORT_SCRIPTING_COMMON_FOLDER);

    if (commonFdrPath.empty()) {
        log_warning("Common script disabled in configuration");
    } else {
        loadFolder(commonFdrPath);
    }

    std::string customFdrPath = config->getOption(ConfigVal::IMPORT_SCRIPTING_CUSTOM_FOLDER);
    if (!customFdrPath.empty()) {
        loadFolder(customFdrPath);
    }
}

Script::~Script()
{
    runtime->destroyContext(contextName);
}

Script* Script::getContextScript(duk_context* ctx)
{
    duk_push_thread_stash(ctx, ctx);
    duk_get_prop_string(ctx, -1, "this");
    auto self = static_cast<Script*>(duk_get_pointer(ctx, -1));
    duk_pop_2(ctx);
    if (!self) {
        log_debug("Could not retrieve class instance from global object");
        duk_error(ctx, DUK_ERR_ERROR, "Could not retrieve current script from stash");
    }
    return self;
}

void Script::defineFunction(
    const std::string& name,
    duk_c_function function,
    std::uint32_t numParams)
{
    duk_push_c_function(ctx, function, numParams);
    duk_put_global_string(ctx, name.c_str());
}

void Script::defineFunctions(const duk_function_list_entry* functions)
{
    duk_push_global_object(ctx);
    duk_put_function_list(ctx, -1, functions);
    duk_pop(ctx);
}

void Script::_load(const fs::path& scriptPath)
{
    std::string scriptText = GrbFile(scriptPath).readTextFile();
    this->scriptPath = scriptPath;
    if (scriptText.empty())
        throw_std_runtime_error("empty script");

    auto j2i = converterManager->j2i();
    try {
        auto [mval, err] = j2i->convert(scriptText, true);
        if (!err.empty()) {
            log_warning("{}: {}", scriptPath.string(), err);
        }
        scriptText = mval;
    } catch (const std::runtime_error& e) {
        throw_std_runtime_error("Failed to convert import script: {}", e.what());
    }

    duk_push_string(ctx, scriptPath.c_str());
    if (duk_pcompile_lstring_filename(ctx, 0, scriptText.c_str(), scriptText.length()) != 0) {
        log_error("Failed to load script {}: {}", scriptPath.c_str(), duk_safe_to_stacktrace(ctx, -1));
        throw_std_runtime_error("Scripting: failed to compile {}", scriptPath.c_str());
    }
}

void Script::_execute()
{
    if (duk_pcall(ctx, 0) != DUK_EXEC_SUCCESS) {
        log_error("Failed to execute script {}: {}", scriptPath.c_str(), duk_safe_to_stacktrace(ctx, -1));
        throw_std_runtime_error("Script: failed to execute script");
    }
    duk_pop(ctx);
}

void Script::loadFolder(const fs::path& scriptFolder)
{
    log_debug("Loading folder {}", scriptFolder.c_str());
    std::error_code ec;
    auto rootEntry = fs::directory_entry(scriptFolder);
    if (!rootEntry.exists(ec) || !rootEntry.is_directory(ec)) {
        log_error("Script folder not found: {}", scriptFolder.c_str());
        return;
    }
    auto dirIterator = fs::directory_iterator(scriptFolder, ec);
    if (ec) {
        log_error("Failed to iterate {}, {}", scriptFolder.c_str(), ec.message());
        return;
    }
    for (auto&& dirEntry : dirIterator) {
        auto&& entryPath = dirEntry.path();
        if (entryPath.extension() == ".js") {
            try {
                _load(entryPath);
                _execute();
                log_debug("Loaded {}", entryPath.c_str());
            } catch (const std::runtime_error& e) {
                log_error("Unable to load {}: {}", entryPath.c_str(), e.what());
            }
        }
    }
}

std::vector<int> Script::call(
    const std::shared_ptr<CdsObject>& obj,
    const std::shared_ptr<CdsContainer>& cont,
    const std::string& functionName,
    const fs::path& rootPath,
    const std::string& containerType)
{
    // write global object used in callback functionss
    cdsObject2dukObject(obj);
    log_debug("wrote global object {} as {}", obj != nullptr, objectName);
    duk_put_global_string(ctx, objectName.c_str());

    // functionName(object, container, rootPath, autoScanId, containerType)
    // Push function onto stack
    if (!duk_get_global_string(ctx, functionName.c_str()) || !duk_is_function(ctx, -1)) {
        log_error("javascript function not found: {}()", functionName);
        duk_pop(ctx);
        throw_std_runtime_error("javascript function not found: {}()", functionName);
    }

    int narg = 0;

    // Push obj structure onto stack
    cdsObject2dukObject(obj);
    narg++;

    // Push cont structure onto stack
    auto par = cont ? cont : (obj->getParentID() >= CDS_ID_ROOT ? database->loadObject(obj->getParentID()) : nullptr);
    cdsObject2dukObject(par ? par : obj);
    narg++;

    // push rootPath onto stack
    duk_push_sprintf(ctx, "%s", rootPath.c_str());
    narg++;

    auto autoScan = content->getAutoscanDirectory(rootPath);

    if (autoScan && !rootPath.empty()) {
        // Push autoScanId onto stack
        duk_push_sprintf(ctx, "%d", autoScan->getScanID());
        narg++;
    } else {
        // Push autoScanId onto stack
        duk_push_sprintf(ctx, "%d", -1);
        narg++;
    }
    // Container Type onto stack
    duk_push_sprintf(ctx, "%s", containerType.c_str());
    narg++;

    if (duk_pcall(ctx, static_cast<duk_idx_t>(narg)) != DUK_EXEC_SUCCESS) {
        // Note: The invoked function will be blamed for execution errors, not the actual offending line of code
        // https://github.com/svaarala/duktape/blob/master/doc/error-objects.rst
        log_error("javascript {} runtime error: {}() - {}\n", contextName, functionName, duk_safe_to_stacktrace(ctx, -1));
        duk_pop(ctx);
        throw_std_runtime_error("javascript runtime error");
    }
    if (duk_is_null_or_undefined(ctx, -1) && needResult) {
        log_warning("Function '{}' did not return a value!", functionName);
        duk_pop(ctx);
        return {};
    }
    std::vector<int> result = ScriptResultProperty(ctx).getIntArrayValue();

    duk_pop(ctx);
    return result;
}

void Script::setMetaData(
    const std::shared_ptr<CdsObject>& obj,
    const std::shared_ptr<CdsItem>& item,
    const std::string& sym,
    const std::string& val) const
{
    if (obj->getTitle().empty() && sym == DC_TITLE)
        obj->setTitle(val);

    if (sym == UPNP_SEARCH_CLASS) {
        obj->setClass(val);
    } else if (sym == "mimetype" && item) {
        item->setMimeType(val);
    } else if (sym == MetaEnumMapper::getMetaFieldName(MetadataFields::M_TRACKNUMBER) && item) {
        int j = stoiString(val, 0);
        if (j > 0) {
            item->addMetaData(sym, val);
            item->setTrackNumber(j);
        } else
            item->setTrackNumber(0);
    } else if (sym == MetaEnumMapper::getMetaFieldName(MetadataFields::M_PARTNUMBER) && item) {
        int j = stoiString(val, 0);
        if (j > 0) {
            item->addMetaData(sym, val);
            item->setPartNumber(j);
        } else
            item->setPartNumber(0);
    } else {
        auto [mval, err] = sc->convert(val);
        if (!err.empty()) {
            log_warning("{}: {}", obj->getLocation().string(), err);
        }
        obj->addMetaData(sym, mval);
    }
}

std::shared_ptr<CdsObject> Script::createObject(const std::shared_ptr<CdsObject>& pcd)
{
    int objType = ScriptNamedProperty(ctx, "objectType").getIntValue(-1);
    if (objType == -1) {
        log_error("missing objectType property");
        return nullptr;
    }

    auto obj = CdsObject::createObject(objType);

    // CdsObject
    obj->setVirtual(true); // JS creates only virtual objects

    int id = ScriptNamedProperty(ctx, "id").getIntValue(INVALID_OBJECT_ID);
    if (id != INVALID_OBJECT_ID) {
        obj->setID(id);
    }
    int refId = ScriptNamedProperty(ctx, "refID").getIntValue(INVALID_OBJECT_ID);
    if (refId != INVALID_OBJECT_ID && refId != id) {
        obj->setRefID(refId);
    }
    int parentID = ScriptNamedProperty(ctx, "parentID").getIntValue(INVALID_OBJECT_ID);
    if (parentID != INVALID_OBJECT_ID) {
        obj->setParentID(parentID);
    }

    ScriptNamedProperty(ctx, "parent").getObject([&]() {
        auto parent = dukObject2cdsObject(nullptr);
        if (parent) {
            obj->setParent(parent);
            log_debug("dukObject2cdsObject: Parent {}", parent->getClass());
        }
    });

    // stuff that has not been exported to js
    if (pcd) {
        obj->setFlags(pcd->getFlags());
        obj->setResources(pcd->getResources());
        obj->setAuxData(pcd->getAuxData());
    } else {
        int flags = ScriptNamedProperty(ctx, "flags").getIntValue(-1);
        if (flags >= 0)
            obj->setFlags(flags);

        // get resources
        ScriptNamedProperty(ctx, "res").getObject([&]() {
            auto keys = ScriptProperty(ctx).getPropertyNames();

            int resCount = 0;
            // read resources
            for (auto&& sym : keys) {
                if (sym.find("handlerType") != std::string::npos) {
                    int ht = ScriptNamedProperty(ctx, sym).getIntValue(-1);
                    auto purpSym = fmt::format("{}:purpose", resCount);
                    int purpose = ScriptNamedProperty(ctx, purpSym).getIntValue(-1);
                    if (ht >= 0 && purpose >= 0) {
                        auto newRes = std::make_shared<CdsResource>(EnumMapper::remapContentHandler(ht), EnumMapper::remapPurpose(purpose));
                        obj->addResource(newRes);
                        newRes->setResId(resCount);
                    }
                    resCount++;
                }
            }
            // update resource attributes
            for (auto&& res : obj->getResources()) {
                resCount = res->getResId();
                // only attributes enumerated in res_names are allowed
                for (auto&& [key, upnp] : res_names) {
                    auto val = ScriptNamedProperty(ctx, resCount == 0 ? EnumMapper::getAttributeName(key) : fmt::format("{}-{}", resCount, EnumMapper::getAttributeName(key))).getStringValue();
                    if (!val.empty()) {
                        auto [mval, err] = sc->convert(val);
                        if (!err.empty()) {
                            log_warning("{}: {}", obj->getLocation().string(), err);
                        }
                        res->addAttribute(key, mval);
                        log_debug("add res attributes {}={}", key, mval);
                    }
                }
                auto head = fmt::format("{}#", resCount);
                for (auto&& sym : keys) {
                    if (sym.find(head) != std::string::npos) {
                        auto key = sym.substr(head.size());
                        auto val = ScriptNamedProperty(ctx, sym).getStringValue();
                        res->addParameter(key, val);
                    }
                }
                head = fmt::format("{}%", resCount);
                for (auto&& sym : keys) {
                    if (sym.find(head) != std::string::npos) {
                        auto key = sym.substr(head.size());
                        auto val = ScriptNamedProperty(ctx, sym).getStringValue();
                        res->addOption(key, val);
                    }
                }
            }
        });

        // update aux data
        ScriptNamedProperty(ctx, "aux").getObject([&]() {
            auto keys = ScriptProperty(ctx).getPropertyNames();
            for (auto&& sym : keys) {
                auto val = ScriptNamedProperty(ctx, sym).getStringValue();
                if (!val.empty()) {
                    auto [mval, err] = sc->convert(val);
                    if (!err.empty()) {
                        log_warning("{}: {}", obj->getLocation().string(), err);
                    }
                    obj->setAuxData(sym, mval);
                }
            }
        });
    }
    return obj;
}

std::shared_ptr<CdsObject> Script::dukObject2cdsObject(const std::shared_ptr<CdsObject>& pcd)
{
    auto obj = createObject(pcd);

    if (!obj)
        return nullptr;

    int mtime = ScriptNamedProperty(ctx, "mtime").getIntValue(0);
    if (mtime > 0) {
        obj->setMTime(std::chrono::seconds(mtime));
    }

    // update title
    {
        auto val = ScriptNamedProperty(ctx, "title").getStringValue();
        if (!val.empty()) {
            auto [mval, err] = sc->convert(val);
            if (!err.empty()) {
                log_warning("{}: {}", obj->getLocation().string(), err);
            }
            obj->setTitle(mval);
        } else if (pcd) {
            obj->setTitle(pcd->getTitle());
        }
    }
    // update sortKey
    {
        auto val = ScriptNamedProperty(ctx, "sortKey").getStringValue();
        if (!val.empty()) {
            auto [mval, err] = sc->convert(val);
            if (!err.empty()) {
                log_warning("{}: {}", obj->getLocation().string(), err);
            }
            obj->setSortKey(mval);
        } else {
            obj->setSortKey(obj->getTitle());
        }
    }

    // update upnpclass
    {
        auto val = ScriptNamedProperty(ctx, "upnpclass").getStringValue();
        if (!val.empty()) {
            auto [mval, err] = sc->convert(val);
            if (!err.empty()) {
                log_warning("{}: {}", obj->getLocation().string(), err);
            }
            obj->setClass(mval);
        } else if (pcd) {
            obj->setClass(pcd->getClass());
        }
    }

    // update restricted
    int restricted = ScriptNamedProperty(ctx, "restricted").getBoolValue();
    if (restricted >= 0)
        obj->setRestricted(restricted);

    // update metaData
    ScriptNamedProperty(ctx, "metaData").getObject([&]() {
        auto item = std::static_pointer_cast<CdsItem>(obj);
        auto keys = ScriptProperty(ctx).getPropertyNames();
        for (auto&& sym : keys) {
            auto arrayVal = ScriptNamedProperty(ctx, sym).getStringArrayValue();
            for (auto&& val : arrayVal) {
                if (!val.empty()) {
                    setMetaData(obj, item, sym, val);
                }
            }
        }
    });

    fs::path location = !hasCaseSensitiveNames && obj->isContainer() ? toLower(ScriptNamedProperty(ctx, "location").getStringValue()) : ScriptNamedProperty(ctx, "location").getStringValue();
    if (!location.empty()) {
        // location must not be touched by character conversion!
        obj->setLocation(location);
    }

    // update description
    auto description = ScriptNamedProperty(ctx, "description").getStringValue();
    if (!description.empty()) {
        auto [mval, err] = sc->convert(description);
        if (!err.empty()) {
            log_warning("{}: {}", obj->getLocation().string(), err);
        }
        obj->removeMetaData(MetadataFields::M_DESCRIPTION);
        obj->addMetaData(MetadataFields::M_DESCRIPTION, mval);
    }

    // CdsItem
    if (obj->isItem()) {
        auto item = std::static_pointer_cast<CdsItem>(obj);
        std::shared_ptr<CdsItem> pcdItem;

        if (pcd)
            pcdItem = std::static_pointer_cast<CdsItem>(pcd);

        // update mimetype
        auto mimetype = ScriptNamedProperty(ctx, "mimetype").getStringValue();
        if (!mimetype.empty()) {
            auto [mval, err] = sc->convert(mimetype);
            if (!err.empty()) {
                log_warning("{}: {}", obj->getLocation().string(), err);
            }
            item->setMimeType(mval);
        } else if (pcdItem) {
            item->setMimeType(pcdItem->getMimeType());
        }

        // update serviceID for onlineservice
        auto serviceID = ScriptNamedProperty(ctx, "serviceID").getStringValue();
        if (!serviceID.empty()) {
            auto [mval, err] = sc->convert(serviceID);
            if (!err.empty()) {
                log_warning("{}: {}", obj->getLocation().string(), err);
            }
            item->setServiceID(mval);
        }

        // update description if not set in script
        {
            auto val = ScriptNamedProperty(ctx, "description").getStringValue();
            if (!val.empty()) {
                auto [mval, err] = sc->convert(val);
                if (!err.empty()) {
                    log_warning("{}: {}", obj->getLocation().string(), err);
                }
                item->removeMetaData(MetadataFields::M_DESCRIPTION);
                item->addMetaData(MetadataFields::M_DESCRIPTION, mval);
            } else if (pcdItem && item->getMetaData(MetadataFields::M_DESCRIPTION).empty() && !pcdItem->getMetaData(MetadataFields::M_DESCRIPTION).empty()) {
                item->addMetaData(MetadataFields::M_DESCRIPTION, pcdItem->getMetaData(MetadataFields::M_DESCRIPTION));
            }
        }

        // update location if not set in script
        if (location.empty() && pcd) {
            obj->setLocation(pcd->getLocation());
        }

        // CdsExternalItem (like links)
        if (obj->isExternalItem()) {
            std::string protocolInfo;

            obj->setRestricted(true);

            // update protocol
            auto protocol = ScriptNamedProperty(ctx, "protocol").getStringValue();
            if (!protocol.empty()) {
                auto [mval, err] = sc->convert(protocol);
                if (!err.empty()) {
                    log_warning("{}: {}", obj->getLocation().string(), err);
                }
                protocolInfo = renderProtocolInfo(item->getMimeType(), mval);
            } else {
                protocolInfo = renderProtocolInfo(item->getMimeType(), PROTOCOL);
            }

            // add resources
            std::shared_ptr<CdsResource> resource;
            if (item->getResourceCount() == 0) {
                resource = std::make_shared<CdsResource>(ContentHandler::DEFAULT, ResourcePurpose::Content);
                item->addResource(resource);
            } else {
                resource = item->getResource(ContentHandler::DEFAULT);
            }
            resource->addAttribute(ResourceAttribute::PROTOCOLINFO, protocolInfo);
            int size = ScriptNamedProperty(ctx, "size").getIntValue(-1);
            if (size > -1) {
                resource->addAttribute(ResourceAttribute::SIZE, fmt::to_string(size));
            }
        }

        handleObject2cdsItem(ctx, pcd, item);
    }

    // CdsDirectory
    if (obj->isContainer()) {
        auto cont = std::static_pointer_cast<CdsContainer>(obj);
        auto id = ScriptNamedProperty(ctx, "updateID").getIntValue(-1);
        if (id >= CDS_ID_ROOT)
            cont->setUpdateID(id);

        // update searchable
        int searchable = ScriptNamedProperty(ctx, "searchable").getBoolValue();
        log_debug("{} searchable {}", cont->getTitle(), searchable);
        if (searchable >= 0)
            cont->setSearchable(searchable);

        // update upnpShortcut
        auto upnpShortcut = ScriptNamedProperty(ctx, "upnpShortcut").getStringValue();
        if (!upnpShortcut.empty()) {
            auto [mval, err] = sc->convert(upnpShortcut);
            if (!err.empty()) {
                log_warning("{}: {}", obj->getLocation().string(), err);
            }
            cont->setUpnpShortcut(mval);
        }

        handleObject2cdsContainer(ctx, pcd, cont);
    }

    return obj;
}

bool Script::isHiddenFile(const std::shared_ptr<CdsObject>& cdsObj, const std::string& rootPath)
{
    AutoScanSetting asSetting;
    asSetting.recursive = true;
    asSetting.followSymlinks = config->getBoolOption(ConfigVal::IMPORT_FOLLOW_SYMLINKS);
    asSetting.hidden = config->getBoolOption(ConfigVal::IMPORT_HIDDEN_FILES);
    asSetting.mergeOptions(config, rootPath);
    return content->isHiddenFile(fs::directory_entry(cdsObj->getLocation()), false, asSetting);
}

void Script::cdsObject2dukObject(const std::shared_ptr<CdsObject>& obj)
{
    duk_push_object(ctx);

    // CdsObject
    setIntProperty("objectType", obj->getObjectType());
    setIntProperty("id", obj->getID(), INVALID_OBJECT_ID);
    setIntProperty("parentID", obj->getParentID(), INVALID_OBJECT_ID);

    setProperty("title", obj->getTitle(), false);
    setProperty("sortKey", obj->getSortKey(), false);
    setProperty("upnpclass", obj->getClass(), false);
    setProperty("location", obj->getLocation(), false);

    setIntProperty("mtime", static_cast<int>(obj->getMTime().count()));
    setIntProperty("utime", static_cast<int>(obj->getUTime().count()));
    setIntProperty("sizeOnDisk", static_cast<int>(obj->getSizeOnDisk()));
    setIntProperty("flags", obj->getFlags());

    setBoolProperty("restricted", obj->isRestricted());
    setBoolProperty("theora", obj->getFlag(OBJECT_FLAG_OGG_THEORA));

#ifdef ONLINE_SERVICES
    if (obj->getFlag(OBJECT_FLAG_ONLINE_SERVICE)) {
        auto service = OnlineServiceType(std::stoi(obj->getAuxData(ONLINE_SERVICE_AUX_ID)));
        setIntProperty("onlineservice", static_cast<int>(service));
    } else
#endif
        setIntProperty("onlineservice", 0);

    // setting legacy metadata

    auto item = std::static_pointer_cast<CdsItem>(obj);
    auto metaGroups = obj->getMetaGroups();
    {
        duk_push_object(ctx);
        // stack: js meta_js

        for (auto&& [key, attr] : metaGroups) {
            setProperty(key, fmt::format("{}", fmt::join(attr, entrySeparator)));
        }
        if (item && item->getTrackNumber() > 0)
            setProperty(MetaEnumMapper::getMetaFieldName(MetadataFields::M_TRACKNUMBER), fmt::to_string(item->getTrackNumber()));

        if (item && item->getPartNumber() > 0)
            setProperty(MetaEnumMapper::getMetaFieldName(MetadataFields::M_PARTNUMBER), fmt::to_string(item->getPartNumber()));

        duk_put_prop_string(ctx, -2, "meta");
        // stack: js
    }
    // setting metadata
    {
        duk_push_object(ctx);
        // stack: js meta_js
        if (item && item->getTrackNumber() > 0) {
            metaGroups[MetaEnumMapper::getMetaFieldName(MetadataFields::M_TRACKNUMBER)] = { fmt::to_string(item->getTrackNumber()) };
        }
        if (item && item->getPartNumber() > 0) {
            metaGroups[MetaEnumMapper::getMetaFieldName(MetadataFields::M_PARTNUMBER)] = { fmt::to_string(item->getPartNumber()) };
        }
        for (auto&& [key, array] : metaGroups) {
            auto dukArray = duk_push_array(ctx);
            for (duk_uarridx_t i = 0; i < array.size(); i++) {
                duk_push_string(ctx, array[i].c_str());
                duk_put_prop_index(ctx, dukArray, i);
            }
            duk_put_prop_string(ctx, -2, key.c_str());
        }

        duk_put_prop_string(ctx, -2, "metaData");
        // stack: js
    }

    // setting auxdata
    {
        duk_push_object(ctx);
        // stack: js aux_js

        auto aux = obj->getAuxData();

        for (auto&& [key, attr] : aux) {
            setProperty(key, attr);
        }

        duk_put_prop_string(ctx, -2, "aux");
        // stack: js
    }

    // setting resource
    {
        duk_push_object(ctx);
        // stack: js res_js
        setProperty("count", fmt::to_string(obj->getResourceCount()));
        if (obj->getResourceCount() > 0) {
            std::size_t resCount = 0;
            for (auto&& res : obj->getResources()) {
                setProperty(fmt::format("{}:handlerType", resCount), fmt::to_string(to_underlying(res->getHandlerType())));
                setProperty(fmt::format("{}:purpose", resCount), fmt::to_string(to_underlying(res->getPurpose())));
                auto attributes = res->getAttributes();
                for (auto&& [key, attr] : attributes) {
                    setProperty(resCount == 0 ? EnumMapper::getAttributeName(key) : fmt::format("{}-{}", resCount, EnumMapper::getAttributeName(key)), attr);
                }
                auto parameters = res->getParameters();
                for (auto&& [key, param] : parameters) {
                    setProperty(fmt::format("{}#{}", resCount, key), param);
                }
                auto options = res->getOptions();
                for (auto&& [key, opt] : options) {
                    setProperty(fmt::format("{}%{}", resCount, key), opt);
                }
                resCount++;
            }
        }

        duk_put_prop_string(ctx, -2, "res");
        // stack: js
    }

    // CdsItem
    if (obj->isItem() && item) {
        setIntProperty("trackNumber", item->getTrackNumber());
        setIntProperty("partNumber", item->getPartNumber());
        setProperty("mimetype", item->getMimeType(), false);
        setProperty("serviceID", item->getServiceID(), false);
    }

    // CdsDirectory
    if (obj->isContainer()) {
        auto cont = std::static_pointer_cast<CdsContainer>(obj);
        setIntProperty("updateID", cont->getUpdateID());
        setBoolProperty("searchable", cont->isSearchable());
    }
}

std::string Script::convertToCharset(const std::string& str, CharsetConversion chr)
{
    auto convCode = [=]() {
        switch (chr) {
        case CharsetConversion::P2I:
            return _p2i->convert(str);
        case CharsetConversion::M2I:
            return _m2i->convert(str);
        case CharsetConversion::F2I:
            return _f2i->convert(str);
        case CharsetConversion::J2I:
            return _j2i->convert(str);
        case CharsetConversion::I2I:
            return _i2i->convert(str);
        default:
            throw_std_runtime_error("Illegal charset given to convertToCharset(): {}", chr);
        }
    };
    auto [mval, err] = convCode();
    if (!err.empty()) {
        log_warning("{}: {}", str, err);
    }
    return mval;
}

#endif // HAVE_JS
