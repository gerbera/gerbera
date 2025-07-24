/*MT*

    MediaTomb - http://www.mediatomb.cc/

    server.h - this file is part of MediaTomb.

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

/// \file server.h
///\brief Definitions of the server class.
#ifndef __SERVER_H__
#define __SERVER_H__

#include <memory>
#include <netinet/in.h>
#include <string>
#include <upnp.h>
#include <vector>

// forward declarations
class ActionRequest;
class ClientManager;
class Config;
class ConfigDefinition;
class Content;
class ConverterManager;
class Context;
class Database;
class MetadataService;
class Mime;
class Quirks;
class RequestHandler;
class SubscriptionRequest;
class Timer;
class UpnpXMLBuilder;
class UpnpService;
namespace Web {
class SessionManager;
}

/// \brief Provides methods to initialize and shutdown
/// and to retrieve various information about the server.
class Server : public std::enable_shared_from_this<Server> {
public:
    explicit Server(std::shared_ptr<Config> config);

    /// \brief Initializes the server.
    ///
    /// This function reads information from the config and initializes
    /// various variables (like server UDN and so forth).
    void init(const std::shared_ptr<ConfigDefinition>& definition, bool offln);

    /// \brief Cleanup routine to shutdown the server.
    ///
    /// Unregisters the device with the SDK, shuts down the
    /// update manager task, database task, content manager.
    void shutdown();

    /// \brief Initializes UPnP portion, only ip or interface can be given
    ///
    /// Reads information from the config and creates a
    /// device description document. Initializes the UPnP SDK,
    /// sets up the virtual web server directories and registers
    /// web callbacks. Starts the update manager task.
    void run();

    /// \brief Tells if the server is about to be terminated.
    ///
    /// This function returns true if the server is about to be
    /// terminated. This is the case when upnp_clean() was called.
    bool getShutdownStatus() const;

    void sendSubscriptionUpdate(const std::string& updateString, const std::string& serviceId);

    std::shared_ptr<Content> getContent() const { return content; }
    std::vector<std::string> getCorsHosts() const { return corsHosts; }

protected:
    std::shared_ptr<Config> config;
    std::shared_ptr<ClientManager> clientManager;
    std::shared_ptr<Mime> mime;
    std::shared_ptr<Database> database;
    std::shared_ptr<Web::SessionManager> sessionManager;
    std::shared_ptr<ConverterManager> converterManager;
    std::shared_ptr<Context> context;

    std::shared_ptr<Timer> timer;
    std::shared_ptr<Content> content;
    std::shared_ptr<MetadataService> metadataService;
    std::shared_ptr<Server> self;

    std::string ip;
    std::string ip6;
    std::string ulaGuaIp6;
    in_port_t port {};
    in_port_t port6 {};
    in_port_t ulaGuaPort6 {};
    std::vector<std::string> validHosts;
    std::vector<std::string> corsHosts;

    /// \brief This flag is set to true by the upnp_cleanup() function.
    bool server_shutdown_flag {};

    bool offline {};

    /// \brief Handle for our upnp callbacks.
    UpnpDevice_Handle rootDeviceHandle {};
    UpnpDevice_Handle clientHandle {};

    /// \brief Unique Device Number of the server.
    ///
    /// The UDN is read from the config, it must be unique and
    /// persistent over reboots.
    std::string serverUDN;

    /// \brief Time interval to send ssdp:alive advertisements.
    ///
    /// The value is read from the configuration.
    int aliveAdvertisementInterval {};

    std::shared_ptr<UpnpXMLBuilder> upnpXmlBuilder;
    std::shared_ptr<UpnpXMLBuilder> webXmlBuilder;
    std::vector<std::unique_ptr<UpnpService>> serviceList;

    /// @brief get string for active ip
    std::string getIp() const;
    /// @brief get active port
    in_port_t getPort() const;

    /// \brief Dispatched an ActionRequest between the services.
    /// \param request Incoming ActionRequest.
    ///
    /// Currently we only support two services (ContentDirectoryService
    /// and ConnectionManagerService), this function looks at the service id
    /// of the request and calls the process_action_request() for the
    /// appropriate service.
    void routeActionRequest(ActionRequest& request);

    /// \brief Dispatched a SubscriptionRequest between the services.
    /// \param request Incoming SubscriptionRequest.
    ///
    /// Currently we only support two services (ContentDirectoryService
    /// and ConnectionManagerService), this function looks at the service id
    /// of the request and calls the process_subscription_request() for the
    /// appropriate service.
    void routeSubscriptionRequest(const SubscriptionRequest& request) const;

    /// \brief Registers callback functions for the internal web server.
    /// \param filename Incoming filename/url.
    /// \param quirks Client specific handling.
    ///
    std::unique_ptr<RequestHandler> createRequestHandler(const char* filename, const std::shared_ptr<Quirks>& quirks) const;

    /// \brief Registers callback functions for the internal web server.
    ///
    /// This function registers callbacks for the internal web server.
    /// The callback functions are:
    /// \arg \c GetInfoCallback Query information on a file.
    /// \arg \c OpenCallback Open a file.
    /// \arg \c ReadCallback Sequentially read from a file.
    /// \arg \c WriteCallback Sequentially write to a file (not supported).
    /// \arg \c SeekCallback Perform a seek on a file.
    /// \arg \c CloseCallback Close file.
    ///
    /// \return \c UPNP_E_SUCCESS Callbacks registered successfully, else error code.
    static int registerVirtualDirCallbacks();

    /// \brief Dispatch incoming UPnP root device events.
    /// \param eventType identifying what kind of event came in.
    /// \param event Pointer to the event.
    ///
    /// This function is called when a UPnP root device event is received,
    /// it then looks at the event type (either an action invocation or
    /// a subscription request) and dispatches the event accordingly.
    /// The event that is coming from the SDK is converted to our internal
    /// data structures (ActionRequest or SubscriptionRequest) and is then
    /// passed on to the appropriate request handler - to routeActionEvent() or
    /// upnp_subscriptions()
    int handleUpnpRootDeviceEvent(Upnp_EventType eventType, const void* event);

    /// \brief Dispatch incoming UPnP client events.
    /// \param eventType identifying what kind of event came in.
    /// \param event Pointer to the event.
    ///
    int handleUpnpClientEvent(Upnp_EventType eventType, const void* event);

    /// \brief Creates a html file that is a redirector to the current server i
    /// instance
    void writeBookmark(const std::string& addr);
    void emptyBookmark();

    int startupInterface(const std::string& iface, in_port_t inPort);

    /// \brief Returns the content url of the server.
    ///
    /// Returns a string representation of the server url. Although
    /// the port is also specified in the config, we can never be sure
    /// that we actually get that port after startup. This function
    /// contains the port on which the server is actually running.
    std::string getVirtualUrl() const;
    std::string getExternalUrl() const;

    std::shared_ptr<Quirks> getQuirks(const UpnpFileInfo* info, bool isWeb) const;
    /// \brief Upnp callbacks
    static int HostValidateCallback(const char* host, void* cookie);
    static int GetInfoCallback(const char* filename, UpnpFileInfo* info, const void* cookie, const void** requestCookie);
    static UpnpWebFileHandle OpenCallback(const char* filename, enum UpnpOpenFileMode mode, const void* cookie, const void* requestCookie);
    static int ReadCallback(UpnpWebFileHandle f, char* buf, std::size_t length, const void* cookie, const void* requestCookie);
    static int WriteCallback(UpnpWebFileHandle f, char* buf, std::size_t length, const void* cookie, const void* requestCookie);
    static int SeekCallback(UpnpWebFileHandle f, off_t offset, int whence, const void* cookie, const void* requestCookie);
    static int CloseCallback(UpnpWebFileHandle f, const void* cookie, const void* requestCookie);
};

#endif // __SERVER_H__
