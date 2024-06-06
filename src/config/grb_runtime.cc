/*GRB*
Gerbera - https://gerbera.io/

    grb_runtime.cc - this file is part of Gerbera.

    Copyright (C) 2024 Gerbera Contributors

    Gerbera is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License version 2
    as published by the Free Software Foundation.

    Gerbera is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with Gerbera.  If not, see <http://www.gnu.org/licenses/>.
*/

/// \file grb_runtime.cc
#define LOG_FAC log_facility_t::server

#include "grb_runtime.h" // API

#include "common.h"
#include "config/config.h"
#include "config/config_definition.h"
#include "config/config_generator.h"
#include "config/config_manager.h"
#include "config/config_setup.h"
#include "content/content_manager.h"
#include "server.h"

// those are needed for -u user and -d daemonize options
#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif
#include <grp.h>
#include <pwd.h>

#include <spdlog/sinks/basic_file_sink.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

static constexpr auto gitBranch = std::string_view(GIT_BRANCH);
static constexpr auto gitCommitHash = std::string_view(GIT_COMMIT_HASH);

GerberaRuntime::GerberaRuntime()
    : confDir(DEFAULT_CONFIG_HOME)
{
}

GerberaRuntime::GerberaRuntime(const cxxopts::Options* options)
    : options(options)
    , confDir(DEFAULT_CONFIG_HOME)
{
    init(options);
}

void GerberaRuntime::init(const cxxopts::Options* options)
{
    this->options = options;
    if (!dataDir.has_value())
        dataDir = PACKAGE_DATADIR;

    argumentCallbacks = std::vector<ArgumentHandler> {
        { "help", [=](const std::string& arg) { return this->printHelpMessage(arg); }, true },
        { "version", [=](const std::string& arg) { return this->printCopyright(arg); }, true },
        { "print-options", [=](const std::string& arg) { return this->printOptions(arg); }, true },
        { "compile-info", [=](const std::string& arg) { return this->printCompileInfo(arg); }, true },
        { "debug", [=](const std::string& arg) { return this->setDebugMode(arg); } },
        { "logfile", [=](const std::string& arg) { return this->setLogFile(arg); } },
        { "create-config", [=](const std::string& arg) { return this->createConfig(arg); }, true, [=]() { return this->printConfig(); } },
        { "create-example-config", [=](const std::string& arg) { return this->createExampleConfig(arg); }, true, [=]() { return this->printConfig(); } },
        { "home", [=](const std::string& arg) { return this->setHome(arg); } },
        { "user", [=](const std::string& arg) { return this->setUser(arg); } },
        { "daemon", [=](const std::string& arg) { return this->runAsDeamon(arg); } },
        { "offline", [=](const std::string& arg) { return this->setOfflineMode(arg); } },
        { "pidfile", [=](const std::string& arg) { return this->createPidFile(arg); } },
        { "config", [=](const std::string& arg) { return this->setConfigFile(arg); } },
        { "cfgdir", [=](const std::string& arg) { return this->setConfigDir(arg); }, false, [=]() { return this->checkDirs(); } },
    };

    argumentOptionCallbacks = std::vector<ArgumentHandler> {
        { "set-option", [=](const std::string& arg) { return this->handleOptionArgs(arg); } },
    };

    argumentServerCallbacks = std::vector<ArgumentHandler> {
        { "add-file", [=](const std::string& arg) { return this->addFile(arg); } },
    };
}

bool GerberaRuntime::handleOptionArgs(const std::string& arg)
{
    if (!configManager)
        return false;

    auto setValueList = (*results)[arg].as<std::vector<std::string>>();
    for (auto&& valueString : setValueList) {
        auto valueList = splitString(valueString, '=');
        auto option = static_cast<config_option_t>(stoiString(valueList[0]));
        auto setup = ConfigDefinition::findConfigSetup(option);
        if (!setup)
            continue;
        if (setup->getTypeString() == "Boolean" && valueList.empty()) {
            setup->makeOption("yes", configManager);
        } else if (valueList.size() >= 2) {
            setup->makeOption(valueList[1], configManager);
        }
        auto value = !setup->getCurrentValue().empty() ? setup->getCurrentValue() : setup->getDefaultValue();
        log_debug("{} {} [{}] = {}", arg, option, setup->getUniquePath(), value);
    }
    return true;
}

