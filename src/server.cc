/*MT*

    MediaTomb - http://www.mediatomb.cc/

    server.cc - this file is part of MediaTomb.

    Copyright (C) 2005 Gena Batyan <bgeradz@mediatomb.cc>,
                       Sergey 'Jin' Bostandzhyan <jin@mediatomb.cc>

    Copyright (C) 2006-2010 Gena Batyan <bgeradz@mediatomb.cc>,
                            Sergey 'Jin' Bostandzhyan <jin@mediatomb.cc>,
                            Leonhard Wimmer <leo@mediatomb.cc>

    Copyright (C) 2016-2024 Gerbera Contributors

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
#define GRB_LOG_FAC GrbLogFacility::server

#include "server.h" // API

#include "action_request.h"
#include "config/config.h"
#include "config/config_option_enum.h"
#include "content/content_manager.h"
#include "database/database.h"
#include "device_description_handler.h"
#include "exceptions.h"
#include "file_request_handler.h"
#include "iohandler/io_handler.h"
#include "metadata/metadata_service.h"
#include "request_handler.h"
#include "subscription_request.h"
#include "ui_handler.h"
#include "upnp/clients.h"
#include "upnp/conn_mgr_service.h"
#include "upnp/cont_dir_service.h"
#include "upnp/mr_reg_service.h"
#include "upnp/upnp_common.h"
#include "upnp/xml_builder.h"
#include "upnp_desc_handler.h"
#include "util/grb_net.h"
#include "util/mime.h"
#include "util/tools.h"
#include "util/url_utils.h"
#include "util/xml_to_json.h"
#include "web/pages.h"
#include "web/session_manager.h"

#ifdef HAVE_CURL
#include "url_request_handler.h"
#endif

#ifdef HAVE_INOTIFY
#include "content/inotify/autoscan_inotify.h"
#endif

#ifdef ONLINE_SERVICES
#include "content/onlineservice/online_service.h"
#endif

#ifdef HAVE_CURL
#include <curl/curl.h>
#endif

#include <upnpconfig.h>
#ifdef UPNP_HAVE_TOOLS
#include <upnptools.h>
#endif

#include <sstream>
#include <thread>

constexpr auto DEVICE_DESCRIPTION_PATH = std::string_view(UPNP_DESC_SCPD_URL UPNP_DESC_DEVICE_DESCRIPTION);

Server::Server(std::shared_ptr<Config> config)
    : config(std::move(config))
{
}

void Server::init(bool offln)
{
    offline = offln;

    serverUDN = config->getOption(CFG_SERVER_UDN);
    aliveAdvertisementInterval = config->getIntOption(CFG_SERVER_ALIVE_INTERVAL);

#ifdef HAVE_CURL
    curl_global_init(CURL_GLOBAL_ALL);
#endif

    // initialize what is needed
    auto self = shared_from_this();
    timer = std::make_shared<Timer>();
    timer->run();

    mime = std::make_shared<Mime>(config);
    database = Database::createInstance(config, mime, timer);
    config->updateConfigFromDatabase(database);

    clientManager = std::make_shared<ClientManager>(config, database);
    sessionManager = std::make_shared<Web::SessionManager>(config, timer);
    context = std::make_shared<Context>(config, clientManager, mime, database, sessionManager);

    content = std::make_shared<ContentManager>(context, self, timer);
    metadataService = std::make_shared<MetadataService>(context, content);
}

struct UpnpDesc {
    enum Upnp_DescType_e mode;
    std::string desc;
    int len;
};

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
        iface = GrbNet::ipToInterface(ip);

    if (!ip.empty() && iface.empty())
        throw_std_runtime_error("Could not find IP: {}", ip);

    // check webroot directory
    std::string webRoot = config->getOption(CFG_SERVER_WEBROOT);
    if (webRoot.empty()) {
        throw_std_runtime_error("Invalid web server root directory {}", webRoot);
    }

    auto configPort = static_cast<in_port_t>(config->getUIntOption(CFG_SERVER_PORT));
    auto ret = startupInterface(iface, configPort);
    if (ret != UPNP_E_SUCCESS) {
        throw UpnpException(ret, fmt::format("UpnpInit failed {} {}", iface, port));
    }

    if (configPort > 0 && port != configPort) {
        throw UpnpException(ret, fmt::format("LibUPnP failed to bind to request port! Bound to {}, requested: {}", port, configPort));
    }

    log_debug("webroot: {}", webRoot);
    log_debug("Adding root virtual dir");
    ret = UpnpAddVirtualDir("/", this, nullptr);
    if (ret != UPNP_E_SUCCESS) {
        throw UpnpException(ret, "run: UpnpAddVirtualDir failed!");
    }

    ret = registerVirtualDirCallbacks();
    if (ret != UPNP_E_SUCCESS) {
        throw UpnpException(ret, "run: UpnpSetVirtualDirCallbacks failed");
    }

    log_debug("Creating UpnpXMLBuilders");
    upnpXmlBuilder = std::make_shared<UpnpXMLBuilder>(context, getVirtualUrl());
    webXmlBuilder = std::make_shared<UpnpXMLBuilder>(context, getExternalUrl());
    auto devDescHdl = std::make_shared<DeviceDescriptionHandler>(content, webXmlBuilder, ip, port);

    int activeUpnpDescription = config->getBoolOption(CFG_UPNP_DYNAMIC_DESCRIPTION) ? 0 : 1;
    // register root device with the library
    auto upnpDesc = std::vector<UpnpDesc> {
        { UPNPREG_URL_DESC, fmt::format("http://{}{}", GrbNet::renderWebUri(ip, port), DEVICE_DESCRIPTION_PATH), -1 },
        { UPNPREG_BUF_DESC, devDescHdl->renderDeviceDescription(ip, port, nullptr), 0 }
    };
    log_debug("Registering with UPnP... ({})", upnpDesc[activeUpnpDescription].desc);
    ret = UpnpRegisterRootDevice2(
        upnpDesc[activeUpnpDescription].mode,
        upnpDesc[activeUpnpDescription].desc.c_str(),
        upnpDesc[activeUpnpDescription].len >= 0 ? upnpDesc[activeUpnpDescription].desc.length() + 1 : upnpDesc[activeUpnpDescription].len,
        false,
        [](auto eventType, auto event, auto cookie) { return static_cast<Server*>(cookie)->handleUpnpRootDeviceEvent(eventType, event); },
        this,
        &rootDeviceHandle);
    if (ret != UPNP_E_SUCCESS) {
        throw UpnpException(ret, "run: UpnpRegisterRootDevice2 failed");
    }

    log_debug("Registering UPnP client...");
    ret = UpnpRegisterClient(
        [](auto eventType, auto event, auto cookie) { return static_cast<Server*>(cookie)->handleUpnpClientEvent(eventType, event); },
        this,
        &clientHandle);
    if (ret != UPNP_E_SUCCESS) {
        throw UpnpException(ret, "run: UpnpRegisterClient failed");
    }

    log_debug("Creating ContentDirectoryService");
    cds = std::make_unique<ContentDirectoryService>(context, upnpXmlBuilder, rootDeviceHandle,
        config->getIntOption(CFG_SERVER_UPNP_TITLE_AND_DESC_STRING_LIMIT));

    log_debug("Creating ConnectionManagerService");
    cmgr = std::make_unique<ConnectionManagerService>(context, upnpXmlBuilder, rootDeviceHandle);

    log_debug("Creating MRRegistrarService");
    mrreg = std::make_unique<MRRegistrarService>(context, upnpXmlBuilder, rootDeviceHandle);

    // The advertisement will be sent by LibUPnP every (A/2)-30 seconds, and will have a cache-control max-age of A where A is
    // the value configured here. Ex: A value of 62 will result in an SSDP advertisement being sent every second.
    log_debug("Will send UPnP Alive advertisements every {} seconds", (aliveAdvertisementInterval / 2) - 30);
    ret = UpnpSendAdvertisement(rootDeviceHandle, aliveAdvertisementInterval);
    if (ret != UPNP_E_SUCCESS) {
        throw UpnpException(ret, fmt::format("run: UpnpSendAdvertisement {} failed", aliveAdvertisementInterval));
    }

#if !defined(USING_NPUPNP)
    if (config->getBoolOption(CFG_UPNP_LITERAL_HOST_REDIRECTION))
        UpnpSetAllowLiteralHostRedirection(1);
#endif

    std::string url = getVirtualUrl();
    writeBookmark(url);
    log_info("The Web UI can be reached by following this link: {}", url);

    validHosts = std::vector<std::string> {
        std::string(UpnpGetServerIpAddress()),
        std::string(UpnpGetServerIp6Address()),
        std::string(UpnpGetServerUlaGuaIp6Address()),
    };
    if (!url.empty()) {
        validHosts.push_back(url);
    }
    url = getExternalUrl();
    if (!url.empty()) {
        validHosts.push_back(url);
    }

    UpnpSetHostValidateCallback(
        [](auto host, auto cookie) -> int {
            auto hostStr = std::string(host);
            for (auto&& name : static_cast<Server*>(cookie)->validHosts) {
                if (hostStr.find(name) != std::string::npos) {
                    return UPNP_E_SUCCESS;
                }
                if (name.find(hostStr) != std::string::npos) {
                    return UPNP_E_SUCCESS;
                }
            }

            log_warning("Rejected attempt to load host '{}' as it does not match configured virtualURL/externalURL: '{}'/'{}'. "
                        "See https://docs.gerbera.io/en/stable/config-server.html#virtualurl",
                host, static_cast<Server*>(cookie)->config->getOption(CFG_VIRTUAL_URL),
                static_cast<Server*>(cookie)->config->getOption(CFG_EXTERNAL_URL));
            return UPNP_E_BAD_HTTPMSG;
        },
        this);
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

    return ret;
}

void Server::writeBookmark(const std::string& addr)
{
    const std::string data = config->getBoolOption(CFG_SERVER_UI_ENABLED)
        ? httpRedirectTo(addr)
        : httpRedirectTo(addr, "disabled.html");

    fs::path path = config->getOption(CFG_SERVER_BOOKMARK_FILE);
    log_debug("Writing bookmark file to: {}", path.c_str());
    GrbFile(path).writeTextFile(data);
}

void Server::emptyBookmark()
{
    const std::string_view data = "<html><body><h1>Gerbera Media Server is not running.</h1><p>Please start it and try again.</p></body></html>";

    fs::path path = config->getOption(CFG_SERVER_BOOKMARK_FILE);
    log_debug("Clearing bookmark file at: {}", path.c_str());
    GrbFile(path).writeTextFile(data);
}

std::string Server::getVirtualUrl() const
{
    auto virtUrl = config->getOption(CFG_VIRTUAL_URL);
    if (virtUrl.empty()) {
        virtUrl = GrbNet::renderWebUri(ip, port);
    }
    if (!startswith(virtUrl, "http")) { // url does not start with http
        virtUrl = fmt::format("http://{}", virtUrl);
    }
    if (virtUrl.back() == '/')
        virtUrl.pop_back();
    return virtUrl;
}

std::string Server::getExternalUrl() const
{
    auto virtUrl = config->getOption(CFG_EXTERNAL_URL);
    if (virtUrl.empty()) {
        virtUrl = config->getOption(CFG_VIRTUAL_URL);
        if (virtUrl.empty()) {
            virtUrl = GrbNet::renderWebUri(ip, port);
        }
    }
    if (!startswith(virtUrl, "http")) { // url does not start with http
        virtUrl = fmt::format("http://{}", virtUrl);
    }
    if (virtUrl.back() == '/')
        virtUrl.pop_back();
    return virtUrl;
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
    UpnpSetHostValidateCallback(nullptr, nullptr);

#ifdef HAVE_CURL
    curl_global_cleanup();
#endif

    log_debug("now calling upnp finish");
    ret = UpnpFinish();
    if (ret != UPNP_E_SUCCESS) {
        log_error("UpnpFinish failed ({})", ret);
    }

    if (content) {
        content->shutdown();
        content.reset();
    }

    sessionManager.reset();

    if (database->threadCleanupRequired()) {
        try {
            database->threadCleanup();
        } catch (const std::runtime_error& ex) {
        }
    }
    database->shutdown();
    database.reset();

    timer->shutdown();
    timer.reset();
    context.reset();
    mime.reset();
    clientManager.reset();
    metadataService.reset();
    upnpXmlBuilder.reset();
    webXmlBuilder.reset();
    cds.reset();
    cmgr.reset();
    mrreg.reset();

    log_debug("Server down");
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
            auto request = ActionRequest(upnpXmlBuilder, clientManager, const_cast<UpnpActionRequest*>(static_cast<const UpnpActionRequest*>(event)));
            routeActionRequest(request);
            request.update();
        } catch (const UpnpException& upnpE) {
            ret = upnpE.getErrorCode();
            UpnpActionRequest_set_ErrCode(const_cast<UpnpActionRequest*>(static_cast<const UpnpActionRequest*>(event)), ret);
        } catch (const std::runtime_error& e) {
            log_info("Exception: {}", e.what());
        }
        break;

    case UPNP_EVENT_SUBSCRIPTION_REQUEST:
        log_debug("UPNP_EVENT_SUBSCRIPTION_REQUEST");
        try {
            auto request = SubscriptionRequest(const_cast<UpnpSubscriptionRequest*>(static_cast<const UpnpSubscriptionRequest*>(event)));
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
        auto destAddr = std::make_shared<GrbNet>(UpnpDiscovery_get_DestAddr(dEvent));
        const char* location = UpnpDiscovery_get_Location_cstr(dEvent);

        clientManager->addClientByDiscovery(destAddr, userAgent, location);
        break;
    }
    default:
        break;
    }

    return 0;
}

void Server::routeActionRequest(ActionRequest& request)
{
    log_debug("start");

    // make sure the request is for our device
    if (request.getUDN() != serverUDN) {
        // not for us
        throw UpnpException(UPNP_E_BAD_REQUEST, "routeActionRequest: request not for this device");
    }

    // we need to match the serviceID to one of our services
    if (request.getServiceID() == UPNP_DESC_CM_SERVICE_ID) {
        // this call is for the lifetime stats service
        log_debug("request for connection manager service");
        cmgr->processActionRequest(request);
    } else if (request.getServiceID() == UPNP_DESC_CDS_SERVICE_ID) {
        // this call is for the toaster control service
        log_debug("request for content directory service");
        if (!offline)
            cds->processActionRequest(request);
        else
            request.setErrorCode(UPNP_E_INVALID_ACTION);

    } else if (request.getServiceID() == UPNP_DESC_MRREG_SERVICE_ID) {
        log_debug("request for MR Registrar service");
        mrreg->processActionRequest(request);
    } else {
        // cp is asking for a nonexistent service, or for a service
        // that does not support any actions
        log_debug("Service {} does not exist or action not supported", request.getServiceID());
        throw UpnpException(UPNP_E_BAD_REQUEST, "Service does not exist or action not supported");
    }
}

void Server::routeSubscriptionRequest(const SubscriptionRequest& request) const
{
    // make sure that the request is for our device
    if (request.getUDN() != serverUDN) {
        // not for us
        log_debug("request not for this device: {} vs {}", request.getUDN(), serverUDN);
        throw UpnpException(UPNP_E_BAD_REQUEST, "routeSubscriptionRequest: request not for this device");
    }

    // we need to match the serviceID to one of our services
    if (request.getServiceID() == UPNP_DESC_CDS_SERVICE_ID) {
        // this call is for the content directory service
        log_debug("request for content directory service");
        if (!offline)
            cds->processSubscriptionRequest(request);
    } else if (request.getServiceID() == UPNP_DESC_CM_SERVICE_ID) {
        // this call is for the connection manager service
        log_debug("request for connection manager service");
        cmgr->processSubscriptionRequest(request);
    } else if (request.getServiceID() == UPNP_DESC_MRREG_SERVICE_ID) {
        log_debug("request for MRRegistrar service");
        mrreg->processSubscriptionRequest(request);
    } else {
        // cp asks for a nonexistent service or for a service that
        // does not support subscriptions
        log_debug("Service {} does not exist or action not supported", request.getServiceID());
        throw UpnpException(UPNP_E_BAD_REQUEST, "Service does not exist or subscriptions not supported");
    }
}

// Temp
void Server::sendCDSSubscriptionUpdate(const std::string& updateString)
{
    if (!offline)
        cds->sendSubscriptionUpdate(updateString);
}

std::unique_ptr<RequestHandler> Server::createRequestHandler(const char* filename) const
{
    std::string link = URLUtils::urlUnescape(filename);
    log_debug("Filename: {}", filename);

    if (startswith(link, fmt::format("/{}", CONTENT_MEDIA_HANDLER))) {
        return std::make_unique<FileRequestHandler>(content, upnpXmlBuilder, metadataService);
    }

    if (startswith(link, fmt::format("/{}", CONTENT_UI_HANDLER))) {
        auto&& [path, parameters] = URLUtils::splitUrl(filename, URL_UI_PARAM_SEPARATOR);

        auto params = URLUtils::dictDecode(parameters);

        auto it = params.find(URL_REQUEST_TYPE);
        std::string rType = it != params.end() && !it->second.empty() ? it->second : "index";

        return Web::createWebRequestHandler(context, content, webXmlBuilder, rType);
    }

    if (startswith(link, DEVICE_DESCRIPTION_PATH) || endswith(link, UPNP_DESC_DEVICE_DESCRIPTION)) {
        return std::make_unique<DeviceDescriptionHandler>(content, upnpXmlBuilder, ip, port);
    }

    if (startswith(link, UPNP_DESC_SCPD_URL) || endswith(link, UPNP_DESC_CDS_SCPD_URL) || endswith(link, UPNP_DESC_CM_SCPD_URL) || endswith(link, UPNP_DESC_MRREG_SCPD_URL)) {
        return std::make_unique<UpnpDescHandler>(content, upnpXmlBuilder);
    }

    if (link == "/" || startswith(link, "/index.html")
        || startswith(link, "/favicon.ico")
        || startswith(link, "/assets")
        || startswith(link, "/vendor")
        || startswith(link, "/js")
        || startswith(link, "/css")
        || startswith(link, "/icons")
        || startswith(link, "/gerbera-config-")) {
        return std::make_unique<UIHandler>(content, upnpXmlBuilder);
    }

#if defined(HAVE_CURL)
    if (startswith(link, fmt::format("/{}", CONTENT_ONLINE_HANDLER))) {
        return std::make_unique<URLRequestHandler>(content, upnpXmlBuilder);
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
            std::string link = URLUtils::urlUnescape(filename);
            *requestCookie = reqHandler->getInfo(startswith(link, fmt::format("/{}", CONTENT_UI_HANDLER)) ? filename : link.c_str(), info);
            return 0;
        } catch (const ServerShutdownException&) {
            return -1;
        } catch (const SubtitlesNotFoundException& stex) {
            log_warning("SubtitlesNotFoundException: {}", stex.what());
            return -1;
        } catch (const std::runtime_error& e) {
            log_error("Exception: {}", e.what());
            return -1;
        }
    });
    if (ret != UPNP_E_SUCCESS)
        return ret;

    log_debug("Setting UpnpVirtualDir OpenCallback");
    ret = UpnpVirtualDir_set_OpenCallback([](const char* filename, enum UpnpOpenFileMode mode, const void* cookie, const void* requestCookie) -> UpnpWebFileHandle {
        try {
            auto client = requestCookie ? static_cast<const ClientInfo*>(requestCookie) : nullptr;
            auto quirks = client ? std::make_shared<Quirks>(client) : nullptr;
            auto reqHandler = static_cast<const Server*>(cookie)->createRequestHandler(filename);
            std::string link = URLUtils::urlUnescape(filename);
            auto ioHandler = reqHandler->open(startswith(link, fmt::format("/{}", CONTENT_UI_HANDLER)) ? filename : link.c_str(), quirks, mode);
            if (ioHandler) {
                ioHandler->open(mode);
                return ioHandler.release();
            }
            log_warning("No Handler for {}", link);
            return nullptr;
        } catch (const ServerShutdownException&) {
            return nullptr;
        } catch (const SubtitlesNotFoundException& stex) {
            log_warning("SubtitlesNotFoundException: {}", stex.what());
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

        auto ioHandler = static_cast<IOHandler*>(f);
        return ioHandler ? ioHandler->read(reinterpret_cast<std::byte*>(buf), length) : 0;
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
            auto ioHandler = static_cast<IOHandler*>(f);
            if (ioHandler)
                ioHandler->seek(offset, whence);
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
        auto ioHandler = std::unique_ptr<IOHandler>(static_cast<IOHandler*>(f));
        if (ioHandler) {
            try {
                ioHandler->close();
            } catch (const std::runtime_error& e) {
                log_error("Exception during close: {}", e.what());
                retClose = -1;
            }
        }

        return retClose;
    });

    return ret;
}
