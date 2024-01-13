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

#include <csignal>
#include <fmt/core.h>
#include <mutex>
#include <spdlog/sinks/basic_file_sink.h>
#include <vector>

#include <upnpconfig.h>
#ifdef UPNP_HAVE_TOOLS
#include <upnptools.h>
#endif

#ifdef SOLARIS
#include <iso/limits_iso.h>
#endif

// those are needed for -u user and -d daemonize options
#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif
#include <grp.h>
#include <pwd.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

#include "common.h"
#include "config/config_definition.h"
#include "config/config_generator.h"
#include "config/config_manager.h"
#include "config/config_setup.h"
#include "content/content_manager.h"
#include "contrib/cxxopts.hpp"
#include "server.h"
#include "upnp/conn_mgr_service.h"
#include "upnp/cont_dir_service.h"
#include "upnp/mr_reg_service.h"

static constexpr auto gitBranch = std::string_view(GIT_BRANCH);
static constexpr auto gitCommitHash = std::string_view(GIT_COMMIT_HASH);

static struct {
    int shutdownFlag = 0;
    int restartFlag = 0;
    pthread_t mainThreadId;

    mutable std::mutex mutex;
    std::unique_lock<std::mutex> lock { mutex };
    std::condition_variable cond;
} _ctx;

static void printCopyright()
{
    fmt::print("\nGerbera UPnP Server {}\n"
               "Copyright 2016-2024 Gerbera Contributors.\n"
               "Licence GPLv2: GNU GPL version 2.\n"
               "This is free software: you are free to change and redistribute it.\n\n",
        GERBERA_VERSION);
}

static void printOptions()
{
    auto setupList = ConfigDefinition::getOptionList();
    auto resultMap = std::map<config_option_t, std::string>();
    fmt::print("Active Options\n");
    for (auto&& setup : setupList) {
        if (setup && setup->getTypeString() != "List" && setup->option < CFG_MAX) {
            auto value = setup->getCurrentValue() != "" ? setup->getCurrentValue() : setup->getDefaultValue();
            resultMap[setup->option] = fmt::format("[{}] : {} = {}", setup->getUniquePath(), setup->getTypeString(), value);
        }
    }
    for (auto&& [option, text] : resultMap) {
        fmt::print("{:3d} {}\n", option, text);
    }
}

static void logCopyright()
{
    log_info("Gerbera UPnP Server {} - {}", GERBERA_VERSION, DESC_MANUFACTURER_URL);
    log_info("Copyright 2016-2024 Gerbera Contributors.");
    log_info("Gerbera is free software, covered by the GNU General Public License version 2");
}