bool GerberaRuntime::printOptions(const std::string& arg)
{
    auto setupList = ConfigDefinition::getOptionList();
    auto resultMap = std::map<config_option_t, std::string>();
    fmt::print("Active Options\n");
    for (auto&& setup : setupList) {
        if (setup && setup->getTypeString() != "List" && setup->option < CFG_MAX) {
            auto value = !setup->getCurrentValue().empty() ? setup->getCurrentValue() : setup->getDefaultValue();
            resultMap[setup->option] = fmt::format("[{}] : {} = {}", setup->getUniquePath(), setup->getTypeString(), value);
        }
    }
    for (auto&& [option, text] : resultMap) {
        fmt::print("{:3d} {}\n", option, text);
    }
    return true;
}

bool GerberaRuntime::setLogFile(const std::string& arg)
{
    logfile = (*results)[arg].as<fs::path>();
    if (!fs::directory_entry(logfile->parent_path()).exists()) {
        log_warning("Log dir {} missing", logfile->parent_path().c_str());
    }
    auto fileLogger = spdlog::basic_logger_mt("basic_logger", *logfile);
    spdlog::set_default_logger(fileLogger);
    spdlog::flush_on(spdlog::level::trace);
    return true;
}

bool GerberaRuntime::setHome(const std::string& arg)
{
    auto homePath = (*results)[arg].as<std::vector<std::string>>();
    if (!homePath.empty()) {
        home = fs::path(homePath[0]);
        if (!fs::directory_entry(*home).exists()) {
            log_warning("Home dir {} missing", home->c_str());
        }
    }
    return true;
}

// are we requested to drop privs?
bool GerberaRuntime::setUser(const std::string& arg)
{
    user = (*results)[arg].as<std::string>();

    // get actual euid/egid of process
    uid_t actualEuid = geteuid();

    // get user info of requested user from passwd
    auto userId = getpwnam(user->c_str());
    if (!userId) {
        log_error("Invalid user requested.");
        this->exit(EXIT_FAILURE);
    }

    // set home according to /etc/passwd entry
    if (!home.has_value()) {
        home = userId->pw_dir;
    }

    // we need to be euid root to become requested user/group
    if (actualEuid != 0) {
        log_error("Need to be root to change user.");
        this->exit(EXIT_FAILURE);
    }

    // set all uids, gids and add. groups
    // macOS does this differently, setgid and setuid are basically doing the same
    // as setresuid and setresgid on linux: setting all of real{u,g}id, effective{u,g}id and saved-set{u,g}id
    // Solaroid systems are likewise missing setresgid and setresuid
#if defined(__APPLE__) || defined(SOLARIS) || defined(__CYGWIN__) || defined(__HAIKU__)
    // set group-ids, then add. groups, last user-ids, all need to succeed
    if (0 != setgid(userId->pw_gid) || 0 != initgroups(userId->pw_name, userId->pw_gid) || 0 != setuid(userId->pw_uid)) //
#else
    if (0 != setresgid(userId->pw_gid, userId->pw_gid, userId->pw_gid) || 0 != initgroups(userId->pw_name, userId->pw_gid) || 0 != setresuid(userId->pw_uid, userId->pw_uid, userId->pw_uid)) //
#endif
    {
        log_error("Unable to change user.");
        this->exit(EXIT_FAILURE);
    }
    log_info("Dropped to User: {}", *user);
    return true;
}

// are we requested to daemonize?
bool GerberaRuntime::runAsDeamon(const std::string& arg)
{
    if (!startup)
        return false;

    // fork
    pid = fork();
    if (pid < 0) {
        log_error("Unable to fork.");
        this->exit(EXIT_FAILURE);
    }

    // terminate parent
    if (pid > 0) {
        log_debug("Leaving first Parent");
        std::exit(EXIT_SUCCESS);
    }

    // become session leader
    if (setsid() < 0) {
        log_error("Unable to setsid.");
        this->exit(EXIT_FAILURE);
    }

    // second fork
    pid = fork();
    if (pid < 0) {
        log_error("Unable to fork.");
        this->exit(EXIT_FAILURE);
    }

    // terminate parent
    if (pid > 0) {
        log_debug("Leaving second Parent");
        std::exit(EXIT_SUCCESS);
    }

    // set new file permissions
    umask(0);

    // change dir to /
    if (chdir("/")) {
        log_error("Unable to chdir to root dir.");
        this->exit(EXIT_FAILURE);
    }
    // close open filedescriptors belonging to a tty
    for (auto fd = static_cast<int>(sysconf(_SC_OPEN_MAX)); fd >= 0; fd--) {
        if (isatty(fd))
            close(fd);
    }
    log_info("Daemonized.");
    return (*results)[arg].as<bool>();
}

