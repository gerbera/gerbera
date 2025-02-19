/*MT*

    MediaTomb - http://www.mediatomb.cc/

    web/pages.h - this file is part of MediaTomb.

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

/// \file web/pages.h
/// \brief Defines the various PageRequest sub classes which process requests coming from WebUi.
#ifndef __WEB_PAGES_H__
#define __WEB_PAGES_H__

#include "page_request.h"

#include "util/grb_fs.h"

#include <chrono>
#include <vector>

// forward declarations
class AutoscanDirectory;
class CdsItemExternalURL;
class CdsItem;
class Config;
class ConfigDefinition;
class ConfigSetup;
class ConfigValue;
class ConverterManager;
class Database;
class UpnpXMLBuilder;
enum class ConfigVal;

namespace Web {

/// \brief Authentication handler (used over AJAX)
class Auth : public PageRequest {
protected:
    std::chrono::seconds timeout = std::chrono::seconds::zero();
    bool accountsEnabled { false };

    void process(pugi::xml_node& root) override;
    void processPageAction(pugi::xml_node& element) override {};
    /// \brief get server configuration parts for UI
    void getConfig(pugi::xml_node& element);
    /// \brief get current session id
    void getSid(pugi::xml_node& element);
    /// \brief get token for login
    void getToken(pugi::xml_node& element);
    /// \brief handle login action
    void login();
    /// \brief handle logout action
    void logout();

public:
    explicit Auth(const std::shared_ptr<Content>& content,
        const std::shared_ptr<Server>& server,
        const std::shared_ptr<UpnpXMLBuilder>& xmlBuilder,
        const std::shared_ptr<Quirks>& quirks);

    const static std::string PAGE;
    std::string getPage() const override { return PAGE; }
};

/// \brief Call from WebUi to create container tree in database view
class Containers : public PageRequest {
    using PageRequest::PageRequest;

public:
    const static std::string PAGE;
    std::string getPage() const override { return PAGE; }

protected:
    void processPageAction(pugi::xml_node& element) override;
};

/// \brief Call from WebUi to create directory tree in filesystem view
class Directories : public PageRequest {
public:
    explicit Directories(const std::shared_ptr<Content>& content,
        std::shared_ptr<ConverterManager> converterManager,
        const std::shared_ptr<Server>& server,
        const std::shared_ptr<UpnpXMLBuilder>& xmlBuilder,
        const std::shared_ptr<Quirks>& quirks);

    const static std::string PAGE;
    std::string getPage() const override { return PAGE; }

protected:
    void processPageAction(pugi::xml_node& element) override;

    std::shared_ptr<ConverterManager> converterManager;
};

/// \brief Call from WebUi to list files in filesystem view
class Files : public PageRequest {
public:
    explicit Files(const std::shared_ptr<Content>& content,
        std::shared_ptr<ConverterManager> converterManager,
        const std::shared_ptr<Server>& server,
        const std::shared_ptr<UpnpXMLBuilder>& xmlBuilder,
        const std::shared_ptr<Quirks>& quirks);

    const static std::string PAGE;
    std::string getPage() const override { return PAGE; }

protected:
    void processPageAction(pugi::xml_node& element) override;

    std::shared_ptr<ConverterManager> converterManager;
};

/// \brief Call from WebUi to list items in database view
class Items : public PageRequest {
    using PageRequest::PageRequest;

public:
    const static std::string PAGE;
    std::string getPage() const override { return PAGE; }

protected:
    void processPageAction(pugi::xml_node& element) override;
    /// \brief run browse according to page request parameters
    std::vector<std::shared_ptr<CdsObject>> doBrowse(
        const std::shared_ptr<CdsObject>& container,
        int start,
        int count,
        pugi::xml_node& items,
        std::string& trackFmt);
    /// \brief run search according to page request parameters
    std::vector<std::shared_ptr<CdsObject>> doSearch(
        const std::shared_ptr<CdsObject>& container,
        int start,
        int count,
        pugi::xml_node& items,
        std::string& trackFmt);
};

/// \brief Call from WebUi to add item from filesystem view
class Add : public PageRequest {
    using PageRequest::PageRequest;

public:
    const static std::string PAGE;
    std::string getPage() const override { return PAGE; }

protected:
    void processPageAction(pugi::xml_node& element) override;
};

/// \brief Call from WebUi to Remove item in database view
class Remove : public PageRequest {
    using PageRequest::PageRequest;

public:
    const static std::string PAGE;
    std::string getPage() const override { return PAGE; }

protected:
    void processPageAction(pugi::xml_node& element) override;
};

/// \brief Call from WebUi to Edit Item in database view
class EditLoad : public PageRequest {
    using PageRequest::PageRequest;

public:
    const static std::string PAGE;
    std::string getPage() const override { return PAGE; }

protected:
    void processPageAction(pugi::xml_node& element) override;
    /// \brief write object core info
    pugi::xml_node writeCoreInfo(
        const std::shared_ptr<CdsObject>& obj,
        pugi::xml_node& element,
        int objectID);
    /// \brief write cdsitem info
    void writeItemInfo(
        const std::shared_ptr<CdsItem>& objItem,
        int objectID,
        pugi::xml_node& item,
        pugi::xml_node& metaData);
    /// \brief write cdscontainer info
    void writeContainerInfo(
        const std::shared_ptr<CdsObject>& obj,
        pugi::xml_node& item);
    /// \brief write resource info
    void writeResourceInfo(
        const std::shared_ptr<CdsObject>& obj,
        pugi::xml_node& item);
    /// \brief write metadata
    pugi::xml_node writeMetadata(
        const std::shared_ptr<CdsObject>& obj,
        pugi::xml_node& item);
    /// \brief write auxdata
    void writeAuxData(
        const std::shared_ptr<CdsObject>& obj,
        pugi::xml_node& item);
};

/// \brief Call from WebUi to Save Item properties in database view
class EditSave : public PageRequest {
    using PageRequest::PageRequest;

public:
    const static std::string PAGE;
    std::string getPage() const override { return PAGE; }

protected:
    void processPageAction(pugi::xml_node& element) override;
};

/// \brief Call from WebUi to Add Object in database view
class AddObject : public PageRequest {
    using PageRequest::PageRequest;

public:
    const static std::string PAGE;
    std::string getPage() const override { return PAGE; }

protected:
    void processPageAction(pugi::xml_node& element) override;
    /// \brief handle page request to add a container
    void addContainer(
        int parentID,
        const std::string& title,
        const std::string& upnp_class);
    /// \brief handle page request to add a file item
    std::shared_ptr<CdsItem> addItem(
        int parentID,
        const std::string& title,
        const std::string& upnp_class,
        const fs::path& location);
    /// \brief handle page request to add an external url as item
    std::shared_ptr<CdsItemExternalURL> addUrl(
        int parentID,
        const std::string& title,
        const std::string& upnp_class,
        bool addProtocol,
        const fs::path& location);
    /// \brief check if file is hidden according to settings
    bool isHiddenFile(const std::shared_ptr<CdsObject>& cdsObj);
};

/// \brief Call from WebUi to add or remove autoscan
class Autoscan : public PageRequest {
    using PageRequest::PageRequest;

public:
    const static std::string PAGE;
    std::string getPage() const override { return PAGE; }

protected:
    void processPageAction(pugi::xml_node& element) override;
    /// \brief Convert autoscan dir to xml for web response
    static void autoscan2XML(
        const std::shared_ptr<AutoscanDirectory>& adir,
        pugi::xml_node& element);
    /// \brief get details for autoscan editor
    void editLoad(
        bool fromFs,
        pugi::xml_node& element,
        const std::string& objID,
        const fs::path& path);
    /// \brief list all autoscan directories
    void list(pugi::xml_node& element);
    /// \brief Save new or changed autoscan
    void editSave(bool fromFs, const fs::path& path);
    /// \brief Run full scan immediately
    void runScan(bool fromFs, const fs::path& path);
};

/// \brief Call from WebUi to do nothing :)
class VoidType : public PageRequest {
    using PageRequest::PageRequest;

public:
    const static std::string PAGE;
    std::string getPage() const override { return PAGE; }

public:
    void processPageAction(pugi::xml_node& element) override;
};

/// \brief task list and task cancel
class Tasks : public PageRequest {
    using PageRequest::PageRequest;

public:
    const static std::string PAGE;
    std::string getPage() const override { return PAGE; }

protected:
    void processPageAction(pugi::xml_node& element) override;
};

/// \brief UI action button
class Action : public PageRequest {
    using PageRequest::PageRequest;

public:
    const static std::string PAGE;
    std::string getPage() const override { return PAGE; }

protected:
    void processPageAction(pugi::xml_node& element) override;
};

/// \brief Browse clients list
class Clients : public PageRequest {
    using PageRequest::PageRequest;

public:
    const static std::string PAGE;
    std::string getPage() const override { return PAGE; }

protected:
    void processPageAction(pugi::xml_node& element) override;
};

/// \brief Call from WebUi to load configuration
class ConfigLoad : public PageRequest {
protected:
    std::vector<ConfigValue> dbEntries;
    std::map<std::string, std::string> allItems;
    std::shared_ptr<ConfigDefinition> definition;

    void processPageAction(pugi::xml_node& element) override;
    /// \brief create config value entry
    void createItem(
        pugi::xml_node& item,
        const std::string& name,
        ConfigVal id,
        ConfigVal aid,
        const std::shared_ptr<ConfigSetup>& cs = nullptr);

    /// \brief write values with counters and statitics
    void writeDatabaseStatus(pugi::xml_node& values);
    /// \brief add shortcut configuration
    void writeShortcuts(pugi::xml_node& values);
    /// \brief add boolean, integer and string properties
    void writeSimpleProperties(pugi::xml_node& values);
    /// \brief add all dictionary configuration options
    void writeDictionaries(pugi::xml_node& values);
    /// \brief add all vector configuration options
    void writeVectors(pugi::xml_node& values);
    /// \brief add all array configuration options
    void writeArrays(pugi::xml_node& values);
    /// \brief add client configuration
    void writeClientConfig(pugi::xml_node& values);
    /// \brief add directory tweaks
    void writeImportTweaks(pugi::xml_node& values);
    /// \brief add dynamic folder configuration
    void writeDynamicContent(pugi::xml_node& values);
    /// \brief add box layout
    void writeBoxLayout(pugi::xml_node& values);
    /// \brief add transcoding
    void writeTranscoding(pugi::xml_node& values);
    /// \brief add autoscan details
    void writeAutoscan(pugi::xml_node& values);
    /// \brief overwrite values from xml with database values
    void updateEntriesFromDatabase(pugi::xml_node& element, pugi::xml_node& values);

    /// \brief write value depending on type
    template <typename T>
    static void setValue(pugi::xml_node& item, const T& value);

    /// \brief create meta information on config value
    static void addTypeMeta(pugi::xml_node& meta, const std::shared_ptr<ConfigSetup>& cs);

public:
    explicit ConfigLoad(const std::shared_ptr<Content>& content,
        const std::shared_ptr<Server>& server,
        const std::shared_ptr<UpnpXMLBuilder>& xmlBuilder,
        const std::shared_ptr<Quirks>& quirks);

    const static std::string PAGE;
    std::string getPage() const override { return PAGE; }
};

/// \brief Call from WebUi to save configuration
class ConfigSave : public PageRequest {
protected:
    std::shared_ptr<Context> context;
    std::shared_ptr<ConfigDefinition> definition;

    void processPageAction(pugi::xml_node& element) override;

public:
    explicit ConfigSave(std::shared_ptr<Context> context,
        const std::shared_ptr<Content>& content,
        const std::shared_ptr<Server>& server,
        const std::shared_ptr<UpnpXMLBuilder>& xmlBuilder,
        const std::shared_ptr<Quirks>& quirks);

    const static std::string PAGE;
    std::string getPage() const override { return PAGE; }
};

} // namespace Web

#endif // __WEB_PAGES_H__
