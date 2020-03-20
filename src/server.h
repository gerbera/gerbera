/*MT*
    
    MediaTomb - http://www.mediatomb.cc/
    
    server.h - this file is part of MediaTomb.
    
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

/// \file server.h
///\brief Definitions of the server class.
#ifndef __SERVER_H__
#define __SERVER_H__

#include "action_request.h"
#include "request_handler.h"
#include "subscription_request.h"
#include "upnp_cds.h"
#include "upnp_cm.h"
#include "upnp_mrreg.h"

// forward declaration
class ConfigManager;
class Storage;
class UpdateManager;
class Timer;
namespace web {
class SessionManager;
}
class TaskProcessor;
class Runtime;
class LastFm;
class ContentManager;

/// \brief Provides methods to initialize and shutdown
/// and to retrieve various information about the server.
class Server : public std::enable_shared_from_this<Server> {
public:
    explicit Server(std::shared_ptr<ConfigManager> config);

    /// \brief Initializes the server.
    ///
    /// This function reads information from the config and initializes
    /// various variables (like server UDN and so forth).
    virtual void init();

    virtual ~Server();

    /// \brief Cleanup routine to shutdown the server.
    ///
    /// Unregisters the device with the SDK, shuts down the
    /// update manager task, storage task, content manager.
    void shutdown();

    /// \brief Initializes UPnP portion, only ip or interface can be given
    ///
    /// Reads information from the config and creates a
    /// device description document. Initializes the UPnP SDK,
    /// sets up the virutal web server directories and registers
    /// web callbacks. Starts the update manager task.
    void run();

    /// \brief Dispatch incoming UPnP events.
    /// \param eventtype Upnp_EventType, identifying what kind of event came in.
    /// \param event Pointer to the event.
    ///
    /// This function is called when a UPnP event is received,
    /// it then looks at the event type (either an action invocation or
    /// a subscription request) and dispatches the event accordingly.
    /// The event that is coming from the SDK is converted to our internal
    /// data structures (ActionRequest or SubscriptionRequest) and is then
    /// passed on to the appropriate request handler - to routeActionEvent() or
    /// upnp_subscriptions()
    int handleUpnpEvent(Upnp_EventType eventtype, const void* event);

    /// \brief Returns the IP address of the server.
    ///
    /// Returns a string representation of the IP where the server is
    /// running. This is useful for constructing URL's, etc.
    static std::string getIP();

    /// \brief Returns the port of the server.
    ///
    /// Returns a string representation of the server port. Allthough
    /// the port is also specified in the config, we can never be sure
    /// that we actually get that port after startup. This function
    /// returns the port on which the server is actually running.
    static std::string getPort();

    /// \brief Tells if the server is about to be terminated.
    ///
    /// This function returns true if the server is about to be
    /// terminated. This is the case when upnp_clean() was called.
    bool getShutdownStatus() const;

    void sendCDSSubscriptionUpdate(const std::string& updateString);

    std::shared_ptr<ContentManager> getContent() const { return content; }

protected:
    std::shared_ptr<ConfigManager> config;
    std::shared_ptr<Storage> storage;
    std::shared_ptr<UpdateManager> update_manager;
    std::shared_ptr<Timer> timer;
    std::shared_ptr<web::SessionManager> session_manager;
    std::shared_ptr<TaskProcessor> task_processor;
    std::shared_ptr<Runtime> scripting_runtime;
    std::shared_ptr<LastFm> last_fm;
    std::shared_ptr<ContentManager> content;

    /// \brief This flag is set to true by the upnp_cleanup() function.
    bool server_shutdown_flag;

    /// \brief Handle for our upnp device.
    ///
    /// This variable is returned by the getDeviceHandle() function.
    UpnpDevice_Handle deviceHandle;

    /// \brief Unique Device Number of the server.
    ///
    /// The UDN is read from the config, it must be unique and
    /// persistent over reboots.
    std::string serverUDN;

    /// \brief Name of the virtual web server directory.
    ///
    /// All requests going to /content/ will be handled by our web
    /// callback functions.
    /// \todo Is there any need that this is a variable? A constant
    /// should be sufficient here.
    std::string virtual_directory;

    /// \brief Full virtual web server url.
    ///
    /// The URL is constructed upon server initialization, since
    /// the real port is not known before. The value of this variable
    /// is returned by the getVirtualURL() function.
    std::string virtualUrl;

    /// \brief Device description document is created on the fly and
    /// stored here.
    ///
    /// All necessary values for the device description document are
    /// read from the configuration and the device desc doc is created
    /// on the fly.
    std::string device_description_document;

    /// \brief Time interval to send ssdp:alive advertisements.
    ///
    /// The value is read from the configuration.
    int aliveAdvertisementInterval;

    std::unique_ptr<UpnpXMLBuilder> xmlbuilder;

    /// \brief ContentDirectoryService instance.
    ///
    /// The ContentDirectoryService class is instantiated in the
    /// constructor. The class is responsible for processing
    /// an ActionRequest or a SubscriptionRequest.
    std::unique_ptr<ContentDirectoryService> cds;

    /// \brief ConnectionManagerService instance.
    ///
    /// The ConnectionManagerService class is instantiated in the
    /// constructor. The class is responsible for processing
    /// an ActionRequest or a SubscriptionRequest.
    std::unique_ptr<ConnectionManagerService> cmgr;

    /// \brief MediaReceiverRegistrarService instance.
    ///
    /// This class is not fully functional, it always returns "true"
    /// on IsAuthorized and IsValidated requests. It added to ensure
    /// Xbox360 compatibility.
    std::unique_ptr<MRRegistrarService> mrreg;

    /// \brief Dispatched an ActionRequest between the services.
    /// \param request Incoming ActionRequest.
    ///
    /// Currently we only support two services (ContentDirectoryService
    /// and ConnectionManagerService), this function looks at the service id
    /// of the request and calls the process_action_request() for the
    /// appropriate service.
    void routeActionRequest(const std::unique_ptr<ActionRequest>& request) const;

    /// \brief Dispatched a SubscriptionRequest between the services.
    /// \param request Incoming SubscriptionRequest.
    ///
    /// Currently we only support two services (ContentDirectoryService
    /// and ConnectionManagerService), this function looks at the service id
    /// of the request and calls the process_subscription_request() for the
    /// appropriate service.
    void routeSubscriptionRequest(const std::unique_ptr<SubscriptionRequest>& request) const;

    /// \brief Registers callback functions for the internal web server.
    /// \param filename Incoming filename.
    ///
    std::unique_ptr<RequestHandler> createRequestHandler(const char* filename) const;

    /// \brief Registers callback functions for the internal web server.
    ///
    /// This function registers callbacks for the internal web server.
    /// The callback functions are:
    /// \b web_get_info Query information on a file.
    /// \b web_open Open a file.
    /// \b web_read Sequentially read from a file.
    /// \b web_write Sequentially write to a file (not supported).
    /// \b web_seek Perform a seek on a file.
    /// \b web_close Close file.
    ///
    /// \return UPNP_E_SUCCESS Callbacks registered successfully, else error code.
    static int registerVirtualDirCallbacks();

    static int handleUpnpEventCallback(Upnp_EventType eventtype, const void* event, void* cookie);
};

#endif // __SERVER_H__
