/*GRB*

    Gerbera - https://gerbera.io/

    web/page_request.cc - this file is part of Gerbera.

    Copyright (C) 2025 Gerbera Contributors

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

/// \file web/page_request.h
/// \brief Defines the WebRequestHandler sub class with common page behaviour
#ifndef __PAGE_REQUEST_H__
#define __PAGE_REQUEST_H__

#include "web_request_handler.h"

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

/// \brief WebRequestHandler sub class with common page behaviour
class PageRequest : public WebRequestHandler {
    using WebRequestHandler::WebRequestHandler;

protected:
    void process(pugi::xml_node& root) override;
    /// \brief This method must be overridden by the subclasses that actually process the given request.
    virtual void processPageAction(pugi::xml_node& element) = 0;
    /// \brief get key for page
    virtual std::string getPage() const = 0;

    /// \brief Checks if the incoming request is valid.
    ///
    /// Each request going to the ui must at least have a valid session id,
    /// and a driver parameter. Also, there can only by a primary or a
    /// a decondary driver.
    void checkRequest(bool checkLogin = true);

    /// \brief get int param from request
    int intParam(const std::string& name, int invalid = 0) const;
    /// \brief get boolean param from request
    bool boolParam(const std::string& name) const;

    /// \brief read resource information from web operation
    /// \param object target object for the resources found
    bool readResources(const std::shared_ptr<CdsObject>& object);

    /// \brief check if ui update ids should be added to the response and add
    /// them in that case.
    /// must only be called after checkRequest
    void handleUpdateIDs(pugi::xml_node& element);

    /// \brief add the content manager task to the given xml element as xml elements
    /// \param task the task to add to the given xml element
    /// \param parent the xml element to add the elements to
    static void appendTask(
        const std::shared_ptr<GenericTask>& task,
        pugi::xml_node& parent);

    /// \brief add the ui update ids from the given session as xml tags to the given root element
    /// \param session the session from which the ui update ids should be taken
    /// \param updateIDsEl the xml element to add the elements to
    static void addUpdateIDs(
        const std::shared_ptr<Session>& session,
        pugi::xml_node& updateIDsEl);
};

} // namespace Web

#endif // __PAGE_REQUEST_H__
