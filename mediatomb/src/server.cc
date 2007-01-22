/*MT*
    
    MediaTomb - http://www.mediatomb.org/
    
    server.cc - this file is part of MediaTomb.
    
    Copyright (C) 2005 Gena Batyan <bgeradz@mediatomb.org>,
                       Sergey 'Jin' Bostandzhyan <jin@mediatomb.org>
    
    Copyright (C) 2006-2007 Gena Batyan <bgeradz@mediatomb.org>,
                            Sergey 'Jin' Bostandzhyan <jin@mediatomb.org>,
                            Leonhard Wimmer <leo@mediatomb.org>
    
    MediaTomb is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License version 2
    as published by the Free Software Foundation.
    
    MediaTomb is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.
    
    You should have received a copy of the GNU General Public License
    version 2 along with MediaTomb; if not, write to the Free Software
    Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
    
    $Id$
*/

/// \file server.cc

#ifdef HAVE_CONFIG_H
    #include "autoconfig.h"
#endif

#include "server.h"
#include "web_callbacks.h"
#include "content_manager.h"
#include "update_manager.h"
#include "dictionary.h"
#include "upnp_xml.h"
#include "tools.h"

using namespace zmm;
using namespace mxml;

SINGLETON_MUTEX(Server, false);

static int static_upnp_callback(Upnp_EventType eventtype, void *event, void *cookie)
{
    return Server::getInstance()->upnp_callback(eventtype, event, cookie);
}

Server::Server() : Singleton<Server>()
{
    server_shutdown_flag = false;
}

void Server::init()
{
    virtual_directory = _(SERVER_VIRTUAL_DIR); 
    
    ContentDirectoryService::setStaticArgs(_(DESC_CDS_SERVICE_TYPE),
                                           _(DESC_CDS_SERVICE_ID));
    cds = ContentDirectoryService::getInstance();
                                                 
    ConnectionManagerService::setStaticArgs(_(DESC_CM_SERVICE_TYPE),
                                            _(DESC_CM_SERVICE_ID));
    cmgr = ConnectionManagerService::getInstance();

#if defined(ENABLE_MRREG)                                                    
    MRRegistrarService::setStaticArgs(_(DESC_MRREG_SERVICE_TYPE),
                                      _(DESC_MRREG_SERVICE_ID));
    mrreg = MRRegistrarService::getInstance();
#endif

    Ref<ConfigManager> config = ConfigManager::getInstance();
    
    serverUDN = config->getOption(_("/server/udn"));
    alive_advertisement = config->getIntOption(_("/server/alive"));
}

