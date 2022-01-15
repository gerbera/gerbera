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

#include <sstream>
#include <vector>

#include "config/config_manager.h"
#include "database/database.h"
#include "database/sql_database.h"
#include "util/upnp_quirks.h"

ContentDirectoryService::ContentDirectoryService(const std::shared_ptr<Context>& context,
    std::shared_ptr<UpnpXMLBuilder> xmlBuilder, UpnpDevice_Handle deviceHandle, int stringLimit)
    : stringLimit(stringLimit)
    , config(context->getConfig())
    , database(context->getDatabase())
    , deviceHandle(deviceHandle)
    , xmlBuilder(std::move(xmlBuilder))
{
    titleSegments = this->config->getArrayOption(CFG_UPNP_SEARCH_ITEM_SEGMENTS);
    resultSeparator = this->config->getOption(CFG_UPNP_SEARCH_SEPARATOR);
    searchableContainers = this->config->getBoolOption(CFG_UPNP_SEARCH_CONTAINER_FLAG);
}

void ContentDirectoryService::doBrowse(const std::unique_ptr<ActionRequest>& request)
{
    log_debug("start");

    auto req = request->getRequest();
    auto reqRoot = req->document_element();
#ifdef DEBUG_UPNP
    for (auto&& child : req_root.children()) {
        log_info("request {} = {}", child.name(), req_root.child(child.name()).text().as_string());
    }
#endif
    std::string objID = reqRoot.child("ObjectID").text().as_string();
    std::string browseFlag = reqRoot.child("BrowseFlag").text().as_string();
    // std::string filter; // not yet supported
    std::string startingIndex = reqRoot.child("StartingIndex").text().as_string();
    std::string requestedCount = reqRoot.child("RequestedCount").text().as_string();
    std::string sortCriteria = reqRoot.child("SortCriteria").text().as_string();

    log_debug("Browse received parameters: ObjectID [{}] BrowseFlag [{}] StartingIndex [{}] RequestedCount [{}] SortCriteria [{}]",
        objID, browseFlag, startingIndex, requestedCount, sortCriteria);

    if (objID.empty())
        throw UpnpException(UPNP_E_NO_SUCH_ID, "empty object id");

    auto&& quirks = request->getQuirks();
    auto arr = quirks->getSamsungFeatureRoot(objID);
    int objectID = stoiString(objID);

    unsigned int flag = BROWSE_ITEMS | BROWSE_CONTAINERS | BROWSE_EXACT_CHILDCOUNT;

    if (browseFlag == "BrowseDirectChildren")
        flag |= BROWSE_DIRECT_CHILDREN;
    else if (browseFlag != "BrowseMetadata")
        throw UpnpException(UPNP_SOAP_E_INVALID_ARGS,
            "Invalid browse flag: " + browseFlag);

    auto parent = database->loadObject(objectID);
    if (sortCriteria.empty() && (parent->getClass() == UPNP_CLASS_MUSIC_ALBUM || parent->getClass() == UPNP_CLASS_PLAYLIST_CONTAINER))
        flag |= BROWSE_TRACK_SORT;

    if (config->getBoolOption(CFG_SERVER_HIDE_PC_DIRECTORY))
        flag |= BROWSE_HIDE_FS_ROOT;

    auto param = BrowseParam(parent, flag);

    param.setDynamicContainers(!quirks->checkFlags(QUIRK_FLAG_SAMSUNG_HIDE_DYNAMIC));
    param.setStartingIndex(stoiString(startingIndex));
    param.setRequestedCount(stoiString(requestedCount));
    param.setSortCriteria(trimString(sortCriteria));

    try {
        if (arr.empty())
            arr = database->browse(param);
    } catch (const std::runtime_error& e) {
        log_error("No such object: {}", e.what());
        throw UpnpException(UPNP_E_NO_SUCH_ID, "no such object");
    }

    pugi::xml_document didlLite;
    if (!quirks->blockXmlDeclaration()) {
        auto decl = didlLite.prepend_child(pugi::node_declaration);
        decl.append_attribute("version") = "1.0";
        decl.append_attribute("encoding") = "UTF-8";
    }
    auto didlLiteRoot = didlLite.append_child("DIDL-Lite");
    didlLiteRoot.append_attribute(UPNP_XML_DIDL_LITE_NAMESPACE_ATTR) = UPNP_XML_DIDL_LITE_NAMESPACE;
    didlLiteRoot.append_attribute(UPNP_XML_DC_NAMESPACE_ATTR) = UPNP_XML_DC_NAMESPACE;
    didlLiteRoot.append_attribute(UPNP_XML_UPNP_NAMESPACE_ATTR) = UPNP_XML_UPNP_NAMESPACE;
    didlLiteRoot.append_attribute(UPNP_XML_SEC_NAMESPACE_ATTR) = UPNP_XML_SEC_NAMESPACE;

    for (auto&& obj : arr) {
        if (!config->getBoolOption(CFG_SERVER_EXTOPTS_MARK_PLAYED_ITEMS_ENABLED) || !obj->getFlag(OBJECT_FLAG_PLAYED)) {
            xmlBuilder->renderObject(obj, stringLimit, didlLiteRoot, quirks);
            continue;
        }

        std::string title = obj->getTitle();
        if (config->getBoolOption(CFG_SERVER_EXTOPTS_MARK_PLAYED_ITEMS_STRING_MODE_PREPEND))
            title = config->getOption(CFG_SERVER_EXTOPTS_MARK_PLAYED_ITEMS_STRING).append(title);
        else
            title.append(config->getOption(CFG_SERVER_EXTOPTS_MARK_PLAYED_ITEMS_STRING));
        obj->setTitle(title);
        xmlBuilder->renderObject(obj, stringLimit, didlLiteRoot, quirks);
    }

    std::string didlLiteXml = UpnpXMLBuilder::printXml(didlLite, "", 0);
    log_debug("didl {}", didlLiteXml);

    auto response = UpnpXMLBuilder::createResponse(request->getActionName(), UPNP_DESC_CDS_SERVICE_TYPE);
    auto respRoot = response->document_element();
    respRoot.append_child("Result").append_child(pugi::node_pcdata).set_value(didlLiteXml.c_str());
    respRoot.append_child("NumberReturned").append_child(pugi::node_pcdata).set_value(fmt::to_string(arr.size()).c_str());
    respRoot.append_child("TotalMatches").append_child(pugi::node_pcdata).set_value(fmt::to_string(param.getTotalMatches()).c_str());
    respRoot.append_child("UpdateID").append_child(pugi::node_pcdata).set_value(fmt::to_string(systemUpdateID).c_str());
    request->setResponse(std::move(response));

    log_debug("end");
}

