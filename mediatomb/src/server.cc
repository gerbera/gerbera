/*  server.cc - this file is part of MediaTomb.
                                                                                
    Copyright (C) 2005 Gena Batyan <bgeradz@deadlock.dhs.org>,
                       Sergey Bostandzhyan <jin@deadlock.dhs.org>
                                                                                
    MediaTomb is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.
                                                                                
    MediaTomb is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.
                                                                                
    You should have received a copy of the GNU General Public License
    along with MediaTomb; if not, write to the Free Software
    Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
*/

#include "server.h"
#include "web_callbacks.h"
#include "content_manager.h"
#include "update_manager.h"
#include "dictionary.h"
#include "upnp_xml.h"
#include "tools.h"

using namespace zmm;

//Ref<Server> server;
static Ref<Server> instance = NULL;

static int static_upnp_callback(Upnp_EventType eventtype, void *event, void *cookie)
{
    return Server::getInstance()->upnp_callback(eventtype, event, cookie);
}


Server::Server() : Object()
{
    server_shutdown_flag = false;
}

Ref<Server> Server::getInstance()
{
    if (instance == NULL)
    {
        instance = Ref<Server>(new Server());
    }
        
    return instance;
}


void Server::init()
{
    virtual_directory = _(SERVER_VIRTUAL_DIR); 

    cds = ContentDirectoryService::createInstance(_(DESC_CDS_SERVICE_TYPE),
                                                  _(DESC_CDS_SERVICE_ID));
                                                 
    cmgr = ConnectionManagerService::createInstance(_(DESC_CM_SERVICE_TYPE),
                                                    _(DESC_CM_SERVICE_ID));

    pthread_mutex_init(&upnp_mutex, NULL);

    Ref<ConfigManager> config = ConfigManager::getInstance();

    serverUDN = config->getOption(_("/server/udn"));
    alive_advertisement = config->getIntOption(_("/server/alive"));
}

void Server::upnp_init(String ip, unsigned short port)
{
    int             ret = 0;        // general purpose error code

//    log_info(("upnp_init: start\n"));

    Ref<ConfigManager> config = ConfigManager::getInstance();

    if (ip == nil)
    {
        ip = config->getOption(_("/server/ip"), _(""));
        if (ip == "") ip = nil;
        log_info(("got ip: %s\n", ip.c_str()));
    }

    if (port == 0)
    {
        port = config->getIntOption(_("/server/port"));
    }
    
    ret = UpnpInit(ip.c_str(), port);

    if (ret != UPNP_E_SUCCESS)
    {
        throw UpnpException(ret, _("upnp_init: UpnpInit failed"));
    }

    port = UpnpGetServerPort();

    log_info(("Initialized port: %d\n", port));

    if (!string_ok(ip))
    {
        ip = String(UpnpGetServerIpAddress());
    }

    log_info(("Server bound to: %s\n", ip.c_str()));

    virtual_url = _("http://") + ip + ":" + port + "/" + virtual_directory;

    /// \TODO who should construct absolute paths??? config_manage or the modules?
    // next set webroot directory
    String web_root = config->getOption(_("/server/webroot"));

    if (!string_ok(web_root))
    {
        throw Exception(_("invalid web server root directory"));
    }
    
    ret = UpnpSetWebServerRootDir(web_root.c_str());
    if (ret != UPNP_E_SUCCESS)
    {
        throw UpnpException(ret, _("upnp_init: UpnpSetWebServerRootDir failed"));
    }

    log_info(("upnp_init: webroot: %s\n", web_root.c_str())); 
    
    ret = UpnpAddVirtualDir(virtual_directory.c_str());
    if (ret != UPNP_E_SUCCESS)
    {
        throw UpnpException(ret, _("upnp_init: UpnpAddVirtualDir failed"));
    }

    ret = register_web_callbacks();
    if (ret != UPNP_E_SUCCESS)
    {
        throw UpnpException(ret, _("upnp_init: UpnpSetVirtualDirCallbacks failed"));
    }

    // register root device with the library
    String device_description = _("<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n") +
                                UpnpXML_RenderDeviceDescription()->print();

//    log_info(("DEVICE DESCRIPTION: \n%s\n", device_description.c_str()));

    // register root device with the library
    ret = UpnpRegisterRootDevice2(UPNPREG_BUF_DESC, device_description.c_str(), 
                                  device_description.length() + 1, true,
                                  static_upnp_callback,
                                  &device_handle,
                                  &device_handle);
    if (ret != UPNP_E_SUCCESS)
    {
        throw UpnpException(ret, _("upnp_init: UpnpRegisterRootDevice failed"));
    }

    // now unregister in order to cleanup previous instances
    // that might still be there due to crash/unclean shutdown/network interruptions
    ret = UpnpUnRegisterRootDevice(device_handle);
    if (ret != UPNP_E_SUCCESS) {   
        throw UpnpException(ret, _("upnp_init: unregistering failed"));
    }
    
    // and register again, we should be clean now
    ret = UpnpRegisterRootDevice2(UPNPREG_BUF_DESC, device_description.c_str(), 
                                  device_description.length() + 1, true,
                                  static_upnp_callback,
                                  &device_handle,
                                  &device_handle);
    if (ret != UPNP_E_SUCCESS)
    {
        throw UpnpException(ret, _("upnp_init: UpnpRegisterRootDevice failed"));
    }


    // send advertisements every 180secs
    ret = UpnpSendAdvertisement(device_handle, alive_advertisement);
    if (ret != UPNP_E_SUCCESS)
    {
        throw UpnpException(ret, _("upnp_init: UpnpSendAdvertisement failed"));
    }

    // initializing UpdateManager
    UpdateManager::getInstance()->init();
    
    // initializing ContentManager
    ContentManager::getInstance()->init();    
   
    config->writeBookmark(ip, String::from(port));
    
//    log_info(("upnp_init: end\n"));
}

