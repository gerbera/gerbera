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
#include <utility>
#endif

#ifdef HAVE_LASTFMLIB
#include "onlineservice/lastfm_scrobbler.h"
#endif

#include "config/config_manager.h"
#include "content_manager.h"
#include "file_request_handler.h"
#include "server.h"
#include "storage/storage.h"
#include "update_manager.h"
#include "util/task_processor.h"
#include "web/session_manager.h"
#ifdef HAVE_JS
#include "scripting/runtime.h"
#endif
#ifdef HAVE_CURL
#include "url_request_handler.h"
#endif
#include "device_description_handler.h"
#include "serve_request_handler.h"
#include "web/pages.h"

static int static_upnp_callback(Upnp_EventType eventtype, const void* event, void* cookie)
{
    return static_cast<Server*>(cookie)->handleUpnpEvent(eventtype, event);
}

Server::Server(std::shared_ptr<ConfigManager> config)
    : config(std::move(config))
{
    server_shutdown_flag = false;
}

void Server::init()
{
    virtual_directory = SERVER_VIRTUAL_DIR;

    serverUDN = config->getOption(CFG_SERVER_UDN);
    aliveAdvertisementInterval = config->getIntOption(CFG_SERVER_ALIVE_INTERVAL);

#ifdef HAVE_CURL
    curl_global_init(CURL_GLOBAL_ALL);
#endif

    // initalize what is needed
    auto self = shared_from_this();
    timer = std::make_shared<Timer>();
    timer->init();
#ifdef ONLINE_SERVICES
    task_processor = std::make_shared<TaskProcessor>();
    task_processor->init();
#endif
#ifdef HAVE_JS
    scripting_runtime = std::make_shared<Runtime>();
#endif
    storage = Storage::createInstance(config, timer);
    update_manager = std::make_shared<UpdateManager>(storage, self);
    update_manager->init();
    session_manager = std::make_shared<web::SessionManager>(config, timer);
#ifdef HAVE_LASTFMLIB
    last_fm = std::make_shared<LastFm>(config);
    last_fm->init();
#endif
    content = std::make_shared<ContentManager>(
        config, storage, update_manager, session_manager, timer, task_processor, scripting_runtime, last_fm);
    content->init();
}

Server::~Server() { log_debug("Server destroyed"); }