bool GerberaRuntime::createPidFile(const std::string& arg)
{
    pidfile = (*results)[arg].as<fs::path>();

    if (!startup)
        return false;

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
        this->exit(EXIT_FAILURE);
    }

    // get the pid of our process
    pid_t pid = getpid();
    if (pid <= 1) {
        log_error("Could not determine pid of running process.");
        this->exit(EXIT_FAILURE);
    }

    try {
        // write pid to file
        fmt::print(pidf, "{}\n", pid);
    } catch (const std::system_error& ex) {
        log_error("Could not write pidfile {}: {}.", pidfile->c_str(), ex.what());
        this->exit(EXIT_FAILURE);
    }
    log_debug("Wrote pidfile {}.", pidfile->c_str());
    return true;
}

bool GerberaRuntime::setConfigFile(const std::string& arg)
{
    configFile = (*results)[arg].as<fs::path>();
    return true;
}

bool GerberaRuntime::setConfigDir(const std::string& arg)
{
    confDir = (*results)[arg].as<fs::path>();
    if (!fs::directory_entry(*confDir).exists()) {
        log_warning("confdir {} missing", confDir->c_str());
    }
    configDirSet = true;
    return true;
}

bool GerberaRuntime::addFile(const std::string& arg)
{
    auto files = (*results)[arg].as<std::vector<fs::path>>();
    for (auto&& file : files) {
        try {
            std::error_code ec;
            auto dirEnt = fs::directory_entry(file, ec);
            if (!ec) {
                // add file/directory recursively and asynchronously
                AutoScanSetting asSetting;
                asSetting.followSymlinks = configManager->getBoolOption(CFG_IMPORT_FOLLOW_SYMLINKS);
                asSetting.recursive = true;
                asSetting.hidden = configManager->getBoolOption(CFG_IMPORT_HIDDEN_FILES);
                asSetting.rescanResource = false;
                asSetting.mergeOptions(configManager, file);
                server->getContent()->addFile(dirEnt, asSetting, true);
            } else {
                log_error("Failed to read {}: {}", file.c_str(), ec.message());
            }
        } catch (const std::runtime_error& e) {
            log_error("{}", e.what());
            this->exit(EXIT_FAILURE);
        }
    }

    return true;
}

bool GerberaRuntime::printCompileInfo(const std::string& arg)
{
    printCopyright(arg);
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

    return true;
}

bool GerberaRuntime::createExampleConfig(const std::string& arg)
{
    exampleConfigSet = true;
    return true;
}

bool GerberaRuntime::printCopyright(const std::string& arg)
{
    fmt::print("\nGerbera UPnP Server {}\n"
               "Copyright 2016-2024 Gerbera Contributors.\n"
               "Licence GPLv2: GNU GPL version 2.\n"
               "This is free software: you are free to change and redistribute it.\n\n",
        GERBERA_VERSION);

    return true;
}

bool GerberaRuntime::createConfig(const std::string& arg)
{
    createConfigSet = true;
    return true;
}

bool GerberaRuntime::setOfflineMode(const std::string& arg)
{
    offline = (*results)[arg].as<bool>();
    if (offline)
        log_info("Running in offline mode");

    return offline;
}

bool GerberaRuntime::checkDirs()
{
    if (!confDir.has_value()) {
        confDir = DEFAULT_CONFIG_HOME;
    }
    // If home is not given by the user, get it from the environment
    // Check XDG first
    const char* h = std::getenv("XDG_CONFIG_HOME");
    if (h) {
        home = h;
        confDir = "gerbera";
    } else {
        // Then look for home
        auto hEnv = getEnv("HOME");
        if (hEnv.has_value())
            home = hEnv;
    }

    if (!home.has_value()) {
        log_error("Could not determine users home directory");
        this->exit(EXIT_FAILURE);
    }
    log_info("Home detected as: {}", home->c_str());

    return true;
}

bool GerberaRuntime::printConfig()
{
    ConfigGenerator configGenerator(exampleConfigSet);
    if (!configDirSet) {
        confDir = "";
    }
    std::string generated = configGenerator.generate(getHome(), getConfDir(), getDataDir(), getMagic().value_or(""));
    fmt::print("{}\n", generated);

    return true;
}

void GerberaRuntime::logCopyright()
{
    log_info("Gerbera UPnP Server {} - {}", GERBERA_VERSION, DESC_MANUFACTURER_URL);
    log_info("Copyright 2016-2024 Gerbera Contributors.");
    log_info("Gerbera is free software, covered by the GNU General Public License version 2");
}

std::optional<std::string> GerberaRuntime::getEnv(const std::string& envVar)
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

