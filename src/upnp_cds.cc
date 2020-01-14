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

#include "upnp_cds.h"
#include "config/config_manager.h"
#include "search_handler.h"
#include "server.h"
#include "storage/storage.h"
#include <memory>
#include <string>
#include <vector>

using namespace zmm;
using namespace mxml;

ContentDirectoryService::ContentDirectoryService(std::shared_ptr<ConfigManager> config,
    std::shared_ptr<Storage> storage,
    UpnpXMLBuilder* xmlBuilder, UpnpDevice_Handle deviceHandle, int stringLimit)
    : systemUpdateID(0)
    , stringLimit(stringLimit)
    , config(config)
    , storage(storage)
    , deviceHandle(deviceHandle)
    , xmlBuilder(xmlBuilder)
{
}

ContentDirectoryService::~ContentDirectoryService() = default;

void ContentDirectoryService::doBrowse(const std::unique_ptr<ActionRequest>& request)
{
    log_debug("start");
    Ref<Element> req = request->getRequest();

    std::string objID = req->getChildText("ObjectID");
    int objectID;
    std::string BrowseFlag = req->getChildText("BrowseFlag");
    //std::string Filter; // not yet supported
    std::string StartingIndex = req->getChildText("StartingIndex");
    std::string RequestedCount = req->getChildText("RequestedCount");
    // std::string SortCriteria; // not yet supported

    log_debug("Browse received parameters: ObjectID [{}] BrowseFlag [{}] StartingIndex [{}] RequestedCount [{}]",
        objID.c_str(), BrowseFlag.c_str(), StartingIndex.c_str(), RequestedCount.c_str());

    if (objID.empty())
        throw UpnpException(UPNP_E_NO_SUCH_ID, "empty object id");
    else
        objectID = std::stoi(objID);

    unsigned int flag = BROWSE_ITEMS | BROWSE_CONTAINERS | BROWSE_EXACT_CHILDCOUNT;

    if (BrowseFlag == "BrowseDirectChildren")
        flag |= BROWSE_DIRECT_CHILDREN;
    else if (BrowseFlag != "BrowseMetadata")
        throw UpnpException(UPNP_SOAP_E_INVALID_ARGS,
            "invalid browse flag: " + BrowseFlag);

    auto parent = storage->loadObject(objectID);
    if ((parent->getClass() == UPNP_DEFAULT_CLASS_MUSIC_ALBUM) || (parent->getClass() == UPNP_DEFAULT_CLASS_PLAYLIST_CONTAINER))
        flag |= BROWSE_TRACK_SORT;

    if (config->getBoolOption(CFG_SERVER_HIDE_PC_DIRECTORY))
        flag |= BROWSE_HIDE_FS_ROOT;

    auto param = std::make_unique<BrowseParam>(objectID, flag);

    param->setStartingIndex(std::stoi(StartingIndex));
    param->setRequestedCount(std::stoi(RequestedCount));

    std::vector<std::shared_ptr<CdsObject>> arr;
    try {
        arr = storage->browse(param);
    } catch (const std::runtime_error& e) {
        throw UpnpException(UPNP_E_NO_SUCH_ID, "no such object");
    }

    Ref<Element> didl_lite(new Element("DIDL-Lite"));
    didl_lite->setAttribute(XML_NAMESPACE_ATTR,
        XML_DIDL_LITE_NAMESPACE);
    didl_lite->setAttribute(XML_DC_NAMESPACE_ATTR,
        XML_DC_NAMESPACE);
    didl_lite->setAttribute(XML_UPNP_NAMESPACE_ATTR,
        XML_UPNP_NAMESPACE);

    if (config->getBoolOption(CFG_SERVER_EXTEND_PROTOCOLINFO_SM_HACK)) {
        didl_lite->setAttribute(XML_SEC_NAMESPACE_ATTR,
            XML_SEC_NAMESPACE);
    }

    for (size_t i = 0; i < arr.size(); i++) {
        auto obj = arr[i];
        if (config->getBoolOption(CFG_SERVER_EXTOPTS_MARK_PLAYED_ITEMS_ENABLED) && obj->getFlag(OBJECT_FLAG_PLAYED)) {
            std::string title = obj->getTitle();
            if (config->getBoolOption(CFG_SERVER_EXTOPTS_MARK_PLAYED_ITEMS_STRING_MODE_PREPEND))
                title = config->getOption(CFG_SERVER_EXTOPTS_MARK_PLAYED_ITEMS_STRING) + title;
            else
                title = title + config->getOption(CFG_SERVER_EXTOPTS_MARK_PLAYED_ITEMS_STRING);

            obj->setTitle(title);
        }

        Ref<Element> didl_object = xmlBuilder->renderObject(obj, false, stringLimit);

        didl_lite->appendElementChild(didl_object);
    }

    Ref<Element> response = xmlBuilder->createResponse(request->getActionName(), DESC_CDS_SERVICE_TYPE);

    response->appendTextChild("Result", didl_lite->print());
    response->appendTextChild("NumberReturned", std::to_string(arr.size()));
    response->appendTextChild("TotalMatches", std::to_string(param->getTotalMatches()));
    response->appendTextChild("UpdateID", std::to_string(systemUpdateID));

    request->setResponse(response);
    log_debug("end");
}

