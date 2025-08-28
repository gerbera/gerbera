/*GRB*
Gerbera - https://gerbera.io/

    grb_runtime.h - this file is part of Gerbera.

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

/// \file grb_runtime.h
/// @brief Handling of command line arguments

#ifndef __GRB_RUNTIME_H__
#define __GRB_RUNTIME_H__

#include "config/config.h"
#include "contrib/cxxopts.hpp"
#include "util/grb_fs.h"

#include <fmt/core.h>
#include <functional>
#include <string>
#include <vector>

class Config;
class ConfigDefinition;
class Server;
enum class ConfigLevel;
namespace spdlog {
class logger;
}

using ParseCallback = std::function<bool(const std::string& arg)>;
using HandleCallback = std::function<bool()>;

#define GRB_OPTION_HELP "help"
#define GRB_OPTION_DEBUG "debug"
#define GRB_OPTION_VERSION "version"
#define GRB_OPTION_COMPILEINFO "compile-info"
#define GRB_OPTION_PRINTOPTIONS "print-options"
#define GRB_OPTION_LOGFILE "logfile"
#define GRB_OPTION_SYSLOG "syslog"
#define GRB_OPTION_ROTATELOG "rotatelog"
#define GRB_OPTION_CREATECONFIG "create-config"
#define GRB_OPTION_CREATEEXAMPLECONFIG "create-example-config"
#define GRB_OPTION_CREATEADVANCEDCONFIG "create-advanced-config"
#define GRB_OPTION_CHECKCONFIG "check-config"
#define GRB_OPTION_OFFLINE "offline"
#define GRB_OPTION_DAEMON "daemon"
#define GRB_OPTION_USER "user"
#define GRB_OPTION_HOME "home"
#define GRB_OPTION_PIDFILE "pidfile"
#define GRB_OPTION_CONFIG "config"
#define GRB_OPTION_CFGDIR "cfgdir"
#define GRB_OPTION_SETOPTION "set-option"
#define GRB_OPTION_ADDFILE "add-file"

#define DEFAULT_CONFIG_HOME ".config/gerbera"

/// @brief Structure for command line handling
struct ArgumentHandler {
    std::string argument;
    ParseCallback parseCallback;
    bool terminateWhenSet = false;
    HandleCallback handleCallback = nullptr;
};

/// @brief Structure for command line arguments
class ConfigOptionArgs {
public:
    ConfigVal option;
    std::string optShort;
    std::string optLong;
    std::string desc;
    std::optional<std::string> defaultValue;
    std::string argDesc;
};

/// @brief class to handle Gerbera command line arguments
class GerberaRuntime {
public:
    explicit GerberaRuntime();
    explicit GerberaRuntime(
        std::shared_ptr<ConfigDefinition> definition,
        const cxxopts::Options* options);
    /// @brief terminate gerbera
    void exit(int status)
    {
        cleanUp();
        shutdown();
        std::exit(status);
    }

    /// @brief: Initialise runtime static object
    void init(
        const std::shared_ptr<ConfigDefinition>& definition,
        const cxxopts::Options* options);
    /// @brief: Handle regular command line options
    void handleOptions(const cxxopts::ParseResult* results, bool startup = true);
    /// @brief: Handle command line options that are linked with configuration values
    void handleConfigOptions(const std::shared_ptr<Config>& configManager, const std::vector<ConfigOptionArgs>& additionalArgs);
    /// @brief: Handle command line options that require a running server
    void handleServerOptions(const std::shared_ptr<Server>& server);
    /// @brief: Release all loggers
    void shutdown();

    /// @brief access property for magic file
    std::optional<std::string> getMagic() { return magic; }
    /// @brief access property for home directory
    fs::path getHome() { return home.value_or(""); }
    /// @brief access property for configuration file
    fs::path getConfigFile() { return configFile.value_or(""); }
    /// @brief access property for configuration directory
    fs::path getConfDir() { return confDir.value_or(""); }
    /// @brief access property for data directory
    std::string getDataDir() { return dataDir.value_or(""); }
    /// @brief access property for PID file
    fs::path getPidFile() { return pidfile.value_or(""); }
    /// @brief access property for active debug flag
    bool getDebug() const { return debug; }
    /// @brief access property for active offline flag
    bool getOffline() const { return offline; }

    /// @brief Name of the appilcation from command line
    static const std::string ProgramName;

private:
    /// @brief Options for command line parsing
    const cxxopts::Options* options;
    /// @brief First run arguments
    std::vector<ArgumentHandler> argumentCallbacks;
    /// @brief Arguments to be read after config file loading
    std::vector<ArgumentHandler> argumentOptionCallbacks;
    /// @brief Arguments to be read after server activation
    std::vector<ArgumentHandler> argumentServerCallbacks;
    std::optional<fs::path> logfile;
    std::optional<fs::path> home;
    std::optional<std::string> user;
    std::optional<fs::path> pidfile;
    std::optional<fs::path> configFile;
    std::optional<fs::path> confDir;
    std::optional<std::string> magic = getEnv("MAGIC_FILE");
    std::optional<std::string> dataDir = getEnv("DATADIR");
    bool startup = false;
    bool debug = false;
    bool offline = false;
    ConfigLevel exampleConfigSet;
    int sections;
    bool createConfigSet = false;
    bool configDirSet = false;
    pid_t pid;
    const cxxopts::ParseResult* results;
    std::shared_ptr<Config> configManager;
    std::shared_ptr<ConfigDefinition> definition;
    std::shared_ptr<Server> server;
    std::vector<std::string> grbLoggers;
    std::shared_ptr<spdlog::logger> defaultLogger;

    /// @brief handler for help message
    bool printHelpMessage(const std::string& arg);
    /// @brief handler for debug mode
    bool setDebugMode(const std::string& arg);
    /// @brief handler for copyright
    bool printCopyright(const std::string& arg);
    /// @brief handler for options
    bool printOptions(const std::string& arg);
    /// @brief handler for compile info
    bool printCompileInfo(const std::string& arg);
    /// @brief handler for log file settings
    bool setLogFile(const std::string& arg);
    /// @brief handler for syslog
    bool setSyslog(const std::string& arg);
    /// @brief handler for log file rotation
    bool setRotatelog(const std::string& arg);
    /// @brief handler for config creation
    bool createConfig(const std::string& arg);
    /// @brief handler for example config creation
    bool createExampleConfig(const std::string& arg);
    /// @brief handler for advanced example config creation
    bool createAdvancedConfig(const std::string& arg);
    /// @brief handler for home directory
    bool setHome(const std::string& arg);
    /// @brief are we requested to drop privs?
    bool setUser(const std::string& arg);
    /// @brief are we requested to daemonize?
    bool runAsDeamon(const std::string& arg);
    /// @brief handler for offline mode
    bool setOfflineMode(const std::string& arg);
    /// @brief are we requested to write a pidfile ?
    bool createPidFile(const std::string& arg);
    /// @brief handler for config file
    bool setConfigFile(const std::string& arg);
    /// @brief handler for config directory
    bool setConfigDir(const std::string& arg);

    /// @brief handle command line arguments in configuration
    bool handleOptionArgs(const std::string& arg);

    /// @brief add file to server
    bool addFile(const std::string& arg);

    /// @brief print configuration
    bool printConfig();
    /// @brief generate configuration
    bool generateConfig();
    /// @brief check folders
    bool checkDirs();

    /// @brief print copyright message
    static void logCopyright();
    /// @brief load environment variables
    static std::optional<std::string> getEnv(const std::string& envVar);

    /// @brief end command line parsing
    void cleanUp();
    /// @brief handle list of command line arguments
    std::vector<std::pair<bool, HandleCallback>> executeOptions(const std::vector<ArgumentHandler>& callbacks);
    /// @brief run handlers after parsing all arguments
    void finalizeOptions(std::vector<std::pair<bool, HandleCallback>> finalHandlers);
    /// @brief run handlers additional arguments as config options
    void handleAdditionalArgs(const std::vector<ConfigOptionArgs>& additionalArgs);
};

#endif // __GRB_RUNTIME_H__