void ContentDirectoryService::doSearch(const std::unique_ptr<ActionRequest>& request)
{
    log_debug("start");

    auto req = request->getRequest();
    auto reqRoot = req->document_element();
#ifdef DEBUG_UPNP
    for (auto&& child : req_root.children()) {
        log_info("request {} = {}", child.name(), req_root.child(child.name()).text().as_string());
    }
#endif
    std::string containerID = reqRoot.child("ContainerID").text().as_string();
    std::string searchCriteria = reqRoot.child("SearchCriteria").text().as_string();
    std::string startingIndex = reqRoot.child("StartingIndex").text().as_string();
    std::string requestedCount = reqRoot.child("RequestedCount").text().as_string();
    std::string sortCriteria = reqRoot.child("SortCriteria").text().as_string();

    log_debug("Search received parameters: ContainerID [{}] SearchCriteria [{}] StartingIndex [{}] RequestedCount [{}] RequestedCount [{}]",
        containerID, searchCriteria, startingIndex, requestedCount, requestedCount);

    auto&& quirks = request->getQuirks();
    pugi::xml_document didlLite;
    if (!quirks->blockXmlDeclaration()) {
        auto decl = didlLite.prepend_child(pugi::node_declaration);
        decl.append_attribute("version") = "1.0";
        decl.append_attribute("encoding") = "UTF-8";
    }
    auto didlLiteRoot = didlLite.append_child("DIDL-Lite");
    didlLiteRoot.append_attribute(UPNP_XML_DIDL_LITE_NAMESPACE_ATTR) = UPNP_XML_DIDL_LITE_NAMESPACE;
    didlLiteRoot.append_attribute(UPNP_XML_DC_NAMESPACE_ATTR) = UPNP_XML_DC_NAMESPACE;
    didlLiteRoot.append_attribute(UPNP_XML_UPNP_NAMESPACE_ATTR) = UPNP_XML_UPNP_NAMESPACE;
    didlLiteRoot.append_attribute(UPNP_XML_SEC_NAMESPACE_ATTR) = UPNP_XML_SEC_NAMESPACE;

    const auto searchParam = SearchParam(containerID, searchCriteria, sortCriteria,
        stoiString(startingIndex), stoiString(requestedCount), searchableContainers);

    std::vector<std::shared_ptr<CdsObject>> results;
    int numMatches = 0;
    try {
        results = database->search(searchParam, &numMatches);
        log_debug("Found {}/{} items", results.size(), numMatches);
    } catch (const std::runtime_error& e) {
        log_debug(e.what());
        throw UpnpException(UPNP_E_NO_SUCH_ID, "no such object");
    }

    for (auto&& cdsObject : results) {
        if (!cdsObject->isItem()) {
            xmlBuilder->renderObject(cdsObject, stringLimit, didlLiteRoot);
            continue;
        }

        std::string title = cdsObject->getTitle();
        if (!titleSegments.empty()) {
            std::vector<std::string> values;
            for (auto&& segment : titleSegments) {
                auto mtField = MetadataHandler::remapMetaDataField(segment);
                auto value = (mtField != M_MAX) ? cdsObject->getMetaData(mtField) : cdsObject->getMetaData(segment);
                if (!value.empty())
                    values.push_back(value);
            }
            if (!values.empty())
                title = fmt::format("{}", fmt::join(values, resultSeparator));
        }

        if (config->getBoolOption(CFG_SERVER_EXTOPTS_MARK_PLAYED_ITEMS_ENABLED) && cdsObject->getFlag(OBJECT_FLAG_PLAYED)) {
            if (config->getBoolOption(CFG_SERVER_EXTOPTS_MARK_PLAYED_ITEMS_STRING_MODE_PREPEND))
                title = config->getOption(CFG_SERVER_EXTOPTS_MARK_PLAYED_ITEMS_STRING).append(title);
            else
                title.append(config->getOption(CFG_SERVER_EXTOPTS_MARK_PLAYED_ITEMS_STRING));
        }

        cdsObject->setTitle(title);
        xmlBuilder->renderObject(cdsObject, stringLimit, didlLiteRoot);
    }

    std::string didlLiteXml = UpnpXMLBuilder::printXml(didlLite, "", 0);
    log_debug("didl {}", didlLiteXml);

    auto response = UpnpXMLBuilder::createResponse(request->getActionName(), UPNP_DESC_CDS_SERVICE_TYPE);
    auto respRoot = response->document_element();
    respRoot.append_child("Result").append_child(pugi::node_pcdata).set_value(didlLiteXml.c_str());
    respRoot.append_child("NumberReturned").append_child(pugi::node_pcdata).set_value(fmt::to_string(results.size()).c_str());
    respRoot.append_child("TotalMatches").append_child(pugi::node_pcdata).set_value(fmt::to_string(numMatches).c_str());
    respRoot.append_child("UpdateID").append_child(pugi::node_pcdata).set_value(fmt::to_string(systemUpdateID).c_str());
    request->setResponse(std::move(response));

    log_debug("end");
}

