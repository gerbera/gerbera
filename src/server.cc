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
#endif

#include <upnpconfig.h>
#ifdef UPNP_HAVE_TOOLS
#include <upnptools.h>
#endif

#include <chrono>
#include <thread>

#include "config/config_manager.h"
#include "content/content_manager.h"
#include "database/database.h"
#include "device_description_handler.h"
#include "file_request_handler.h"
#include "util/mime.h"
#include "util/upnp_clients.h"
#include "web/pages.h"
#include "web/session_manager.h"

#ifdef HAVE_CURL
#include "url_request_handler.h"
#endif

Server::Server(std::shared_ptr<Config> config)
    : config(std::move(config))
{
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
    timer = std::make_shared<Timer>(config);

    clients = std::make_shared<Clients>(config);
    mime = std::make_shared<Mime>(config);
    timer->run();
    database = Database::createInstance(config, mime, timer);
    config->updateConfigFromDatabase(database);
    session_manager = std::make_shared<Web::SessionManager>(config, timer);
    context = std::make_shared<Context>(config, clients, mime, database, self, session_manager);

    content = std::make_shared<ContentManager>(context, self, timer);
}

Server::~Server() { log_debug("Server destroyed"); }

void Server::run()
{
    log_debug("Starting...");

    // run what is needed
    try {
        content->run();
    } catch (const std::runtime_error& ex) {
        log_error("Activating ContentService failed {}", ex.what());
        throw UpnpException(500, fmt::format("run: Activating ContentService failed {}", ex.what()));
    }

    std::string iface = config->getOption(CFG_SERVER_NETWORK_INTERFACE);
    ip = config->getOption(CFG_SERVER_IP);

    if (!ip.empty() && !iface.empty())
        throw_std_runtime_error("You cannot specify interface {} and IP {} at the same time", iface, ip);

    if (iface.empty())
        iface = ipToInterface(ip);

    if (!ip.empty() && iface.empty())
        throw_std_runtime_error("Could not find IP: {}", ip);

    // check webroot directory
    std::string webRoot = config->getOption(CFG_SERVER_WEBROOT);
    if (webRoot.empty()) {
        throw_std_runtime_error("Invalid web server root directory {}", webRoot);
    }

    auto configPort = config->getIntOption(CFG_SERVER_PORT);
    auto ret = startupInterface(iface, configPort);
    if (ret != UPNP_E_SUCCESS) {
        throw UpnpException(ret, fmt::format("UpnpInit failed {} {}", iface, port));
    }

    if (configPort > 0 && port != configPort) {
        throw UpnpException(ret, fmt::format("LibUPnP failed to bind to request port! Bound to {}, requested: {}", port, configPort));
    }

    ret = UpnpSetWebServerRootDir(webRoot.c_str());
    if (ret != UPNP_E_SUCCESS) {
        throw UpnpException(ret, fmt::format("UpnpSetWebServerRootDir failed {}", webRoot));
    }

    log_debug("webroot: {}", webRoot);

    log_debug("Setting virtual dir to: {}", virtual_directory);
    ret = UpnpAddVirtualDir(virtual_directory.c_str(), this, nullptr);
    if (ret != UPNP_E_SUCCESS) {
        throw UpnpException(ret, fmt::format("run: UpnpAddVirtualDir failed {}", virtual_directory));
    }

    ret = registerVirtualDirCallbacks();

    if (ret != UPNP_E_SUCCESS) {
        throw UpnpException(ret, "run: UpnpSetVirtualDirCallbacks failed");
    }

    std::string presentationURL = getPresentationUrl();

    log_debug("Creating UpnpXMLBuilder");
    xmlbuilder = std::make_shared<UpnpXMLBuilder>(context, virtualUrl, presentationURL);

    // register root device with the library
    auto desc = xmlbuilder->renderDeviceDescription();
    std::ostringstream buf;
    desc->print(buf, "", 0);
    std::string deviceDescription = buf.str();
    // log_debug("Device Description: {}", deviceDescription.c_str());

    log_debug("Registering with UPnP...");
    ret = UpnpRegisterRootDevice2(
        UPNPREG_BUF_DESC,
        deviceDescription.c_str(),
        deviceDescription.length() + 1,
        true,
        [](auto eventType, auto event, auto cookie) { return static_cast<Server*>(cookie)->handleUpnpRootDeviceEvent(eventType, event); },
        this,
        &rootDeviceHandle);
    if (ret != UPNP_E_SUCCESS) {
        throw UpnpException(ret, "run: UpnpRegisterRootDevice2 failed");
    }

    ret = UpnpRegisterClient(
        [](auto eventType, auto event, auto cookie) { return static_cast<Server*>(cookie)->handleUpnpClientEvent(eventType, event); },
        this,
        &clientHandle);
    if (ret != UPNP_E_SUCCESS) {
        throw UpnpException(ret, "run: UpnpRegisterClient failed");
    }

    log_debug("Creating ContentDirectoryService");
    cds = std::make_unique<ContentDirectoryService>(context, xmlbuilder.get(), rootDeviceHandle,
        config->getIntOption(CFG_SERVER_UPNP_TITLE_AND_DESC_STRING_LIMIT));

    log_debug("Creating ConnectionManagerService");
    cmgr = std::make_unique<ConnectionManagerService>(context, xmlbuilder.get(), rootDeviceHandle);

    log_debug("Creating MRRegistrarService");
    mrreg = std::make_unique<MRRegistrarService>(context, xmlbuilder.get(), rootDeviceHandle);

    // The advertisement will be sent by LibUPnP every (A/2)-30 seconds, and will have a cache-control max-age of A where A is
    // the value configured here. Ex: A value of 62 will result in an SSDP advertisement being sent every second.
    log_info("Will send UPnP Alive advertisements every {} seconds", (aliveAdvertisementInterval / 2) - 30);
    ret = UpnpSendAdvertisement(rootDeviceHandle, aliveAdvertisementInterval);
    if (ret != UPNP_E_SUCCESS) {
        throw UpnpException(ret, fmt::format("run: UpnpSendAdvertisement {} failed", aliveAdvertisementInterval));
    }

    std::string url = config->getOption(CFG_VIRTUAL_URL);
    if (url.empty()) {
        url = renderWebUri(ip, port);
    }
    if (url.find("http") != 0) { // url does not start with http
        url = fmt::format("http://{}", url);
    }
    writeBookmark(url);
    log_info("The Web UI can be reached by following this link: {}/", url);
}

