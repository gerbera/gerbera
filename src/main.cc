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
#include <mutex>
#ifdef SOLARIS
#include <iso/limits_iso.h>
#endif

#include "common.h"
#include "config/config_generator.h"
#include "content_manager.h"
#include "contrib/cxxopts.hpp"
#include "server.h"

using namespace zmm;

int shutdown_flag = 0;
int restart_flag = 0;
pthread_t main_thread_id;

std::mutex mutex;
std::unique_lock<std::mutex> lock { mutex };
std::condition_variable cond;

void print_copyright()
{
    printf("\nGerbera UPnP Server version %s - %s\n\n", VERSION, DESC_MANUFACTURER_URL);
    printf("===============================================================================\n");
    printf("Gerbera is free software, covered by the GNU General Public License version 2\n\n");
    printf("Copyright 2016-2019 Gerbera Contributors.\n");
    printf("Gerbera is based on MediaTomb: Copyright 2005-2010 Gena Batsyan, Sergey Bostandzhyan, Leonhard Wimmer.\n");
    printf("===============================================================================\n");
}

void log_copyright()
{
    log_info("Gerbera UPnP Server version %s - %s\n", VERSION, DESC_MANUFACTURER_URL);
    log_info("===============================================================================\n");
    log_info("Gerbera is free software, covered by the GNU General Public License version 2\n");
    log_info("Copyright 2016-2019 Gerbera Contributors.\n");
    log_info("Gerbera is based on MediaTomb: Copyright 2005-2010 Gena Batsyan, Sergey Bostandzhyan, Leonhard Wimmer.\n");
    log_info("===============================================================================\n");
}

void signal_handler(int signum);