bool GerberaRuntime::setDebugMode(const std::string& arg)
{
    debug = (*results)[arg].as<bool>();
    spdlog::set_level(spdlog::level::trace);
    spdlog::set_pattern("%Y-%m-%d %X.%e %^%6l%$: [%s:%#] %!(): %v");

    return debug;
}

bool GerberaRuntime::printHelpMessage(const std::string& arg)
{
    fmt::print("{}\n", options->help({ "", "Group" }));
    return true;
}

void GerberaRuntime::handleOptions(const cxxopts::ParseResult* results, bool startup)
{
    if (!options || !results)
        return;

    this->startup = startup;
    if (startup) {
        spdlog::set_level(spdlog::level::info);
        spdlog::set_pattern("%Y-%m-%d %X %^%6l%$: %v");
    }
    this->results = results;
    auto finalHandlers = executeOptions(argumentCallbacks);
    // Action starts here
    offline = (*results)["offline"].as<bool>();
    if (!createConfigSet && !exampleConfigSet && startup)
        logCopyright();
    finalizeOptions(finalHandlers);

    log_debug("Datadir is: '{}'", dataDir.value_or("unset"));
}

std::vector<std::pair<bool, HandleCallback>> GerberaRuntime::executeOptions(const std::vector<ArgumentHandler>& callbacks)
{
    std::vector<std::pair<bool, HandleCallback>> finalHandlers = {};
    for (auto&& ocb : callbacks) {
        if (results->count(ocb.argument) > 0) {
            if (ocb.parseCallback) {
                auto parsed = ocb.parseCallback(ocb.argument);
                if (parsed && ocb.terminateWhenSet && !ocb.handleCallback)
                    this->exit(EXIT_SUCCESS);
                if (parsed && ocb.handleCallback)
                    finalHandlers.emplace_back(ocb.terminateWhenSet, ocb.handleCallback);
            }
        }
    }
    return finalHandlers;
}

void GerberaRuntime::finalizeOptions(std::vector<std::pair<bool, HandleCallback>> finalHandlers)
{
    for (auto&& [terminate, handler] : finalHandlers) {
        bool handled = handler();
        if (terminate && handled)
            this->exit(EXIT_SUCCESS);
    }
}

void GerberaRuntime::handleConfigOptions(const std::shared_ptr<ConfigManager>& configManager, const std::vector<ConfigOptionArgs>& additionalArgs)
{
    if (!options || !results)
        return;

    this->configManager = configManager;

    auto finalHandlers = executeOptions(argumentOptionCallbacks);
    finalizeOptions(finalHandlers);
    handleAdditionalArgs(additionalArgs);
}

void GerberaRuntime::handleServerOptions(const std::shared_ptr<Server>& server)
{
    if (!options || !results)
        return;

    this->server = server;

    auto finalHandlers = executeOptions(argumentOptionCallbacks);
    finalizeOptions(finalHandlers);
}

void GerberaRuntime::handleAdditionalArgs(const std::vector<ConfigOptionArgs>& additionalArgs)
{
    for (auto&& addArg : additionalArgs) {
        if (results->count(addArg.optLong) > 0) {
            auto valueList = (*results)[addArg.optLong].as<std::vector<std::string>>();
            auto setup = ConfigDefinition::findConfigSetup(addArg.option);
            if (!setup)
                continue;
            if (setup->getTypeString() == "Boolean" && valueList.empty()) {
                setup->makeOption("yes", configManager);
            } else if (!valueList.empty()) {
                setup->makeOption(fmt::format("{}", fmt::join(valueList, ",")), configManager);
            }
            log_debug("addArg {} [{}] = {}", addArg.option, setup->getUniquePath(), !setup->getCurrentValue().empty() ? setup->getCurrentValue() : setup->getDefaultValue());
        } else if (addArg.defaultValue) {
            auto setup = ConfigDefinition::findConfigSetup(addArg.option);
            setup->makeOption(addArg.defaultValue.value(), configManager);
            auto value = !setup->getCurrentValue().empty() ? setup->getCurrentValue() : setup->getDefaultValue();
            log_debug("addArg {} [{}] = {}", addArg.option, setup->getUniquePath(), !setup->getCurrentValue().empty() ? setup->getCurrentValue() : setup->getDefaultValue());
        }
    }
}

void GerberaRuntime::cleanUp()
{
    // remove pidfile if one was written
    if (pidfile && startup) {
        std::error_code ec;
        if (fs::remove(*pidfile, ec)) {
            log_debug("Pidfile {} removed.", pidfile->c_str());
        } else {
            log_info("Could not remove pidfile {}: {}", pidfile->c_str(), ec.message());
        }
    }
}