int Server::startupInterface(const std::string& iface, in_port_t inPort)
{
    log_info("Initialising UPnP with interface: {}, port: {}",
        iface.empty() ? "<unset>" : iface, inPort == 0 ? "<unset>" : fmt::to_string(inPort));
    const char* ifName = iface.empty() ? nullptr : iface.c_str();

    int ret = UPNP_E_INIT_FAILED;
    for (std::size_t attempt = 0; attempt < 4; attempt++) {
        ret = UpnpInit2(ifName, inPort);
        if (ret == UPNP_E_SUCCESS)
            break;

#ifdef UPNP_HAVE_TOOLS
        log_warning("UPnP Init {}:{} failed: {} ({}). Retrying in {} seconds...", ifName, inPort, UpnpGetErrorMessage(ret), ret, attempt + 1);
#else
        log_warning("UPnP Init {}:{} failed: ({}). Retrying in {} seconds...", ifName, inPort, ret, attempt + 1);
#endif
        std::this_thread::sleep_for(std::chrono::seconds(attempt + 1));
    }

    if (ret != UPNP_E_SUCCESS) {
        throw UpnpException(ret, fmt::format("run: UpnpInit failed with {} {}", ifName, inPort));
    }

    port = UpnpGetServerPort();
    /* The IP libupnp picks is not always the same as passed into config, as we map it to an interface */
    ip = UpnpGetServerIpAddress();

    log_info("IPv4: Server bound to: {}:{}", ip, port);
    log_info("IPv6: Server bound to: {}:{}", UpnpGetServerIp6Address(), UpnpGetServerPort6());
    log_info("IPv6 ULA/GLA: Server bound to: {}:{}", UpnpGetServerUlaGuaIp6Address(), UpnpGetServerUlaGuaPort6());

    virtualUrl = fmt::format("http://{}:{}/{}", ip, port, virtual_directory);

    return ret;
}

std::string Server::getPresentationUrl() const
{
    std::string presentationURL = config->getOption(CFG_SERVER_PRESENTATION_URL);
    if (presentationURL.empty()) {
        presentationURL = fmt::format("http://{}:{}/", ip, port);
    } else {
        std::string appendto = config->getOption(CFG_SERVER_APPEND_PRESENTATION_URL_TO);
        if (appendto == "ip") {
            presentationURL = fmt::format("http://{}:{}", ip, presentationURL);
        } else if (appendto == "port") {
            presentationURL = fmt::format("http://{}:{}/{}", ip, port, presentationURL);
        } // else appendto is none and we take the URL as it entered by user
    }
    return presentationURL;
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
    const std::string_view data = "<html><body><h1>Gerbera Media Server is not running.</h1><p>Please start it and try again.</p></body></html>";

    fs::path path = config->getOption(CFG_SERVER_BOOKMARK_FILE);
    log_debug("Clearing bookmark file at: {}", path.c_str());
    writeTextFile(path, data);
}

