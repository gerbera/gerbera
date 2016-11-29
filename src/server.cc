/*MT*
    
    MediaTomb - http://www.mediatomb.cc/
    
    server.cc - this file is part of MediaTomb.
    
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

/// \file server.cc

#ifdef HAVE_CURL
    #include <curl/curl.h>
#endif

#ifdef HAVE_LASTFMLIB
    #include "lastfm_scrobbler.h"
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

Ref<Storage> Server::storage = nil;

SINGLETON_MUTEX(Server, false);

static int static_upnp_callback(Upnp_EventType eventtype, void const *event, void *cookie)
{
    return Server::getInstance()->upnp_callback(eventtype, event, cookie);
}

void Server::static_cleanup_callback()
{
    if (storage != nil)
    {
        try
        {
            storage->threadCleanup();
        }
        catch (Exception ex) {}
    }
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
    
    serverUDN = config->getOption(CFG_SERVER_UDN);
    alive_advertisement = config->getIntOption(CFG_SERVER_ALIVE_INTERVAL);

#ifdef HAVE_CURL
    curl_global_init(CURL_GLOBAL_ALL);
#endif

#ifdef HAVE_LASTFMLIB
    LastFm::getInstance();
#endif
}

void Server::upnp_init(String iface, String ip_address, int port)
{
    int ret = 0;        // general purpose error code
    String ip;

    log_debug("start\n");

    Ref<ConfigManager> config = ConfigManager::getInstance();

    if (!string_ok(iface))
        iface = config->getOption(CFG_SERVER_NETWORK_INTERFACE);

    if (!string_ok(ip_address))
        ip = config->getOption(CFG_SERVER_IP);
    else
        ip = ip_address;

    if (string_ok(ip) && string_ok(iface))
        throw _Exception(_("You can not specify interface and IP at the same time!"));


    if (!string_ok(ip))
        ip = interfaceToIP(iface);

    if (string_ok(iface) && !string_ok(ip))
        throw _Exception(_("Could not find interface: ") + iface);
/*
    // without this lod_debug coredumped on Solaris...
    if (iface == nil)
        iface = _("");

    if (ip == nil)
        ip = _("");
    
    log_debug("interface: %s ip: %s\n", iface.c_str(), ip.c_str());

    if (ip == "")
        ip = nil;
*/
    if (port < 0)
    {
        port = config->getIntOption(CFG_SERVER_PORT);
    }

    if (port < 0)
        port = 0;


    void *cb = NULL;
    // this is important, so the storage lives a little longer when
    // shutdown is initiated
    storage = Storage::getInstance();

    if (storage->threadCleanupRequired())
        cb = (void *)static_cleanup_callback;

    ret = UpnpInit(ip.c_str(), port);

    if (ret != UPNP_E_SUCCESS)
    {
        throw _UpnpException(ret, _("upnp_init: UpnpInit failed"));
    }

    port = UpnpGetServerPort();

    log_info("Initialized port: %d\n", port);

    if (!string_ok(ip))
    {
        ip = UpnpGetServerIpAddress();
    }

    log_info("Server bound to: %s\n", ip.c_str());

    virtual_url = _("http://") + ip + ":" + port + "/" + virtual_directory;

    // next set webroot directory
    String web_root = config->getOption(CFG_SERVER_WEBROOT);

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

        Ref<Array<StringBase> > arr = config->getStringArrayOption(CFG_SERVER_CUSTOM_HTTP_HEADERS);

        if (arr != nil)
        {
            String tmp;
            for (int i = 0; i < arr->size(); i++)
            {
                tmp = arr->get(i);
                if (string_ok(tmp))
                {
                    log_info("(NOT) Adding HTTP header \"%s\"\n", tmp.c_str());
                    // FIXME upstream upnp
                    //ret = UpnpAddCustomHTTPHeader(tmp.c_str());
                    //if (ret != UPNP_E_SUCCESS)
                    //{
                    //    throw _UpnpException(ret, _("upnp_init: UpnpAddCustomHTTPHeader failed"));
                    //}
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

    String presentationURL =  config->getOption(CFG_SERVER_PRESENTATION_URL);
    if (!string_ok(presentationURL))
    {
        presentationURL = _("http://") + ip + ":" + port + "/";
    }
    else
    {
        String appendto = 
           config->getOption(CFG_SERVER_APPEND_PRESENTATION_URL_TO);
        if (appendto == "ip")
        {
            presentationURL = _("http://") + ip + ":" + presentationURL;
        }
        else if (appendto == "port")
        {
            presentationURL = _("http://") + ip + ":" + port + "/" + 
                              presentationURL;
        } // else appendto is none and we take the URL as it entered by user
    }

    // register root device with the library
    String device_description = 
        _("<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n") +
         UpnpXML_RenderDeviceDescription(presentationURL)->print();
         
    //log_debug("DEVICE DESCRIPTION: \n%s\n", device_description.c_str());

    // register root device with the library
    ret = UpnpRegisterRootDevice2(UPNPREG_BUF_DESC,
                                  device_description.c_str(),
                                  device_description.length() + 1,
                                  true,
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
    ret = UpnpRegisterRootDevice2(UPNPREG_BUF_DESC,
                                  device_description.c_str(),
                                  device_description.length() + 1,
                                  true,
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
    if (ret != UPNP_E_SUCCESS) 
    {   
        throw _UpnpException(ret, _("upnp_cleanup: UpnpUnRegisterRootDevice failed"));
    }

#ifdef HAVE_CURL
    curl_global_cleanup();
#endif

    log_debug("now calling upnp finish\n");
    UpnpFinish();
    storage = nil;

    log_debug("end\n");
}

String Server::getVirtualURL()
{
    return virtual_url;
}


int Server::upnp_callback(Upnp_EventType eventtype, const void *event, void *cookie)
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
                // https://github.com/mrjimenez/pupnp/blob/master/upnp/sample/common/tv_device.c

                Ref<ActionRequest> request(new ActionRequest((UpnpActionRequest *)event));
                upnp_actions(request);
                request->update();
               // set in update() ((struct Upnp_Action_Request *)event)->ErrCode = ret;
            }
            catch(UpnpException upnp_e)
            {
                ret = upnp_e.getErrorCode();
                UpnpActionRequest_set_ErrCode((UpnpActionRequest *)event, ret);
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
                Ref<SubscriptionRequest> request(new SubscriptionRequest((UpnpSubscriptionRequest *)event));
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
    return UpnpGetServerIpAddress();
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
