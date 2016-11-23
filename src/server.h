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

#include "common.h"
#include "singleton.h"
#include "action_request.h"
#include "subscription_request.h"
#include "upnp_cds.h"
#include "upnp_cm.h"
#include "upnp_mrreg.h"
#include "config_manager.h"
#include "sync.h"
#include "storage.h"

/// \brief Provides methods to initialize and shutdown
/// and to retrieve various information about the server.
class Server : public Singleton<Server>
{
public:
    
    Server();
    
    /// \brief Initializes the server.
    /// 
    /// This function reads information from the config and initializes
    /// various variables (like server UDN and so forth).
    virtual void init();
    
    
    /// \brief Initializes UPnP portion, only ip or interface can be given
    /// \param interface to bind to
    /// \param ip address to bind to
    /// \param port Port where the server will be listening for requests.
    ///
    /// Reads information from the config and creates a 
    /// device description document. Initializes the UPnP SDK,
    /// sets up the virutal web server directories and registers
    /// web callbacks. Starts the update manager task.
    void upnp_init(zmm::String interface, zmm::String ip_address, int port);
    
    /// \brief Cleanup routine to shutdown the server.
    ///
    /// Unregisters the device with the SDK, shuts down the
    /// update manager task, storage task, content manager.
    virtual void shutdown();
    
    /// \brief Returns the virtual web server URL.
    ///
    /// A special virtual URL is registered with the internal web server,
    /// all content and all ui requests are server from there and are
    /// handled by the web callbacks.
    zmm::String getVirtualURL();
    
    /// \brief Dispatch incoming UPnP events.
    /// \param eventtype Upnp_EventType, identifying what kind of event came in.
    /// \param event Pointer to the event.
    /// \param cookie Unused.
    ///
    /// This function is called when a UPnP event is received,
    /// it then looks at the event type (either an action invocation or 
    /// a subscription request) and dispatches the event accordingly.
    /// The event that is coming from the SDK is converted to our internal
    /// data structures (ActionRequest or SubscriptionRequest) and is then
    /// passed on to the appropriate request handler - to upnp_actions() or
    /// upnp_subscriptions()
    int upnp_callback(Upnp_EventType eventtype, const void *event, void *cookie);
  
    /// \brief Returns the device handle.
    ///
    /// This function returns the handle for our device (it is needed to
    /// take care of subscriptions)
    UpnpDevice_Handle getDeviceHandle();
    
    /// \brief Returns the IP address of the server.
    ///
    /// Returns a string representation of the IP where the server is 
    /// running. This is useful for constructing URL's, etc.
    zmm::String getIP();
    
    /// \brief Returns the port of the server.
    ///
    /// Returns a string representation of the server port. Allthough
    /// the port is also specified in the config, we can never be sure
    /// that we actually get that port after startup. This function
    /// returns the port on which the server is actually running.
    zmm::String getPort();
    
    
    /// \brief Tells if the server is about to be terminated.
    ///
    /// This function returns true if the server is about to be
    /// terminated. This is the case when upnp_clean() was called.
    bool getShutdownStatus();

    static void static_cleanup_callback();
 
protected:
    static zmm::Ref<Storage> storage;

    /// \brief This flag is set to true by the upnp_cleanup() function.
    bool server_shutdown_flag;
    
    /// \brief Handle for our upnp device.
    ///
    /// This variable is returned by the getDeviceHandle() function.
    UpnpDevice_Handle device_handle;
    
    /// \brief Unique Device Number of the server.
    ///
    /// The UDN is read from the config, it must be unique and
    /// persistent over reboots.
    zmm::String serverUDN;
    
    /// \brief Name of the virtual web server directory.
    ///
    /// All requests going to /content/ will be handled by our web
    /// callback functions.
    /// \todo Is there any need that this is a variable? A constant
    /// should be sufficient here.
    zmm::String virtual_directory;
    
    /// \brief Full virtual web server url.
    /// 
    /// The URL is constructed upon server initialization, since
    /// the real port is not known before. The value of this variable
    /// is returned by the getVirtualURL() function.
    zmm::String virtual_url;
    
    /// \brief Device description document is created on the fly and 
    /// stored here.
    ///
    /// All necessary values for the device description document are 
    /// read from the configuration and the device desc doc is created
    /// on the fly.
    zmm::String device_description_document;
    
    /// \brief Time interval to send ssdp:alive advertisements.
    ///
    /// The value is read from the configuration.
    int alive_advertisement;
    
    /// \brief ContentDirectoryService instance.
    /// 
    /// The ContentDirectoryService class is instantiated in the
    /// constructor. The class is responsible for processing
    /// an ActionRequest or a SubscriptionRequest.
    zmm::Ref<ContentDirectoryService> cds;
    
    /// \brief ConnectionManagerService instance.
    /// 
    /// The ConnectionManagerService class is instantiated in the
    /// constructor. The class is responsible for processing
    /// an ActionRequest or a SubscriptionRequest.
    zmm::Ref<ConnectionManagerService> cmgr;

#if defined(ENABLE_MRREG)    
    /// \brief MediaReceiverRegistrarService instance.
    /// 
    /// This class is not fully functional, it always returns "true"
    /// on IsAuthorized and IsValidated requests. It added to ensure
    /// Xbos360 compatibility.
    zmm::Ref<MRRegistrarService> mrreg;
#endif

    /// \brief Dispatched an ActionRequest between the services.
    /// \param request Incoming ActionRequest.
    ///
    /// Currently we only support two services (ContentDirectoryService 
    /// and ConnectionManagerService), this function looks at the service id
    /// of the request and calls the process_action_request() for the 
    /// appropriate service.
    void upnp_actions(zmm::Ref<ActionRequest> request);
    
    /// \brief Dispatched a SubscriptionRequest between the services.
    /// \param request Incoming SubscriptionRequest.
    ///
    /// Currently we only support two services (ContentDirectoryService 
    /// and ConnectionManagerService), this function looks at the service id
    /// of the request and calls the process_subscription_request() for the 
    /// appropriate service.
    void upnp_subscriptions(zmm::Ref<SubscriptionRequest> request);
};

#endif // __SERVER_H__