std::string Server::getVirtualUrl() const
{
    auto cfgVirt = config->getOption(CFG_VIRTUAL_URL);
    return cfgVirt.empty() ? virtualUrl : fmt::format("{}/{}", cfgVirt, virtual_directory);
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

    session_manager = nullptr;

    if (database->threadCleanupRequired()) {
        try {
            database->threadCleanup();
        } catch (const std::runtime_error& ex) {
        }
    }
    database->shutdown();
    database = nullptr;

    timer->shutdown();
    timer = nullptr;

    mime = nullptr;
    clients = nullptr;
}

int Server::handleUpnpRootDeviceEvent(Upnp_EventType eventType, const void* event)
{
    int ret = UPNP_E_SUCCESS; // general purpose return code

    log_debug("start");

    // check parameters
    if (!event) {
        log_debug("handleUpnpRootDeviceEvent: NULL event structure");
        return UPNP_E_BAD_REQUEST;
    }

    // dispatch event based on event type
    switch (eventType) {
    case UPNP_CONTROL_ACTION_REQUEST:
        log_debug("UPNP_CONTROL_ACTION_REQUEST");
        try {
            auto request = std::make_unique<ActionRequest>(context, static_cast<UpnpActionRequest*>(const_cast<void*>(event)));
            routeActionRequest(request);
            request->update();
        } catch (const UpnpException& upnpE) {
            ret = upnpE.getErrorCode();
            UpnpActionRequest_set_ErrCode(static_cast<UpnpActionRequest*>(const_cast<void*>(event)), ret);
        } catch (const std::runtime_error& e) {
            log_info("Exception: {}", e.what());
        }
        break;

    case UPNP_EVENT_SUBSCRIPTION_REQUEST:
        log_debug("UPNP_EVENT_SUBSCRIPTION_REQUEST");
        try {
            auto request = std::make_unique<SubscriptionRequest>(static_cast<UpnpSubscriptionRequest*>(const_cast<void*>(event)));
            routeSubscriptionRequest(request);
        } catch (const UpnpException& upnpE) {
            log_warning("Subscription exception: {}", upnpE.what());
            ret = upnpE.getErrorCode();
        }
        break;

    default:
        // unhandled event type
        log_warning("unsupported event type: {}", eventType);
        ret = UPNP_E_BAD_REQUEST;
        break;
    }

    log_debug("returning {}", ret);
    return ret;
}

int Server::handleUpnpClientEvent(Upnp_EventType eventType, const void* event)
{
    // check parameters
    if (!event) {
        log_debug("handleUpnpClientEvent: NULL event structure");
        return UPNP_E_BAD_REQUEST;
    }

    switch (eventType) {
    case UPNP_DISCOVERY_ADVERTISEMENT_ALIVE:
    case UPNP_DISCOVERY_SEARCH_RESULT: {
        auto dEvent = static_cast<const UpnpDiscovery*>(event);
        const char* userAgent = UpnpDiscovery_get_Os_cstr(dEvent);
        const struct sockaddr_storage* destAddr = UpnpDiscovery_get_DestAddr(dEvent);
        const char* location = UpnpDiscovery_get_Location_cstr(dEvent);

        clients->addClientByDiscovery(destAddr, userAgent, location);
        break;
    }
    default:
        break;
    }

    return 0;
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
        // log_debug("routeActionRequest: request for content directory service");
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
        log_debug("routeSubscriptionRequest: request not for this device: {} vs {}", request->getUDN(), serverUDN);
        throw UpnpException(UPNP_E_BAD_REQUEST, "routeActionRequest: request not for this device");
    }

    // we need to match the serviceID to one of our services
    if (request->getServiceID() == UPNP_DESC_CDS_SERVICE_ID) {
        // this call is for the content directory service
        // log_debug("routeSubscriptionRequest: request for content directory service");
        cds->processSubscriptionRequest(request);
    } else if (request->getServiceID() == UPNP_DESC_CM_SERVICE_ID) {
        // this call is for the connection manager service
        // log_debug("routeSubscriptionRequest: request for connection manager service");
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

    if (startswith(link, fmt::format("/{}/{}", SERVER_VIRTUAL_DIR, CONTENT_MEDIA_HANDLER))) {
        return std::make_unique<FileRequestHandler>(content, xmlbuilder);
    }

    if (startswith(link, fmt::format("/{}/{}", SERVER_VIRTUAL_DIR, CONTENT_UI_HANDLER))) {
        auto&& [path, parameters] = RequestHandler::splitUrl(filename, URL_UI_PARAM_SEPARATOR);

        auto params = dictDecode(parameters);

        auto it = params.find(URL_REQUEST_TYPE);
        std::string rType = it != params.end() && !it->second.empty() ? it->second : "index";

        return Web::createWebRequestHandler(context, content, xmlbuilder, rType);
    }

    if (startswith(link, fmt::format("/{}/{}", SERVER_VIRTUAL_DIR, DEVICE_DESCRIPTION_PATH))) {
        return std::make_unique<DeviceDescriptionHandler>(content, xmlbuilder.get());
    }

#if defined(HAVE_CURL)
    if (startswith(link, fmt::format("/{}/{}", SERVER_VIRTUAL_DIR, CONTENT_ONLINE_HANDLER))) {
        return std::make_unique<URLRequestHandler>(content);
    }
#endif

    throw_std_runtime_error("No valid handler type in {}", filename);
}

