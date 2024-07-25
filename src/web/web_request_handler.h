/*MT*

    MediaTomb - http://www.mediatomb.cc/

    web/web_request_handler.h - this file is part of MediaTomb.

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

/// \file web/web_request_handler.h
/// \brief Definition of the WebRequestHandler class.
#ifndef __WEB_REQUEST_HANDLER_H__
#define __WEB_REQUEST_HANDLER_H__

#include "request_handler.h"
#include "util/tools.h"

#include <pugixml.hpp>

// URL FORMATTING CONSTANTS
#define URL_UI_PARAM_SEPARATOR '?'

class GenericTask;
class Server;
class Xml2Json;

namespace Web {

class SessionException : public std::runtime_error {
    using std::runtime_error::runtime_error;
};

class LoginException : public std::runtime_error {
    using std::runtime_error::runtime_error;
};

class Session;
class SessionManager;

static constexpr auto SID = "GerberaSID";

/// \brief This class is responsible for processing requests that come to the user interface.
class WebRequestHandler : public RequestHandler {
protected:
    std::shared_ptr<Web::SessionManager> sessionManager;
    std::shared_ptr<Server> server;

    bool checkRequestCalled {};

    /// \brief Decoded URL parameters are stored as a dictionary.
    std::map<std::string, std::string> params;

    /// \brief The original filename from url if anyone needs it.
    std::string filename;

    /// \brief We can also always see what mode was requested.
    enum UpnpOpenFileMode mode {};

    /// \brief This is the xml document, the root node to be populated by process() method.
    std::unique_ptr<pugi::xml_document> xmlDoc;

    /// \brief Hints for Xml2Json, such that we know when to create an array
    std::unique_ptr<Xml2Json> xml2Json;

    /// \brief The current session, used for this request; will be filled by
    /// checkRequest()
    std::shared_ptr<Session> session;

    /// \brief Little support function to access stuff from the dictionary in
    /// in an easier fashion.
    /// \param name of the parameter we are looking for.
    /// \return Value of the parameter with the given name or nullptr if not found.
    std::string param(const std::string& name) const { return getValueOrDefault(params, name); }

    int intParam(const std::string& name, int invalid = 0) const;
    bool boolParam(const std::string& name) const;

    /// \brief Checks if the incoming request is valid.
    ///
    /// Each request going to the ui must at least have a valid session id,
    /// and a driver parameter. Also, there can only by a primary or a
    /// a decondary driver.
    void checkRequest(bool checkLogin = true);

    /// \brief add the ui update ids from the given session as xml tags to the given root element
    /// \param session the session from which the ui update ids should be taken
    /// \param updateIDsEl the xml element to add the elements to
    static void addUpdateIDs(const std::shared_ptr<Session>& session, pugi::xml_node& updateIDsEl);

    /// \brief check if ui update ids should be added to the response and add
    /// them in that case.
    /// must only be called after checkRequest
    void handleUpdateIDs();

    /// \brief add the content manager task to the given xml element as xml elements
    /// \param task the task to add to the given xml element
    /// \param parent the xml element to add the elements to
    static void appendTask(const std::shared_ptr<GenericTask>& task, pugi::xml_node& parent);

    static std::string_view mapAutoscanType(int type);

public:
    /// \brief Constructor, currently empty.
    explicit WebRequestHandler(const std::shared_ptr<Content>& content, std::shared_ptr<Server> server);

    /// \brief Returns information about the requested content.
    /// \param filename Requested URL
    /// \param info File_Info structure, quite similar to statbuf.
    /// \return the ClientInfo details to be provided to quirks
    ///
    /// We need to override this, because for serving UI pages (mostly generated from
    /// dynamic XML) we do not know the size of the data. This is of course different
    /// for the FileRequestHandler, where we can check the file and return all
    /// information about it.
    const struct ClientInfo* getInfo(const char* filename, UpnpFileInfo* info) override;

    /// \brief Decodes the parameters from the filename (URL) and calls the internal open() function.
    /// \param filename The requested URL
    /// \param quirks allows modifying the content of the response based on the client
    /// \param mode either UPNP_READ or UPNP_WRITE
    /// \return the appropriate IOHandler for the request.
    std::unique_ptr<IOHandler> open(const char* filename, const std::shared_ptr<Quirks>& quirks, enum UpnpOpenFileMode mode) override;

    /// \brief This method must be overridden by the subclasses that actually process the given request.
    virtual void process() = 0;
};

} // namespace Web

#endif // __WEB_REQUEST_HANDLER_H__