void ContentDirectoryService::doSearch(const std::unique_ptr<ActionRequest>& request)
{
    log_debug("start");

    Ref<Element> req = request->getRequest();
    std::string containerID(req->getChildText("ContainerID").c_str());
    std::string searchCriteria(req->getChildText("SearchCriteria").c_str());
    std::string startingIndex(req->getChildText("StartingIndex").c_str());
    std::string requestedCount(req->getChildText("RequestedCount").c_str());
    log_debug("Search received parameters: ContainerID [{}] SearchCriteria [{}] StartingIndex [{}] RequestedCount [{}]",
        containerID.c_str(), searchCriteria.c_str(), startingIndex.c_str(), requestedCount.c_str());

    Ref<Element> didl_lite(new Element("DIDL-Lite"));
    didl_lite->setAttribute(XML_NAMESPACE_ATTR,
        XML_DIDL_LITE_NAMESPACE);
    didl_lite->setAttribute(XML_DC_NAMESPACE_ATTR,
        XML_DC_NAMESPACE);
    didl_lite->setAttribute(XML_UPNP_NAMESPACE_ATTR,
        XML_UPNP_NAMESPACE);

    if (config->getBoolOption(CFG_SERVER_EXTEND_PROTOCOLINFO_SM_HACK)) {
        didl_lite->setAttribute(XML_SEC_NAMESPACE_ATTR,
            XML_SEC_NAMESPACE);
    }

    auto searchParam = std::make_unique<SearchParam>(containerID, searchCriteria,
        std::stoi(startingIndex.c_str(), nullptr), std::stoi(requestedCount.c_str(), nullptr));

    std::vector<std::shared_ptr<CdsObject>> results;
    int numMatches = 0;
    try {
        results = storage->search(searchParam, &numMatches);
    } catch (const std::runtime_error& e) {
        log_debug(e.what());
        throw UpnpException(UPNP_E_NO_SUCH_ID, "no such object");
    }

    for (size_t i = 0; i < results.size(); i++) {
        auto cdsObject = results[i];
        if (config->getBoolOption(CFG_SERVER_EXTOPTS_MARK_PLAYED_ITEMS_ENABLED) && cdsObject->getFlag(OBJECT_FLAG_PLAYED)) {
            std::string title = cdsObject->getTitle();
            if (config->getBoolOption(CFG_SERVER_EXTOPTS_MARK_PLAYED_ITEMS_STRING_MODE_PREPEND))
                title = config->getOption(CFG_SERVER_EXTOPTS_MARK_PLAYED_ITEMS_STRING) + title;
            else
                title = title + config->getOption(CFG_SERVER_EXTOPTS_MARK_PLAYED_ITEMS_STRING);

            cdsObject->setTitle(title);
        }

        Ref<Element> didl_object = xmlBuilder->renderObject(cdsObject, false, stringLimit);
        didl_lite->appendElementChild(didl_object);
    }

    Ref<Element> response = xmlBuilder->createResponse(request->getActionName(), DESC_CDS_SERVICE_TYPE);

    response->appendTextChild("Result", didl_lite->print());
    response->appendTextChild("NumberReturned", std::to_string(results.size()));
    response->appendTextChild("TotalMatches", std::to_string(numMatches));
    response->appendTextChild("UpdateID", std::to_string(systemUpdateID));

    request->setResponse(response);
    log_debug("end");
}