int Server::registerVirtualDirCallbacks()
{
    log_debug("Setting UpnpVirtualDir GetInfoCallback");
    int ret = UpnpVirtualDir_set_GetInfoCallback([](const char* filename, UpnpFileInfo* info, const void* cookie, const void** requestCookie) -> int {
        try {
            auto reqHandler = static_cast<const Server*>(cookie)->createRequestHandler(filename);
            std::string link = urlUnescape(filename);
            reqHandler->getInfo(startswith(link, fmt::format("/{}/{}", SERVER_VIRTUAL_DIR, CONTENT_UI_HANDLER)) ? filename : link.c_str(), info);
            *requestCookie = reinterpret_cast<const void**>(reqHandler.release());
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
    if (ret != UPNP_E_SUCCESS)
        return ret;

    log_debug("Setting UpnpVirtualDir OpenCallback");
    ret = UpnpVirtualDir_set_OpenCallback([](const char* filename, enum UpnpOpenFileMode mode, const void* cookie, const void* requestCookie) -> UpnpWebFileHandle {
        try {
            auto reqHandler = std::unique_ptr<RequestHandler>(static_cast<RequestHandler*>(const_cast<void*>(requestCookie)));
            std::string link = urlUnescape(filename);
            auto ioHandler = reqHandler->open(startswith(link, fmt::format("/{}/{}", SERVER_VIRTUAL_DIR, CONTENT_UI_HANDLER)) ? filename : link.c_str(), mode);
            return ioHandler.release();
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
    ret = UpnpVirtualDir_set_ReadCallback([](UpnpWebFileHandle f, char* buf, std::size_t length, const void* cookie, const void* requestCookie) -> int {
        // log_debug("{} read({})", f, length);
        if (static_cast<const Server*>(cookie)->getShutdownStatus())
            return -1;

        auto handler = static_cast<IOHandler*>(f);
        return handler->read(buf, length);
    });
    if (ret != UPNP_E_SUCCESS)
        return ret;

    log_debug("Setting UpnpVirtualDir WriteCallback");
    ret = UpnpVirtualDir_set_WriteCallback([](UpnpWebFileHandle f, char* buf, std::size_t length, const void* cookie, const void* requestCookie) -> int {
        // log_debug("{} write({})", f, length);
        return 0;
    });
    if (ret != UPNP_E_SUCCESS)
        return ret;

    log_debug("Setting UpnpVirtualDir SeekCallback");
    ret = UpnpVirtualDir_set_SeekCallback([](UpnpWebFileHandle f, off_t offset, int whence, const void* cookie, const void* requestCookie) -> int {
        // log_debug("{} seek({}, {})", f, offset, whence);
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
        // log_debug("{} close()", f);
        int retClose = 0;
        auto handler = std::unique_ptr<IOHandler>(static_cast<IOHandler*>(f));
        try {
            handler->close();
        } catch (const std::runtime_error& e) {
            log_error("Exception during close: {}", e.what());
            retClose = -1;
        }

        return retClose;
    });

    return ret;
}