void Server::upnp_init(String ip, int port)
{
    int             ret = 0;        // general purpose error code

    log_debug("start\n");

    Ref<ConfigManager> config = ConfigManager::getInstance();

    if (ip == nil)
    {
        ip = config->getOption(_("/server/ip"), _(""));
        if (ip == "") 
            ip = nil;
        else
            log_info("got ip: %s\n", ip.c_str());
    }

    if (port < 0)
    {
        port = config->getIntOption(_("/server/port"));
    }

    if (port < 0)
        port = 0;
    
    ret = UpnpInit(ip.c_str(), port);

    if (ret != UPNP_E_SUCCESS)
    {
        throw _UpnpException(ret, _("upnp_init: UpnpInit failed"));
    }

    port = UpnpGetServerPort();

    log_info("Initialized port: %d\n", port);

    if (!string_ok(ip))
    {
        ip = String(UpnpGetServerIpAddress());
    }

    log_info("Server bound to: %s\n", ip.c_str());

    virtual_url = _("http://") + ip + ":" + port + "/" + virtual_directory;

    /// \todo who should construct absolute paths??? config_manage or the modules?
    // next set webroot directory
    String web_root = config->getOption(_("/server/webroot"));

    if (!string_ok(web_root))
    {
        throw _Exception(_("invalid web server root directory"));
    }
    
    ret = UpnpSetWebServerRootDir(web_root.c_str());
    if (ret != UPNP_E_SUCCESS)
    {
        throw _UpnpException(ret, _("upnp_init: UpnpSetWebServerRootDir failed"));
    }

    log_debug("webroot: %s\n", web_root.c_str()); 

    Ref<Element> headers = config->getElement(_("/server/custom-http-headers"));
    if (headers != nil)
    {
        Ref<Array<StringBase> > arr = config->createArrayFromNodeset(headers, _("add"), _("header"));
        if (arr != nil)
        {
            String tmp;
            for (int i = 0; i < arr->size(); i++)
            {
                tmp = arr->get(i);
                if (string_ok(tmp))
                {
                    log_info("Adding HTTP header \"%s\"\n", tmp.c_str());
                    ret = UpnpAddCustomHTTPHeader(tmp.c_str());
                    if (ret != UPNP_E_SUCCESS)
                    {
                        throw _UpnpException(ret, _("upnp_init: UpnpAddCustomHTTPHeader failed"));
                    }
                }
            }
        }
    }

    ret = UpnpAddVirtualDir(virtual_directory.c_str());
    if (ret != UPNP_E_SUCCESS)
    {
        throw _UpnpException(ret, _("upnp_init: UpnpAddVirtualDir failed"));
    }

    ret = register_web_callbacks();
    if (ret != UPNP_E_SUCCESS)
    {
        throw _UpnpException(ret, _("upnp_init: UpnpSetVirtualDirCallbacks failed"));
    }

    String presentationURL =  config->getOption(_("/server/presentationURL"));
    if (!string_ok(presentationURL))
        presentationURL = _("http://") + ip + ":" + port + "/";

    // register root device with the library
    String device_description = 
        _("<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n") +
         UpnpXML_RenderDeviceDescription(presentationURL)->print();

//    log_debug("DEVICE DESCRIPTION: \n%s\n", device_description.c_str());

    // register root device with the library
    ret = UpnpRegisterRootDevice2(UPNPREG_BUF_DESC, device_description.c_str(), 
                                  device_description.length() + 1, true,
                                  static_upnp_callback,
                                  &device_handle,
                                  &device_handle);
    if (ret != UPNP_E_SUCCESS)
    {
        throw _UpnpException(ret, _("upnp_init: UpnpRegisterRootDevice failed"));
    }

    // now unregister in order to cleanup previous instances
    // that might still be there due to crash/unclean shutdown/network interruptions
    ret = UpnpUnRegisterRootDevice(device_handle);
    if (ret != UPNP_E_SUCCESS) {   
        throw _UpnpException(ret, _("upnp_init: unregistering failed"));
    }
    
    // and register again, we should be clean now
    ret = UpnpRegisterRootDevice2(UPNPREG_BUF_DESC, device_description.c_str(), 
                                  device_description.length() + 1, true,
                                  static_upnp_callback,
                                  &device_handle,
                                  &device_handle);
    if (ret != UPNP_E_SUCCESS)
    {
        throw _UpnpException(ret, _("upnp_init: UpnpRegisterRootDevice failed"));
    }


    // send advertisements every 180secs
    ret = UpnpSendAdvertisement(device_handle, alive_advertisement);
    if (ret != UPNP_E_SUCCESS)
    {
        throw _UpnpException(ret, _("upnp_init: UpnpSendAdvertisement failed"));
    }
    
    // initializing Storage
    Storage::getInstance();
    
    // initializing UpdateManager
    UpdateManager::getInstance();
    
    // initializing ContentManager
    ContentManager::getInstance();
    
    config->writeBookmark(ip, String::from(port));
    log_info("MediaTomb Web UI can be reached by following this link:\n");
    log_info("http://%s:%d/\n", ip.c_str(), port);
    
    log_debug("end\n");
}

bool Server::getShutdownStatus()
{
    return server_shutdown_flag;
}

void Server::shutdown()
{
    int ret = 0; // return code
    
    /*
    ContentManager::getInstance()->shutdown();
    UpdateManager::getInstance()->shutdown();
    Storage::getInstance()->shutdown();
    */
    
    server_shutdown_flag = true;
    
    log_debug("start\n");
    
    // unregister device
    
    ret = UpnpUnRegisterRootDevice(device_handle);
    if (ret != UPNP_E_SUCCESS) {   
        throw _UpnpException(ret, _("upnp_cleanup: UpnpUnRegisterRootDevice failed"));
    }
    
    log_debug("now calling upnp finish\n");
    UpnpFinish();
    
    log_debug("end\n");
}