static void signalHandler(int signum)
{
    if (_ctx.mainThreadId != pthread_self()) {
        return;
    }

    log_debug("Got a SIG: {}", signum);

    if (signum == SIGCHLD) {
        log_debug("Got a SIGCHLD!");
        int savedErrno = errno;
        while (waitpid(pid_t(-1), nullptr, WNOHANG) > 0) { }
        errno = savedErrno;
        return;
    }

    if (signum == SIGSEGV) {
        log_error("This should never happen {}:{}, killing Gerbera!", errno, std::strerror(errno));
        std::exit(EXIT_FAILURE);
    }

    if ((signum == SIGINT) || (signum == SIGTERM)) {
        _ctx.shutdownFlag++;
        if (_ctx.shutdownFlag == 1) {
            log_info("Gerbera shutting down. Please wait...");
        } else if (_ctx.shutdownFlag == 2) {
            log_info("Gerbera still shutting down, signal again to kill.");
        } else if (_ctx.shutdownFlag > 2) {
            log_error("Clean shutdown failed, killing Gerbera!");
            std::exit(EXIT_FAILURE);
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

class ConfigOptionArgs {
public:
    config_option_t option;
    std::string optShort;
    std::string optLong;
    std::string desc;
    std::optional<std::string> defaultValue;
    std::string argDesc;
};

static void handleOptionArgs(cxxopts::ParseResult& opts, const std::shared_ptr<Config>& configManager)
{
    if (opts["set-option"].count() > 0) {
        auto setValueList = opts["set-option"].as<std::vector<std::string>>();
        for (auto&& valueString : setValueList) {
            auto valueList = splitString(valueString, '=');
            auto option = (config_option_t)stoiString(valueList[0]);
            auto setup = ConfigDefinition::findConfigSetup(option);
            if (!setup)
                continue;
            if (setup->getTypeString() == "Boolean" && valueList.size() < 1) {
                setup->makeOption("yes", configManager);
            } else if (valueList.size() >= 2) {
                setup->makeOption(valueList[1], configManager);
            }
            auto value = setup->getCurrentValue() != "" ? setup->getCurrentValue() : setup->getDefaultValue();
            log_debug("set-option {} [{}] = {}", option, setup->getUniquePath(), value);
        }
    }
}

static void handleAdditionalArgs(cxxopts::ParseResult& opts, const std::vector<ConfigOptionArgs> additionalArgs, const std::shared_ptr<Config>& configManager)
{
    for (auto&& addArg : additionalArgs) {
        if (opts[addArg.optLong].count() > 0) {
            auto valueList = opts[addArg.optLong].as<std::vector<std::string>>();
            auto setup = ConfigDefinition::findConfigSetup(addArg.option);
            if (!setup)
                continue;
            if (setup->getTypeString() == "Boolean" && valueList.size() < 1) {
                setup->makeOption("yes", configManager);
            } else if (valueList.size() >= 1) {
                setup->makeOption(fmt::format("{}", fmt::join(valueList, ",")), configManager);
            }
            log_debug("addArg {} [{}] = {}", addArg.option, setup->getUniquePath(), setup->getCurrentValue() != "" ? setup->getCurrentValue() : setup->getDefaultValue());
        } else if (addArg.defaultValue) {
            auto setup = ConfigDefinition::findConfigSetup(addArg.option);
            setup->makeOption(addArg.defaultValue.value(), configManager);
            auto value = setup->getCurrentValue() != "" ? setup->getCurrentValue() : setup->getDefaultValue();
            log_debug("addArg {} [{}] = {}", addArg.option, setup->getUniquePath(), setup->getCurrentValue() != "" ? setup->getCurrentValue() : setup->getDefaultValue());
        }
    }
}

static std::optional<std::string> getEnv(const std::string& envVar)
{
    std::optional<std::string> result;
    char* envVal = std::getenv(fmt::format("GERBERA_{}", envVar).c_str());
    if (envVal) {
        result = envVal;
    } else {
        envVal = std::getenv(fmt::format("MEDIATOMB_{}", envVar).c_str());
        if (envVal) {
            result = envVal;
        } else {
            envVal = std::getenv(envVar.c_str());
            if (envVal)
                result = envVal;
        }
    }
    log_debug("getEnv {} = {}", envVar, result.value_or(envVar));
    return result;
}

int main(int argc, char** argv, char** envp)
{
    std::optional<std::string> magic = getEnv("MAGIC_FILE");
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

    cxxopts::Options options("gerbera", "Gerbera UPnP Media Server - https://gerbera.io");

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
        cxxopts::ParseResult opts = options.parse(argc, argv);

        if (opts.count("help") > 0) {
            fmt::print("{}\n", options.help({ "", "Group" }));
            std::exit(EXIT_SUCCESS);
        }

        if (opts.count("version") > 0) {
            printCopyright();
            std::exit(EXIT_SUCCESS);
        }

        if (opts.count("print-options") > 0) {
            printOptions();
            std::exit(EXIT_SUCCESS);
        }

        if (opts.count("compile-info") > 0) {
            printCopyright();
            fmt::print(
                "Compile info\n"
                "-------------\n"
                "{}\n\n",
                COMPILE_INFO);
            if (!gitBranch.empty()) {
                fmt::print(
                    "Git info:\n"
                    "-------------\n"
                    "Git Branch: {}\n",
                    gitBranch);
            }
            if (!gitCommitHash.empty()) {
                fmt::print("Git Commit: {}\n", gitCommitHash);
            }

            std::exit(EXIT_SUCCESS);
        }

        bool debug = opts["debug"].as<bool>();
        if (debug) {
            spdlog::set_level(spdlog::level::trace);
            spdlog::set_pattern("%Y-%m-%d %X.%e %^%6l%$: [%s:%#] %!(): %v");
        } else {
            spdlog::set_level(spdlog::level::info);
            spdlog::set_pattern("%Y-%m-%d %X %^%6l%$: %v");
        }

        std::optional<fs::path> logfile;
        if (opts.count("logfile") > 0) {
            logfile = opts["logfile"].as<fs::path>();
            if (!fs::directory_entry(logfile->parent_path()).exists()) {
                log_warning("Log dir {} missing", logfile->parent_path().c_str());
            }
            auto fileLogger = spdlog::basic_logger_mt("basic_logger", *logfile);
            spdlog::set_default_logger(fileLogger);
            spdlog::flush_on(spdlog::level::trace);
        }

        // Action starts here
        if (opts.count("create-config") == 0) {
            logCopyright();
        }

        std::optional<fs::path> home;
        if (opts.count("home") > 0) {
            auto homePath = opts["home"].as<std::vector<std::string>>();
            if (!homePath.empty()) {
                home = fs::path(homePath[0]);
                if (!fs::directory_entry(*home).exists()) {
                    log_warning("Home dir {} missing", home->c_str());
                }
            }
        }

        // are we requested to drop privs?
        std::optional<std::string> user;
        if (opts.count("user") > 0) {
            user = opts["user"].as<std::string>();

            // get actual euid/egid of process
            uid_t actualEuid = geteuid();

            // get user info of requested user from passwd
            auto userId = getpwnam(user->c_str());
            if (!userId) {
                log_error("Invalid user requested.");
                std::exit(EXIT_FAILURE);
            }

            // set home according to /etc/passwd entry
            if (!home.has_value()) {
                home = userId->pw_dir;
            }

            // we need to be euid root to become requested user/group
            if (actualEuid != 0) {
                log_error("Need to be root to change user.");
                std::exit(EXIT_FAILURE);
            }

            // set all uids, gids and add. groups
// macOS does this differently, setgid and setuid are basically doing the same
// as setresuid and setresgid on linux: setting all of real{u,g}id, effective{u,g}id and saved-set{u,g}id
// Solaroid systems are likewise missing setresgid and setresuid
#if defined(__APPLE__) || defined(SOLARIS) || defined(__CYGWIN__) || defined(__HAIKU__)
            // set group-ids, then add. groups, last user-ids, all need to succeed
            if (0 != setgid(userId->pw_gid) || 0 != initgroups(userId->pw_name, userId->pw_gid) || 0 != setuid(userId->pw_uid)) {
#else
            if (0 != setresgid(userId->pw_gid, userId->pw_gid, userId->pw_gid) || 0 != initgroups(userId->pw_name, userId->pw_gid) || 0 != setresuid(userId->pw_uid, userId->pw_uid, userId->pw_uid)) {
#endif
                log_error("Unable to change user.");
                std::exit(EXIT_FAILURE);
            }
            log_info("Dropped to User: {}", *user);
        }

        // are we requested to daemonize?
        bool daemon = opts["daemon"].as<bool>();
        if (daemon) {
            pid_t pid;

            // fork
            pid = fork();
            if (pid < 0) {
                log_error("Unable to fork.");
                std::exit(EXIT_FAILURE);
            }

            // terminate parent
            if (pid > 0)
                std::exit(EXIT_SUCCESS);

            // become session leader
            if (setsid() < 0) {
                log_error("Unable to setsid.");
                std::exit(EXIT_FAILURE);
            }

            // second fork
            pid = fork();
            if (pid < 0) {
                log_error("Unable to fork.");
                std::exit(EXIT_FAILURE);
            }

            // terminate parent
            if (pid > 0)
                std::exit(EXIT_SUCCESS);

            // set new file permissions
            umask(0);

            // change dir to /
            if (chdir("/")) {
                log_error("Unable to chdir to root dir.");
                std::exit(EXIT_FAILURE);
            }
            // close open filedescriptors belonging to a tty
            for (auto fd = int(sysconf(_SC_OPEN_MAX)); fd >= 0; fd--) {
                if (isatty(fd))
                    close(fd);
            }
            log_info("Daemonized.");
        }

        // are we requested to write a pidfile ?
        std::optional<fs::path> pidfile;
        if (opts.count("pidfile") > 0) {
            pidfile = opts["pidfile"].as<fs::path>();
            if (!fs::directory_entry(pidfile->parent_path()).exists()) {
                log_warning("pidfile parent dir {} missing", pidfile->parent_path().c_str());
            }

            // x will make it fail if file exists
            GrbFile pidFile(pidfile.value());
            auto pidf = pidFile.open("wx", false);
            if (!pidf) {
                log_error("fopen {} failed: {}", pidfile->c_str(), std::strerror(errno));
                log_error("Pidfile already exists or is not writable. It may be that gerbera is already");
                log_error("running or the file is a leftover from an unclean shutdown.");
                log_error("In that case, remove the file before starting gerbera.");
                std::exit(EXIT_FAILURE);
            }

            // get the pid of our process
            pid_t pid = getpid();
            if (pid <= 1) {
                log_error("Could not determine pid of running process.");
                std::exit(EXIT_FAILURE);
            }

            try {
                // write pid to file
                fmt::print(pidf, "{}\n", pid);
            } catch (const std::system_error& ex) {
                log_error("Could not write pidfile {}: {}.", pidfile->c_str(), ex.what());
                std::exit(EXIT_FAILURE);
            }
            log_debug("Wrote pidfile {}.", pidfile->c_str());
        }

        std::optional<fs::path> configFile;
        if (opts.count("config") > 0) {
            configFile = opts["config"].as<fs::path>();
        }

        std::optional<fs::path> confdir;
        if (opts.count("cfgdir") > 0) {
            confdir = opts["cfgdir"].as<fs::path>();
            if (!fs::directory_entry(*confdir).exists()) {
                log_warning("confdir {} missing", confdir->c_str());
            }
        }

        if (!confdir.has_value()) {
            confdir = DEFAULT_CONFIG_HOME;
        }

        // If home is not given by the user, get it from the environment
        if (!configFile.has_value() && !home.has_value()) {
            // Check XDG first
            const char* h = std::getenv("XDG_CONFIG_HOME");
            if (h) {
                home = h;
                confdir = "gerbera";
            } else {
                // Then look for home
                auto hEnv = getEnv("HOME");
                if (hEnv.has_value())
                    home = hEnv;
            }

            if (!home.has_value()) {
                log_error("Could not determine users home directory");
                std::exit(EXIT_FAILURE);
            }
            log_info("Home detected as: {}", home->c_str());
        }

        std::optional<std::string> dataDir = getEnv("DATADIR");
        if (!dataDir.has_value())
            dataDir = PACKAGE_DATADIR;

        if (opts.count("create-config") > 0 || opts.count("create-example-config") > 0) {
            ConfigGenerator configGenerator(opts.count("create-example-config") > 0);
            if (opts.count("cfgdir") == 0) {
                confdir = "";
            }
            std::string generated = configGenerator.generate(home.value_or(""), confdir.value_or(""), dataDir.value_or(""), magic.value_or(""));
            fmt::print("{}\n", generated);
            std::exit(EXIT_SUCCESS);
        }

        log_debug("Datadir is: {}", dataDir.value_or("unset"));

        bool offline = opts["offline"].as<bool>();
        if (offline)
            log_info("Running in offline mode");

        std::shared_ptr<ConfigManager> configManager;
        int portnum = -1;
        try {
            configManager = std::make_shared<ConfigManager>(
                configFile.value_or(""), home.value_or(""), confdir.value_or(""),
                dataDir.value_or(""), debug);
            configManager->load(home.value_or(""));
            handleOptionArgs(opts, configManager);
            handleAdditionalArgs(opts, additionalArgs, configManager);
            configManager->validate();
#ifdef GRBDEBUG
            GrbLogger::Logger.init(configManager->getIntOption(CFG_SERVER_DEBUG_MODE));
#endif
            if (opts.count("check-config") > 0) {
                std::exit(EXIT_SUCCESS);
            }
            portnum = configManager->getIntOption(CFG_SERVER_PORT);
        } catch (const ConfigParseException& ce) {
            log_error("Error parsing config file '{}': {}", configFile.value_or("").c_str(), ce.what());
            std::exit(EXIT_FAILURE);
        } catch (const std::runtime_error& e) {
            log_error("{}", e.what());
            std::exit(EXIT_FAILURE);
        }

        sigset_t maskSet;
        sigfillset(&maskSet);
        pthread_sigmask(SIG_SETMASK, &maskSet, nullptr);

        installSignalHandler();

        auto server = std::make_shared<Server>(configManager);

        try {
            server->init(offline);
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

            std::exit(EXIT_FAILURE);
        } catch (const std::runtime_error& e) {
            log_error("{}", e.what());
            std::exit(EXIT_FAILURE);
        }

        if (opts.count("add-file") > 0) {
            auto files = opts["add-file"].as<std::vector<fs::path>>();
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
                    std::exit(EXIT_FAILURE);
                }
            }
        }

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

                    try {
                        configManager = std::make_shared<ConfigManager>(
                            configFile.value_or(""), home.value_or(""), confdir.value_or(""),
                            dataDir.value_or(""), debug);
                        configManager->load(home.value_or(""));
                        handleOptionArgs(opts, configManager);
                        handleAdditionalArgs(opts, additionalArgs, configManager);
                        configManager->validate();
                        portnum = configManager->getIntOption(CFG_SERVER_PORT);
                    } catch (const ConfigParseException& ce) {
                        log_error("Error parsing config file '{}': {}", configFile.value_or("").c_str(), ce.what());
                        log_error("Could not restart Gerbera");
                        // at this point upnp shutdown has already been called
                        // therefore it is safe to exit
                        std::exit(EXIT_FAILURE);
                    } catch (const std::runtime_error& e) {
                        log_error("Error reloading configuration: {}",
                            e.what());
                        std::exit(EXIT_FAILURE);
                    }

                    ///  \todo fix this for SIGHUP
                    server = std::make_shared<Server>(configManager);
                    server->init(offline);
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

        // remove pidfile if one was written
        if (pidfile) {
            if (fs::remove(*pidfile)) {
                log_debug("Pidfile {} removed.", pidfile->c_str());
            } else {
                log_info("Could not remove pidfile {}", pidfile->c_str());
            }
        }

        log_info("Gerbera exiting. Have a nice day.");
        std::exit(ret);
    } catch (const cxxopts::OptionException& e) {
        fmt::print(stderr, "Failed to parse arguments: {}\n", e.what());
        std::exit(EXIT_FAILURE);
    }
}
