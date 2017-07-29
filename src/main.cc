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

#include <getopt.h>

#include <csignal>
#include <mutex>
#ifdef SOLARIS
#include <iso/limits_iso.h>
#endif

#include "common.h"
#include "server.h"
#include "content_manager.h"

#define OPTSTR "i:e:p:c:m:f:a:l:P:dhD"

using namespace zmm;

int shutdown_flag = 0;
int restart_flag = 0;
pthread_t main_thread_id;

std::mutex mutex;
std::unique_lock<std::mutex> lock{mutex};
std::condition_variable cond;

void print_copyright()
{
    printf("\nGerbera UPnP Server version %s - %s\n\n", VERSION, DESC_MANUFACTURER_URL);
    printf("===============================================================================\n");
    printf("Gerbera is free software, covered by the GNU General Public License version 2\n\n");
    printf("Copyright 2016-2017 Gerbera Contributors.\n");
    printf("Gerbera is based on MediaTomb: Copyright 2005-2010 Gena Batsyan, Sergey Bostandzhyan, Leonhard Wimmer.\n");
    printf("===============================================================================\n");
}

void log_copyright()
{
    log_info("Gerbera UPnP Server version %s - %s\n", VERSION, DESC_MANUFACTURER_URL);
    log_info("===============================================================================\n");
    log_info("Gerbera is free software, covered by the GNU General Public License version 2\n");
    log_info("Copyright 2016-2017 Gerbera Contributors.\n");
    log_info("Gerbera is based on MediaTomb: Copyright 2005-2010 Gena Batsyan, Sergey Bostandzhyan, Leonhard Wimmer.\n");
    log_info("===============================================================================\n");
}

void signal_handler(int signum);

