/*MT*

    MediaTomb - http://www.mediatomb.cc/

    main.cc - this file is part of MediaTomb.

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

/// \file main.cc

/// \mainpage Sourcecode Documentation.
///
/// This documentation was generated using doxygen, you can reproduce it by
/// running "doxygen doxygen.conf" from the mediatomb/doc/ directory.

#define LOG_FAC log_facility_t::server

#include "common.h"
#include "config/config_manager.h"
#include "config/grb_runtime.h"
#include "contrib/cxxopts.hpp"
#include "server.h"
#include "upnp/conn_mgr_service.h"
#include "upnp/cont_dir_service.h"
#include "upnp/mr_reg_service.h"

#include <csignal>
#include <fmt/core.h>
#include <mutex>
#include <vector>

#include <upnpconfig.h>
#ifdef UPNP_HAVE_TOOLS
#include <upnptools.h>
#endif

#ifdef SOLARIS
#include <iso/limits_iso.h>
#endif

#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

static struct {
    int shutdownFlag = 0;
    int restartFlag = 0;
    pthread_t mainThreadId;

    mutable std::mutex mutex;
    std::unique_lock<std::mutex> lock { mutex };
    std::condition_variable cond;
} _ctx;

static GerberaRuntime runtime;

static void signalHandler(int signum)
{
    if (_ctx.mainThreadId != pthread_self()) {
        return;
    }

    log_debug("Got a SIG: {}", signum);

    if (signum == SIGCHLD) {
        log_debug("Got a SIGCHLD!");
        int savedErrno = errno;
        while (waitpid(static_cast<pid_t>(-1), nullptr, WNOHANG) > 0) { }
        errno = savedErrno;
        return;
    }

    if (signum == SIGSEGV) {
        log_error("This should never happen {}:{}, killing Gerbera!", errno, std::strerror(errno));
        runtime.exit(EXIT_FAILURE);
    }

    if ((signum == SIGINT) || (signum == SIGTERM)) {
        _ctx.shutdownFlag++;
        if (_ctx.shutdownFlag == 1) {
            log_info("Gerbera shutting down. Please wait...");
        } else if (_ctx.shutdownFlag == 2) {
            log_info("Gerbera still shutting down, signal again to kill.");
        } else if (_ctx.shutdownFlag > 2) {
            log_error("Clean shutdown failed, killing Gerbera!");
            runtime.exit(EXIT_FAILURE);
        }
    } else if (signum == SIGHUP) {
        _ctx.restartFlag = 1;
    }

    _ctx.cond.notify_one();
}

static void installSignalHandler()
{
    _ctx.mainThreadId = pthread_self();

    struct sigaction action = {};
    action.sa_handler = signalHandler;
    action.sa_flags = SA_NOCLDSTOP;
    sigfillset(&action.sa_mask);
    if (sigaction(SIGSEGV, &action, nullptr) < 0) {
        log_error("Could not register SIGSEGV handler!");
    }

    if (sigaction(SIGINT, &action, nullptr) < 0) {
        log_error("Could not register SIGINT handler!");
    }

    if (sigaction(SIGTERM, &action, nullptr) < 0) {
        log_error("Could not register SIGTERM handler!");
    }

    if (sigaction(SIGHUP, &action, nullptr) < 0) {
        log_error("Could not register SIGHUP handler!");
    }

    if (sigaction(SIGPIPE, &action, nullptr) < 0) {
        log_error("Could not register SIGPIPE handler!");
    }

    if (sigaction(SIGCHLD, &action, nullptr) < 0) {
        log_error("Could not register SIGCHLD handler!");
    }
}

int main(int argc, char** argv, char** envp)
{
    cxxopts::Options options("gerbera", "Gerbera UPnP Media Server - https://gerbera.io");
    runtime.init(&options);
    std::optional<std::string> magic = runtime.getMagic();
    const auto additionalArgs = std::vector<ConfigOptionArgs> {
        { CFG_SERVER_PORT, "p", "port", "Port to bind with, must be >=49152", std::optional<std::string>(), "PORT" },
        { CFG_SERVER_IP, "i", "ip", "IP to bind with", std::optional<std::string>(), "IP" },
        { CFG_SERVER_NETWORK_INTERFACE, "e", "interface", "Interface to bind with", std::optional<std::string>(), "IF" },
        { CFG_SERVER_HOME, "m", "home", "Search this directory for a .config/gerbera folder containing a config file", std::optional<std::string>(), "DIR" },
#ifdef HAVE_MAGIC
        { CFG_IMPORT_MAGIC_FILE, "", "magic", "Set magic FILE", magic, "FILE" },
#endif
        { CFG_IMPORT_LAYOUT_MODE, "", "import-mode", "Activate import MODE ('mt' or 'grb')", std::optional<std::string>(), "MODE" },
#ifdef GRBDEBUG
        { CFG_SERVER_DEBUG_MODE, "", "debug-mode", "Set debugging mode to FACILITY", std::optional<std::string>(), "FACILITY" },
#endif
    };

    options.add_options() //
        ("h,help", "Print this help and exit") //
        ("D,debug", "Enable debugging", cxxopts::value<bool>()->default_value("false")) //
        ("v,version", "Print version info and exit") //
        ("compile-info", "Print compile info and exit") //
        ("create-config", "Print a default config.xml file and exit") //
        ("create-example-config", "Print a example config.xml file and exit") //
        ("check-config", "Check config.xml file and exit") //
        ("print-options", "Print simple config options and exit") //
        ("offline", "Do not answer UPnP content requests") // good for initial scans
        ("d,daemon", "Daemonize after startup", cxxopts::value<bool>()->default_value("false")) //
        ("u,user", "Drop privs to user UID", cxxopts::value<std::string>(), "UID") //
        ("P,pidfile", "Write a pidfile to the specified location, e.g. /run/gerbera.pid", cxxopts::value<fs::path>(), "FILE") //
        ("c,config", "Path to config file", cxxopts::value<fs::path>(), "DIR") //
        ("f,cfgdir", "Override name of config folder (.config/gerbera) in home folder.", cxxopts::value<fs::path>(), "DIR") //
        ("l,logfile", "Set log location", cxxopts::value<fs::path>(), "FILE") //
        ("add-file", "Scan a file into the DB on startup, can be specified multiple times", cxxopts::value<std::vector<fs::path>>(), "FILE") //
        ("set-option", "Set simple config option OPT to value VAL, can be specified multiple times use --print-options for value OPTs", cxxopts::value<std::vector<std::string>>(), "OPT=VAL") //
        ;

    try {
        for (auto&& addArg : additionalArgs) {
            auto value = cxxopts::value<std::vector<std::string>>();
            if (addArg.defaultValue) {
                log_debug("defValue {} = {}", addArg.optLong, addArg.defaultValue.value());
                value->default_value(addArg.defaultValue.value());
            }
            options.add_option("", addArg.optShort, addArg.optLong, addArg.desc, value, addArg.argDesc);
        }
        cxxopts::ParseResult results = options.parse(argc, argv);
        runtime.handleOptions(&results);

        std::shared_ptr<ConfigManager> configManager;
        int portnum = -1;
        try {
            configManager = std::make_shared<ConfigManager>(
                runtime.getConfigFile(), runtime.getHome(), runtime.getConfDir(),
                runtime.getDataDir(), runtime.getDebug());
            configManager->load(runtime.getHome());
            runtime.handleConfigOptions(configManager, additionalArgs);
            configManager->validate();
#ifdef GRBDEBUG
            GrbLogger::Logger.init(configManager->getIntOption(CFG_SERVER_DEBUG_MODE));
#endif
            if (results.count("check-config") > 0) {
                runtime.exit(EXIT_SUCCESS);
            }
            portnum = configManager->getUIntOption(CFG_SERVER_PORT);
        } catch (const ConfigParseException& ce) {
            log_error("Error parsing config file '{}': {}", runtime.getConfigFile().c_str(), ce.what());
            runtime.exit(EXIT_FAILURE);
        } catch (const std::runtime_error& e) {
            log_error("{}", e.what());
            runtime.exit(EXIT_FAILURE);
        }

        sigset_t maskSet;
        sigfillset(&maskSet);
        pthread_sigmask(SIG_SETMASK, &maskSet, nullptr);

        installSignalHandler();

        auto server = std::make_shared<Server>(configManager);

        try {
            server->init(runtime.getOffline());
            server->run();
        } catch (const UpnpException& ue) {
            log_error("{}", ue.what());

            sigemptyset(&maskSet);
            pthread_sigmask(SIG_SETMASK, &maskSet, nullptr);

            if (ue.getErrorCode() == UPNP_E_SOCKET_BIND) {
                log_error("LibUPnP could not bind to socket");
                log_error("Please check if another instance of Gerbera or another application is running on port TCP {} or UDP 1900.", portnum);
            } else if (ue.getErrorCode() == UPNP_E_SOCKET_ERROR) {
                log_error("LibUPnP Socket error");
                log_error("Please check if your network interface was configured for multicast!");
            } else if (ue.getErrorCode() == UPNP_E_SUCCESS) {
                /* This means we died, not libupnp explicitly, likely because they gave us the wrong port */
                log_error(ue.what());
            } else {
#ifdef UPNP_HAVE_TOOLS
                log_error("Failed to start LibUPnP: {} error code: {}", UpnpGetErrorMessage(ue.getErrorCode()), ue.getErrorCode());
#else
                log_error("Failed to start LibUPnP: error code: {}", ue.getErrorCode());
#endif
            }

            try {
                if (server)
                    server->shutdown();
                server.reset();
                configManager.reset();
            } catch (const std::runtime_error& e) {
                log_error("{}", e.what());
            }

            runtime.exit(EXIT_FAILURE);
        } catch (const std::runtime_error& e) {
            log_error("{}", e.what());
            runtime.exit(EXIT_FAILURE);
        }

        runtime.handleServerOptions(server);

        sigemptyset(&maskSet);
        pthread_sigmask(SIG_SETMASK, &maskSet, nullptr);

        // wait until signalled to terminate
        while (!_ctx.shutdownFlag) {
            _ctx.cond.wait(_ctx.lock);

            if (_ctx.restartFlag != 0) {
                log_info("Restarting Gerbera!");
                try {
                    server->shutdown();
                    server.reset();
                    configManager.reset();

                    GerberaRuntime runtimeChild(&options);

                    installSignalHandler();
                    try {
                        runtimeChild.handleOptions(&results, false);
                        configManager = std::make_shared<ConfigManager>(
                            runtimeChild.getConfigFile(), runtimeChild.getHome(), runtimeChild.getConfDir(),
                            runtimeChild.getDataDir(), runtimeChild.getDebug());
                        configManager->load(runtimeChild.getHome());

                        runtimeChild.handleConfigOptions(configManager, additionalArgs);
                        configManager->validate();
#ifdef GRBDEBUG
                        GrbLogger::Logger.init(configManager->getIntOption(CFG_SERVER_DEBUG_MODE));
#endif
                        portnum = configManager->getUIntOption(CFG_SERVER_PORT);
                    } catch (const ConfigParseException& ce) {
                        log_error("Error parsing config file '{}': {}", runtimeChild.getConfigFile().c_str(), ce.what());
                        log_error("Could not restart Gerbera");
                        // at this point upnp shutdown has already been called
                        // therefore it is safe to exit
                        runtime.exit(EXIT_FAILURE);
                    } catch (const std::runtime_error& e) {
                        log_error("Error reloading configuration: {}", e.what());
                        runtime.exit(EXIT_FAILURE);
                    }

                    server = std::make_shared<Server>(configManager);
                    server->init(runtimeChild.getOffline());
                    server->run();

                    _ctx.restartFlag = 0;
                } catch (const std::runtime_error& e) {
                    _ctx.restartFlag = 0;
                    _ctx.shutdownFlag = 1;
                    sigemptyset(&maskSet);
                    pthread_sigmask(SIG_SETMASK, &maskSet, nullptr);
                    log_error("Could not restart Gerbera");
                }
            }
        }

        // shutting down
        int ret = EXIT_SUCCESS;
        try {
            server->shutdown();
            server.reset();
            configManager.reset();
        } catch (const UpnpException& upnpE) {
            log_error("main: upnp error {}", upnpE.getErrorCode());
            ret = EXIT_FAILURE;
        } catch (const std::runtime_error& e) {
            log_error("main: error {}", e.what());
            ret = EXIT_FAILURE;
        }

        log_info("Gerbera exiting. Have a nice day.");
        runtime.exit(ret);
    } catch (const cxxopts::OptionException& e) {
        fmt::print(stderr, "Failed to parse arguments: {}\n", e.what());
        runtime.exit(EXIT_FAILURE);
    }
}