int main(int argc, char** argv, char** envp)
{
#ifdef SOLARIS
    String ld_preload;
    char* preload = getenv("LD_PRELOAD");
    if (preload != NULL)
        ld_preload = String(preload);

    if ((preload == NULL) || (ld_preload.find("0@0") == -1)) {
        printf("Gerbera: Solaris check failed!\n");
        printf("Please set the environment to match glibc behaviour!\n");
        printf("LD_PRELOAD=/usr/lib/0@0.so.1\n");
        exit(EXIT_FAILURE);
    }
#endif

    cxxopts::Options options("gerbera", "Gerbera UPnP Media Server - https://gerbera.io");

    options.add_options()
    ("D,debug", "Enable debugging", cxxopts::value<bool>()->default_value("false"))
    ("e,interface", "Interface to bind with", cxxopts::value<std::string>())
    ("p,port", "Port to bind with, must be >=49152", cxxopts::value<int>())
    ("i,ip", "IP to bind with", cxxopts::value<std::string>())
    ("c,config", "Path to config file", cxxopts::value<std::string>())
    ("m,home", "Search this directory for a .gerbera folder containing a config file", cxxopts::value<std::string>())
    ("f,cfgdir", "Override name of config folder (.config/gerbera) by default. -h must also be set.", cxxopts::value<std::string>())
    ("l,logfile", "Set log location", cxxopts::value<std::string>())("compile-info", "Print compile info and exit")
    ("v,version", "Print version info and exit")("h,help", "Print this help and exit")
    ("create-config", "Print a default config.xml file and exit")
    ("add-file", "Scan a file into the DB on startup, can be specified multiple times", cxxopts::value<std::vector<std::string>>(), "FILE");

    try {
        cxxopts::ParseResult opts = options.parse(argc, argv);

        if (opts.count("help") > 0) {
            std::cout << options.help({ "", "Group" }) << std::endl;
            exit(EXIT_SUCCESS);
        }

        if (opts.count("version") > 0) {
            print_copyright();
            exit(EXIT_SUCCESS);
        }

        if (opts.count("compile-info") > 0) {
            print_copyright();
            std::cout << "Compile info" << std::endl
                      << "-------------" << std::endl
                      << COMPILE_INFO << std::endl
                      << std::endl;
            if (strlen(GIT_BRANCH) > 0 || strlen(GIT_COMMIT_HASH) > 0) {
                std::cout << "Git info:" << std::endl
                          << "-------------" << std::endl
                          << "Git Branch: " << GIT_BRANCH << std::endl
                          << "Git Commit: " << GIT_COMMIT_HASH << std::endl;
            }
            exit(EXIT_SUCCESS);
        }

        std::optional<std::string> logfile;
        if (opts.count("logfile") > 0) {
            log_open(opts["logfile"].as<std::string>().c_str());
        }

        // Action starts here
        log_copyright();

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
                log_error("Could not determine users home directory\n");
                exit(EXIT_FAILURE);
            }
            log_debug("Home detected as: %s\n", home->c_str());
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
            std::string magicStr;
            if (!magic.has_value()) {
                magicStr = "";
            } else {
                magicStr = magic.value().c_str();
            }

            ConfigGenerator configGenerator;

            std::string generated = configGenerator.generate(home.value(), confdir.value(), prefix.value(), magicStr);
            std::cout << generated.c_str() << std::endl;
            exit(EXIT_SUCCESS);
        }

        std::optional<int> portnum;
        if (opts.count("port") > 0) {
            portnum = opts["port"].as<int>();
        }

        bool debug = opts["debug"].as<bool>();

        std::optional<std::string> ip;
        if (opts.count("ip") > 0) {
            ip = opts["ip"].as<std::string>();
        }

        std::optional<std::string> interface;
        if (opts.count("interface") > 0) {
            interface = opts["interface"].as<std::string>();
        }

        // FIXME: We will remove the singletons at some point.. and this compat
        String configFile_ = _(config_file);
        String home_ = _(home);
        String confdir_ = _(confdir);
        String prefix_ = _(prefix);
        String magic_ = _(magic);
        String ip_ = _(ip);
        String interface_ = _(interface);

        ConfigManager::setStaticArgs(configFile_, home_, confdir_, prefix_, magic_, debug, ip_, interface_, portnum.value_or(-1));
        try {
            ConfigManager::getInstance();
            portnum = ConfigManager::getInstance()->getIntOption(CFG_SERVER_PORT);
        } catch (const mxml::ParseException& pe) {
            log_error("Error parsing config file: %s line %d:\n%s\n",
                pe.context->location.c_str(),
                pe.context->line,
                pe.getMessage().c_str());
            exit(EXIT_FAILURE);
        } catch (const Exception& e) {
            log_error("%s\n", e.getMessage().c_str());
            exit(EXIT_FAILURE);
        }

        struct sigaction action;
        sigset_t mask_set;
        main_thread_id = pthread_self();
        // install signal handlers
        sigfillset(&mask_set);
        pthread_sigmask(SIG_SETMASK, &mask_set, nullptr);

        memset(&action, 0, sizeof(action));
        action.sa_handler = signal_handler;
        action.sa_flags = 0;
        sigfillset(&action.sa_mask);
        if (sigaction(SIGINT, &action, nullptr) < 0) {
            log_error("Could not register SIGINT handler!\n");
        }

        if (sigaction(SIGTERM, &action, nullptr) < 0) {
            log_error("Could not register SIGTERM handler!\n");
        }

        if (sigaction(SIGHUP, &action, nullptr) < 0) {
            log_error("Could not register SIGHUP handler!\n");
        }

        if (sigaction(SIGPIPE, &action, nullptr) < 0) {
            log_error("Could not register SIGPIPE handler!\n");
        }

        Ref<SingletonManager> singletonManager = SingletonManager::getInstance();
        Ref<Server> server;
        try {
            server = Server::getInstance();
            server->run();
        } catch (const UpnpException& upnp_e) {

            sigemptyset(&mask_set);
            pthread_sigmask(SIG_SETMASK, &mask_set, nullptr);

            upnp_e.printStackTrace();
            if (upnp_e.getErrorCode() == UPNP_E_SOCKET_BIND) {
                log_error("LibUPnP could not bind to socket.\n");
                log_info("Please check if another instance of Gerbera or\n");
                log_info("another application is running on port TCP %d or UDP 1900.\n", portnum.value());
            } else if (upnp_e.getErrorCode() == UPNP_E_SOCKET_ERROR) {
                log_error("LibUPnP Socket error.\n");
                log_info("Please check if your network interface was configured for multicast!\n");
                log_info("Refer to the README file for more information.\n");
            } else {
                log_error("LibUPnP error code: %d\n", upnp_e.getErrorCode());
            }

            try {
                singletonManager->shutdown(true);
            } catch (const Exception& e) {
                log_error("%s\n", e.getMessage().c_str());
                e.printStackTrace();
            }
            exit(EXIT_FAILURE);
        } catch (const Exception& e) {
            log_error("%s\n", e.getMessage().c_str());
            e.printStackTrace();
            exit(EXIT_FAILURE);
        }

        if (opts.count("add-file") > 0) {
            auto files = opts["add-file"].as<std::vector<std::string>>();
            for (const auto& f : files) {
                try {
                    // add file/directory recursively and asynchronously
                    ContentManager::getInstance()->addFile(String(f), true, true,
                        ConfigManager::getInstance()->getBoolOption(CFG_IMPORT_HIDDEN_FILES));
                } catch (const Exception& e) {
                    e.printStackTrace();
                    exit(EXIT_FAILURE);
                }
            }
        }

        sigemptyset(&mask_set);
        pthread_sigmask(SIG_SETMASK, &mask_set, nullptr);

        // wait until signalled to terminate
        while (!shutdown_flag) {
            cond.wait(lock);

            if (restart_flag != 0) {
                log_info("Restarting Gerbera!\n");
                try {
                    server = nullptr;

                    singletonManager->shutdown(true);
                    singletonManager = nullptr;
                    singletonManager = SingletonManager::getInstance();

                    try {
                        ConfigManager::setStaticArgs(config_file.value(), home.value(), confdir.value(),
                            prefix.value(), magic.value());
                        ConfigManager::getInstance();
                    } catch (const mxml::ParseException& pe) {
                        log_error("Error parsing config file: %s line %d:\n%s\n",
                            pe.context->location.c_str(),
                            pe.context->line,
                            pe.getMessage().c_str());
                        log_error("Could not restart Gerbera\n");
                        // at this point upnp shutdown has already been called
                        // so it is safe to exit
                        exit(EXIT_FAILURE);
                    } catch (const Exception& e) {
                        log_error("Error reloading configuration: %s\n",
                            e.getMessage().c_str());
                        e.printStackTrace();
                        exit(EXIT_FAILURE);
                    }

                    ///  \todo fix this for SIGHUP
                    server = Server::getInstance();
                    server->run();

                    restart_flag = 0;
                } catch (const Exception& e) {
                    restart_flag = 0;
                    shutdown_flag = 1;
                    sigemptyset(&mask_set);
                    pthread_sigmask(SIG_SETMASK, &mask_set, nullptr);
                    log_error("Could not restart Gerbera\n");
                }
            }
        }

        // shutting down
        int ret = EXIT_SUCCESS;
        try {
            singletonManager->shutdown(true);
        } catch (const UpnpException& upnp_e) {
            log_error("main: upnp error %d\n", upnp_e.getErrorCode());
            ret = EXIT_FAILURE;
        } catch (const Exception e) {
            e.printStackTrace();
            ret = EXIT_FAILURE;
        }

        log_info("Gerbera exiting. Have a nice day.\n");
        log_close();
        exit(ret);

    } catch (const cxxopts::OptionException& e) {
        std::cerr << "Failed to parse arguments: " << e.what() << std::endl;
        exit(EXIT_FAILURE);
    }
}

void signal_handler(int signum)
{
    if (main_thread_id != pthread_self()) {
        return;
    }

    if ((signum == SIGINT) || (signum == SIGTERM)) {
        shutdown_flag++;
        if (shutdown_flag == 1)
            log_info("Gerbera shutting down. Please wait...\n");
        else if (shutdown_flag == 2)
            log_info("Gerbera still shutting down, signal again to kill.\n");
        else if (shutdown_flag > 2) {
            log_error("Clean shutdown failed, killing Gerbera!\n");
            exit(1);
        }
    } else if (signum == SIGHUP) {
        restart_flag = 1;
    }

    cond.notify_one();
}