void ContentDirectoryService::doGetSearchCapabilities(const std::unique_ptr<ActionRequest>& request)
{
    log_debug("start");

    auto response = UpnpXMLBuilder::createResponse(request->getActionName(), UPNP_DESC_CDS_SERVICE_TYPE);
    auto root = response->document_element();
    root.append_child("SearchCaps").append_child(pugi::node_pcdata).set_value(SQLDatabase::getSearchCapabilities().c_str());
    request->setResponse(std::move(response));

    log_debug("end");
}

void ContentDirectoryService::doGetSortCapabilities(const std::unique_ptr<ActionRequest>& request)
{
    log_debug("start");

    auto response = UpnpXMLBuilder::createResponse(request->getActionName(), UPNP_DESC_CDS_SERVICE_TYPE);
    auto root = response->document_element();
    root.append_child("SortCaps").append_child(pugi::node_pcdata).set_value(SQLDatabase::getSortCapabilities().c_str());
    request->setResponse(std::move(response));

    log_debug("end");
}

void ContentDirectoryService::doGetSystemUpdateID(const std::unique_ptr<ActionRequest>& request) const
{
    log_debug("start");

    auto response = UpnpXMLBuilder::createResponse(request->getActionName(), UPNP_DESC_CDS_SERVICE_TYPE);
    auto root = response->document_element();
    root.append_child("Id").append_child(pugi::node_pcdata).set_value(fmt::to_string(systemUpdateID).c_str());
    request->setResponse(std::move(response));

    log_debug("end");
}

