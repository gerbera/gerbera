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
#include <iostream>
#endif

#ifdef HAVE_LASTFMLIB
#include "lastfm_scrobbler.h"
#endif

#include "content_manager.h"
#include "file_request_handler.h"
#include "server.h"
#include "update_manager.h"
#ifdef HAVE_CURL
#include "url_request_handler.h"
#endif
#include "serve_request_handler.h"
#include "web/pages.h"

using namespace zmm;
using namespace mxml;

Ref<Storage> Server::storage = nullptr;

static int static_upnp_callback(Upnp_EventType eventtype, const void* event, void* cookie)
{
    return static_cast<Server*>(cookie)->upnp_callback(eventtype, event);
}

void Server::static_cleanup_callback()
{
    if (storage != nullptr) {
        try {
            storage->threadCleanup();
        } catch (const Exception& ex) {
        }
    }
}

Server::Server()
{
    server_shutdown_flag = false;
}

void Server::init()
{
    virtual_directory = _(SERVER_VIRTUAL_DIR);

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

void Server::upnp_init()
{
    int ret = 0; // general purpose error code
    log_debug("start\n");

    Ref<ConfigManager> config = ConfigManager::getInstance();

    String iface = config->getOption(CFG_SERVER_NETWORK_INTERFACE);
    String ip = config->getOption(CFG_SERVER_IP);

    if (string_ok(ip) && string_ok(iface))
        throw _Exception(_("You can not specify interface and IP at the same time!"));

    if (!string_ok(iface))
        iface = ipToInterface(ip);

    if (string_ok(ip) && !string_ok(iface))
        throw _Exception(_("Could not find ip: ") + ip);

    int port = config->getIntOption(CFG_SERVER_PORT);

    // this is important, so the storage lives a little longer when
    // shutdown is initiated
    // FIMXE: why?
    storage = Storage::getInstance();

    log_debug("Initialising libupnp with interface: %s, port: %d\n", iface.c_str(), port);
    ret = UpnpInit2(iface.c_str(), port);
    if (ret != UPNP_E_SUCCESS) {
        throw _UpnpException(ret, _("upnp_init: UpnpInit failed"));
    }

    port = UpnpGetServerPort();
    log_info("Initialized port: %d\n", port);

    if (!string_ok(ip)) {
        ip = UpnpGetServerIpAddress();
    }

    log_info("Server bound to: %s\n", ip.c_str());

    virtual_url = _("http://") + ip + ":" + port + "/" + virtual_directory;

    // next set webroot directory
    String web_root = config->getOption(CFG_SERVER_WEBROOT);

    if (!string_ok(web_root)) {
        throw _Exception(_("invalid web server root directory"));
    }

    ret = UpnpSetWebServerRootDir(web_root.c_str());
    if (ret != UPNP_E_SUCCESS) {
        throw _UpnpException(ret, _("upnp_init: UpnpSetWebServerRootDir failed"));
    }

    log_debug("webroot: %s\n", web_root.c_str());

    Ref<Array<StringBase>> arr = config->getStringArrayOption(CFG_SERVER_CUSTOM_HTTP_HEADERS);

    if (arr != nullptr) {
        String tmp;
        for (int i = 0; i < arr->size(); i++) {
            tmp = arr->get(i);
            if (string_ok(tmp)) {
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

    log_debug("Setting virtual dir to: %s\n", virtual_directory.c_str());
    ret = UpnpAddVirtualDir(virtual_directory.c_str(), this, nullptr);
    if (ret != UPNP_E_SUCCESS) {
        throw _UpnpException(ret, _("upnp_init: UpnpAddVirtualDir failed"));
    }

    ret = registerVirtualDirCallbacks();

    if (ret != UPNP_E_SUCCESS) {
        throw _UpnpException(ret, _("upnp_init: UpnpSetVirtualDirCallbacks failed"));
    }

    String presentationURL = config->getOption(CFG_SERVER_PRESENTATION_URL);
    if (!string_ok(presentationURL)) {
        presentationURL = _("http://") + ip + ":" + port + "/";
    } else {
        String appendto = config->getOption(CFG_SERVER_APPEND_PRESENTATION_URL_TO);
        if (appendto == "ip") {
            presentationURL = _("http://") + ip + ":" + presentationURL;
        } else if (appendto == "port") {
            presentationURL = _("http://") + ip + ":" + port + "/" + presentationURL;
        } // else appendto is none and we take the URL as it entered by user
    }

    log_debug("Creating UpnpXMLBuilder\n");
    xmlbuilder = std::make_unique<UpnpXMLBuilder>(virtual_url);

    // register root device with the library
    String device_description = _("<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n") + xmlbuilder->renderDeviceDescription(presentationURL)->print();

    //log_debug("Device Description: \n%s\n", device_description.c_str());

    log_debug("Registering with UPnP...\n");
    ret = UpnpRegisterRootDevice2(UPNPREG_BUF_DESC,
        device_description.c_str(),
        device_description.length() + 1,
        true,
        static_upnp_callback,
        this,
        &deviceHandle);
    if (ret != UPNP_E_SUCCESS) {
        throw _UpnpException(ret, _("upnp_init: UpnpRegisterRootDevice failed"));
    }

    log_debug("Creating ContentDirectoryService\n");
    cds = std::make_unique<ContentDirectoryService>(xmlbuilder.get(), deviceHandle,
        ConfigManager::getInstance()->getIntOption(CFG_SERVER_UPNP_TITLE_AND_DESC_STRING_LIMIT));

    log_debug("Creating ConnectionManagerService\n");
    cmgr = std::make_unique<ConnectionManagerService>(xmlbuilder.get(), deviceHandle);

    log_debug("Creating MRRegistrarService\n");
    mrreg = std::make_unique<MRRegistrarService>(xmlbuilder.get(), deviceHandle);

    log_debug("Sending UPnP Alive advertisements\n");
    ret = UpnpSendAdvertisement(deviceHandle, alive_advertisement);
    if (ret != UPNP_E_SUCCESS) {
        throw _UpnpException(ret, _("upnp_init: UpnpSendAdvertisement failed"));
    }

    // initializing UpdateManager
    UpdateManager::getInstance();

    // initializing ContentManager
    ContentManager::getInstance();

    config->writeBookmark(ip, String::from(port));
    log_info("The Web UI can be reached by following this link: http://%s:%d/\n", ip.c_str(), port);

    log_debug("end\n");
}

bool Server::getShutdownStatus() const
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

    log_debug("Server shutting down\n");

    ret = UpnpUnRegisterRootDevice(deviceHandle);
    if (ret != UPNP_E_SUCCESS) {
        throw _UpnpException(ret, _("upnp_cleanup: UpnpUnRegisterRootDevice failed"));
    }

#ifdef HAVE_CURL
    curl_global_cleanup();
#endif

    log_debug("now calling upnp finish\n");
    UpnpFinish();
    if (storage != nullptr && storage->threadCleanupRequired()) {
        static_cleanup_callback();
    }
    storage = nullptr;
}

String Server::getVirtualURL() const
{
    return virtual_url;
}

int Server::upnp_callback(Upnp_EventType eventtype, const void* event)
{
    int ret = UPNP_E_SUCCESS; // general purpose return code

    log_debug("start\n");

    // check parameters
    if (event == nullptr) {
        log_debug("upnp_callback: NULL event structure\n");
        return UPNP_E_BAD_REQUEST;
    }

    // dispatch event based on event type
    switch (eventtype) {

    case UPNP_CONTROL_ACTION_REQUEST:
        // a CP is invoking an action
        log_debug("UPNP_CONTROL_ACTION_REQUEST\n");
        try {
            Ref<ActionRequest> request(new ActionRequest((UpnpActionRequest*)event));
            routeActionRequest(request);
            request->update();
            // set in update() ((struct Upnp_Action_Request *)event)->ErrCode = ret;
        } catch (const UpnpException& upnp_e) {
            ret = upnp_e.getErrorCode();
            UpnpActionRequest_set_ErrCode((UpnpActionRequest*)event, ret);
        } catch (const Exception& e) {
            log_info("Exception: %s\n", e.getMessage().c_str());
        }

        break;

    case UPNP_EVENT_SUBSCRIPTION_REQUEST:
        // a cp wants a subscription
        log_debug("UPNP_EVENT_SUBSCRIPTION_REQUEST\n");
        try {
            Ref<SubscriptionRequest> request(new SubscriptionRequest((UpnpSubscriptionRequest*)event));
            routeSubscriptionRequest(request);
        } catch (const UpnpException& upnp_e) {
            log_warning("Subscription exception: %s\n", upnp_e.getMessage().c_str());
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

zmm::String Server::getIP() const
{
    return UpnpGetServerIpAddress();
}

zmm::String Server::getPort() const
{
    return String::from(UpnpGetServerPort());
}

void Server::routeActionRequest(Ref<ActionRequest> request)
{
    log_debug("start\n");

    // make sure the request is for our device
    if (request->getUDN() != serverUDN) {
        // not for us
        throw _UpnpException(UPNP_E_BAD_REQUEST,
            _("routeActionRequest: request not for this device"));
    }

    // we need to match the serviceID to one of our services
    if (request->getServiceID() == DESC_CM_SERVICE_ID) {
        // this call is for the lifetime stats service
        // log_debug("request for connection manager service\n");
        cmgr->process_action_request(request);
    } else if (request->getServiceID() == DESC_CDS_SERVICE_ID) {
        // this call is for the toaster control service
        //log_debug("routeActionRequest: request for content directory service\n");
        cds->process_action_request(request);
    } else if (request->getServiceID() == DESC_MRREG_SERVICE_ID) {
        mrreg->process_action_request(request);
    } else {
        // cp is asking for a nonexistent service, or for a service
        // that does not support any actions
        throw _UpnpException(UPNP_E_BAD_REQUEST,
            _("Service does not exist or action not supported"));
    }
}

void Server::routeSubscriptionRequest(Ref<SubscriptionRequest> request)
{
    // make sure that the request is for our device
    if (request->getUDN() != serverUDN) {
        // not for us
        log_debug("routeSubscriptionRequest: request not for this device: %s vs %s\n",
            request->getUDN().c_str(), serverUDN.c_str());
        throw _UpnpException(UPNP_E_BAD_REQUEST,
            _("routeActionRequest: request not for this device"));
    }

    // we need to match the serviceID to one of our services

    if (request->getServiceID() == DESC_CDS_SERVICE_ID) {
        // this call is for the content directory service
        //log_debug("routeSubscriptionRequest: request for content directory service\n");
        cds->process_subscription_request(request);
    } else if (request->getServiceID() == DESC_CM_SERVICE_ID) {
        // this call is for the connection manager service
        //log_debug("routeSubscriptionRequest: request for connection manager service\n");
        cmgr->process_subscription_request(request);
    } else if (request->getServiceID() == DESC_MRREG_SERVICE_ID) {
        mrreg->process_subscription_request(request);
    } else {
        // cp asks for a nonexistent service or for a service that
        // does not support subscriptions
        throw _UpnpException(UPNP_E_BAD_REQUEST,
            _("Service does not exist or subscriptions not supported"));
    }
}

// Temp
void Server::send_subscription_update(zmm::String updateString)
{
    cmgr->subscription_update(updateString);
}

Ref<RequestHandler> Server::createRequestHandler(const char *filename)
{
    String path;
    String parameters;
    String link = url_unescape((char*)filename);

    log_debug("Filename: %s, Path: %s\n", filename, path.c_str());
    // log_debug("create_handler: got url parameters: [%s]\n", parameters.c_str());

    RequestHandler* ret = nullptr;

    if (link.startsWith(_("/") + SERVER_VIRTUAL_DIR + "/" + CONTENT_MEDIA_HANDLER)) {
        ret = new FileRequestHandler(xmlbuilder.get());
    } else if (link.startsWith(_("/") + SERVER_VIRTUAL_DIR + "/" + CONTENT_UI_HANDLER)) {
        RequestHandler::split_url(filename, URL_UI_PARAM_SEPARATOR, path, parameters);

        Ref<Dictionary> dict(new Dictionary());
        dict->decode(parameters);

        String r_type = dict->get(_(URL_REQUEST_TYPE));
        if (r_type != nullptr) {
            ret = create_web_request_handler(r_type);
        } else {
            ret = create_web_request_handler(_("index"));
        }
    } else if (link.startsWith(_("/") + SERVER_VIRTUAL_DIR + "/" + CONTENT_SERVE_HANDLER)) {
        if (string_ok(ConfigManager::getInstance()->getOption(CFG_SERVER_SERVEDIR)))
            ret = new ServeRequestHandler();
        else
            throw _Exception(_("Serving directories is not enabled in configuration"));
    }

#if defined(HAVE_CURL)
    else if (link.startsWith(_("/") + SERVER_VIRTUAL_DIR + "/" + CONTENT_ONLINE_HANDLER)) {
        ret = new URLRequestHandler();
    }
#endif
    else {
        throw _Exception(_("no valid handler type in ") + filename);
    }
    return Ref<RequestHandler>(ret);
}

int Server::registerVirtualDirCallbacks()
{
    log_debug("Setting UpnpVirtualDir GetInfoCallback\n");
    int ret = UpnpVirtualDir_set_GetInfoCallback([](IN const char* filename, OUT UpnpFileInfo* info, const void* cookie) -> int {
        try {
            Ref<RequestHandler> reqHandler = const_cast<Server *>(static_cast<const Server *>(cookie))->createRequestHandler(
                    filename);
            reqHandler->get_info(filename, info);
        } catch (const ServerShutdownException& se) {
            return -1;
        } catch (const SubtitlesNotFoundException& sex) {
            log_info("%s\n", sex.getMessage().c_str());
            return -1;
        } catch (const Exception& e) {
            log_error("%s\n", e.getMessage().c_str());
            return -1;
        }
        return 0; });
    if (ret != 0)
        return ret;

    log_debug("Setting UpnpVirtualDir OpenCallback\n");
    ret = UpnpVirtualDir_set_OpenCallback([](IN const char* filename, IN enum UpnpOpenFileMode mode, IN const void* cookie) -> UpnpWebFileHandle {
        String link = url_unescape((char*)filename);

        try {
            Ref<RequestHandler> reqHandler = const_cast<Server *>(static_cast<const Server *>(cookie))->createRequestHandler(
                    filename);
            Ref<IOHandler> ioHandler = reqHandler->open(link.c_str(), mode, nullptr);
            ioHandler->retain();
            //log_debug("%p open(%s)\n", ioHandler.getPtr(), filename);
            return (UpnpWebFileHandle)ioHandler.getPtr();
        } catch (const ServerShutdownException& se) {
            return nullptr;
        } catch (const SubtitlesNotFoundException& sex) {
            log_info("SubtitlesNotFoundException: %s\n", sex.getMessage().c_str());
            return nullptr;
        } catch (const Exception& ex) {
            log_error("Exception: %s\n", ex.getMessage().c_str());
            return nullptr;
        }
    });
    if (ret != UPNP_E_SUCCESS)
        return ret;

    log_debug("Setting UpnpVirtualDir ReadCallback\n");
    ret = UpnpVirtualDir_set_ReadCallback([](IN UpnpWebFileHandle f, OUT char* buf, IN size_t length, IN const void* cookie) -> int {
        //log_debug("%p read(%d)\n", f, length);
        if (static_cast<const Server*>(cookie)->getShutdownStatus())
            return -1;

        auto* handler = (IOHandler*)f;
        return handler->read(buf, length);
    });
    if (ret != UPNP_E_SUCCESS)
        return ret;

    log_debug("Setting UpnpVirtualDir WriteCallback\n");
    ret = UpnpVirtualDir_set_WriteCallback([](IN UpnpWebFileHandle f, IN char* buf, IN size_t length, IN const void* cookie) -> int {
        //log_debug("%p write(%d)\n", f, length);
        return 0;
    });
    if (ret != UPNP_E_SUCCESS)
        return ret;

    log_debug("Setting UpnpVirtualDir SeekCallback\n");
    ret = UpnpVirtualDir_set_SeekCallback([](IN UpnpWebFileHandle f, IN off_t offset, IN int whence, IN const void* cookie) -> int {
        //log_debug("%p seek(%d, %d)\n", f, offset, whence);
        try {
            auto* handler = (IOHandler*)f;
            handler->seek(offset, whence);
        } catch (const Exception& e) {
            log_error("web_seek(): Exception during seek: %s\n", e.getMessage().c_str());
            e.printStackTrace();
            return -1;
        }

        return 0;
    });
    if (ret != UPNP_E_SUCCESS)
        return ret;

    log_debug("Setting UpnpVirtualDir CloseCallback\n");
    UpnpVirtualDir_set_CloseCallback([](IN UpnpWebFileHandle f, IN const void* cookie) -> int {
        //log_debug("%p close()\n", f);
        Ref<IOHandler> handler((IOHandler*)f);
        handler->release();
        try {
            handler->close();
        } catch (const Exception& e) {
            log_error("web_close(): Exception during close: %s\n", e.getMessage().c_str());
            e.printStackTrace();
            return -1;
        }
        return 0;
    });

    return ret;
}
