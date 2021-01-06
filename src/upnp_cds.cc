/*MT*

    MediaTomb - http://www.mediatomb.cc/

    upnp_cds.cc - this file is part of MediaTomb.

    Copyright (C) 2005 Gena Batyan <bgeradz@mediatomb.cc>,
                       Sergey 'Jin' Bostandzhyan <jin@mediatomb.cc>

    Copyright (C) 2006-2010 Gena Batyan <bgeradz@mediatomb.cc>,
                            Sergey 'Jin' Bostandzhyan <jin@mediatomb.cc>,
                            Leonhard Wimmer <leo@mediatomb.cc>

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

/// \file upnp_cds.cc

#include "upnp_cds.h" // API

#include <memory>
#include <string>
#include <utility>
#include <vector>

#include "config/config_manager.h"
#include "database/database.h"
#include "search_handler.h"
#include "server.h"
#include "util/upnp_quirks.h"

ContentDirectoryService::ContentDirectoryService(std::shared_ptr<Config> config,
    std::shared_ptr<Database> database,
    UpnpXMLBuilder* xmlBuilder, UpnpDevice_Handle deviceHandle, int stringLimit)
    : systemUpdateID(0)
    , stringLimit(stringLimit)
    , config(std::move(config))
    , database(std::move(database))
    , deviceHandle(deviceHandle)
    , xmlBuilder(xmlBuilder)
{
}

ContentDirectoryService::~ContentDirectoryService() = default;

void ContentDirectoryService::doBrowse(const std::unique_ptr<ActionRequest>& request)
{
    log_debug("start");

    auto req = request->getRequest();
    auto req_root = req->document_element();
    std::string objID = req_root.child("ObjectID").text().as_string();
    std::string BrowseFlag = req_root.child("BrowseFlag").text().as_string();
    //std::string Filter; // not yet supported
    std::string StartingIndex = req_root.child("StartingIndex").text().as_string();
    std::string RequestedCount = req_root.child("RequestedCount").text().as_string();
    // std::string SortCriteria; // not yet supported

    log_debug("Browse received parameters: ObjectID [{}] BrowseFlag [{}] StartingIndex [{}] RequestedCount [{}]",
        objID.c_str(), BrowseFlag.c_str(), StartingIndex.c_str(), RequestedCount.c_str());

    int objectID;
    if (objID.empty())
        throw UpnpException(UPNP_E_NO_SUCH_ID, "empty object id");

    objectID = std::stoi(objID);

    unsigned int flag = BROWSE_ITEMS | BROWSE_CONTAINERS | BROWSE_EXACT_CHILDCOUNT;

    if (BrowseFlag == "BrowseDirectChildren")
        flag |= BROWSE_DIRECT_CHILDREN;
    else if (BrowseFlag != "BrowseMetadata")
        throw UpnpException(UPNP_SOAP_E_INVALID_ARGS,
            "invalid browse flag: " + BrowseFlag);

    auto parent = database->loadObject(objectID);
    if ((parent->getClass() == UPNP_CLASS_MUSIC_ALBUM) || (parent->getClass() == UPNP_CLASS_PLAYLIST_CONTAINER))
        flag |= BROWSE_TRACK_SORT;

    if (config->getBoolOption(CFG_SERVER_HIDE_PC_DIRECTORY))
        flag |= BROWSE_HIDE_FS_ROOT;

    auto param = std::make_unique<BrowseParam>(objectID, flag);

    param->setStartingIndex(std::stoi(StartingIndex));
    param->setRequestedCount(std::stoi(RequestedCount));

    std::vector<std::shared_ptr<CdsObject>> arr;
    try {
        arr = database->browse(param);
    } catch (const std::runtime_error& e) {
        throw UpnpException(UPNP_E_NO_SUCH_ID, "no such object");
    }

    pugi::xml_document didl_lite;
    auto decl = didl_lite.prepend_child(pugi::node_declaration);
    decl.append_attribute("version") = "1.0";
    decl.append_attribute("encoding") = "UTF-8";
    auto didl_lite_root = didl_lite.append_child("DIDL-Lite");
    didl_lite_root.append_attribute(UPNP_XML_DIDL_LITE_NAMESPACE_ATTR) = UPNP_XML_DIDL_LITE_NAMESPACE;
    didl_lite_root.append_attribute(UPNP_XML_DC_NAMESPACE_ATTR) = UPNP_XML_DC_NAMESPACE;
    didl_lite_root.append_attribute(UPNP_XML_UPNP_NAMESPACE_ATTR) = UPNP_XML_UPNP_NAMESPACE;
    didl_lite_root.append_attribute(UPNP_XML_SEC_NAMESPACE_ATTR) = UPNP_XML_SEC_NAMESPACE;

    for (const auto& obj : arr) {
        if (config->getBoolOption(CFG_SERVER_EXTOPTS_MARK_PLAYED_ITEMS_ENABLED) && obj->getFlag(OBJECT_FLAG_PLAYED)) {
            std::string title = obj->getTitle();
            if (config->getBoolOption(CFG_SERVER_EXTOPTS_MARK_PLAYED_ITEMS_STRING_MODE_PREPEND))
                title = config->getOption(CFG_SERVER_EXTOPTS_MARK_PLAYED_ITEMS_STRING).append(title);
            else
                title.append(config->getOption(CFG_SERVER_EXTOPTS_MARK_PLAYED_ITEMS_STRING));

            obj->setTitle(title);
        }

        xmlBuilder->renderObject(obj, stringLimit, &didl_lite_root);
    }

    std::ostringstream buf;
    didl_lite.print(buf, "", 0);
    std::string didl_lite_xml = buf.str();

    auto response = UpnpXMLBuilder::createResponse(request->getActionName(), UPNP_DESC_CDS_SERVICE_TYPE);
    auto resp_root = response->document_element();
    resp_root.append_child("Result").append_child(pugi::node_pcdata).set_value(didl_lite_xml.c_str());
    resp_root.append_child("NumberReturned").append_child(pugi::node_pcdata).set_value(std::to_string(arr.size()).c_str());
    resp_root.append_child("TotalMatches").append_child(pugi::node_pcdata).set_value(std::to_string(param->getTotalMatches()).c_str());
    resp_root.append_child("UpdateID").append_child(pugi::node_pcdata).set_value(std::to_string(systemUpdateID).c_str());
    request->setResponse(response);

    log_debug("end");
}

void ContentDirectoryService::doSearch(const std::unique_ptr<ActionRequest>& request)
{
    log_debug("start");

    auto req = request->getRequest();
    auto req_root = req->document_element();
    std::string containerID = req_root.child("ContainerID").text().as_string();
    std::string searchCriteria = req_root.child("SearchCriteria").text().as_string();
    std::string startingIndex = req_root.child("StartingIndex").text().as_string();
    std::string requestedCount = req_root.child("RequestedCount").text().as_string();

    log_debug("Search received parameters: ContainerID [{}] SearchCriteria [{}] StartingIndex [{}] RequestedCount [{}]",
        containerID.c_str(), searchCriteria.c_str(), startingIndex.c_str(), requestedCount.c_str());

    pugi::xml_document didl_lite;
    auto decl = didl_lite.prepend_child(pugi::node_declaration);
    decl.append_attribute("version") = "1.0";
    decl.append_attribute("encoding") = "UTF-8";
    auto didl_lite_root = didl_lite.append_child("DIDL-Lite");
    didl_lite_root.append_attribute(UPNP_XML_DIDL_LITE_NAMESPACE_ATTR) = UPNP_XML_DIDL_LITE_NAMESPACE;
    didl_lite_root.append_attribute(UPNP_XML_DC_NAMESPACE_ATTR) = UPNP_XML_DC_NAMESPACE;
    didl_lite_root.append_attribute(UPNP_XML_UPNP_NAMESPACE_ATTR) = UPNP_XML_UPNP_NAMESPACE;
    didl_lite_root.append_attribute(UPNP_XML_SEC_NAMESPACE_ATTR) = UPNP_XML_SEC_NAMESPACE;

    auto searchParam = std::make_unique<SearchParam>(containerID, searchCriteria,
        std::stoi(startingIndex, nullptr), std::stoi(requestedCount, nullptr));

    std::vector<std::shared_ptr<CdsObject>> results;
    int numMatches = 0;
    try {
        results = database->search(searchParam, &numMatches);
    } catch (const std::runtime_error& e) {
        log_debug(e.what());
        throw UpnpException(UPNP_E_NO_SUCH_ID, "no such object");
    }

    for (const auto& cdsObject : results) {
        if (config->getBoolOption(CFG_SERVER_EXTOPTS_MARK_PLAYED_ITEMS_ENABLED) && cdsObject->getFlag(OBJECT_FLAG_PLAYED)) {
            std::string title = cdsObject->getTitle();
            if (config->getBoolOption(CFG_SERVER_EXTOPTS_MARK_PLAYED_ITEMS_STRING_MODE_PREPEND))
                title = config->getOption(CFG_SERVER_EXTOPTS_MARK_PLAYED_ITEMS_STRING).append(title);
            else
                title.append(config->getOption(CFG_SERVER_EXTOPTS_MARK_PLAYED_ITEMS_STRING));

            cdsObject->setTitle(title);
        }

        xmlBuilder->renderObject(cdsObject, stringLimit, &didl_lite_root);
    }

    std::ostringstream buf;
    didl_lite.print(buf, "", 0);
    std::string didl_lite_xml = buf.str();

    auto response = UpnpXMLBuilder::createResponse(request->getActionName(), UPNP_DESC_CDS_SERVICE_TYPE);
    auto resp_root = response->document_element();
    resp_root.append_child("Result").append_child(pugi::node_pcdata).set_value(didl_lite_xml.c_str());
    resp_root.append_child("NumberReturned").append_child(pugi::node_pcdata).set_value(std::to_string(results.size()).c_str());
    resp_root.append_child("TotalMatches").append_child(pugi::node_pcdata).set_value(std::to_string(numMatches).c_str());
    resp_root.append_child("UpdateID").append_child(pugi::node_pcdata).set_value(std::to_string(systemUpdateID).c_str());
    request->setResponse(response);

    log_debug("end");
}

void ContentDirectoryService::doGetSearchCapabilities(const std::unique_ptr<ActionRequest>& request)
{
    log_debug("start");

    auto response = UpnpXMLBuilder::createResponse(request->getActionName(), UPNP_DESC_CDS_SERVICE_TYPE);
    auto root = response->document_element();
    root.append_child("SearchCaps").append_child(pugi::node_pcdata).set_value("dc:title,upnp:class,upnp:artist,upnp:album");
    request->setResponse(response);

    log_debug("end");
}

void ContentDirectoryService::doGetSortCapabilities(const std::unique_ptr<ActionRequest>& request)
{
    log_debug("start");

    auto response = UpnpXMLBuilder::createResponse(request->getActionName(), UPNP_DESC_CDS_SERVICE_TYPE);
    auto root = response->document_element();
    root.append_child("SortCaps").append_child(pugi::node_pcdata).set_value("");
    request->setResponse(response);

    log_debug("end");
}

void ContentDirectoryService::doGetSystemUpdateID(const std::unique_ptr<ActionRequest>& request) const
{
    log_debug("start");

    auto response = UpnpXMLBuilder::createResponse(request->getActionName(), UPNP_DESC_CDS_SERVICE_TYPE);
    auto root = response->document_element();
    root.append_child("Id").append_child(pugi::node_pcdata).set_value(std::to_string(systemUpdateID).c_str());
    request->setResponse(response);

    log_debug("end");
}

void ContentDirectoryService::doSamsungBookmark(const std::unique_ptr<ActionRequest>& request)
{
    log_warning("Stub method for Samsung extension: X_SetBookmark");
}

void ContentDirectoryService::processActionRequest(const std::unique_ptr<ActionRequest>& request)
{
    log_debug("start");

    if (request->getActionName() == "Browse") {
        doBrowse(request);
    } else if (request->getActionName() == "GetSearchCapabilities") {
        doGetSearchCapabilities(request);
    } else if (request->getActionName() == "GetSortCapabilities") {
        doGetSortCapabilities(request);
    } else if (request->getActionName() == "GetSystemUpdateID") {
        doGetSystemUpdateID(request);
    } else if (request->getActionName() == "Search") {
        doSearch(request);
    } else if (request->getActionName() == "X_SetBookmark") {
        doSamsungBookmark(request);
    } else {
        // invalid or unsupported action
        log_warning("Unrecognized action {}", request->getActionName().c_str());
        request->setErrorCode(UPNP_E_INVALID_ACTION);
        // throw UpnpException(UPNP_E_INVALID_ACTION, "unrecognized action");
    }

    log_debug("ContentDirectoryService::processActionRequest: end");
}

void ContentDirectoryService::processSubscriptionRequest(const std::unique_ptr<SubscriptionRequest>& request)
{
    log_debug("start");

    auto propset = UpnpXMLBuilder::createEventPropertySet();
    auto property = propset->document_element().first_child();
    property.append_child("SystemUpdateID").append_child(pugi::node_pcdata).set_value(std::to_string(systemUpdateID).c_str());
    auto obj = database->loadObject(0);
    auto cont = std::static_pointer_cast<CdsContainer>(obj);
    property.append_child("ContainerUpdateIDs").append_child(pugi::node_pcdata).set_value(fmt::format("0,{}", +cont->getUpdateID()).c_str());

    std::ostringstream buf;
    propset->print(buf, "", 0);
    std::string xml = buf.str();

#if defined(USING_NPUPNP)
    UpnpAcceptSubscriptionXML(
        deviceHandle, config->getOption(CFG_SERVER_UDN).c_str(),
        UPNP_DESC_CDS_SERVICE_ID, xml, request->getSubscriptionID().c_str());
#else
    IXML_Document* event = nullptr;
    int err = ixmlParseBufferEx(xml.c_str(), &event);
    if (err != IXML_SUCCESS) {
        throw UpnpException(UPNP_E_SUBSCRIPTION_FAILED, "Could not convert property set to ixml");
    }

    UpnpAcceptSubscriptionExt(deviceHandle,
        config->getOption(CFG_SERVER_UDN).c_str(),
        UPNP_DESC_CDS_SERVICE_ID, event, request->getSubscriptionID().c_str());

    ixmlDocument_free(event);
#endif
    log_debug("end");
}

void ContentDirectoryService::sendSubscriptionUpdate(const std::string& containerUpdateIDs_CSV)
{
    log_debug("start");

    systemUpdateID++;

    auto propset = UpnpXMLBuilder::createEventPropertySet();
    auto property = propset->document_element().first_child();
    property.append_child("ContainerUpdateIDs").append_child(pugi::node_pcdata).set_value(containerUpdateIDs_CSV.c_str());
    property.append_child("SystemUpdateID").append_child(pugi::node_pcdata).set_value(std::to_string(systemUpdateID).c_str());

    std::ostringstream buf;
    propset->print(buf, "", 0);
    std::string xml = buf.str();

#if defined(USING_NPUPNP)
    UpnpNotifyXML(deviceHandle, config->getOption(CFG_SERVER_UDN).c_str(),
        UPNP_DESC_CDS_SERVICE_ID, xml);
#else
    IXML_Document* event = nullptr;
    int err = ixmlParseBufferEx(xml.c_str(), &event);
    if (err != IXML_SUCCESS) {
        /// \todo add another error code
        throw UpnpException(UPNP_E_SUBSCRIPTION_FAILED, "Could not convert property set to ixml");
    }

    UpnpNotifyExt(deviceHandle,
        config->getOption(CFG_SERVER_UDN).c_str(),
        UPNP_DESC_CDS_SERVICE_ID, event);

    ixmlDocument_free(event);
#endif

    log_debug("end");
}