int main(int argc, char** argv, char** envp)
{
    char* err = nullptr;
    int port = 0;
    struct sigaction action;
    sigset_t mask_set;

    int opt_index = 0;
    int o;
    static struct option long_options[] = {
        { (char*)"ip", 1, nullptr, 'i' }, // 0
        { (char*)"interface", 1, nullptr, 'e' }, // 1
        { (char*)"port", 1, nullptr, 'p' }, // 2
        { (char*)"config", 1, nullptr, 'c' }, // 3
        { (char*)"home", 1, nullptr, 'm' }, // 4
        { (char*)"cfgdir", 1, nullptr, 'f' }, // 5
        { (char*)"pidfile", 1, nullptr, 'P' }, // 8
        { (char*)"add", 1, nullptr, 'a' }, // 9
        { (char*)"logfile", 1, nullptr, 'l' }, // 10
        { (char*)"debug", 0, nullptr, 'D' }, // 11
        { (char*)"compile-info", 0, nullptr, 0 }, // 12
        { (char*)"version", 0, nullptr, 0 }, // 13
        { (char*)"help", 0, nullptr, 'h' }, // 14
        { (char*)nullptr, 0, nullptr, 0 }
    };

    String config_file;
    String home;
    String confdir;
    String pid_file;
    String interface;
    String ip;
    String prefix;
    String magic;
    bool debug_logging = false;
    bool print_version = false;

    Ref<Array<StringBase> > addFile(new Array<StringBase>());

#ifdef SOLARIS
    String ld_preload;
    char* preload = getenv("LD_PRELOAD");
    if (preload != NULL)
        ld_preload = String(preload);

    if ((preload == NULL) || (ld_preload.find("0@0") == -1)) {
        printf("MediaTomb: Solaris check failed!\n");
        printf("Please set the environment to match glibc behaviour!\n");
        printf("LD_PRELOAD=/usr/lib/0@0.so.1\n");
        exit(EXIT_FAILURE);
    }
#endif

    while (true) {
        o = getopt_long(argc, argv, OPTSTR, long_options, &opt_index);
        if (o == -1)
            break;

        switch (o) {
        case 'i':
            log_debug("Option ip with param %s\n", optarg);
            ip = optarg;
            break;

        case 'e':
            log_debug("Option interface with param %s\n", optarg);
            interface = optarg;
            break;

        case 'p':
            log_debug("Option port with param %s\n", optarg);
            errno = 0;
            port = strtol(optarg, &err, 10);
            if ((port == 0) && (*err)) {
                log_error("Invalid port argument: %s\n", optarg);
                exit(EXIT_FAILURE);
            }

            if (port > USHRT_MAX) {
                log_error("Invalid port value %d. Maximum allowed port value is %d\n", port, USHRT_MAX);
            }
            log_debug("port set to: %d\n", port);
            break;

        case 'c':
            log_debug("Option config with param %s\n", optarg);
            config_file = optarg;
            break;

        case 'a':
            log_debug("Adding file/directory:: %s\n", optarg);
            addFile->append(String::copy(optarg));
            break;

        case 'P':
            log_debug("Pid file: %s\n", optarg);
            pid_file = optarg;
            break;

        case 'l':
            log_debug("Log file: %s\n", optarg);
            log_open(optarg);
            break;

        case 'm':
            log_debug("Home setting: %s\n", optarg);
            home = optarg;
            break;

        case 'f':
            log_debug("Confdir setting: %s\n", optarg);
            confdir = optarg;
            break;

        case 'D':

#ifndef TOMBDEBUG
            print_copyright();
            printf("ERROR: MediaTomb wasn't compiled with debug output, but was run with -D/--debug.\n");
            log_error("ERROR: MediaTomb wasn't compiled with debug output, but was run with -D/--debug.\n");
            exit(EXIT_FAILURE);
#endif
            log_debug("enabling debug output\n");
            debug_logging = true;
            break;
        case '?':
        case 'h':
            print_copyright();
            printf("Usage: gerbera [options]\n\
                        \n\
Supported options:\n\
    --ip or -i         ip address to bind to\n\
    --interface or -e  network interface to bind to\n\
    --port or -p       server port (the SDK only permits values >= 49152)\n\
    --config or -c     configuration file to use\n\
    --home or -m       define the home directory\n\
    --cfgdir or -f     name of the directory that is holding the configuration\n\
    --pidfile or -P    file to hold the process id\n\
    --add or -a        add the given file/directory\n\
    --logfile or -l    log to specified file\n\
    --debug or -D      enable debug output\n\
    --compile-info     print configuration summary and exit\n\
    --version          print version information and exit\n\
    --help or -h       this help message\n\
\n\
For more information visit " DESC_MANUFACTURER_URL "\n\n");

            exit(EXIT_FAILURE);
        case 0:
            switch (opt_index) {
            case 13: // --compile-info
                print_copyright();
                printf("Compile info:\n");
                printf("-------------\n");
                printf("%s\n\n", COMPILE_INFO);
                exit(EXIT_SUCCESS);
            case 11: // --version
                print_version = true;
                break;
            }
            break;
        default:
            break;
        }
    }

    if (print_version) {
        print_copyright();
        exit(EXIT_FAILURE);
    }

    log_copyright();

    // create pid file
    if (pid_file != nullptr) {
        log_debug("Writing PID to %s\n", pid_file.c_str());
        FILE* pid_fd = fopen(pid_file.c_str(), "w");

        if (pid_fd == nullptr) {
            log_error("Could not write pid file %s : %s\n", pid_file.c_str(),
                      strerror(errno));
            exit(EXIT_FAILURE);
        }

        pid_t cur_pid = getpid();
        String pid = String::from(cur_pid);

        size_t size = fwrite(pid.c_str(), sizeof(char), pid.length(), pid_fd);
        fclose(pid_fd);

        if (static_cast<int>(size) < pid.length()) {
            log_error("Error when writing pid file %s : %s\n",
                      pid_file.c_str(), strerror(errno));
            exit(EXIT_FAILURE);
        }
    }

    // If home is not given by the user, get it from the environment
    if (!string_ok(config_file) && !string_ok(home)) {

        if (!string_ok(confdir)) {
            confdir = _(DEFAULT_CONFIG_HOME);
        }

        // Check XDG first
        char *h = getenv("XDG_CONFIG_HOME");
        if (h != nullptr) {
            home = h;
            confdir = "gerbera";
        } else {
            // Then look for home
            h = getenv("HOME");
            if (h != nullptr)
                home = h;
        }

        if (!string_ok(home)) {
            log_error("Could not determine users home directory\n");
            exit(EXIT_FAILURE);
        }
    }

    char* pref = getenv("MEDIATOMB_DATADIR");
    if (pref != nullptr)
        prefix = pref;

    if (!string_ok(prefix))
        prefix = _(PACKAGE_DATADIR);

    char* mgc = getenv("MEDIATOMB_MAGIC_FILE");
    if (mgc != nullptr)
        magic = mgc;

    if (!string_ok(magic))
        magic = nullptr;

    ConfigManager::setStaticArgs(config_file, home, confdir, prefix, magic, debug_logging, ip, interface, port);
    try {
        ConfigManager::getInstance();
        port = ConfigManager::getInstance()->getIntOption(CFG_SERVER_PORT);
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
        server->upnp_init();
    } catch (const UpnpException& upnp_e) {

        sigemptyset(&mask_set);
        pthread_sigmask(SIG_SETMASK, &mask_set, nullptr);

        upnp_e.printStackTrace();
        log_error("main: upnp error %d\n", upnp_e.getErrorCode());
        if (upnp_e.getErrorCode() == UPNP_E_SOCKET_BIND) {
            log_error("Could not bind to socket.\n");
            log_info("Please check if another instance of Gerbera or\n");
            log_info("another application is running on port %d.\n", port);
        } else if (upnp_e.getErrorCode() == UPNP_E_SOCKET_ERROR) {
            log_error("Socket error.\n");
            log_info("Please check if your network interface was configured for multicast!\n");
            log_info("Refer to the README file for more information.\n");
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

    if (addFile->size() > 0) {
        for (int i = 0; i < addFile->size(); i++) {
            try {
                // add file/directory recursively and asynchronously
                log_info("Adding %s\n", String(addFile->get(i)).c_str());
                ContentManager::getInstance()->addFile(String(addFile->get(i)),
                    true, true,
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
                    ConfigManager::setStaticArgs(config_file, home, confdir,
                        prefix, magic);
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
                server->upnp_init();

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

    return;
}
