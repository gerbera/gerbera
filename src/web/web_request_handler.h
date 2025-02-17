/*MT*

    MediaTomb - http://www.mediatomb.cc/

    web/web_request_handler.h - this file is part of MediaTomb.

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

/// \file web/web_request_handler.h
/// \brief Definition of the WebRequestHandler class.
#ifndef __WEB_REQUEST_HANDLER_H__
#define __WEB_REQUEST_HANDLER_H__

#include "request_handler/request_handler.h"
#include "util/tools.h"

#include <pugixml.hpp>

class ConfigValue;
class GenericTask;
class Server;
class UpnpXMLBuilder;
class Xml2Json;
enum class AutoscanType;
enum class ConfigVal;

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

    /// \brief Decoded URL parameters are stored as a dictionary.
    std::map<std::string, std::string> params;

    /// \brief The original filename from url if anyone needs it.
    std::string filename;

    /// \brief We can also always see what mode was requested.
    enum UpnpOpenFileMode mode {};

    /// \brief This is the xml document, the root node to be populated by \c process() method.
    std::unique_ptr<pugi::xml_document> xmlDoc;

    /// \brief Hints for \c Xml2Json, such that we know when to create an array
    std::unique_ptr<Xml2Json> xml2Json;

    /// \brief The current session, used for this request; will be filled by
    /// \c checkRequest()
    std::shared_ptr<Session> session;

    /// \brief Little support function to access stuff from the dictionary in
    /// in an easier fashion.
    /// \param name of the parameter we are looking for.
    /// \return Value of the parameter with the given name or \c nullptr if not found.
    std::string param(const std::string& name) const { return getValueOrDefault(params, name); }

    /// \brief Get name of user group on web frontend
    std::string getGroup() const;

    /// \brief This method must be overridden by the subclasses that actually process the given request.
    virtual void process(pugi::xml_node& root) = 0;

    /// \brief Convert Autoscan type to string representation
    static std::string_view mapAutoscanType(AutoscanType type);

public:
    /// \brief Constructor
    explicit WebRequestHandler(
        const std::shared_ptr<Content>& content,
        std::shared_ptr<Server> server,
        const std::shared_ptr<UpnpXMLBuilder>& xmlBuilder,
        const std::shared_ptr<Quirks>& quirks);

    /// \brief Returns information about the requested content.
    /// \param filename Requested URL
    /// \param info \c UpnpFileInfo structure, quite similar to statbuf.
    /// \return \c true if there was client info available
    ///
    /// We need to override this, because for serving UI pages (mostly generated from
    /// dynamic XML) we do not know the size of the data. This is of course different
    /// for the FileRequestHandler, where we can check the file and return all
    /// information about it.
    bool getInfo(const char* filename, UpnpFileInfo* info) override;

    /// \brief Decodes the parameters from the filename (URL) and calls the internal open() function.
    /// \param filename The requested URL
    /// \param quirks allows modifying the content of the response based on the client
    /// \param mode either \c UPNP_READ or \c UPNP_WRITE
    /// \return the appropriate IOHandler for the request.
    std::unique_ptr<IOHandler> open(
        const char* filename,
        const std::shared_ptr<Quirks>& quirks,
        enum UpnpOpenFileMode mode) override;
};

/// \brief Chooses and creates the appropriate handler for processing the request.
/// \param context runtime context
/// \param content content handler
/// \param server server instance
/// \param xmlBuilder builder for xml string
/// \param quirks hook to client specific behaviour
/// \param page identifies what type of the request we are dealing with.
/// \return the appropriate request handler.
std::unique_ptr<WebRequestHandler> createWebRequestHandler(
    const std::shared_ptr<Context>& context,
    const std::shared_ptr<Content>& content,
    const std::shared_ptr<Server>& server,
    const std::shared_ptr<UpnpXMLBuilder>& xmlBuilder,
    const std::shared_ptr<Quirks>& quirks,
    const std::string& page);

} // namespace Web

#endif // __WEB_REQUEST_HANDLER_H__
