/*MT*

    MediaTomb - http://www.mediatomb.cc/

    cont_dir_service.cc - this file is part of MediaTomb.

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

/// \file cont_dir_service.cc
#define GRB_LOG_FAC GrbLogFacility::cds

#include "cont_dir_service.h" // API

#include <vector>

#include "action_request.h"
#include "cds/cds_container.h"
#include "cds/cds_item.h"
#include "config/config.h"
#include "config/config_val.h"
#include "context.h"
#include "database/database.h"
#include "database/db_param.h"
#include "database/sql_database.h"
#include "exceptions.h"
#include "subscription_request.h"
#include "upnp/clients.h"
#include "upnp/compat.h"
#include "upnp/quirks.h"
#include "upnp/xml_builder.h"
#include "util/tools.h"

ContentDirectoryService::ContentDirectoryService(const std::shared_ptr<Context>& context,
    const std::shared_ptr<UpnpXMLBuilder>& xmlBuilder, UpnpDevice_Handle deviceHandle,
    int stringLimit, bool offline)
    : UpnpService(context->getConfig(), xmlBuilder, deviceHandle, UPNP_DESC_CDS_SERVICE_ID, offline)
    , stringLimit(stringLimit)
    , database(context->getDatabase())
{
    actionMap = {
        { "Browse", [this](ActionRequest& r) { doBrowse(r); } },
        { "GetSearchCapabilities", [this](ActionRequest& r) { doGetSearchCapabilities(r); } },
        { "GetSortCapabilities", [this](ActionRequest& r) { doGetSortCapabilities(r); } },
        { "GetSystemUpdateID", [this](ActionRequest& r) { doGetSystemUpdateID(r); } },
        { "GetFeatureList", [this](ActionRequest& r) { doGetFeatureList(r); } },
        { "GetSortExtensionCapabilities", [this](ActionRequest& r) { doGetSortExtensionCapabilities(r); } },
        { "Search", [this](ActionRequest& r) { doSearch(r); } },
        { "X_SetBookmark", [this](ActionRequest& r) { doSamsungBookmark(r); } },
        { "X_GetFeatureList", [this](ActionRequest& r) { doSamsungFeatureList(r); } },
        { "X_GetObjectIDfromIndex", [this](ActionRequest& r) { doSamsungGetObjectIDfromIndex(r); } },
        { "X_GetIndexfromRID", [this](ActionRequest& r) { doSamsungGetIndexfromRID(r); } },
    };
    titleSegments = this->config->getArrayOption(ConfigVal::UPNP_SEARCH_ITEM_SEGMENTS);
    resultSeparator = this->config->getOption(ConfigVal::UPNP_SEARCH_SEPARATOR);
    searchableContainers = this->config->getBoolOption(ConfigVal::UPNP_SEARCH_CONTAINER_FLAG);
}

void ContentDirectoryService::doBrowse(ActionRequest& request)
{
    log_debug("start");

    auto req = request.getRequest();
    auto reqRoot = req->document_element();

#ifdef GRBDEBUG
    if (GrbLogger::Logger.isDebugging(GRB_LOG_FAC))
        for (auto&& child : reqRoot.children()) {
            log_debug("request {} = {}", child.name(), reqRoot.child(child.name()).text().as_string());
        }
#endif

    // prepare browse parameters
    std::string objID = reqRoot.child("ObjectID").text().as_string();
    std::string browseFlag = reqRoot.child("BrowseFlag").text().as_string();
    std::string startingIndex = reqRoot.child("StartingIndex").text().as_string();
    std::string filter = reqRoot.child("Filter").text().as_string();
    std::string requestedCount = reqRoot.child("RequestedCount").text().as_string();
    std::string sortCriteria = reqRoot.child("SortCriteria").text().as_string();

    log_debug("Browse received parameters: ObjectID [{}] BrowseFlag [{}] StartingIndex [{}] Filter [{}] RequestedCount [{}] SortCriteria [{}]",
        objID, browseFlag, startingIndex, filter, requestedCount, sortCriteria);

    if (objID.empty())
        throw UpnpException(UPNP_E_NO_SUCH_ID, "empty object id");

    auto&& quirks = request.getQuirks();
    auto arr = quirks->getSamsungFeatureRoot(database, objID);
    int objectID = stoiString(objID);

    unsigned int flag = BROWSE_ITEMS | BROWSE_CONTAINERS | BROWSE_EXACT_CHILDCOUNT;

    if (browseFlag == "BrowseDirectChildren")
        flag |= BROWSE_DIRECT_CHILDREN;
    else if (browseFlag != "BrowseMetadata")
        throw UpnpException(UPNP_SOAP_E_INVALID_ARGS,
            "Invalid browse flag: " + browseFlag);

    auto parent = database->loadObject(quirks->getGroup(), objectID);
    auto upnpClass = parent->getClass();
    if (sortCriteria.empty() && (startswith(upnpClass, UPNP_CLASS_MUSIC_ALBUM) || startswith(upnpClass, UPNP_CLASS_PLAYLIST_CONTAINER)))
        flag |= BROWSE_TRACK_SORT;
    if (filter.empty())
        filter = "*";
    if (config->getBoolOption(ConfigVal::SERVER_HIDE_PC_DIRECTORY))
        flag |= BROWSE_HIDE_FS_ROOT;

    auto param = BrowseParam(parent, flag);

    param.setDynamicContainers(!quirks || !quirks->checkFlags(QUIRK_FLAG_SAMSUNG_HIDE_DYNAMIC));
    param.setStartingIndex(stoiString(startingIndex));
    param.setRequestedCount(stoiString(requestedCount));
    param.setSortCriteria(trimString(sortCriteria));
    param.setGroup(quirks->getGroup());
    if (quirks)
        param.setForbiddenDirectories(quirks->getForbiddenDirectories());

    // Execute database browse
    try {
        if (arr.empty())
            arr = database->browse(param);
        else
            param.setTotalMatches(arr.size());
    } catch (const SearchParseException& srcEx) {
        log_warning(srcEx.what());
        throw UpnpException(UPNP_E_INVALID_ARGUMENT, srcEx.getUserMessage());
    } catch (const DatabaseException& dbEx) {
        log_warning(dbEx.what());
        throw UpnpException(UPNP_E_NO_SUCH_ID, dbEx.getUserMessage());
    } catch (const std::runtime_error& e) {
        log_warning(e.what());
        throw UpnpException(UPNP_E_BAD_REQUEST, UpnpXMLBuilder::encodeEscapes(e.what()));
    }

    // build response
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

    auto stringLimitClient = stringLimit;
    if (quirks->getStringLimit() > -1) {
        stringLimitClient = quirks->getStringLimit();
    }

    for (auto&& obj : arr) {
        markPlayedItem(obj, obj->getTitle());
        xmlBuilder->renderObject(obj, splitString(filter, ','), stringLimitClient, didlLiteRoot, quirks);
    }

    std::string didlLiteXml = UpnpXMLBuilder::printXml(didlLite, "", quirks && quirks->needsStrictXml() ? pugi::format_no_escapes : 0);
    log_debug("didl {}", didlLiteXml);

    auto response = xmlBuilder->createResponse(request.getActionName(), UPNP_DESC_CDS_SERVICE_TYPE);
    auto respRoot = response->document_element();
    respRoot.append_child("Result").append_child(pugi::node_pcdata).set_value(didlLiteXml.c_str());
    respRoot.append_child("NumberReturned").append_child(pugi::node_pcdata).set_value(fmt::to_string(arr.size()).c_str());
    respRoot.append_child("TotalMatches").append_child(pugi::node_pcdata).set_value(fmt::to_string(param.getTotalMatches()).c_str());
    respRoot.append_child("UpdateID").append_child(pugi::node_pcdata).set_value(fmt::to_string(systemUpdateID).c_str());
    request.setResponse(std::move(response));

    log_debug("end");
}

void ContentDirectoryService::doSearch(ActionRequest& request)
{
    log_debug("start");

    auto req = request.getRequest();
    auto reqRoot = req->document_element();

#ifdef GRBDEBUG
    if (GrbLogger::Logger.isDebugging(GRB_LOG_FAC))
        for (auto&& child : reqRoot.children()) {
            log_debug("request {} = {}", child.name(), reqRoot.child(child.name()).text().as_string());
        }
#endif

    // prepare search parameters
    std::string containerID = reqRoot.child("ContainerID").text().as_string();
    std::string searchCriteria = reqRoot.child("SearchCriteria").text().as_string();
    std::string startingIndex = reqRoot.child("StartingIndex").text().as_string();
    std::string filter = reqRoot.child("Filter").text().as_string();
    std::string requestedCount = reqRoot.child("RequestedCount").text().as_string();
    std::string sortCriteria = reqRoot.child("SortCriteria").text().as_string();

    log_debug("Search received parameters: ContainerID [{}] SearchCriteria [{}] SortCriteria [{}] StartingIndex [{}] Filter [{}] RequestedCount [{}]",
        containerID, searchCriteria, sortCriteria, startingIndex, filter, requestedCount);

    auto&& quirks = request.getQuirks();
    pugi::xml_document didlLite;
    if (!quirks || !quirks->blockXmlDeclaration()) {
        auto decl = didlLite.prepend_child(pugi::node_declaration);
        decl.append_attribute("version") = "1.0";
        decl.append_attribute("encoding") = "UTF-8";
    }
    auto didlLiteRoot = didlLite.append_child("DIDL-Lite");
    didlLiteRoot.append_attribute(UPNP_XML_DIDL_LITE_NAMESPACE_ATTR) = UPNP_XML_DIDL_LITE_NAMESPACE;
    didlLiteRoot.append_attribute(UPNP_XML_DC_NAMESPACE_ATTR) = UPNP_XML_DC_NAMESPACE;
    didlLiteRoot.append_attribute(UPNP_XML_UPNP_NAMESPACE_ATTR) = UPNP_XML_UPNP_NAMESPACE;
    didlLiteRoot.append_attribute(UPNP_XML_SEC_NAMESPACE_ATTR) = UPNP_XML_SEC_NAMESPACE;
    if (quirks->checkFlags(QUIRK_FLAG_PV_SUBTITLES))
        didlLiteRoot.append_attribute("xmlns:pv") = "http://www.pv.com/pvns/";
    if (sortCriteria.empty()) {
        sortCriteria = fmt::format("+{}", MetaEnumMapper::getMetaFieldName(MetadataFields::M_TITLE));
    }
    if (filter.empty())
        filter = "*";
    auto searchParam = SearchParam(containerID, searchCriteria, sortCriteria,
        stoiString(startingIndex), stoiString(requestedCount), searchableContainers, quirks->getGroup());
    if (quirks)
        searchParam.setForbiddenDirectories(quirks->getForbiddenDirectories());

    // Execute database search
    std::vector<std::shared_ptr<CdsObject>> results;
    try {
        results = database->search(searchParam);
        log_debug("Found {}/{} items", results.size(), searchParam.getTotalMatches());
    } catch (const SearchParseException& srcEx) {
        log_warning(srcEx.what());
        throw UpnpException(UPNP_E_INVALID_ARGUMENT, srcEx.getUserMessage());
    } catch (const DatabaseException& dbEx) {
        log_warning(dbEx.what());
        throw UpnpException(UPNP_E_NO_SUCH_ID, dbEx.getUserMessage());
    } catch (const std::runtime_error& e) {
        log_warning(e.what());
        throw UpnpException(UPNP_E_BAD_REQUEST, UpnpXMLBuilder::encodeEscapes(e.what()));
    }

    // build response
    auto stringLimitClient = stringLimit;
    if (quirks->getStringLimit() > -1) {
        stringLimitClient = quirks->getStringLimit();
    }

    for (auto&& cdsObject : results) {
        if (!cdsObject->isItem()) {
            xmlBuilder->renderObject(cdsObject, splitString(filter, ','), stringLimitClient, didlLiteRoot);
            continue;
        }

        std::string title = cdsObject->getTitle();
        if (!titleSegments.empty()) {
            std::vector<std::string> values;
            for (auto&& segment : titleSegments) {
                auto mtField = MetaEnumMapper::remapMetaDataField(segment);
                auto value = (mtField != MetadataFields::M_MAX) ? cdsObject->getMetaData(mtField) : cdsObject->getMetaData(segment);
                if (!value.empty())
                    values.push_back(value);
            }
            if (!values.empty())
                title = fmt::format("{}", fmt::join(values, resultSeparator));
        }

        markPlayedItem(cdsObject, title);
        xmlBuilder->renderObject(cdsObject, splitString(filter, ','), stringLimitClient, didlLiteRoot);
    }

    std::string didlLiteXml = UpnpXMLBuilder::printXml(didlLite, "", quirks && quirks->needsStrictXml() ? pugi::format_no_escapes : 0);
    log_debug("didl {}", didlLiteXml);

    auto response = xmlBuilder->createResponse(request.getActionName(), UPNP_DESC_CDS_SERVICE_TYPE);
    auto respRoot = response->document_element();
    respRoot.append_child("Result").append_child(pugi::node_pcdata).set_value(didlLiteXml.c_str());
    respRoot.append_child("NumberReturned").append_child(pugi::node_pcdata).set_value(fmt::to_string(results.size()).c_str());
    respRoot.append_child("TotalMatches").append_child(pugi::node_pcdata).set_value(fmt::to_string(searchParam.getTotalMatches()).c_str());
    respRoot.append_child("UpdateID").append_child(pugi::node_pcdata).set_value(fmt::to_string(systemUpdateID).c_str());
    request.setResponse(std::move(response));

    log_debug("end");
}

static const auto markContentMap = std::map<std::string, std::string> {
    { DEFAULT_MARK_PLAYED_CONTENT_VIDEO, UPNP_CLASS_VIDEO_ITEM },
    { DEFAULT_MARK_PLAYED_CONTENT_AUDIO, UPNP_CLASS_AUDIO_ITEM },
    { DEFAULT_MARK_PLAYED_CONTENT_IMAGE, UPNP_CLASS_IMAGE_ITEM },
};

void ContentDirectoryService::markPlayedItem(const std::shared_ptr<CdsObject>& cdsObject, std::string title) const
{
    // Return early if object is not an item
    if (!cdsObject->isItem())
        return;

    auto item = std::static_pointer_cast<CdsItem>(cdsObject);
    auto playStatus = item->getPlayStatus();
    if (config->getBoolOption(ConfigVal::SERVER_EXTOPTS_MARK_PLAYED_ITEMS_ENABLED) && playStatus && playStatus->getPlayCount() > 0) {
        std::vector<std::string> markList = config->getArrayOption(ConfigVal::SERVER_EXTOPTS_MARK_PLAYED_ITEMS_CONTENT_LIST);
        bool mark = std::any_of(markList.begin(), markList.end(), [&](auto&& i) { return startswith(item->getClass(), markContentMap.at(i)); });
        if (mark) {
            if (config->getBoolOption(ConfigVal::SERVER_EXTOPTS_MARK_PLAYED_ITEMS_STRING_MODE_PREPEND))
                title = config->getOption(ConfigVal::SERVER_EXTOPTS_MARK_PLAYED_ITEMS_STRING).append(title);
            else
                title.append(config->getOption(ConfigVal::SERVER_EXTOPTS_MARK_PLAYED_ITEMS_STRING));
        }
    }

    // Always update title
    item->setTitle(title);
}

void ContentDirectoryService::doGetSearchCapabilities(ActionRequest& request) const
{
    log_debug("start");

    auto response = xmlBuilder->createResponse(request.getActionName(), UPNP_DESC_CDS_SERVICE_TYPE);
    auto root = response->document_element();
    root.append_child("SearchCaps").append_child(pugi::node_pcdata).set_value(SQLDatabase::getSearchCapabilities().c_str());
    request.setResponse(std::move(response));

    log_debug("end");
}

void ContentDirectoryService::doGetSortCapabilities(ActionRequest& request) const
{
    log_debug("start");

    auto response = xmlBuilder->createResponse(request.getActionName(), UPNP_DESC_CDS_SERVICE_TYPE);
    auto root = response->document_element();
    root.append_child("SortCaps").append_child(pugi::node_pcdata).set_value(SQLDatabase::getSortCapabilities().c_str());
    request.setResponse(std::move(response));

    log_debug("end");
}

void ContentDirectoryService::doGetFeatureList(ActionRequest& request) const
{
    log_debug("start");

    auto response = xmlBuilder->createResponse(request.getActionName(), UPNP_DESC_CDS_SERVICE_TYPE);
    auto root = response->document_element();
    pugi::xml_document respRoot;
    auto features = respRoot.append_child("Features");
    features.append_attribute("xmlns") = "urn:schemas-upnp-org:av:avs";
    features.append_attribute("xmlns:xsi") = "http://www.w3.org/2001/XMLSchema-instance";
    features.append_attribute("xsi:schemaLocation") = "urn:schemas-upnp-org:av:avs http://www.upnp.org/schemas/av/avs.xsd";
    auto&& quirks = request.getQuirks();
    if (quirks)
        quirks->getShortCutList(database, features);
    root.append_child("FeatureList").append_child(pugi::node_pcdata).set_value(UpnpXMLBuilder::printXml(respRoot, "", 0).c_str());
    request.setResponse(std::move(response));

    log_debug("end");
}

void ContentDirectoryService::doGetSortExtensionCapabilities(ActionRequest& request) const
{
    log_debug("start");

    auto response = xmlBuilder->createResponse(request.getActionName(), UPNP_DESC_CDS_SERVICE_TYPE);
    auto root = response->document_element();
    root.append_child("SortExtensionCaps").append_child(pugi::node_pcdata).set_value("+,-"); // TIME+, TIME-
    request.setResponse(std::move(response));

    log_debug("end");
}

void ContentDirectoryService::doGetSystemUpdateID(ActionRequest& request) const
{
    log_debug("start");

    auto response = xmlBuilder->createResponse(request.getActionName(), UPNP_DESC_CDS_SERVICE_TYPE);
    auto root = response->document_element();
    root.append_child("Id").append_child(pugi::node_pcdata).set_value(fmt::to_string(systemUpdateID).c_str());
    request.setResponse(std::move(response));

    log_debug("end");
}

void ContentDirectoryService::doSamsungBookmark(ActionRequest& request) const
{
    log_debug("start");

    request.getQuirks()->saveSamsungBookMarkedPosition(database, request);

    log_debug("end");
}

void ContentDirectoryService::doSamsungFeatureList(ActionRequest& request)
{
    log_debug("start");

    request.getQuirks()->getSamsungFeatureList(request);

    log_debug("end");
}

void ContentDirectoryService::doSamsungGetObjectIDfromIndex(ActionRequest& request)
{
    log_debug("start");

    request.getQuirks()->getSamsungObjectIDfromIndex(request);

    log_debug("end");
}

void ContentDirectoryService::doSamsungGetIndexfromRID(ActionRequest& request)
{
    log_debug("start");

    request.getQuirks()->getSamsungIndexfromRID(request);

    log_debug("end");
}

bool ContentDirectoryService::processSubscriptionRequest(const SubscriptionRequest& request)
{
    log_debug("start");

    auto propset = xmlBuilder->createEventPropertySet();
    auto property = propset->document_element().first_child();
    property.append_child("SystemUpdateID").append_child(pugi::node_pcdata).set_value(fmt::to_string(systemUpdateID).c_str());
    auto obj = database->loadObject(DEFAULT_CLIENT_GROUP, 0);
    auto cont = std::static_pointer_cast<CdsContainer>(obj);
    property.append_child("ContainerUpdateIDs").append_child(pugi::node_pcdata).set_value(fmt::format("0,{}", cont->getUpdateID()).c_str());

    std::string xml = UpnpXMLBuilder::printXml(*propset, "", 0);

    return UPNP_E_SUCCESS == GrbUpnpAcceptSubscription( //
               deviceHandle, config->getOption(ConfigVal::SERVER_UDN), //
               serviceID, xml, request.getSubscriptionID());
}

bool ContentDirectoryService::sendSubscriptionUpdate(const std::string& containerUpdateIDsCsv)
{
    log_debug("start {}", containerUpdateIDsCsv);

    systemUpdateID++;

    auto propset = xmlBuilder->createEventPropertySet();
    auto property = propset->document_element().first_child();
    property.append_child("ContainerUpdateIDs").append_child(pugi::node_pcdata).set_value(containerUpdateIDsCsv.c_str());
    property.append_child("SystemUpdateID").append_child(pugi::node_pcdata).set_value(fmt::to_string(systemUpdateID).c_str());

    std::string xml = UpnpXMLBuilder::printXml(*propset, "", 0);
    return UPNP_E_SUCCESS == GrbUpnpNotify( //
               deviceHandle, config->getOption(ConfigVal::SERVER_UDN), //
               serviceID, xml);
}
