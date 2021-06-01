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
#include <cstdio>
#include <cstdlib>
#include <filesystem>
#include <mutex>
#include <spdlog/sinks/basic_file_sink.h>

#include <upnpconfig.h>
#ifdef UPNP_HAVE_TOOLS
#include <upnptools.h>
#endif

#ifdef SOLARIS
#include <iso/limits_iso.h>
#endif

// those are needed for -P pidfile, -u user and -d daemonize options
#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif
#include <fcntl.h>
#include <grp.h>
#include <pwd.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#include "common.h"
#include "config/config_generator.h"
#include "config/config_manager.h"
#include "content/content_manager.h"
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
    printf("\nGerbera UPnP Server %s\n"
           "Copyright 2016-2021 Gerbera Contributors.\n"
           "Licence GPLv2: GNU GPL version 2.\n"
           "This is free software: you are free to change and redistribute it.\n\n",
        GERBERA_VERSION);
}

static void logCopyright()
{
    log_info("Gerbera UPnP Server {} - {}", GERBERA_VERSION, DESC_MANUFACTURER_URL);
    log_info("Copyright 2016-2021 Gerbera Contributors.");
    log_info("Gerbera is free software, covered by the GNU General Public License version 2");
}

static void signalHandler(int signum)
{
    if (_ctx.main_thread_id != pthread_self()) {
        return;
    }

    if (signum == SIGSEGV) {
        log_error("This should never happen {}:{}, killing Gerbera!", errno, std::strerror(errno));
        exit(EXIT_FAILURE);
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
}

int main(int argc, char** argv, char** envp)
{
    cxxopts::Options options("gerbera", "Gerbera UPnP Media Server - https://gerbera.io");

    options.add_options() //
        ("D,debug", "Enable debugging", cxxopts::value<bool>()->default_value("false")) //
        ("d,daemon", "Daemonize after startup", cxxopts::value<bool>()->default_value("false")) //
        ("u,user", "Drop privs to user", cxxopts::value<std::string>()) //
        ("P,pidfile", "Write a pidfile to the specified location, e.g. /run/gerbera.pid", cxxopts::value<std::string>()) //
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
            spdlog::set_pattern("%Y-%m-%d %X.%e %6l: [%s:%#] %!(): %v");
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

        // are we requested to drop privs?
        std::optional<std::string> user;
        if (opts.count("user") > 0) {
            user = opts["user"].as<std::string>();

            // get actual euid/egid of process
            uid_t actual_euid = geteuid();

            // get user info of requested user from passwd
            struct passwd* user_id = getpwnam(user->c_str());

            if (user_id == nullptr) {
                log_error("Invalid user requested.");
                exit(EXIT_FAILURE);
            }

            // set home according to /etc/passwd entry
            if (!home.has_value()) {
                home = user_id->pw_dir;
            }

            // we need to be euid root to become requested user/group
            if (actual_euid != 0) {
                log_error("Need to be root to change user.");
                exit(EXIT_FAILURE);
            }

            // set all uids, gids and add. groups
// mac os x does this differently, setgid and setuid are basically doing the same
// as setresuid and setresgid on linux: setting all of real{u,g}id, effective{u,g}id and saved-set{u,g}id
// Solaroid systems are likewise missing setresgid and setresuid
#if defined(__APPLE__) || defined(SOLARIS) || defined(__CYGWIN__)
            // set group-ids, then add. groups, last user-ids, all need to succeed
            if (0 != setgid(user_id->pw_gid) || 0 != initgroups(user_id->pw_name, user_id->pw_gid) || 0 != setuid(user_id->pw_uid)) {
#else
            if (0 != setresgid(user_id->pw_gid, user_id->pw_gid, user_id->pw_gid) || 0 != initgroups(user_id->pw_name, user_id->pw_gid) || 0 != setresuid(user_id->pw_uid, user_id->pw_uid, user_id->pw_uid)) {
#endif
                log_error("Unable to change user.");
                exit(EXIT_FAILURE);
            }
            log_info("Dropped to User: {}", user->c_str());
        }

        // are we requested to daemonize?
        bool daemon = opts["daemon"].as<bool>();
        if (daemon) {
            pid_t pid;

            // fork
            pid = fork();
            if (pid < 0) {
                log_error("Unable to fork.");
                exit(EXIT_FAILURE);
            }

            // terminate parent
            if (pid > 0)
                exit(EXIT_SUCCESS);

            // become session leader
            if (setsid() < 0) {
                log_error("Unable to setsid.");
                exit(EXIT_FAILURE);
            }

            // second fork
            pid = fork();
            if (pid < 0) {
                log_error("Unable to fork.");
                exit(EXIT_FAILURE);
            }

            // terminate parent
            if (pid > 0)
                exit(EXIT_SUCCESS);

            // set new file permissions
            umask(0);

            // change dir to /
            if (chdir("/")) {
                log_error("Unable to chdir to root dir.");
                exit(EXIT_FAILURE);
            }
            // close open filedescriptors belonging to a tty
            for (auto fd = int(sysconf(_SC_OPEN_MAX)); fd >= 0; fd--) {
                if (isatty(fd))
                    close(fd);
            }
            log_info("Daemonized.");
        }

        // are we requested to write a pidfile ?
        std::optional<std::string> pidfile;
        if (opts.count("pidfile") > 0) {
            pidfile = opts["pidfile"].as<std::string>();

            // exit if the pidfile already exists
            struct stat s;
            errno = 0;
            // for pidfile to be not there stat needs to fail and it needs
            // to fail due to ENOENT (no such file or dir)
            if (!(-1 == stat(pidfile->c_str(), &s) && errno == ENOENT)) {
                log_error("Pidfile {} exists. It may be that gerbera is already", pidfile->c_str());
                log_error("running or the file is a leftover from an unclean shutdown.");
                log_error("In that case, remove the file before starting gerbera.");
                exit(EXIT_FAILURE);
            }

            // get the pid of our process
            pid_t pid = getpid();

            // convert to a string
            char pidstr[20];
            if (0 > snprintf(pidstr, 18, "%d", pid)) {
                log_error("Could not determine pid of running process.");
                exit(EXIT_FAILURE);
            }

            // add a newline
            strcat(pidstr, "\n");

            // open the pidfile
            int pidfd = open(pidfile->c_str(), O_CREAT | O_WRONLY, S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH);
            if (-1 == pidfd) {
                log_error("Could not create pidfile {}.", pidfile->c_str());
                exit(EXIT_FAILURE);
            }

            // write pid to file
            if (ssize_t(strlen(pidstr)) != write(pidfd, pidstr, strlen(pidstr))) {
                log_error("Could not write pidfile {}.", pidfile->c_str());
                exit(EXIT_FAILURE);
            }
            log_debug("Wrote pidfile {}.", pidfile->c_str());

            // close filedescriptor
            close(pidfd);
        }

        std::optional<std::string> config_file;
        if (opts.count("config") > 0) {
            config_file = opts["config"].as<std::string>();
        }

        std::optional<std::string> confdir;
        if (opts.count("cfgdir") > 0) {
            confdir = opts["cfgdir"].as<std::string>();
        }

        if (!confdir.has_value()) {
            confdir = DEFAULT_CONFIG_HOME;
        }

        // If home is not given by the user, get it from the environment
        if (!config_file.has_value() && !home.has_value()) {
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

        std::optional<std::string> dataDir;
        char* pref = std::getenv("GERBERA_DATADIR");
        if (pref != nullptr) {
            dataDir = pref;
        } else {
            pref = std::getenv("MEDIATOMB_DATADIR");
            if (pref != nullptr)
                dataDir = pref;
        }
        if (!dataDir.has_value())
            dataDir = PACKAGE_DATADIR;

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

            std::string generated = configGenerator.generate(home.value_or(""), confdir.value_or(""), dataDir.value_or(""), magic.value_or(""));
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

        log_debug("Datadir is: {}", dataDir.value_or("unset"));

        std::shared_ptr<ConfigManager> configManager;
        try {
            configManager = std::make_shared<ConfigManager>(
                config_file.value_or(""), home.value_or(""), confdir.value_or(""),
                dataDir.value_or(""), magic.value_or(""),
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
        } catch (const UpnpException& ue) {

            sigemptyset(&mask_set);
            pthread_sigmask(SIG_SETMASK, &mask_set, nullptr);

            if (ue.getErrorCode() == UPNP_E_SOCKET_BIND) {
                log_error("LibUPnP could not bind to socket");
                log_error("Please check if another instance of Gerbera or another application is running on port TCP {} or UDP 1900.", portnum.value());
            } else if (ue.getErrorCode() == UPNP_E_SOCKET_ERROR) {
                log_error("LibUPnP Socket error");
                log_error("Please check if your network interface was configured for multicast!");
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
            for (auto&& f : files) {
                try {
                    std::error_code ec;
                    auto dirEnt = fs::directory_entry(f, ec);
                    if (!ec) {
                        // add file/directory recursively and asynchronously
                        AutoScanSetting asSetting;
                        asSetting.followSymlinks = configManager->getBoolOption(CFG_IMPORT_FOLLOW_SYMLINKS);
                        asSetting.recursive = true;
                        asSetting.hidden = configManager->getBoolOption(CFG_IMPORT_HIDDEN_FILES);
                        asSetting.rescanResource = false;
                        asSetting.mergeOptions(configManager, f);
                        server->getContent()->addFile(dirEnt, asSetting, true);
                    } else {
                        log_error("Failed to read {}: {}", f.c_str(), ec.message());
                    }
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
                            dataDir.value_or(""), magic.value_or(""),
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

        // remove pidfile if one was written
        if (opts.count("pidfile") > 0) {
            if (0 == remove(pidfile->c_str())) {
                log_debug("Pidfile {} removed.", pidfile->c_str());
            } else {
                log_info("Could not remove pidfile {}", pidfile->c_str());
            }
        }

        log_info("Gerbera exiting. Have a nice day.");
        exit(ret);

    } catch (const cxxopts::OptionException& e) {
        std::cerr << "Failed to parse arguments: " << e.what() << std::endl;
        exit(EXIT_FAILURE);
    }
}
