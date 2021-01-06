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

#include "server.h" // API

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
#include "database/database.h"
#include "file_request_handler.h"
#include "update_manager.h"
#include "util/task_processor.h"
#include "util/upnp_clients.h"
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

Server::Server(std::shared_ptr<Config> config)
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

    // initialize what is needed
    auto self = shared_from_this();
    timer = std::make_shared<Timer>();
#ifdef ONLINE_SERVICES
    task_processor = std::make_shared<TaskProcessor>();
#endif
#ifdef HAVE_JS
    scripting_runtime = std::make_shared<Runtime>();
#endif
    database = Database::createInstance(config, timer);
    config->updateConfigFromDatabase(database);
    update_manager = std::make_shared<UpdateManager>(database, self);
    session_manager = std::make_shared<web::SessionManager>(config, timer);
#ifdef HAVE_LASTFMLIB
    last_fm = std::make_shared<LastFm>(config);
#endif
    content = std::make_shared<ContentManager>(
        config, database, update_manager, session_manager, timer, task_processor, scripting_runtime, last_fm);
}

Server::~Server() { log_debug("Server destroyed"); }

void Server::run()
{
    int ret = 0; // general purpose error code
    log_debug("start");

    std::string iface = config->getOption(CFG_SERVER_NETWORK_INTERFACE);
    std::string ip = config->getOption(CFG_SERVER_IP);

    if (!ip.empty() && !iface.empty())
        throw_std_runtime_error("You can not specify interface and IP at the same time");

    if (iface.empty())
        iface = ipToInterface(ip);

    if (!ip.empty() && iface.empty())
        throw_std_runtime_error("Could not find ip: " + ip);

    auto port = in_port_t(config->getIntOption(CFG_SERVER_PORT));

    log_info("Initialising libupnp with interface: '{}', port: {}", iface.c_str(), port);
    const char* IfName = iface.empty() ? nullptr : iface.c_str();
    ret = UpnpInit2(IfName, port);
    if (ret != UPNP_E_SUCCESS) {
        throw UpnpException(ret, "run: UpnpInit failed");
    }

    port = UpnpGetServerPort();
    log_info("Initialized port: {}", port);

    if (ip.empty()) {
        ip = UpnpGetServerIpAddress();
    }

    log_info("Server bound to: {}", ip.c_str());

    virtualUrl = "http://" + ip + ":" + std::to_string(port) + "/" + virtual_directory;

    // next set webroot directory
    std::string web_root = config->getOption(CFG_SERVER_WEBROOT);

    if (web_root.empty()) {
        throw_std_runtime_error("invalid web server root directory");
    }

    ret = UpnpSetWebServerRootDir(web_root.c_str());
    if (ret != UPNP_E_SUCCESS) {
        throw UpnpException(ret, "run: UpnpSetWebServerRootDir failed");
    }

    log_debug("webroot: {}", web_root.c_str());

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
    if (presentationURL.empty()) {
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
    xmlbuilder = std::make_unique<UpnpXMLBuilder>(config, database, virtualUrl, presentationURL);

    // register root device with the library
    auto desc = xmlbuilder->renderDeviceDescription();
    std::ostringstream buf;
    desc->print(buf, "", 0);
    std::string deviceDescription = buf.str();
    //log_debug("Device Description: {}", deviceDescription.c_str());

    log_debug("Registering with UPnP...");
    ret = UpnpRegisterRootDevice2(
        UPNPREG_BUF_DESC,
        deviceDescription.c_str(),
        static_cast<size_t>(deviceDescription.length()) + 1,
        true,
        handleUpnpRootDeviceEventCallback,
        this,
        &rootDeviceHandle);
    if (ret != UPNP_E_SUCCESS) {
        throw UpnpException(ret, "run: UpnpRegisterRootDevice2 failed");
    }

    ret = UpnpRegisterClient(
        handleUpnpClientEventCallback,
        this,
        &clientHandle);
    if (ret != UPNP_E_SUCCESS) {
        throw UpnpException(ret, "run: UpnpRegisterClient failed");
    }

    log_debug("Creating ContentDirectoryService");
    cds = std::make_unique<ContentDirectoryService>(config, database, xmlbuilder.get(), rootDeviceHandle,
        config->getIntOption(CFG_SERVER_UPNP_TITLE_AND_DESC_STRING_LIMIT));

    log_debug("Creating ConnectionManagerService");
    cmgr = std::make_unique<ConnectionManagerService>(config, database, xmlbuilder.get(), rootDeviceHandle);

    log_debug("Creating MRRegistrarService");
    mrreg = std::make_unique<MRRegistrarService>(config, xmlbuilder.get(), rootDeviceHandle);

    // The advertisement will be sent by LibUPnP every (A/2)-30 seconds, and will have a cache-control max-age of A where A is
    // the value configured here. Ex: A value of 62 will result in an SSDP advertisement being sent every second.
    log_debug("Sending UPnP Alive advertisements every {} seconds", (aliveAdvertisementInterval / 2) - 30);
    ret = UpnpSendAdvertisement(rootDeviceHandle, aliveAdvertisementInterval);
    if (ret != UPNP_E_SUCCESS) {
        throw UpnpException(ret, "run: UpnpSendAdvertisement failed");
    }

    // run what is needed
    timer->run();
#ifdef ONLINE_SERVICES
    task_processor->run();
#endif
    update_manager->run();
#ifdef HAVE_LASTFMLIB
    last_fm->run();
#endif
    content->run();

    std::string url = renderWebUri(ip, port);
    writeBookmark(url);
    log_info("The Web UI can be reached by following this link: http://{}/", url);
    log_debug("end");
}

void Server::writeBookmark(const std::string& addr)
{
    const std::string data = config->getBoolOption(CFG_SERVER_UI_ENABLED)
        ? httpRedirectTo(addr)
        : httpRedirectTo(addr, "disabled.html");

    fs::path path = config->getOption(CFG_SERVER_BOOKMARK_FILE);
    log_debug("Writing bookmark file to: {}", path.c_str());
    writeTextFile(path, data);
}

void Server::emptyBookmark()
{
    const std::string data = "<html><body><h1>Gerbera Media Server is not running.</h1><p>Please start it and try again.</p></body></html>";

    fs::path path = config->getOption(CFG_SERVER_BOOKMARK_FILE);
    log_debug("Clearing bookmark file at: {}", path.c_str());
    writeTextFile(path, data);
}

bool Server::getShutdownStatus() const
{
    return server_shutdown_flag;
}

void Server::shutdown()
{
    int ret = 0; // return code

    emptyBookmark();
    server_shutdown_flag = true;

    log_debug("Server shutting down");

    ret = UpnpUnRegisterClient(clientHandle);
    if (ret != UPNP_E_SUCCESS) {
        log_error("UpnpUnRegisterClient failed ({})", ret);
    }

    ret = UpnpUnRegisterRootDevice(rootDeviceHandle);
    if (ret != UPNP_E_SUCCESS) {
        log_error("UpnpUnRegisterRootDevice failed ({})", ret);
    }

#ifdef HAVE_CURL
    curl_global_cleanup();
#endif

    log_debug("now calling upnp finish");
    UpnpFinish();

    if (content) {
        content->shutdown();
        content = nullptr;
    }
#ifdef HAVE_LASTFMLIB
    last_fm->shutdown();
    last_fm = nullptr;
#endif
    session_manager = nullptr;
    update_manager->shutdown();
    update_manager = nullptr;

    if (database->threadCleanupRequired()) {
        try {
            database->threadCleanup();
        } catch (const std::runtime_error& ex) {
        }
    }
    database->shutdown();
    database = nullptr;

    scripting_runtime = nullptr;
#ifdef ONLINE_SERVICES
    task_processor->shutdown();
    task_processor = nullptr;
#endif
    timer->shutdown();
    timer = nullptr;
}

int Server::handleUpnpRootDeviceEventCallback(Upnp_EventType eventtype, const void* event, void* cookie)
{
    return static_cast<Server*>(cookie)->handleUpnpRootDeviceEvent(eventtype, event);
}

int Server::handleUpnpRootDeviceEvent(Upnp_EventType eventtype, const void* event)
{
    int ret = UPNP_E_SUCCESS; // general purpose return code

    log_debug("start");

    // check parameters
    if (event == nullptr) {
        log_debug("handleUpnpRootDeviceEvent: NULL event structure");
        return UPNP_E_BAD_REQUEST;
    }

    // dispatch event based on event type
    switch (eventtype) {

    case UPNP_CONTROL_ACTION_REQUEST:
        // a CP is invoking an action
        log_debug("UPNP_CONTROL_ACTION_REQUEST");
        try {
            auto request = std::make_unique<ActionRequest>(config, static_cast<UpnpActionRequest*>(const_cast<void*>(event)));
            routeActionRequest(request);
            request->update();
            // set in update() ((struct Upnp_Action_Request *)event)->ErrCode = ret;
        } catch (const UpnpException& upnp_e) {
            ret = upnp_e.getErrorCode();
            UpnpActionRequest_set_ErrCode(static_cast<UpnpActionRequest*>(const_cast<void*>(event)), ret);
        } catch (const std::runtime_error& e) {
            log_info("Exception: {}", e.what());
        }
        break;

    case UPNP_EVENT_SUBSCRIPTION_REQUEST:
        // a cp wants a subscription
        log_debug("UPNP_EVENT_SUBSCRIPTION_REQUEST");
        try {
            auto request = std::make_unique<SubscriptionRequest>(static_cast<UpnpSubscriptionRequest*>(const_cast<void*>(event)));
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

int Server::handleUpnpClientEventCallback(Upnp_EventType eventType, const void* event, void* cookie)
{
    return Server::handleUpnpClientEvent(eventType, event);
}

int Server::handleUpnpClientEvent(Upnp_EventType eventType, const void* event)
{
    // check parameters
    if (event == nullptr) {
        log_debug("handleUpnpClientEvent: NULL event structure");
        return UPNP_E_BAD_REQUEST;
    }

    switch (eventType) {
    case UPNP_DISCOVERY_ADVERTISEMENT_ALIVE:
    case UPNP_DISCOVERY_SEARCH_RESULT: {
        auto d_event = reinterpret_cast<const UpnpDiscovery*>(event);
#if defined(USING_NPUPNP)
        const char* userAgent = UpnpDiscovery_get_Os_cstr(d_event);
#else
        const char* userAgent = UpnpString_get_String(UpnpDiscovery_get_Os(d_event));
#endif
        const struct sockaddr_storage* destAddr = UpnpDiscovery_get_DestAddr(d_event);
#if defined(USING_NPUPNP)
        const char* location = UpnpDiscovery_get_Location_cstr(d_event);
#else
        const char* location = UpnpString_get_String(UpnpDiscovery_get_Location(d_event));
#endif

        Clients::addClientByDiscovery(destAddr, userAgent, location);
        break;
    }
    default:
        break;
    }

    return 0;
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
    if (request->getServiceID() == UPNP_DESC_CM_SERVICE_ID) {
        // this call is for the lifetime stats service
        // log_debug("request for connection manager service");
        cmgr->processActionRequest(request);
    } else if (request->getServiceID() == UPNP_DESC_CDS_SERVICE_ID) {
        // this call is for the toaster control service
        //log_debug("routeActionRequest: request for content directory service");
        cds->processActionRequest(request);
    } else if (request->getServiceID() == UPNP_DESC_MRREG_SERVICE_ID) {
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
    if (request->getServiceID() == UPNP_DESC_CDS_SERVICE_ID) {
        // this call is for the content directory service
        //log_debug("routeSubscriptionRequest: request for content directory service");
        cds->processSubscriptionRequest(request);
    } else if (request->getServiceID() == UPNP_DESC_CM_SERVICE_ID) {
        // this call is for the connection manager service
        //log_debug("routeSubscriptionRequest: request for connection manager service");
        cmgr->processSubscriptionRequest(request);
    } else if (request->getServiceID() == UPNP_DESC_MRREG_SERVICE_ID) {
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
        ret = std::make_unique<FileRequestHandler>(content, xmlbuilder.get());
    } else if (startswith(link, std::string("/") + SERVER_VIRTUAL_DIR + "/" + CONTENT_UI_HANDLER)) {
        std::string parameters;
        std::string path;
        RequestHandler::splitUrl(filename, URL_UI_PARAM_SEPARATOR, path, parameters);

        std::map<std::string, std::string> params;
        dictDecode(parameters, &params);

        auto it = params.find(URL_REQUEST_TYPE);
        std::string r_type = it != params.end() && !it->second.empty() ? it->second : "index";

        ret = web::createWebRequestHandler(content, r_type);
    } else if (startswith(link, std::string("/") + SERVER_VIRTUAL_DIR + "/" + DEVICE_DESCRIPTION_PATH)) {
        ret = std::make_unique<DeviceDescriptionHandler>(content, xmlbuilder.get());
    } else if (startswith(link, std::string("/") + SERVER_VIRTUAL_DIR + "/" + CONTENT_SERVE_HANDLER)) {
        if (!config->getOption(CFG_SERVER_SERVEDIR).empty())
            ret = std::make_unique<ServeRequestHandler>(content);
        else
            throw_std_runtime_error("Serving directories is not enabled in configuration");
    }
#if defined(HAVE_CURL)
    else if (startswith(link, std::string("/") + SERVER_VIRTUAL_DIR + "/" + CONTENT_ONLINE_HANDLER)) {
        ret = std::make_unique<URLRequestHandler>(content);
    }
#endif
    else {
        throw_std_runtime_error(std::string("no valid handler type in ") + filename);
    }

    return ret;
}

int Server::registerVirtualDirCallbacks()
{
    log_debug("Setting UpnpVirtualDir GetInfoCallback");
    int ret = UpnpVirtualDir_set_GetInfoCallback([](const char* filename, UpnpFileInfo* info, const void* cookie, const void** requestCookie) -> int {
        try {
            auto reqHandler = static_cast<const Server*>(cookie)->createRequestHandler(filename);
            std::string link = urlUnescape(filename);
            reqHandler->getInfo(link.c_str(), info);
            return 0;
        } catch (const ServerShutdownException& se) {
            return -1;
        } catch (const SubtitlesNotFoundException& sex) {
            log_warning("{}", sex.what());
            return -1;
        } catch (const std::runtime_error& e) {
            log_error("{}", e.what());
            return -1;
        }
    });
    if (ret != 0)
        return ret;

    log_debug("Setting UpnpVirtualDir OpenCallback");
    ret = UpnpVirtualDir_set_OpenCallback([](const char* filename, enum UpnpOpenFileMode mode, const void* cookie, const void* requestCookie) -> UpnpWebFileHandle {
        try {
            auto reqHandler = static_cast<const Server*>(cookie)->createRequestHandler(filename);
            std::string link = urlUnescape(filename);
            auto ioHandler = reqHandler->open(link.c_str(), mode);
            auto ioPtr = static_cast<UpnpWebFileHandle>(ioHandler.release());
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
    ret = UpnpVirtualDir_set_ReadCallback([](UpnpWebFileHandle f, char* buf, size_t length, const void* cookie, const void* requestCookie) -> int {
        //log_debug("%p read({})", f, length);
        if (static_cast<const Server*>(cookie)->getShutdownStatus())
            return -1;

        auto handler = static_cast<IOHandler*>(f);
        return handler->read(buf, length);
    });
    if (ret != UPNP_E_SUCCESS)
        return ret;

    log_debug("Setting UpnpVirtualDir WriteCallback");
    ret = UpnpVirtualDir_set_WriteCallback([](UpnpWebFileHandle f, char* buf, size_t length, const void* cookie, const void* requestCookie) -> int {
        //log_debug("%p write({})", f, length);
        return 0;
    });
    if (ret != UPNP_E_SUCCESS)
        return ret;

    log_debug("Setting UpnpVirtualDir SeekCallback");
    ret = UpnpVirtualDir_set_SeekCallback([](UpnpWebFileHandle f, off_t offset, int whence, const void* cookie, const void* requestCookie) -> int {
        //log_debug("%p seek({}, {})", f, offset, whence);
        try {
            auto handler = static_cast<IOHandler*>(f);
            handler->seek(offset, whence);
            return 0;
        } catch (const std::runtime_error& e) {
            log_error("Exception during seek: {}", e.what());
            return -1;
        }
    });
    if (ret != UPNP_E_SUCCESS)
        return ret;

    log_debug("Setting UpnpVirtualDir CloseCallback");
    ret = UpnpVirtualDir_set_CloseCallback([](UpnpWebFileHandle f, const void* cookie, const void* requestCookie) -> int {
        //log_debug("%p close()", f);
        int ret_close = 0;
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