String Server::getVirtualURL()
{
    return virtual_url;
}


int Server::upnp_callback(Upnp_EventType eventtype, void *event, void *cookie)
{
    int ret = UPNP_E_SUCCESS; // general purpose return code

    log_debug("start\n");

    // check parameters
    if (event == NULL) {
        log_debug("upnp_callback: NULL event structure\n");
        return UPNP_E_BAD_REQUEST;
    }

//    log_info("event is ok\n");
    // get device wide mutex (have to figure out what the hell that is)
    AUTOLOCK(mutex);

//    log_info("got device mutex\n");

    // dispatch event based on event type
    switch (eventtype) {

        case UPNP_CONTROL_ACTION_REQUEST:
            // a CP is invoking an action
//            log_info("UPNP_CONTROL_ACTION_REQUEST\n");
            try
            {
                Ref<ActionRequest> request(new ActionRequest((struct Upnp_Action_Request *)event));
                upnp_actions(request);
                request->update();
               // set in update() ((struct Upnp_Action_Request *)event)->ErrCode = ret;
            }
            catch(UpnpException upnp_e)
            {
                ret = upnp_e.getErrorCode();
                ((struct Upnp_Action_Request *)event)->ErrCode = ret;
            }
            catch(Exception e)
            {
                log_info("Exception: %s\n", e.getMessage().c_str());
            }
            
            break;
            
        case UPNP_EVENT_SUBSCRIPTION_REQUEST:
            // a cp wants a subscription
//            log_info("UPNP_EVENT_SUBSCRIPTION_REQUEST\n");
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
            log_warning("unsupported event type: %d\n", eventtype);
            ret = UPNP_E_BAD_REQUEST;
            break;
    }
    
    log_debug("returning %d\n", ret);
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
    log_debug("start\n");

    // make sure the request is for our device
    if (request->getUDN() != serverUDN)
    {
        // not for us
        throw _UpnpException(UPNP_E_BAD_REQUEST, 
                            _("upnp_actions: request not for this device"));
    }

    // we need to match the serviceID to one of our services
    if (request->getServiceID() == DESC_CM_SERVICE_ID)
    {
        // this call is for the lifetime stats service
//        log_debug("request for connection manager service\n");
        cmgr->process_action_request(request);
    } 
    else if (request->getServiceID() == DESC_CDS_SERVICE_ID) 
    {
        // this call is for the toaster control service
//        log_debug("upnp_actions: request for content directory service\n");
        cds->process_action_request(request);
    } 
#if defined(ENABLE_MRREG)
    else if (request->getServiceID() == DESC_MRREG_SERVICE_ID)
    {
        mrreg->process_action_request(request);
    }
#endif
    else 
    {
        // cp is asking for a nonexistent service, or for a service
        // that does not support any actions
        throw _UpnpException(UPNP_E_BAD_REQUEST, 
                            _("Service does not exist or action not supported"));
    }
}


void Server::upnp_subscriptions(Ref<SubscriptionRequest> request)
{
    // make sure that the request is for our device
    if (request->getUDN() != serverUDN)
    {
        // not for us
//        log_debug("upnp_subscriptions: request not for this device\n");
        throw _UpnpException(UPNP_E_BAD_REQUEST,
                            _("upnp_actions: request not for this device"));
    }
                                                             
    // we need to match the serviceID to one of our services

    if (request->getServiceID() == DESC_CDS_SERVICE_ID)
    {
        // this call is for the content directory service
//        log_debug("upnp_subscriptions: request for content directory service\n");
        cds->process_subscription_request(request);
    }
    else if (request->getServiceID() == DESC_CM_SERVICE_ID)
    {
        // this call is for the connection manager service
//        log_debug("upnp_subscriptions: request for connection manager service\n");
        cmgr->process_subscription_request(request);
    }
#if defined(ENABLE_MRREG)
    else if (request->getServiceID() == DESC_MRREG_SERVICE_ID)
    {
        mrreg->process_subscription_request(request);
    }
#endif
    else 
    {
        // cp asks for a nonexistent service or for a service that
        // does not support subscriptions
        throw _UpnpException(UPNP_E_BAD_REQUEST, 
                            _("Service does not exist or subscriptions not supported"));
    }
}


