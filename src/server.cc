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
#include "device_description_handler.h"
#include "serve_request_handler.h"
#include "web/pages.h"

using namespace zmm;
using namespace mxml;

Ref<Storage> Server::storage = nullptr;

static int static_upnp_callback(Upnp_EventType eventtype, const void* event, void* cookie)
{
    return static_cast<Server*>(cookie)->handleUpnpEvent(eventtype, event);
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
    virtual_directory = SERVER_VIRTUAL_DIR;

    Ref<ConfigManager> config = ConfigManager::getInstance();

    serverUDN = config->getOption(CFG_SERVER_UDN);
    aliveAdvertisementInterval = config->getIntOption(CFG_SERVER_ALIVE_INTERVAL);

#ifdef HAVE_CURL
    curl_global_init(CURL_GLOBAL_ALL);
#endif

#ifdef HAVE_LASTFMLIB
    LastFm::getInstance();
#endif
}

void Server::run()
{
    int ret = 0; // general purpose error code
    log_debug("start\n");

    Ref<ConfigManager> config = ConfigManager::getInstance();

    std::string iface = config->getOption(CFG_SERVER_NETWORK_INTERFACE);
    std::string ip = config->getOption(CFG_SERVER_IP);

    if (string_ok(ip) && string_ok(iface))
        throw _Exception("You can not specify interface and IP at the same time!");

    if (!string_ok(iface))
        iface = ipToInterface(ip);

    if (string_ok(ip) && !string_ok(iface))
        throw _Exception("Could not find ip: " + ip);

    int port = config->getIntOption(CFG_SERVER_PORT);

    // this is important, so the storage lives a little longer when
    // shutdown is initiated
    // FIMXE: why?
    storage = Storage::getInstance();

    log_debug("Initialising libupnp with interface: '%s', port: %d\n", iface.c_str(), port);
    const char* IfName = NULL;
    if (!iface.empty()) IfName = iface.c_str();
    ret = UpnpInit2(IfName, port);
    if (ret != UPNP_E_SUCCESS) {
        throw _UpnpException(ret, "run: UpnpInit failed");
    }

    port = UpnpGetServerPort();
    log_info("Initialized port: %d\n", port);

    if (!string_ok(ip)) {
        ip = UpnpGetServerIpAddress();
    }

    log_info("Server bound to: %s\n", ip.c_str());

    virtualUrl = "http://" + ip + ":" + std::to_string(port) + "/" + virtual_directory;

    // next set webroot directory
    std::string web_root = config->getOption(CFG_SERVER_WEBROOT);

    if (!string_ok(web_root)) {
        throw _Exception("invalid web server root directory");
    }

    ret = UpnpSetWebServerRootDir(web_root.c_str());
    if (ret != UPNP_E_SUCCESS) {
        throw _UpnpException(ret, "run: UpnpSetWebServerRootDir failed");
    }

    log_debug("webroot: %s\n", web_root.c_str());

    std::vector<std::string> arr = config->getStringArrayOption(CFG_SERVER_CUSTOM_HTTP_HEADERS);
    for (size_t i = 0; i < arr.size(); i++) {
        std::string tmp = arr[i];
        if (string_ok(tmp)) {
            log_info("(NOT) Adding HTTP header \"%s\"\n", tmp.c_str());
            // FIXME upstream upnp
            //ret = UpnpAddCustomHTTPHeader(tmp.c_str());
            //if (ret != UPNP_E_SUCCESS)
            //{
            //    throw _UpnpException(ret, "run: UpnpAddCustomHTTPHeader failed");
            //}
        }
    }

    log_debug("Setting virtual dir to: %s\n", virtual_directory.c_str());
    ret = UpnpAddVirtualDir(virtual_directory.c_str(), this, nullptr);
    if (ret != UPNP_E_SUCCESS) {
        throw _UpnpException(ret, "run: UpnpAddVirtualDir failed");
    }

    ret = registerVirtualDirCallbacks();

    if (ret != UPNP_E_SUCCESS) {
        throw _UpnpException(ret, "run: UpnpSetVirtualDirCallbacks failed");
    }

    std::string presentationURL = config->getOption(CFG_SERVER_PRESENTATION_URL);
    if (!string_ok(presentationURL)) {
        presentationURL = "http://" + ip + ":" + std::to_string(port) + "/";
    } else {
        std::string appendto = config->getOption(CFG_SERVER_APPEND_PRESENTATION_URL_TO);
        if (appendto == "ip") {
            presentationURL = "http://" + ip + ":" + presentationURL;
        } else if (appendto == "port") {
            presentationURL = "http://" + ip + ":" + std::to_string(port) + "/" + presentationURL;
        } // else appendto is none and we take the URL as it entered by user
    }

    log_debug("Creating UpnpXMLBuilder\n");
    xmlbuilder = std::make_unique<UpnpXMLBuilder>(virtualUrl, presentationURL);

    // register root device with the library
    std::string deviceDescription = "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n" + xmlbuilder->renderDeviceDescription()->print();
    //log_debug("Device Description: \n%s\n", deviceDescription.c_str());

    log_debug("Registering with UPnP...\n");
    ret = UpnpRegisterRootDevice2(UPNPREG_BUF_DESC,
        deviceDescription.c_str(),
        (size_t)deviceDescription.length() + 1,
        true,
        static_upnp_callback,
        this,
        &deviceHandle);

    if (ret != UPNP_E_SUCCESS) {
        throw _UpnpException(ret, "run: UpnpRegisterRootDevice failed");
    }

    log_debug("Creating ContentDirectoryService\n");
    cds = std::make_unique<ContentDirectoryService>(xmlbuilder.get(), deviceHandle,
        ConfigManager::getInstance()->getIntOption(CFG_SERVER_UPNP_TITLE_AND_DESC_STRING_LIMIT));

    log_debug("Creating ConnectionManagerService\n");
    cmgr = std::make_unique<ConnectionManagerService>(xmlbuilder.get(), deviceHandle);

    log_debug("Creating MRRegistrarService\n");
    mrreg = std::make_unique<MRRegistrarService>(xmlbuilder.get(), deviceHandle);

    // The advertisement will be sent by LibUPnP every (A/2)-30 seconds, and will have a cache-control max-age of A where A is
    // the value configured here. Ex: A value of 62 will result in an SSDP advertisement being sent every second.
    log_debug("Sending UPnP Alive advertisements every %d seconds\n", (aliveAdvertisementInterval / 2) - 30);
    ret = UpnpSendAdvertisement(deviceHandle, aliveAdvertisementInterval);
    if (ret != UPNP_E_SUCCESS) {
        throw _UpnpException(ret, "run: UpnpSendAdvertisement failed");
    }

    // initializing UpdateManager
    UpdateManager::getInstance();

    // initializing ContentManager
    ContentManager::getInstance();

    config->writeBookmark(ip, std::to_string(port));
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

    ConfigManager::getInstance()->emptyBookmark();
    server_shutdown_flag = true;

    log_debug("Server shutting down\n");

    ret = UpnpUnRegisterRootDevice(deviceHandle);
    if (ret != UPNP_E_SUCCESS) {
        throw _UpnpException(ret, "upnp_cleanup: UpnpUnRegisterRootDevice failed");
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

int Server::handleUpnpEvent(Upnp_EventType eventtype, const void* event)
{
    int ret = UPNP_E_SUCCESS; // general purpose return code

    log_debug("start\n");

    // check parameters
    if (event == nullptr) {
        log_debug("handleUpnpEvent: NULL event structure\n");
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

std::string Server::getIP() const
{
    return UpnpGetServerIpAddress();
}

std::string Server::getPort() const
{
    return std::to_string(UpnpGetServerPort());
}

void Server::routeActionRequest(Ref<ActionRequest> request) const
{
    log_debug("start\n");

    // make sure the request is for our device
    if (request->getUDN() != serverUDN) {
        // not for us
        throw _UpnpException(UPNP_E_BAD_REQUEST, "routeActionRequest: request not for this device");
    }

    // we need to match the serviceID to one of our services
    if (request->getServiceID() == DESC_CM_SERVICE_ID) {
        // this call is for the lifetime stats service
        // log_debug("request for connection manager service\n");
        cmgr->processActionRequest(request);
    } else if (request->getServiceID() == DESC_CDS_SERVICE_ID) {
        // this call is for the toaster control service
        //log_debug("routeActionRequest: request for content directory service\n");
        cds->processActionRequest(request);
    } else if (request->getServiceID() == DESC_MRREG_SERVICE_ID) {
        mrreg->processActionRequest(request);
    } else {
        // cp is asking for a nonexistent service, or for a service
        // that does not support any actions
        throw _UpnpException(UPNP_E_BAD_REQUEST, "Service does not exist or action not supported");
    }
}

void Server::routeSubscriptionRequest(Ref<SubscriptionRequest> request) const
{
    // make sure that the request is for our device
    if (request->getUDN() != serverUDN) {
        // not for us
        log_debug("routeSubscriptionRequest: request not for this device: %s vs %s\n",
            request->getUDN().c_str(), serverUDN.c_str());
        throw _UpnpException(UPNP_E_BAD_REQUEST, "routeActionRequest: request not for this device");
    }

    // we need to match the serviceID to one of our services
    if (request->getServiceID() == DESC_CDS_SERVICE_ID) {
        // this call is for the content directory service
        //log_debug("routeSubscriptionRequest: request for content directory service\n");
        cds->processSubscriptionRequest(request);
    } else if (request->getServiceID() == DESC_CM_SERVICE_ID) {
        // this call is for the connection manager service
        //log_debug("routeSubscriptionRequest: request for connection manager service\n");
        cmgr->processSubscriptionRequest(request);
    } else if (request->getServiceID() == DESC_MRREG_SERVICE_ID) {
        mrreg->processSubscriptionRequest(request);
    } else {
        // cp asks for a nonexistent service or for a service that
        // does not support subscriptions
        throw _UpnpException(UPNP_E_BAD_REQUEST, "Service does not exist or subscriptions not supported");
    }
}

// Temp
void Server::sendCDSSubscriptionUpdate(std::string updateString)
{
    cds->sendSubscriptionUpdate(updateString);
}

Ref<RequestHandler> Server::createRequestHandler(const char* filename) const
{
    std::string link = urlUnescape(filename);
    log_debug("Filename: %s\n", filename);

    RequestHandler* ret = nullptr;

    if (startswith(link, std::string("/") + SERVER_VIRTUAL_DIR + "/" + CONTENT_MEDIA_HANDLER)) {
        ret = new FileRequestHandler(xmlbuilder.get());
    } else if (startswith(link, std::string("/") + SERVER_VIRTUAL_DIR + "/" + CONTENT_UI_HANDLER)) {
        std::string parameters;
        std::string path;
        RequestHandler::splitUrl(filename, URL_UI_PARAM_SEPARATOR, path, parameters);

        Ref<Dictionary> dict(new Dictionary());
        dict->decode(parameters);

        std::string r_type = dict->get(URL_REQUEST_TYPE);
        if (!r_type.empty()) {
            ret = createWebRequestHandler(r_type);
        } else {
            ret = createWebRequestHandler("index");
        }
    } else if (startswith(link, std::string("/") + SERVER_VIRTUAL_DIR + "/" + DEVICE_DESCRIPTION_PATH)) {
        ret = new DeviceDescriptionHandler(xmlbuilder.get());
    } else if (startswith(link, std::string("/") + SERVER_VIRTUAL_DIR + "/" + CONTENT_SERVE_HANDLER)) {
        if (string_ok(ConfigManager::getInstance()->getOption(CFG_SERVER_SERVEDIR)))
            ret = new ServeRequestHandler();
        else
            throw _Exception("Serving directories is not enabled in configuration");
    }

#if defined(HAVE_CURL)
    else if (startswith(link, std::string("/") + SERVER_VIRTUAL_DIR + "/" + CONTENT_ONLINE_HANDLER)) {
        ret = new URLRequestHandler();
    }
#endif
    else {
        throw _Exception(std::string("no valid handler type in ") + filename);
    }
    return Ref<RequestHandler>(ret);
}

int Server::registerVirtualDirCallbacks()
{
    log_debug("Setting UpnpVirtualDir GetInfoCallback\n");
#ifdef UPNP_HAS_REQUEST_COOKIES
    int ret = UpnpVirtualDir_set_GetInfoCallback([](const char* filename, UpnpFileInfo* info, const void* cookie, const void** requestCookie) -> int {
#else
    int ret = UpnpVirtualDir_set_GetInfoCallback([](const char* filename, UpnpFileInfo* info, const void* cookie) -> int {
#endif
        try {
            Ref<RequestHandler> reqHandler = static_cast<const Server *>(cookie)->createRequestHandler(filename);
            reqHandler->getInfo(filename, info);
        } catch (const ServerShutdownException& se) {
            return -1;
        } catch (const SubtitlesNotFoundException& sex) {
            log_warning("%s\n", sex.getMessage().c_str());
            return -1;
        } catch (const Exception& e) {
            log_error("%s\n", e.getMessage().c_str());
            return -1;
        }
        return 0; });
    if (ret != 0)
        return ret;

    log_debug("Setting UpnpVirtualDir OpenCallback\n");
#ifdef UPNP_HAS_REQUEST_COOKIES
    ret = UpnpVirtualDir_set_OpenCallback([](const char* filename, enum UpnpOpenFileMode mode, const void* cookie, const void* requestCookie) -> UpnpWebFileHandle {
#else
    ret = UpnpVirtualDir_set_OpenCallback([](const char* filename, enum UpnpOpenFileMode mode, const void* cookie) -> UpnpWebFileHandle {
#endif
        std::string link = urlUnescape(filename);

        try {
            Ref<RequestHandler> reqHandler = static_cast<const Server*>(cookie)->createRequestHandler(filename);
            Ref<IOHandler> ioHandler = reqHandler->open(link.c_str(), mode, "");
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
#ifdef UPNP_HAS_REQUEST_COOKIES
    ret = UpnpVirtualDir_set_ReadCallback([](UpnpWebFileHandle f, char* buf, size_t length, const void* cookie, const void* requestCookie) -> int {
#else
    ret = UpnpVirtualDir_set_ReadCallback([](UpnpWebFileHandle f, char* buf, size_t length, const void* cookie) -> int {
#endif
        //log_debug("%p read(%d)\n", f, length);
        if (static_cast<const Server*>(cookie)->getShutdownStatus())
            return -1;

        auto* handler = (IOHandler*)f;
        return handler->read(buf, length);
    });
    if (ret != UPNP_E_SUCCESS)
        return ret;

    log_debug("Setting UpnpVirtualDir WriteCallback\n");
#ifdef UPNP_HAS_REQUEST_COOKIES
    ret = UpnpVirtualDir_set_WriteCallback([](UpnpWebFileHandle f, char* buf, size_t length, const void* cookie, const void* requestCookie) -> int {
#else
    ret = UpnpVirtualDir_set_WriteCallback([](UpnpWebFileHandle f, char* buf, size_t length, const void* cookie) -> int {
#endif
        //log_debug("%p write(%d)\n", f, length);
        return 0;
    });
    if (ret != UPNP_E_SUCCESS)
        return ret;

    log_debug("Setting UpnpVirtualDir SeekCallback\n");
#ifdef UPNP_HAS_REQUEST_COOKIES
    ret = UpnpVirtualDir_set_SeekCallback([](UpnpWebFileHandle f, off_t offset, int whence, const void* cookie, const void* requestCookie) -> int {
#else
    ret = UpnpVirtualDir_set_SeekCallback([](UpnpWebFileHandle f, off_t offset, int whence, const void* cookie) -> int {
#endif
        //log_debug("%p seek(%d, %d)\n", f, offset, whence);
        try {
            auto* handler = static_cast<IOHandler*>(f);
            handler->seek(offset, whence);
        } catch (const Exception& e) {
            log_error("Exception during seek: %s\n", e.getMessage().c_str());
            e.printStackTrace();
            return -1;
        }

        return 0;
    });
    if (ret != UPNP_E_SUCCESS)
        return ret;

    log_debug("Setting UpnpVirtualDir CloseCallback\n");
#ifdef UPNP_HAS_REQUEST_COOKIES
    UpnpVirtualDir_set_CloseCallback([](UpnpWebFileHandle f, const void* cookie, const void* requestCookie) -> int {
#else
    UpnpVirtualDir_set_CloseCallback([](UpnpWebFileHandle f, const void* cookie) -> int {
#endif
        //log_debug("%p close()\n", f);
        Ref<IOHandler> handler((IOHandler*)f);
        handler->release();
        try {
            handler->close();
        } catch (const Exception& e) {
            log_error("Exception during close: %s\n", e.getMessage().c_str());
            e.printStackTrace();
            return -1;
        }
        return 0;
    });

    return ret;
}