void Server::run()
{
    int ret = 0; // general purpose error code
    log_debug("start");

    std::string iface = config->getOption(CFG_SERVER_NETWORK_INTERFACE);
    std::string ip = config->getOption(CFG_SERVER_IP);

    if (string_ok(ip) && string_ok(iface))
        throw std::runtime_error("You can not specify interface and IP at the same time!");

    if (!string_ok(iface))
        iface = ipToInterface(ip);

    if (string_ok(ip) && !string_ok(iface))
        throw std::runtime_error("Could not find ip: " + ip);

    int port = config->getIntOption(CFG_SERVER_PORT);

    log_debug("Initialising libupnp with interface: '{}', port: {}", iface.c_str(), port);
    const char* IfName = nullptr;
    if (!iface.empty())
        IfName = iface.c_str();
    ret = UpnpInit2(IfName, port);
    if (ret != UPNP_E_SUCCESS) {
        throw UpnpException(ret, "run: UpnpInit failed");
    }

    port = UpnpGetServerPort();
    log_info("Initialized port: {}", port);

    if (!string_ok(ip)) {
        ip = UpnpGetServerIpAddress();
    }

    log_info("Server bound to: {}", ip.c_str());

    virtualUrl = "http://" + ip + ":" + std::to_string(port) + "/" + virtual_directory;

    // next set webroot directory
    std::string web_root = config->getOption(CFG_SERVER_WEBROOT);

    if (!string_ok(web_root)) {
        throw std::runtime_error("invalid web server root directory");
    }

    ret = UpnpSetWebServerRootDir(web_root.c_str());
    if (ret != UPNP_E_SUCCESS) {
        throw UpnpException(ret, "run: UpnpSetWebServerRootDir failed");
    }

    log_debug("webroot: {}", web_root.c_str());

    std::vector<std::string> arr = config->getStringArrayOption(CFG_SERVER_CUSTOM_HTTP_HEADERS);
    for (const auto& tmp : arr) {
        if (string_ok(tmp)) {
            log_info("(NOT) Adding HTTP header \"{}\"", tmp.c_str());
            // FIXME upstream upnp
            //ret = UpnpAddCustomHTTPHeader(tmp.c_str());
            //if (ret != UPNP_E_SUCCESS)
            //{
            //    throw UpnpException(ret, "run: UpnpAddCustomHTTPHeader failed");
            //}
        }
    }

    log_debug("Setting virtual dir to: {}", virtual_directory.c_str());
    ret = UpnpAddVirtualDir(virtual_directory.c_str(), this, nullptr);
    if (ret != UPNP_E_SUCCESS) {
        throw UpnpException(ret, "run: UpnpAddVirtualDir failed");
    }

    ret = registerVirtualDirCallbacks();

    if (ret != UPNP_E_SUCCESS) {
        throw UpnpException(ret, "run: UpnpSetVirtualDirCallbacks failed");
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

    log_debug("Creating UpnpXMLBuilder");
    xmlbuilder = std::make_unique<UpnpXMLBuilder>(config, storage, virtualUrl, presentationURL);

    // register root device with the library
    auto desc = xmlbuilder->renderDeviceDescription();
    std::ostringstream buf;
    desc->print(buf, "", 0);
    std::string deviceDescription = buf.str();
    //log_debug("Device Description: {}", deviceDescription.c_str());

    log_debug("Registering with UPnP...");
    ret = UpnpRegisterRootDevice2(UPNPREG_BUF_DESC,
        deviceDescription.c_str(),
        static_cast<size_t>(deviceDescription.length()) + 1,
        true,
        static_upnp_callback,
        this,
        &deviceHandle);

    if (ret != UPNP_E_SUCCESS) {
        throw UpnpException(ret, "run: UpnpRegisterRootDevice failed");
    }

    log_debug("Creating ContentDirectoryService");
    cds = std::make_unique<ContentDirectoryService>(config, storage, xmlbuilder.get(), deviceHandle,
        config->getIntOption(CFG_SERVER_UPNP_TITLE_AND_DESC_STRING_LIMIT));

    log_debug("Creating ConnectionManagerService");
    cmgr = std::make_unique<ConnectionManagerService>(config, storage, xmlbuilder.get(), deviceHandle);

    log_debug("Creating MRRegistrarService");
    mrreg = std::make_unique<MRRegistrarService>(config, xmlbuilder.get(), deviceHandle);

    // The advertisement will be sent by LibUPnP every (A/2)-30 seconds, and will have a cache-control max-age of A where A is
    // the value configured here. Ex: A value of 62 will result in an SSDP advertisement being sent every second.
    log_debug("Sending UPnP Alive advertisements every {} seconds", (aliveAdvertisementInterval / 2) - 30);
    ret = UpnpSendAdvertisement(deviceHandle, aliveAdvertisementInterval);
    if (ret != UPNP_E_SUCCESS) {
        throw UpnpException(ret, "run: UpnpSendAdvertisement failed");
    }

    config->writeBookmark(ip, std::to_string(port));
    log_info("The Web UI can be reached by following this link: http://{}:{}/", ip.c_str(), port);

    log_debug("end");
}

bool Server::getShutdownStatus() const
{
    return server_shutdown_flag;
}

void Server::shutdown()
{
    int ret = 0; // return code

    config->emptyBookmark();
    server_shutdown_flag = true;

    log_debug("Server shutting down");

    ret = UpnpUnRegisterRootDevice(deviceHandle);
    if (ret != UPNP_E_SUCCESS) {
        log_error("upnp_cleanup: UpnpUnRegisterRootDevice failed: %i", ret);
    }

#ifdef HAVE_CURL
    curl_global_cleanup();
#endif

    log_debug("now calling upnp finish");
    UpnpFinish();

    content->shutdown();
    content = nullptr;
#ifdef HAVE_LASTFMLIB
    last_fm->shutdown();
    last_fm = nullptr;
#endif
    session_manager = nullptr;
    update_manager->shutdown();
    update_manager = nullptr;

    if (storage->threadCleanupRequired()) {
        try {
            storage->threadCleanup();
        } catch (const std::runtime_error& ex) {
        }
    }
    storage->shutdown();
    storage = nullptr;

    scripting_runtime = nullptr;
#ifdef ONLINE_SERVICES
    task_processor->shutdown();
    task_processor = nullptr;
#endif
    timer->shutdown();
    timer = nullptr;
}

int Server::handleUpnpEvent(Upnp_EventType eventtype, const void* event)
{
    int ret = UPNP_E_SUCCESS; // general purpose return code

    log_debug("start");

    // check parameters
    if (event == nullptr) {
        log_debug("handleUpnpEvent: NULL event structure");
        return UPNP_E_BAD_REQUEST;
    }

    // dispatch event based on event type
    switch (eventtype) {

    case UPNP_CONTROL_ACTION_REQUEST:
        // a CP is invoking an action
        log_debug("UPNP_CONTROL_ACTION_REQUEST");
        try {
            auto request = std::make_unique<ActionRequest>((UpnpActionRequest*)event);
            routeActionRequest(request);
            request->update();
            // set in update() ((struct Upnp_Action_Request *)event)->ErrCode = ret;
        } catch (const UpnpException& upnp_e) {
            ret = upnp_e.getErrorCode();
            UpnpActionRequest_set_ErrCode((UpnpActionRequest*)event, ret);
        } catch (const std::runtime_error& e) {
            log_info("Exception: {}", e.what());
        }
        break;

    case UPNP_EVENT_SUBSCRIPTION_REQUEST:
        // a cp wants a subscription
        log_debug("UPNP_EVENT_SUBSCRIPTION_REQUEST");
        try {
            auto request = std::make_unique<SubscriptionRequest>((UpnpSubscriptionRequest*)event);
            routeSubscriptionRequest(request);
        } catch (const UpnpException& upnp_e) {
            log_warning("Subscription exception: {}", upnp_e.what());
            ret = upnp_e.getErrorCode();
        }
        break;

    default:
        // unhandled event type
        log_warning("unsupported event type: {}", eventtype);
        ret = UPNP_E_BAD_REQUEST;
        break;
    }

    log_debug("returning {}", ret);
    return ret;
}

std::string Server::getIP()
{
    return UpnpGetServerIpAddress();
}

std::string Server::getPort()
{
    return std::to_string(UpnpGetServerPort());
}

void Server::routeActionRequest(const std::unique_ptr<ActionRequest>& request) const
{
    log_debug("start");

    // make sure the request is for our device
    if (request->getUDN() != serverUDN) {
        // not for us
        throw UpnpException(UPNP_E_BAD_REQUEST, "routeActionRequest: request not for this device");
    }

    // we need to match the serviceID to one of our services
    if (request->getServiceID() == DESC_CM_SERVICE_ID) {
        // this call is for the lifetime stats service
        // log_debug("request for connection manager service");
        cmgr->processActionRequest(request);
    } else if (request->getServiceID() == DESC_CDS_SERVICE_ID) {
        // this call is for the toaster control service
        //log_debug("routeActionRequest: request for content directory service");
        cds->processActionRequest(request);
    } else if (request->getServiceID() == DESC_MRREG_SERVICE_ID) {
        mrreg->processActionRequest(request);
    } else {
        // cp is asking for a nonexistent service, or for a service
        // that does not support any actions
        throw UpnpException(UPNP_E_BAD_REQUEST, "Service does not exist or action not supported");
    }
}

void Server::routeSubscriptionRequest(const std::unique_ptr<SubscriptionRequest>& request) const
{
    // make sure that the request is for our device
    if (request->getUDN() != serverUDN) {
        // not for us
        log_debug("routeSubscriptionRequest: request not for this device: {} vs {}",
            request->getUDN().c_str(), serverUDN.c_str());
        throw UpnpException(UPNP_E_BAD_REQUEST, "routeActionRequest: request not for this device");
    }

    // we need to match the serviceID to one of our services
    if (request->getServiceID() == DESC_CDS_SERVICE_ID) {
        // this call is for the content directory service
        //log_debug("routeSubscriptionRequest: request for content directory service");
        cds->processSubscriptionRequest(request);
    } else if (request->getServiceID() == DESC_CM_SERVICE_ID) {
        // this call is for the connection manager service
        //log_debug("routeSubscriptionRequest: request for connection manager service");
        cmgr->processSubscriptionRequest(request);
    } else if (request->getServiceID() == DESC_MRREG_SERVICE_ID) {
        mrreg->processSubscriptionRequest(request);
    } else {
        // cp asks for a nonexistent service or for a service that
        // does not support subscriptions
        throw UpnpException(UPNP_E_BAD_REQUEST, "Service does not exist or subscriptions not supported");
    }
}

// Temp
void Server::sendCDSSubscriptionUpdate(const std::string& updateString)
{
    cds->sendSubscriptionUpdate(updateString);
}

std::unique_ptr<RequestHandler> Server::createRequestHandler(const char* filename) const
{
    std::string link = urlUnescape(filename);
    log_debug("Filename: {}", filename);

    std::unique_ptr<RequestHandler> ret = nullptr;

    if (startswith(link, std::string("/") + SERVER_VIRTUAL_DIR + "/" + CONTENT_MEDIA_HANDLER)) {
        ret = std::make_unique<FileRequestHandler>(config, storage, content, update_manager, session_manager, xmlbuilder.get());
    } else if (startswith(link, std::string("/") + SERVER_VIRTUAL_DIR + "/" + CONTENT_UI_HANDLER)) {
        std::string parameters;
        std::string path;
        RequestHandler::splitUrl(filename, URL_UI_PARAM_SEPARATOR, path, parameters);

        std::map<std::string, std::string> dict;
        dict_decode(parameters, &dict);

        std::string r_type = getValueOrDefault(dict, URL_REQUEST_TYPE);
        if (r_type.empty())
            r_type = "index";

        ret = web::createWebRequestHandler(config, storage, content, session_manager, r_type);
    } else if (startswith(link, std::string("/") + SERVER_VIRTUAL_DIR + "/" + DEVICE_DESCRIPTION_PATH)) {
        ret = std::make_unique<DeviceDescriptionHandler>(config, storage, xmlbuilder.get());
    } else if (startswith(link, std::string("/") + SERVER_VIRTUAL_DIR + "/" + CONTENT_SERVE_HANDLER)) {
        if (string_ok(config->getOption(CFG_SERVER_SERVEDIR)))
            ret = std::make_unique<ServeRequestHandler>(config, storage);
        else
            throw std::runtime_error("Serving directories is not enabled in configuration");
    }
#if defined(HAVE_CURL)
    else if (startswith(link, std::string("/") + SERVER_VIRTUAL_DIR + "/" + CONTENT_ONLINE_HANDLER)) {
        ret = std::make_unique<URLRequestHandler>(config, storage, content);
    }
#endif
    else {
        throw std::runtime_error(std::string("no valid handler type in ") + filename);
    }

    return ret;
}

int Server::registerVirtualDirCallbacks()
{
    log_debug("Setting UpnpVirtualDir GetInfoCallback");
#ifdef UPNP_HAS_REQUEST_COOKIES
    int ret = UpnpVirtualDir_set_GetInfoCallback([](const char* filename, UpnpFileInfo* info, const void* cookie, const void** requestCookie) -> int {
#else
    int ret = UpnpVirtualDir_set_GetInfoCallback([](const char* filename, UpnpFileInfo* info, const void* cookie) -> int {
#endif
        try {
            auto reqHandler = static_cast<const Server *>(cookie)->createRequestHandler(filename);
            reqHandler->getInfo(filename, info);
        } catch (const ServerShutdownException& se) {
            return -1;
        } catch (const SubtitlesNotFoundException& sex) {
            log_warning("{}", sex.what());
            return -1;
        } catch (const std::runtime_error& e) {
            log_error("{}", e.what());
            return -1;
        }
        return 0; });
    if (ret != 0)
        return ret;

    log_debug("Setting UpnpVirtualDir OpenCallback");
#ifdef UPNP_HAS_REQUEST_COOKIES
    ret = UpnpVirtualDir_set_OpenCallback([](const char* filename, enum UpnpOpenFileMode mode, const void* cookie, const void* requestCookie) -> UpnpWebFileHandle {
#else
    ret = UpnpVirtualDir_set_OpenCallback([](const char* filename, enum UpnpOpenFileMode mode, const void* cookie) -> UpnpWebFileHandle {
#endif
        std::string link = urlUnescape(filename);

        try {
            auto reqHandler = static_cast<const Server*>(cookie)->createRequestHandler(filename);
            auto ioHandler = reqHandler->open(link.c_str(), mode, "");
            auto ioPtr = (UpnpWebFileHandle)ioHandler.release();
            //log_debug("%p open({})", ioPtr, filename);
            return ioPtr;
        } catch (const ServerShutdownException& se) {
            return nullptr;
        } catch (const SubtitlesNotFoundException& sex) {
            log_info("SubtitlesNotFoundException: {}", sex.what());
            return nullptr;
        } catch (const std::runtime_error& ex) {
            log_error("Exception: {}", ex.what());
            return nullptr;
        }
    });
    if (ret != UPNP_E_SUCCESS)
        return ret;

    log_debug("Setting UpnpVirtualDir ReadCallback");
#ifdef UPNP_HAS_REQUEST_COOKIES
    ret = UpnpVirtualDir_set_ReadCallback([](UpnpWebFileHandle f, char* buf, size_t length, const void* cookie, const void* requestCookie) -> int {
#else
    ret = UpnpVirtualDir_set_ReadCallback([](UpnpWebFileHandle f, char* buf, size_t length, const void* cookie) -> int {
#endif
        //log_debug("%p read({})", f, length);
        if (static_cast<const Server*>(cookie)->getShutdownStatus())
            return -1;

        auto handler = static_cast<IOHandler*>(f);
        return handler->read(buf, length);
    });
    if (ret != UPNP_E_SUCCESS)
        return ret;

    log_debug("Setting UpnpVirtualDir WriteCallback");
#ifdef UPNP_HAS_REQUEST_COOKIES
    ret = UpnpVirtualDir_set_WriteCallback([](UpnpWebFileHandle f, char* buf, size_t length, const void* cookie, const void* requestCookie) -> int {
#else
    ret = UpnpVirtualDir_set_WriteCallback([](UpnpWebFileHandle f, char* buf, size_t length, const void* cookie) -> int {
#endif
        //log_debug("%p write({})", f, length);
        return 0;
    });
    if (ret != UPNP_E_SUCCESS)
        return ret;

    log_debug("Setting UpnpVirtualDir SeekCallback");
#ifdef UPNP_HAS_REQUEST_COOKIES
    ret = UpnpVirtualDir_set_SeekCallback([](UpnpWebFileHandle f, off_t offset, int whence, const void* cookie, const void* requestCookie) -> int {
#else
    ret = UpnpVirtualDir_set_SeekCallback([](UpnpWebFileHandle f, off_t offset, int whence, const void* cookie) -> int {
#endif
        //log_debug("%p seek({}, {})", f, offset, whence);
        try {
            auto handler = static_cast<IOHandler*>(f);
            handler->seek(offset, whence);
        } catch (const std::runtime_error& e) {
            log_error("Exception during seek: {}", e.what());
            return -1;
        }

        return 0;
    });
    if (ret != UPNP_E_SUCCESS)
        return ret;

    log_debug("Setting UpnpVirtualDir CloseCallback");
#ifdef UPNP_HAS_REQUEST_COOKIES
    UpnpVirtualDir_set_CloseCallback([](UpnpWebFileHandle f, const void* cookie, const void* requestCookie) -> int {
#else
    UpnpVirtualDir_set_CloseCallback([](UpnpWebFileHandle f, const void* cookie) -> int {
#endif
        int ret_close = 0;
        //log_debug("%p close()", f);
        auto handler = static_cast<IOHandler*>(f);
        try {
            handler->close();
        } catch (const std::runtime_error& e) {
            log_error("Exception during close: {}", e.what());
            ret_close = -1;
        }

        delete handler;
        handler = nullptr;

        return ret_close;
    });

    return ret;
}