void ContentDirectoryService::doGetSearchCapabilities(const std::unique_ptr<ActionRequest>& request)
{
    log_debug("start");

    Ref<Element> response;
    response = xmlBuilder->createResponse(request->getActionName(), DESC_CDS_SERVICE_TYPE);
    response->appendTextChild("SearchCaps", "dc:title,upnp:class,upnp:artist,upnp:album");

    request->setResponse(response);

    log_debug("end");
}

void ContentDirectoryService::doGetSortCapabilities(const std::unique_ptr<ActionRequest>& request)
{
    log_debug("start");

    Ref<Element> response;
    response = xmlBuilder->createResponse(request->getActionName(), DESC_CDS_SERVICE_TYPE);
    response->appendTextChild("SortCaps", "");

    request->setResponse(response);

    log_debug("end");
}

void ContentDirectoryService::doGetSystemUpdateID(const std::unique_ptr<ActionRequest>& request)
{
    log_debug("start");

    Ref<Element> response;
    response = xmlBuilder->createResponse(request->getActionName(), DESC_CDS_SERVICE_TYPE);
    response->appendTextChild("Id", std::to_string(systemUpdateID));

    request->setResponse(response);

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
    } else {
        // invalid or unsupported action
        log_info("unrecognized action {}", request->getActionName().c_str());
        request->setErrorCode(UPNP_E_INVALID_ACTION);
        // throw UpnpException(UPNP_E_INVALID_ACTION, "unrecognized action");
    }

    log_debug("ContentDirectoryService::processActionRequest: end");
}

void ContentDirectoryService::processSubscriptionRequest(const std::unique_ptr<SubscriptionRequest>& request)
{
    int err;
    IXML_Document* event = nullptr;

    Ref<Element> propset, property;

    log_debug("start");

    propset = xmlBuilder->createEventPropertySet();
    property = propset->getFirstElementChild();
    property->appendTextChild("SystemUpdateID", std::to_string(systemUpdateID));
    auto obj = storage->loadObject(0);
    auto cont = std::static_pointer_cast<CdsContainer>(obj);
    property->appendTextChild("ContainerUpdateIDs", fmt::format("0,{}", + cont->getUpdateID()));
    std::string xml = propset->print();
    err = ixmlParseBufferEx(xml.c_str(), &event);
    if (err != IXML_SUCCESS) {
        throw UpnpException(UPNP_E_SUBSCRIPTION_FAILED, "Could not convert property set to ixml");
    }

    UpnpAcceptSubscriptionExt(deviceHandle,
        config->getOption(CFG_SERVER_UDN).c_str(),
        DESC_CDS_SERVICE_ID, event, request->getSubscriptionID().c_str());

    ixmlDocument_free(event);
    log_debug("end");
}

void ContentDirectoryService::sendSubscriptionUpdate(std::string containerUpdateIDs_CSV)
{
    int err;
    IXML_Document* event = nullptr;

    Ref<Element> propset, property;

    log_debug("start");

    systemUpdateID++;

    propset = xmlBuilder->createEventPropertySet();
    property = propset->getFirstElementChild();
    property->appendTextChild("ContainerUpdateIDs", containerUpdateIDs_CSV);
    property->appendTextChild("SystemUpdateID", std::to_string(systemUpdateID));

    std::string xml = propset->print();

    err = ixmlParseBufferEx(xml.c_str(), &event);
    if (err != IXML_SUCCESS) {
        /// \todo add another error code
        throw UpnpException(UPNP_E_SUBSCRIPTION_FAILED, "Could not convert property set to ixml");
    }

    UpnpNotifyExt(deviceHandle,
        config->getOption(CFG_SERVER_UDN).c_str(),
        DESC_CDS_SERVICE_ID, event);

    ixmlDocument_free(event);

    log_debug("end");
}
