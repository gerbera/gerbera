/*GRB*
Gerbera - https://gerbera.io/

    grb_runtime.cc - this file is part of Gerbera.

    Copyright (C) 2024-2025 Gerbera Contributors

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
#define GRB_LOG_FAC GrbLogFacility::server

#include "grb_runtime.h" // API

#include "common.h"
#include "config/config.h"
#include "config/config_definition.h"
#include "config/config_generator.h"
#include "config/config_setup.h"
#include "config/config_val.h"
#include "content/autoscan_setting.h"
#include "content/content.h"
#include "server.h"

// those are needed for -u user and -d daemonize options
#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif

#include <grp.h>
#include <pwd.h>
#include <spdlog/sinks/basic_file_sink.h>
#include <spdlog/sinks/rotating_file_sink.h>
#include <spdlog/sinks/syslog_sink.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

GerberaRuntime::GerberaRuntime()
    : confDir(DEFAULT_CONFIG_HOME)
{
}

const std::string GerberaRuntime::ProgramName = "gerbera";

static constexpr std::array syslogFlags {
    std::pair("USER", LOG_USER),
    std::pair("LOCAL0", LOG_LOCAL0),
    std::pair("LOCAL1", LOG_LOCAL1),
    std::pair("LOCAL2", LOG_LOCAL2),
    std::pair("LOCAL3", LOG_LOCAL3),
    std::pair("LOCAL4", LOG_LOCAL4),
    std::pair("LOCAL5", LOG_LOCAL5),
    std::pair("LOCAL6", LOG_LOCAL6),
    std::pair("LOCAL7", LOG_LOCAL7),
};

static int remapSyslogFlag(const std::string& flag)
{
    for (auto [qLabel, qFlag] : syslogFlags) {
        if (flag == qLabel) {
            return qFlag;
        }
    }
    return stoiString(flag, 0, 0);
}

GerberaRuntime::GerberaRuntime(
    std::shared_ptr<ConfigDefinition> definition,
    const cxxopts::Options* options)
    : options(options)
    , confDir(DEFAULT_CONFIG_HOME)
    , definition(std::move(definition))
{
    init(nullptr, options);
}

void GerberaRuntime::init(
    const std::shared_ptr<ConfigDefinition>& definition,
    const cxxopts::Options* options)
{
    if (definition)
        this->definition = definition;
    this->options = options;

    argumentCallbacks = std::vector<ArgumentHandler> {
        { GRB_OPTION_HELP, [=](const std::string& arg) { return this->printHelpMessage(arg); }, true },
        { GRB_OPTION_DEBUG, [=](const std::string& arg) { return this->setDebugMode(arg); } },
        { GRB_OPTION_VERSION, [=](const std::string& arg) { return this->printCopyright(arg); }, true },
        { GRB_OPTION_COMPILEINFO, [=](const std::string& arg) { return this->printCompileInfo(arg); }, true },
        { GRB_OPTION_PRINTOPTIONS, [=](const std::string& arg) { return this->printOptions(arg); }, true },
        { GRB_OPTION_LOGFILE, [=](const std::string& arg) { return this->setLogFile(arg); } },
        { GRB_OPTION_SYSLOG, [=](const std::string& arg) { return this->setSyslog(arg); } },
        { GRB_OPTION_CREATECONFIG, [=](const std::string& arg) { return this->createConfig(arg); }, true, [=]() { return this->printConfig(); } },
        { GRB_OPTION_CREATEEXAMPLECONFIG, [=](const std::string& arg) { return this->createExampleConfig(arg); }, true, [=]() { return this->printConfig(); } },
        { GRB_OPTION_HOME, [=](const std::string& arg) { return this->setHome(arg); } },
        { GRB_OPTION_USER, [=](const std::string& arg) { return this->setUser(arg); } },
        { GRB_OPTION_DAEMON, [=](const std::string& arg) { return this->runAsDeamon(arg); } },
        { GRB_OPTION_OFFLINE, [=](const std::string& arg) { return this->setOfflineMode(arg); } },
        { GRB_OPTION_PIDFILE, [=](const std::string& arg) { return this->createPidFile(arg); } },
        { GRB_OPTION_CONFIG, [=](const std::string& arg) { return this->setConfigFile(arg); } },
        { GRB_OPTION_CFGDIR, [=](const std::string& arg) { return this->setConfigDir(arg); } },
    };

    argumentOptionCallbacks = std::vector<ArgumentHandler> {
        { GRB_OPTION_SETOPTION, [=](const std::string& arg) { return this->handleOptionArgs(arg); } },
        { GRB_OPTION_ROTATELOG, [=](const std::string& arg) { return this->setRotatelog(arg); } },
    };

    argumentServerCallbacks = std::vector<ArgumentHandler> {
        { GRB_OPTION_ADDFILE, [=](const std::string& arg) { return this->addFile(arg); } },
    };
}

void GerberaRuntime::shutdown()
{
    for (auto&& grbLogger : grbLoggers) {
        spdlog::drop(grbLogger);
    }
    if (defaultLogger)
        spdlog::set_default_logger(defaultLogger);
}

bool GerberaRuntime::handleOptionArgs(const std::string& arg)
{
    if (!configManager)
        return false;

    auto setValueList = (*results)[arg].as<std::vector<std::string>>();
    for (auto&& valueString : setValueList) {
        auto valueList = splitString(valueString, '=');
        auto option = static_cast<ConfigVal>(stoiString(valueList[0]));
        auto setup = definition->findConfigSetup(option);
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
    auto setupList = definition->getOptionList();
    auto resultMap = std::map<ConfigVal, std::string>();
    fmt::print("Active Options\n");
    for (auto&& setup : setupList) {
        if (setup && setup->getTypeString() != "List" && setup->option < ConfigVal::MAX) {
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
    auto fileLogger = spdlog::basic_logger_mt("file_logger", *logfile);
    if (!defaultLogger)
        defaultLogger = spdlog::default_logger();
    spdlog::set_default_logger(fileLogger);
    spdlog::flush_on(spdlog::level::trace);
    grbLoggers.push_back("file_logger");
    return true;
}

bool GerberaRuntime::setRotatelog(const std::string& arg)
{
    auto max_size = configManager->getUIntOption(ConfigVal::SERVER_LOG_ROTATE_SIZE);
    auto max_files = configManager->getUIntOption(ConfigVal::SERVER_LOG_ROTATE_COUNT);
    logfile = (*results)[arg].as<fs::path>();
    if (!fs::directory_entry(logfile->parent_path()).exists()) {
        log_warning("Log dir {} missing", logfile->parent_path().c_str());
    }
    auto rotateLogger = spdlog::rotating_logger_mt("rotate_logger", *logfile, max_size, max_files);
    if (!defaultLogger)
        defaultLogger = spdlog::default_logger();
    spdlog::set_default_logger(rotateLogger);
    spdlog::flush_on(spdlog::level::trace);
    grbLoggers.push_back("rotate_logger");
    return true;
}

bool GerberaRuntime::setSyslog(const std::string& arg)
{
    std::optional<std::string> logLevelStr = (*results)[arg].as<std::string>();
    auto logLevel = remapSyslogFlag(logLevelStr.value_or("USER"));
    if (logLevel == 0) {
        log_warning("Unknown Log facility {}, using USER", logLevelStr.value_or(""));
    }
    auto sysLogger = spdlog::syslog_logger_mt("syslog_logger", ProgramName, LOG_PID, logLevel, true);
    if (!defaultLogger)
        defaultLogger = spdlog::default_logger();
    spdlog::set_default_logger(sysLogger);
    spdlog::flush_on(spdlog::level::trace);
    grbLoggers.push_back("syslog_logger");
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
        log_warning("Config Dir {} missing", confDir->c_str());
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
                asSetting.followSymlinks = configManager->getBoolOption(ConfigVal::IMPORT_FOLLOW_SYMLINKS);
                asSetting.recursive = true;
                asSetting.hidden = configManager->getBoolOption(ConfigVal::IMPORT_HIDDEN_FILES);
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

bool GerberaRuntime::printCopyright(const std::string& arg)
{
    fmt::print("\nGerbera UPnP Server {}\n"
               "Copyright 2016-2025 Gerbera Contributors.\n"
               "Licence GPLv2: GNU GPL version 2.\n"
               "This is free software: you are free to change and redistribute it.\n\n",
        GERBERA_VERSION);

    return true;
}

bool GerberaRuntime::createExampleConfig(const std::string& arg)
{
    exampleConfigSet = true;
    std::optional<std::string> sectionString = (*results)[arg].as<std::string>();

    sections = ConfigGenerator::makeSections(sectionString.value_or("All"));
    return true;
}

bool GerberaRuntime::createConfig(const std::string& arg)
{
    createConfigSet = true;
    std::optional<std::string> sectionString = (*results)[arg].as<std::string>();

    sections = ConfigGenerator::makeSections(sectionString.value_or("All"));
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
    if (!configFile.has_value() && !home.has_value()) {
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
    }

    if (!dataDir.has_value())
        dataDir = PACKAGE_DATADIR;
    return true;
}

bool GerberaRuntime::printConfig()
{
    ConfigGenerator configGenerator(definition, GERBERA_VERSION, exampleConfigSet, sections);
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
    log_info("Copyright 2016-2025 Gerbera Contributors.");
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
    checkDirs();
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

void GerberaRuntime::handleConfigOptions(const std::shared_ptr<Config>& configManager, const std::vector<ConfigOptionArgs>& additionalArgs)
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

    auto finalHandlers = executeOptions(argumentServerCallbacks);
    finalizeOptions(finalHandlers);
}

void GerberaRuntime::handleAdditionalArgs(const std::vector<ConfigOptionArgs>& additionalArgs)
{
    for (auto&& addArg : additionalArgs) {
        if (results->count(addArg.optLong) > 0) {
            auto valueList = (*results)[addArg.optLong].as<std::vector<std::string>>();
            auto setup = definition->findConfigSetup(addArg.option);
            if (!setup)
                continue;
            if (setup->getTypeString() == "Boolean" && valueList.empty()) {
                setup->makeOption("yes", configManager);
            } else if (!valueList.empty()) {
                setup->makeOption(fmt::format("{}", fmt::join(valueList, ",")), configManager);
            }
            log_debug("addArg {} [{}] = {}", addArg.option, setup->getUniquePath(), !setup->getCurrentValue().empty() ? setup->getCurrentValue() : setup->getDefaultValue());
        } else if (addArg.defaultValue) {
            auto setup = definition->findConfigSetup(addArg.option);
            setup->makeOption(addArg.defaultValue.value(), configManager);
            auto value = !setup->getCurrentValue().empty() ? setup->getCurrentValue() : setup->getDefaultValue();
            log_debug("addArg {} [{}] = {}", addArg.option, setup->getUniquePath(), !setup->getCurrentValue().empty() ? setup->getCurrentValue() : setup->getDefaultValue());
        }
    }
}

void GerberaRuntime::cleanUp()
{
    // remove pidfile if one was written
    log_debug("Cleanup {}", pidfile && startup);
    if (pidfile && startup) {
        std::error_code ec;
        if (fs::remove(*pidfile, ec)) {
            log_debug("Pidfile {} removed.", pidfile->c_str());
        } else if (ec) {
            log_info("Could not remove pidfile {}: {}", pidfile->c_str(), ec.message());
        }
    }
}
