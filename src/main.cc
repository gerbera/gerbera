/*MT*
    
    MediaTomb - http://www.mediatomb.cc/
    
    main.cc - this file is part of MediaTomb.
    
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

/// \file main.cc

/// \mainpage Sourcecode Documentation.
///
/// This documentation was generated using doxygen, you can reproduce it by
/// running "doxygen doxygen.conf" from the mediatomb/doc/ directory.

#include <csignal>
#include <filesystem>
#include <mutex>
#include <spdlog/sinks/basic_file_sink.h>
#include <spdlog/spdlog.h>

#ifdef SOLARIS
#include <iso/limits_iso.h>
#endif

#include "common.h"
#include "config/config_generator.h"
#include "config/config_manager.h"
#include "content_manager.h"

#include "contrib/cxxopts.hpp"
#include "server.h"

static struct {
    int shutdown_flag = 0;
    int restart_flag = 0;
    pthread_t main_thread_id;

    std::mutex mutex;
    std::unique_lock<std::mutex> lock { mutex };
    std::condition_variable cond;
} _ctx;

static void printCopyright()
{
    printf("\nGerbera UPnP Server version %s - %s\n\n", GERBERA_VERSION, DESC_MANUFACTURER_URL);
    printf("===============================================================================\n");
    printf("Gerbera is free software, covered by the GNU General Public License version 2\n\n");
    printf("Copyright 2016-2021 Gerbera Contributors.\n");
    printf("Gerbera is based on MediaTomb: Copyright 2005-2010 Gena Batsyan, Sergey Bostandzhyan, Leonhard Wimmer.\n");
    printf("===============================================================================\n");
}

static void logCopyright()
{
    log_info("Gerbera UPnP Server version {} - {}", GERBERA_VERSION, DESC_MANUFACTURER_URL);
    log_info("===============================================================================");
    log_info("Gerbera is free software, covered by the GNU General Public License version 2");
    log_info("Copyright 2016-2021 Gerbera Contributors.");
    log_info("Gerbera is based on MediaTomb: Copyright 2005-2010 Gena Batsyan, Sergey Bostandzhyan, Leonhard Wimmer.");
    log_info("===============================================================================");
}

static void signalHandler(int signum)
{
    if (_ctx.main_thread_id != pthread_self()) {
        return;
    }

    if ((signum == SIGINT) || (signum == SIGTERM)) {
        _ctx.shutdown_flag++;
        if (_ctx.shutdown_flag == 1) {
            log_info("Gerbera shutting down. Please wait...");
        } else if (_ctx.shutdown_flag == 2) {
            log_info("Gerbera still shutting down, signal again to kill.");
        } else if (_ctx.shutdown_flag > 2) {
            log_error("Clean shutdown failed, killing Gerbera!");
            exit(EXIT_FAILURE);
        }
    } else if (signum == SIGHUP) {
        _ctx.restart_flag = 1;
    }

    _ctx.cond.notify_one();
}

static void installSignalHandler()
{
    _ctx.main_thread_id = pthread_self();

    struct sigaction action = {};
    action.sa_handler = signalHandler;
    action.sa_flags = 0;
    sigfillset(&action.sa_mask);
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
}

int main(int argc, char** argv, char** envp)
{
#ifdef SOLARIS
    std::string ld_preload;
    char* preload = getenv("LD_PRELOAD");
    if (preload != nullptr)
        ld_preload = std::string(preload);

    if ((preload == nullptr) || (ld_preload.find("0@0") == -1)) {
        printf("Gerbera: Solaris check failed!\n");
        printf("Please set the environment to match glibc behaviour!\n");
        printf("LD_PRELOAD=/usr/lib/0@0.so.1\n");
        exit(EXIT_FAILURE);
    }
#endif
    cxxopts::Options options("gerbera", "Gerbera UPnP Media Server - https://gerbera.io");

    options.add_options() //
        ("D,debug", "Enable debugging", cxxopts::value<bool>()->default_value("false")) //
        ("e,interface", "Interface to bind with", cxxopts::value<std::string>()) //
        ("p,port", "Port to bind with, must be >=49152", cxxopts::value<in_port_t>()) //
        ("i,ip", "IP to bind with", cxxopts::value<std::string>()) //
        ("c,config", "Path to config file", cxxopts::value<std::string>()) //
        ("m,home", "Search this directory for a .gerbera folder containing a config file", cxxopts::value<std::string>()) //
        ("f,cfgdir", "Override name of config folder (.config/gerbera) by default. -h must also be set.", cxxopts::value<std::string>()) //
        ("l,logfile", "Set log location", cxxopts::value<std::string>()) //
        ("compile-info", "Print compile info and exit") //
        ("v,version", "Print version info and exit") //
        ("h,help", "Print this help and exit") //
        ("create-config", "Print a default config.xml file and exit") //
        ("add-file", "Scan a file into the DB on startup, can be specified multiple times", cxxopts::value<std::vector<std::string>>(), "FILE") //
        ;

    try {
        cxxopts::ParseResult opts = options.parse(argc, argv);

        if (opts.count("help") > 0) {
            std::cout << options.help({ "", "Group" }) << std::endl;
            exit(EXIT_SUCCESS);
        }

        if (opts.count("version") > 0) {
            printCopyright();
            exit(EXIT_SUCCESS);
        }

        if (opts.count("compile-info") > 0) {
            printCopyright();
            std::cout << "Compile info" << std::endl
                      << "-------------" << std::endl
                      << COMPILE_INFO << std::endl
                      << std::endl;
            if (strlen(GIT_BRANCH) > 0) {
                std::cout << "Git info:" << std::endl
                          << "-------------" << std::endl
                          << "Git Branch: " << GIT_BRANCH << std::endl;
            }
            if (strlen(GIT_COMMIT_HASH) > 0) {
                std::cout << "Git Commit: " << GIT_COMMIT_HASH << std::endl;
            }

            exit(EXIT_SUCCESS);
        }

        bool debug = opts["debug"].as<bool>();
        if (debug) {
            spdlog::set_level(spdlog::level::debug);
            spdlog::set_pattern("%Y-%m-%d %X %6l: [%s:%#] %!(): %v");
        } else {
            spdlog::set_level(spdlog::level::info);
            spdlog::set_pattern("%Y-%m-%d %X %6l: %v");
        }

        std::optional<std::string> logfile;
        if (opts.count("logfile") > 0) {
            auto file_logger = spdlog::basic_logger_mt("basic_logger", opts["logfile"].as<std::string>());
            spdlog::set_default_logger(file_logger);
            spdlog::flush_on(spdlog::level::trace);
        }

        // Action starts here
        if (opts.count("create-config") == 0) {
            logCopyright();
        }

        std::optional<std::string> home;
        if (opts.count("home") > 0) {
            home = opts["home"].as<std::string>();
        }

        std::optional<std::string> config_file;
        if (opts.count("config") > 0) {
            config_file = opts["config"].as<std::string>();
        }

        std::optional<std::string> confdir;
        if (opts.count("cfgdir") > 0) {
            confdir = opts["cfgdir"].as<std::string>();
        }

        // If home is not given by the user, get it from the environment
        if (!config_file.has_value() && !home.has_value()) {
            if (!confdir.has_value()) {
                confdir = DEFAULT_CONFIG_HOME;
            }

            // Check XDG first
            const char* h = std::getenv("XDG_CONFIG_HOME");
            if (h != nullptr) {
                home = h;
                confdir = "gerbera";
            } else {
                // Then look for home
                h = std::getenv("HOME");
                if (h != nullptr)
                    home = h;
            }

            if (!home.has_value()) {
                log_error("Could not determine users home directory");
                exit(EXIT_FAILURE);
            }
            log_debug("Home detected as: {}", home->c_str());
        }

        std::optional<std::string> prefix;
        char* pref = std::getenv("GERBERA_DATADIR");
        if (pref != nullptr) {
            prefix = pref;
        } else {
            pref = std::getenv("MEDIATOMB_DATADIR");
            if (pref != nullptr)
                prefix = pref;
        }
        if (!prefix.has_value())
            prefix = PACKAGE_DATADIR;

        std::optional<std::string> magic;
        char* mgc = std::getenv("GERBERA_MAGIC_FILE");
        if (mgc != nullptr) {
            magic = mgc;
        } else {
            mgc = std::getenv("MEDIATOMB_MAGIC_FILE");
            if (mgc != nullptr)
                magic = mgc;
        }

        if (opts.count("create-config") > 0) {
            ConfigGenerator configGenerator;

            std::string generated = ConfigGenerator::generate(home.value_or(""), confdir.value_or(""), prefix.value_or(""), magic.value_or(""));
            std::cout << generated.c_str() << std::endl;
            exit(EXIT_SUCCESS);
        }

        std::optional<in_port_t> portnum;
        if (opts.count("port") > 0) {
            portnum = opts["port"].as<in_port_t>();
        }

        std::optional<std::string> ip;
        if (opts.count("ip") > 0) {
            ip = opts["ip"].as<std::string>();
        }

        std::optional<std::string> interface;
        if (opts.count("interface") > 0) {
            interface = opts["interface"].as<std::string>();
        }

        std::shared_ptr<ConfigManager> configManager;
        try {
            configManager = std::make_shared<ConfigManager>(
                config_file.value_or(""), home.value_or(""), confdir.value_or(""),
                prefix.value_or(""), magic.value_or(""),
                ip.value_or(""), interface.value_or(""), portnum.value_or(0),
                debug);
            configManager->load(home.value_or(""));
            portnum = in_port_t(configManager->getIntOption(CFG_SERVER_PORT));
        } catch (const ConfigParseException& ce) {
            log_error("Error parsing config file '{}': {}", (*config_file).c_str(), ce.what());
            exit(EXIT_FAILURE);
        } catch (const std::runtime_error& e) {
            log_error("{}", e.what());
            exit(EXIT_FAILURE);
        }

        sigset_t mask_set;
        sigfillset(&mask_set);
        pthread_sigmask(SIG_SETMASK, &mask_set, nullptr);

        installSignalHandler();

        std::shared_ptr<Server> server;
        try {
            auto config = std::static_pointer_cast<Config>(configManager);
            server = std::make_shared<Server>(config);
            server->init();
            server->run();
        } catch (const UpnpException& upnp_e) {

            sigemptyset(&mask_set);
            pthread_sigmask(SIG_SETMASK, &mask_set, nullptr);

            if (upnp_e.getErrorCode() == UPNP_E_SOCKET_BIND) {
                log_error("LibUPnP could not bind to socket.");
                log_info("Please check if another instance of Gerbera or");
                log_info("another application is running on port TCP {} or UDP 1900.", portnum.value());
            } else if (upnp_e.getErrorCode() == UPNP_E_SOCKET_ERROR) {
                log_error("LibUPnP Socket error.");
                log_info("Please check if your network interface was configured for multicast!");
                log_info("Refer to the README file for more information.");
            } else {
                log_error("LibUPnP error code: {}", upnp_e.getErrorCode());
            }

            try {
                if (server)
                    server->shutdown();
                server = nullptr;
                configManager = nullptr;
            } catch (const std::runtime_error& e) {
                log_error("{}", e.what());
            }

            exit(EXIT_FAILURE);
        } catch (const std::runtime_error& e) {
            log_error("{}", e.what());
            exit(EXIT_FAILURE);
        }

        if (opts.count("add-file") > 0) {
            auto files = opts["add-file"].as<std::vector<std::string>>();
            for (const auto& f : files) {
                try {
                    // add file/directory recursively and asynchronously
                    AutoScanSetting asSetting;
                    asSetting.followSymlinks = configManager->getBoolOption(CFG_IMPORT_FOLLOW_SYMLINKS);
                    asSetting.recursive = true;
                    asSetting.hidden = configManager->getBoolOption(CFG_IMPORT_HIDDEN_FILES);
                    asSetting.rescanResource = false;
                    asSetting.mergeOptions(configManager, f);
                    server->getContent()->addFile(std::string(f), asSetting, true);
                } catch (const std::runtime_error& e) {
                    log_error("{}", e.what());
                    exit(EXIT_FAILURE);
                }
            }
        }

        sigemptyset(&mask_set);
        pthread_sigmask(SIG_SETMASK, &mask_set, nullptr);

        // wait until signalled to terminate
        while (!_ctx.shutdown_flag) {
            _ctx.cond.wait(_ctx.lock);

            if (_ctx.restart_flag != 0) {
                log_info("Restarting Gerbera!");
                try {
                    server->shutdown();
                    server = nullptr;
                    configManager = nullptr;

                    try {
                        configManager = std::make_shared<ConfigManager>(
                            config_file.value_or(""), home.value_or(""), confdir.value_or(""),
                            prefix.value_or(""), magic.value_or(""),
                            ip.value_or(""), interface.value_or(""), portnum.value_or(-1),
                            debug);
                    } catch (const ConfigParseException& ce) {
                        log_error("Error parsing config file '{}': {}", (*config_file).c_str(), ce.what());
                        log_error("Could not restart Gerbera");
                        // at this point upnp shutdown has already been called
                        // so it is safe to exit
                        exit(EXIT_FAILURE);
                    } catch (const std::runtime_error& e) {
                        log_error("Error reloading configuration: {}",
                            e.what());
                        exit(EXIT_FAILURE);
                    }

                    ///  \todo fix this for SIGHUP
                    auto config = std::static_pointer_cast<Config>(configManager);
                    server = std::make_shared<Server>(config);
                    server->init();
                    server->run();

                    _ctx.restart_flag = 0;
                } catch (const std::runtime_error& e) {
                    _ctx.restart_flag = 0;
                    _ctx.shutdown_flag = 1;
                    sigemptyset(&mask_set);
                    pthread_sigmask(SIG_SETMASK, &mask_set, nullptr);
                    log_error("Could not restart Gerbera");
                }
            }
        }

        // shutting down
        int ret = EXIT_SUCCESS;
        try {
            server->shutdown();
            server = nullptr;
            configManager = nullptr;
        } catch (const UpnpException& upnp_e) {
            log_error("main: upnp error {}", upnp_e.getErrorCode());
            ret = EXIT_FAILURE;
        } catch (const std::runtime_error& e) {
            log_error("main: error {}", e.what());
            ret = EXIT_FAILURE;
        }

        log_info("Gerbera exiting. Have a nice day.");
        exit(ret);

    } catch (const cxxopts::OptionException& e) {
        std::cerr << "Failed to parse arguments: " << e.what() << std::endl;
        exit(EXIT_FAILURE);
    }
}