void ContentDirectoryService::doSamsungBookmark(const std::unique_ptr<ActionRequest>& request)
{
    log_debug("start");

    request->getQuirks()->saveSamsungBookMarkedPosition(request);

    log_debug("end");
}

void ContentDirectoryService::doSamsungFeatureList(const std::unique_ptr<ActionRequest>& request)
{
    log_debug("start");

    request->getQuirks()->getSamsungFeatureList(request);

    log_debug("end");
}

void ContentDirectoryService::doSamsungGetObjectIDfromIndex(const std::unique_ptr<ActionRequest>& request)
{
    log_debug("start");

    request->getQuirks()->getSamsungObjectIDfromIndex(request);

    log_debug("end");
}

void ContentDirectoryService::doSamsungGetIndexfromRID(const std::unique_ptr<ActionRequest>& request)
{
    log_debug("start");

    request->getQuirks()->getSamsungIndexfromRID(request);

    log_debug("end");
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
    } else if (request->getActionName() == "X_GetFeatureList") {
        doSamsungFeatureList(request);
    } else if (request->getActionName() == "X_GetObjectIDfromIndex") {
        doSamsungGetObjectIDfromIndex(request);
    } else if (request->getActionName() == "X_GetIndexfromRID") {
        doSamsungGetIndexfromRID(request);
    } else {
        // invalid or unsupported action
        log_warning("Unrecognized action {}", request->getActionName().c_str());
        request->setErrorCode(UPNP_E_INVALID_ACTION);
    }

    log_debug("ContentDirectoryService::processActionRequest: end");
}

void ContentDirectoryService::processSubscriptionRequest(const std::unique_ptr<SubscriptionRequest>& request)
{
    log_debug("start");

    auto propset = UpnpXMLBuilder::createEventPropertySet();
    auto property = propset->document_element().first_child();
    property.append_child("SystemUpdateID").append_child(pugi::node_pcdata).set_value(fmt::to_string(systemUpdateID).c_str());
    auto obj = database->loadObject(0);
    auto cont = std::static_pointer_cast<CdsContainer>(obj);
    property.append_child("ContainerUpdateIDs").append_child(pugi::node_pcdata).set_value(fmt::format("0,{}", cont->getUpdateID()).c_str());

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

void ContentDirectoryService::sendSubscriptionUpdate(const std::string& containerUpdateIDsCsv)
{
    log_debug("start");

    systemUpdateID++;

    auto propset = UpnpXMLBuilder::createEventPropertySet();
    auto property = propset->document_element().first_child();
    property.append_child("ContainerUpdateIDs").append_child(pugi::node_pcdata).set_value(containerUpdateIDsCsv.c_str());
    property.append_child("SystemUpdateID").append_child(pugi::node_pcdata).set_value(fmt::to_string(systemUpdateID).c_str());

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
