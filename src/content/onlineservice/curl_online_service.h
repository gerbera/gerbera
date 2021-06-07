/*GRB*

    Gerbera - https://gerbera.io/

    curl_online_service.h - this file is part of Gerbera.

    Copyright (C) 2021 Gerbera Contributors

    Gerbera is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License version 2
    as published by the Free Software Foundation.

    Gerbera is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with Gerbera.  If not, see <http://www.gnu.org/licenses/>.

    $Id$
*/

/// \file curl_online_service.h
/// \brief Definition of the CurlContentHandler class.
/// \brief Definition of the CurlOnlineService class.

#ifdef HAVE_CURL

#ifndef __CURL_ONLINE_SERVICE_H__
#define __CURL_ONLINE_SERVICE_H__

#include <curl/curl.h>
#include <memory>
#include <pugixml.hpp>

#include "context.h"
#include "online_service.h"

// forward declaration
class ContentManager;
class CdsObject;

/// \brief this class is responsible for creating objects from the CurlContentHandler
/// metadata XML.
class CurlContentHandler {
public:
    explicit CurlContentHandler(const std::shared_ptr<Context>& context);
    virtual ~CurlContentHandler() = default;

    /// \brief Sets the service XML from which we will extract the objects.
    /// \return false if service XML contained an error status.
    virtual void setServiceContent(std::unique_ptr<pugi::xml_document> service) = 0;

    /// \brief retrieves an object from the service.
    ///
    /// Each invokation of this funtion will return a new object,
    /// when the whole service XML is parsed and no more objects are left,
    /// this function will return nullptr.
    ///
    /// \return CdsObject or nullptr if there are no more objects to parse.
    virtual std::shared_ptr<CdsObject> getNextObject() = 0;

protected:
    std::shared_ptr<Config> config;
    std::shared_ptr<Database> database;

    std::unique_ptr<pugi::xml_document> service_xml;
};

/// \brief This is an interface for all online services, the function
/// handles adding/refreshing content in the database.
class CurlOnlineService : public OnlineService {
public:
    explicit CurlOnlineService(std::shared_ptr<ContentManager> content, std::string serviceName);
    ~CurlOnlineService() override;

    CurlOnlineService(const CurlOnlineService&) = delete;
    CurlOnlineService& operator=(const CurlOnlineService&) = delete;

    /// \brief Retrieves user specified content from the service and adds
    /// the items to the database.
    bool refreshServiceData(std::shared_ptr<Layout> layout) override;

    /// \brief Get the human readable name for the service
    std::string getServiceName() const override;

protected:
    virtual std::unique_ptr<CurlContentHandler> getContentHandler() const = 0;

    std::unique_ptr<pugi::xml_document> getData();

    // the handle *must never be used from multiple threads*
    CURL* curl_handle;
    // safeguard to ensure the above
    pthread_t pid;

    std::string service_url;
    std::string serviceName;
};

#endif //__CURL_ONLINE_SERVICE_H__

#endif //HAVE_CURL
