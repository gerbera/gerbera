/*MT*

    MediaTomb - http://www.mediatomb.cc/

    main.cc - this file is part of MediaTomb.

    Copyright (C) 2005 Gena Batyan <bgeradz@mediatomb.cc>,
                       Sergey 'Jin' Bostandzhyan <jin@mediatomb.cc>

    Copyright (C) 2006-2010 Gena Batyan <bgeradz@mediatomb.cc>,
                            Sergey 'Jin' Bostandzhyan <jin@mediatomb.cc>,
                            Leonhard Wimmer <leo@mediatomb.cc>

    Copyright (C) 2016-2025 Gerbera Contributors

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

/// @file main.cc

/// \mainpage Sourcecode Documentation.
///
/// This documentation was generated using doxygen, you can reproduce it by
/// setting BUILD_DOC="YES" for cmake and then
/// running "make doc" in your build directory.

#define GRB_LOG_FAC GrbLogFacility::server

#include "config/config_definition.h"
#include "config/config_manager.h"
#include "config/config_val.h"
#include "config/grb_runtime.h"
#include "contrib/cxxopts.hpp"
#include "exceptions.h"
#include "server.h"
#include "upnp/conn_mgr_service.h"
#include "upnp/cont_dir_service.h"
#include "upnp/mr_reg_service.h"
#include "util/logger.h"

#include <condition_variable>
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

#ifdef HAVE_EXECINFO
#include <execinfo.h>
#endif

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
#ifdef HAVE_EXECINFO
    if (signum == SIGSEGV || signum == SIGABRT || signum == SIGILL) {
        void* array[50];

        // get void*'s for all entries on the stack
        std::size_t size = backtrace(array, 50);
        // print out all the frames to stderr
        auto strings = backtrace_symbols(array, size);
        for (std::size_t i = 0; i < size; i++)
            log_error("{}\n", strings[i]);
    }
#endif

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

    if (signum == SIGINT || signum == SIGTERM) {
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

    sigset_t maskSet;
    sigfillset(&maskSet);
    if (pthread_sigmask(SIG_SETMASK, &maskSet, nullptr))
        log_error("Error installing masked signals {}", std::strerror(errno));

    struct sigaction action = { { 0 } };
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
    std::shared_ptr<ConfigDefinition> definition = std::make_shared<ConfigDefinition>();
    definition->init(definition);

    cxxopts::Options options(GerberaRuntime::ProgramName, "Gerbera UPnP Media Server - https://gerbera.io");
    runtime.init(definition, &options);
    std::optional<std::string> magic = runtime.getMagic();
    const auto additionalArgs = std::vector<ConfigOptionArgs> {
        { ConfigVal::SERVER_PORT, "p", "port", "Port to bind with, must be >=49152", std::optional<std::string>(), "PORT" },
        { ConfigVal::SERVER_IP, "i", "ip", "IP to bind with", std::optional<std::string>(), "IP" },
        { ConfigVal::SERVER_NETWORK_INTERFACE, "e", "interface", "Interface to bind with", std::optional<std::string>(), "IF" },
        { ConfigVal::SERVER_HOME, "m", GRB_OPTION_HOME, "Search this directory for a .config/gerbera folder containing a config file", std::optional<std::string>(), "DIR" },
#ifdef HAVE_MAGIC
        { ConfigVal::IMPORT_MAGIC_FILE, "", "magic", "Set magic FILE", magic, "FILE" },
#endif
        { ConfigVal::IMPORT_LAYOUT_MODE, "", "import-mode", "Activate import MODE ('mt' or 'grb')", std::optional<std::string>(), "MODE" },
#ifdef GRBDEBUG
        { ConfigVal::SERVER_LOG_DEBUG_MODE, "", "debug-mode", "Set debugging mode to FACILITY", std::optional<std::string>(), "FACILITY" },
#endif
    };

    options.add_options() //
        ("h," GRB_OPTION_HELP, "Print this help and exit") //
        ("D," GRB_OPTION_DEBUG, "Enable debugging", cxxopts::value<bool>()->default_value("false")) //
        ("v," GRB_OPTION_VERSION, "Print version info and exit") //
        (GRB_OPTION_COMPILEINFO, "Print compile info and exit") //
        (GRB_OPTION_CREATECONFIG, "Print a default config.xml file and exit", cxxopts::value<std::string>()->implicit_value("All"), "Section") //
        (GRB_OPTION_CREATEEXAMPLECONFIG, "Print a example config.xml file and exit", cxxopts::value<std::string>()->implicit_value("All"), "Section") //
        (GRB_OPTION_CREATEADVANCEDCONFIG, "Print a advanced example config.xml file and exit", cxxopts::value<std::string>()->implicit_value("All"), "Section") //
        (GRB_OPTION_CHECKCONFIG, "Check config.xml file and exit") //
        (GRB_OPTION_PRINTOPTIONS, "Print simple config options and exit") //
        (GRB_OPTION_OFFLINE, "Do not answer UPnP content requests", cxxopts::value<bool>()->default_value("false")) // good for initial scans
        (GRB_OPTION_DROPTABLES, "Drop all database tables and exit", cxxopts::value<bool>()->default_value("false")) //
#ifdef HAVE_LASTFM
#ifndef HAVE_LASTFMLIB
        (GRB_OPTION_INITLASTFM, "Get Last.FM session key", cxxopts::value<bool>()->default_value("false")) //
#endif
#endif
        ("d," GRB_OPTION_DAEMON, "Daemonize after startup", cxxopts::value<bool>()->default_value("false")) //
        ("u," GRB_OPTION_USER, "Drop privs to user UID", cxxopts::value<std::string>(), "UID") //
        ("P," GRB_OPTION_PIDFILE, "Write a pidfile to the specified location, e.g. /run/gerbera.pid", cxxopts::value<fs::path>(), "FILE") //
        ("c," GRB_OPTION_CONFIG, "Path to config file", cxxopts::value<fs::path>(), "FILE") //
        ("f," GRB_OPTION_CFGDIR, "Override name of config folder (.config/gerbera) in home folder", cxxopts::value<fs::path>(), "DIR") //
        ("l," GRB_OPTION_LOGFILE, "Set log location", cxxopts::value<fs::path>(), "FILE") //
        (GRB_OPTION_ROTATELOG, "Set rotating log location", cxxopts::value<fs::path>(), "FILE") //
        (GRB_OPTION_SYSLOG, "Log to syslog", cxxopts::value<std::string>()->implicit_value("USER"), "LOG") //
        (GRB_OPTION_ADDFILE, "Scan a file into the DB on startup, can be specified multiple times", cxxopts::value<std::vector<fs::path>>(), "FILE") //
        (GRB_OPTION_SETOPTION, "Set simple config option OPT to value VAL, can be specified multiple times use --print-options for value OPTs", cxxopts::value<std::vector<std::string>>(), "OPT=VAL") //
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
            runtime.notifySystemd(ServiceStatus::Reloading);
            configManager = std::make_shared<ConfigManager>(
                definition,
                runtime.getConfigFile(), runtime.getHome(), runtime.getConfDir(),
                runtime.getDataDir(), runtime.getDebug());
            configManager->load(runtime.getHome());
            runtime.handleConfigOptions(configManager, additionalArgs);
            configManager->validate();
#ifdef GRBDEBUG
            GrbLogger::Logger.init(configManager->getULongOption(ConfigVal::SERVER_LOG_DEBUG_MODE));
#endif
            if (results.count(GRB_OPTION_CHECKCONFIG) > 0) {
                runtime.exit(EXIT_SUCCESS);
            }
            portnum = configManager->getUIntOption(ConfigVal::SERVER_PORT);
        } catch (const ConfigParseException& ex) {
            log_error("Error parsing config file '{}': {}", runtime.getConfigFile().c_str(), ex.what());
            runtime.exit(EXIT_FAILURE);
        } catch (const std::runtime_error& ex) {
            log_error("Runtime error parsing config file '{}': {}", runtime.getConfigFile().c_str(), ex.what());
            runtime.exit(EXIT_FAILURE);
        } catch (const std::logic_error& ex) {
            log_error("Logic error parsing config file '{}': {}", runtime.getConfigFile().c_str(), ex.what());
            runtime.exit(EXIT_FAILURE);
        }

        installSignalHandler();
        sigset_t emptyMaskSet;
        sigemptyset(&emptyMaskSet);

        auto server = std::make_shared<Server>(configManager);

        try {
            auto dropDatabase = runtime.getDropDatabase();
            auto initLastFM = runtime.getLastFM();

            server->init(definition, runtime.getOffline(), dropDatabase, initLastFM);

            if (!dropDatabase && !initLastFM)
                server->run();
            else
                runtime.exit(EXIT_SUCCESS);

            runtime.notifySystemd(ServiceStatus::Ready);
        } catch (const UpnpException& ue) {
            log_error("{}", ue.what());

            if (pthread_sigmask(SIG_SETMASK, &emptyMaskSet, nullptr))
                log_error("Error clearing masked signals {}", std::strerror(errno));

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
                runtime.notifySystemd(ServiceStatus::Stopping);
                if (server)
                    server->shutdown();
                server.reset();
                configManager.reset();
                runtime.shutdown();
            } catch (const std::runtime_error& ex) {
                log_error("Shutdown: {}", ex.what());
            }

            runtime.exit(EXIT_FAILURE);
        } catch (const std::runtime_error& ex) {
            log_error("Run: runtime error {}", ex.what());
            runtime.exit(EXIT_FAILURE);
        } catch (const std::logic_error& ex) {
            log_error("Run: logic error {}", ex.what());
            runtime.exit(EXIT_FAILURE);
        }

        runtime.handleServerOptions(server);

        // wait until signalled to terminate
        while (!_ctx.shutdownFlag) {
            if (pthread_sigmask(SIG_SETMASK, &emptyMaskSet, nullptr))
                log_error("Error clearing masked signals {}", std::strerror(errno));

            _ctx.cond.wait(_ctx.lock);

            if (_ctx.restartFlag != 0) {
                log_info("Restarting Gerbera!");
                runtime.notifySystemd(ServiceStatus::Reloading);
                try {
                    server->shutdown();
                    server.reset();
                    configManager.reset();
                    definition.reset();
                    runtime.shutdown();

                    definition = std::make_shared<ConfigDefinition>();
                    definition->init(definition);
                    GerberaRuntime runtimeChild(definition, &options);
                    try {
                        runtimeChild.handleOptions(&results, false);
                        configManager = std::make_shared<ConfigManager>(
                            definition,
                            runtimeChild.getConfigFile(), runtimeChild.getHome(), runtimeChild.getConfDir(),
                            runtimeChild.getDataDir(), runtimeChild.getDebug());
                        configManager->load(runtimeChild.getHome());

                        runtimeChild.handleConfigOptions(configManager, additionalArgs);
                        configManager->validate();
#ifdef GRBDEBUG
                        GrbLogger::Logger.init(configManager->getULongOption(ConfigVal::SERVER_LOG_DEBUG_MODE));
#endif
                        portnum = configManager->getUIntOption(ConfigVal::SERVER_PORT);
                    } catch (const ConfigParseException& ex) {
                        log_error("Error parsing config file '{}': {}", runtimeChild.getConfigFile().c_str(), ex.what());
                        log_error("Could not restart Gerbera");
                        // at this point upnp shutdown has already been called
                        // therefore it is safe to exit
                        runtime.exit(EXIT_FAILURE);
                    } catch (const std::runtime_error& ex) {
                        log_error("Runtime error reloading configuration: {}", ex.what());
                        runtime.exit(EXIT_FAILURE);
                    } catch (const std::logic_error& ex) {
                        log_error("Logic error reloading configuration: {}", ex.what());
                        runtime.exit(EXIT_FAILURE);
                    }

                    installSignalHandler();

                    server = std::make_shared<Server>(configManager);
                    server->init(definition, runtimeChild.getOffline(), runtimeChild.getDropDatabase(), runtimeChild.getLastFM());
                    server->run();
                    runtime.notifySystemd(ServiceStatus::Ready);

                    _ctx.restartFlag = 0;
                } catch (const std::runtime_error& ex) {
                    _ctx.restartFlag = 0;
                    _ctx.shutdownFlag = 1;

                    if (pthread_sigmask(SIG_SETMASK, &emptyMaskSet, nullptr))
                        log_error("Error clearing masked signals {}", std::strerror(errno));
                    log_error("Could not restart Gerbera {}", ex.what());
                } catch (const std::logic_error& ex) {
                    _ctx.restartFlag = 0;
                    _ctx.shutdownFlag = 1;

                    if (pthread_sigmask(SIG_SETMASK, &emptyMaskSet, nullptr))
                        log_error("Error clearing masked signals {}", std::strerror(errno));
                    log_error("Restart: logic error {}", ex.what());
                }
            }
        }

        // shutting down
        int ret = EXIT_SUCCESS;
        try {
            runtime.notifySystemd(ServiceStatus::Stopping);
            server->shutdown();
            server.reset();
            configManager.reset();
        } catch (const UpnpException& upnpE) {
            log_error("Shutdown: upnp error {}", upnpE.getErrorCode());
            ret = EXIT_FAILURE;
        } catch (const std::runtime_error& ex) {
            log_error("Shutdown: runtime error {}", ex.what());
            ret = EXIT_FAILURE;
        } catch (const std::logic_error& ex) {
            log_error("Shutdown: logic error {}", ex.what());
            ret = EXIT_FAILURE;
        }

        log_info("Gerbera exiting. Have a nice day.");
        runtime.exit(ret);
    } catch (const cxxopts::exceptions::exception& e) {
        fmt::print(stderr, "Failed to parse arguments: {}\n", e.what());
        runtime.exit(EXIT_FAILURE);
    }
}