bool Server::getShutdownStatus()
{
    return server_shutdown_flag;
}

void Server::upnp_cleanup()
{
    int ret = 0; // return code    

    UpdateManager::getInstance()->shutdown();
    ContentManager::getInstance()->shutdown();

    server_shutdown_flag = true;

//    log_info(("upnp_cleanup: start\n"));

    // unregister device
    
    ret = UpnpUnRegisterRootDevice(device_handle);
    if (ret != UPNP_E_SUCCESS) {   
        throw UpnpException(ret, _("upnp_cleanup: UpnpUnRegisterRootDevice failed"));
    }
   
//    log_info(("now calling upnp finish\n"));
    UpnpFinish();

//    log_info(("upnp_cleanup: end\n"));

    pthread_mutex_destroy(&upnp_mutex);
}

String Server::getVirtualURL()
{
    return virtual_url;
}


int Server::upnp_callback(Upnp_EventType eventtype, void *event, void *cookie)
{
    int ret = UPNP_E_SUCCESS; // general purpose return code

//    log_info(("upnp_callback: start\n"));

    // check parameters
    if (event == NULL) {
        log_info(("upnp_callback: NULL event structure\n"));
        return UPNP_E_BAD_REQUEST;
    }

//    log_info(("event is ok\n"));
    // get device wide mutex (have to figure out what the hell that is)
    pthread_mutex_lock(&upnp_mutex);

//    log_info(("got device mutex\n"));

    // dispatch event based on event type
    switch (eventtype) {

        case UPNP_CONTROL_ACTION_REQUEST:
            // a CP is invoking an action
//            log_info(("upnp_callback: UPNP_CONTROL_ACTION_REQUEST\n"));
            try
            {
                Ref<ActionRequest> request(new ActionRequest((struct Upnp_Action_Request *)event));
                upnp_actions(request);
                request->update();
                ((struct Upnp_Action_Request *)event)->ErrCode = ret;
            }
            catch(UpnpException upnp_e)
            {
                ret = upnp_e.getErrorCode();
                ((struct Upnp_Action_Request *)event)->ErrCode = ret;
            }
            catch(Exception e)
            {
                log_info(("Exception: %s\n", e.getMessage().c_str()));
            }
            
            break;

        case UPNP_EVENT_SUBSCRIPTION_REQUEST:
            // a cp wants a subscription
//            log_info(("upnp_callback: UPNP_EVENT_SUBSCRIPTION_REQUEST\n"));
            try
            {
                Ref<SubscriptionRequest> request(new SubscriptionRequest((struct Upnp_Subscription_Request *)event));
                upnp_subscriptions(request);
            }
            catch(UpnpException upnp_e)
            {
                ret = upnp_e.getErrorCode();
            }
 
            break;

        default:
            // unhandled event type
            log_info(("upnp_callback: unsupported event type: %d\n", eventtype));
            ret = UPNP_E_BAD_REQUEST;
            break;
    }

    // release device wide mutex
    pthread_mutex_unlock(&upnp_mutex);

//    log_info(("upnp_callback: returning %d\n", ret));
//    log_info(("upnp_callback: end\n"));
    return ret;
}

UpnpDevice_Handle Server::getDeviceHandle()
{
    return device_handle; 
}

zmm::String Server::getIP()
{
    return String(UpnpGetServerIpAddress());
}

zmm::String Server::getPort()
{
    return String::from(UpnpGetServerPort());
}

void Server::upnp_actions(Ref<ActionRequest> request)
{
//    log_info(("upnp_actions: start\n"));

    // make sure the request is for our device
    if (request->getUDN() != serverUDN)
    {
        // not for us
        throw UpnpException(UPNP_E_BAD_REQUEST, 
                            _("upnp_actions: request not for this device"));
    }

    // we need to match the serviceID to one of our services
    if (request->getServiceID() == DESC_CM_SERVICE_ID)
    {
        // this call is for the lifetime stats service
//        log_info(("upnp_actions: request for connection manager service\n"));
        cmgr->process_action_request(request);
    } 
    else if (request->getServiceID() == DESC_CDS_SERVICE_ID) 
    {
        // this call is for the toaster control service
//        log_info(("upnp_actions: request for content directory service\n"));
        cds->process_action_request(request);
    } 
    else 
    {
        // cp is asking for a nonexistent service, or for a service
        // that does not support any actions
        throw UpnpException(UPNP_E_BAD_REQUEST, 
                            _("Service does not exist or action not supported"));
    }
}


void Server::upnp_subscriptions(Ref<SubscriptionRequest> request)
{
    // make sure that the request is for our device
    if (request->getUDN() != serverUDN)
    {
        // not for us
//        log_info(("upnp_subscriptions: request not for this device\n"));
        throw UpnpException(UPNP_E_BAD_REQUEST,
                            _("upnp_actions: request not for this device"));
    }
                                                             
    // we need to match the serviceID to one of our services

    if (request->getServiceID() == DESC_CDS_SERVICE_ID)
    {
        // this call is for the content directory service
//        log_info(("upnp_subscriptions: request for content directory service\n"));
        cds->process_subscription_request(request);
    }
    else if (request->getServiceID() == DESC_CM_SERVICE_ID)
    {
        // this call is for the connection manager service
//        log_info(("upnp_subscriptions: request for connection manager service\n"));
        cmgr->process_subscription_request(request);
    } else {
        // cp asks for a nonexistent service or for a service that
        // does not support subscriptions
        throw UpnpException(UPNP_E_BAD_REQUEST, 
                            _("Service does not exist or subscriptions not supported"));
    }
}


