/*GRB*
Gerbera - https://gerbera.io/

    grb_runtime.h - this file is part of Gerbera.

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

/// \file grb_runtime.h
/// \brief Handling of command line arguments

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
class Server;
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

struct ArgumentHandler {
    std::string argument;
    ParseCallback parseCallback;
    bool terminateWhenSet = false;
    HandleCallback handleCallback = nullptr;
};

class ConfigOptionArgs {
public:
    ConfigVal option;
    std::string optShort;
    std::string optLong;
    std::string desc;
    std::optional<std::string> defaultValue;
    std::string argDesc;
};

class GerberaRuntime {
public:
    explicit GerberaRuntime();
    explicit GerberaRuntime(const cxxopts::Options* options);
    void exit(int status)
    {
        cleanUp();
        shutdown();
        std::exit(status);
    }

    void init(const cxxopts::Options* options);
    // \brief: Handle regular command line options
    void handleOptions(const cxxopts::ParseResult* results, bool startup = true);
    // \brief: Handle command line options that are linked with configuration values
    void handleConfigOptions(const std::shared_ptr<Config>& configManager, const std::vector<ConfigOptionArgs>& additionalArgs);
    // \brief: Handle command line options that require a running server
    void handleServerOptions(const std::shared_ptr<Server>& server);
    // \brief: Release all loggers
    void shutdown();

    std::optional<std::string> getMagic() { return magic; }
    fs::path getHome() { return home.value_or(""); }
    fs::path getConfigFile() { return configFile.value_or(""); }
    fs::path getConfDir() { return confDir.value_or(""); }
    std::string getDataDir() { return dataDir.value_or(""); }
    fs::path getPidFile() { return pidfile.value_or(""); }
    bool getDebug() { return debug; }
    bool getOffline() { return offline; }

    static const std::string ProgramName;

private:
    const cxxopts::Options* options;
    std::vector<ArgumentHandler> argumentCallbacks;
    std::vector<ArgumentHandler> argumentOptionCallbacks;
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
    bool exampleConfigSet = false;
    bool createConfigSet = false;
    bool configDirSet = false;
    pid_t pid;
    const cxxopts::ParseResult* results;
    std::shared_ptr<Config> configManager;
    std::shared_ptr<Server> server;
    std::vector<std::string> grbLoggers;
    std::shared_ptr<spdlog::logger> defaultLogger;

    bool printHelpMessage(const std::string& arg);
    bool setDebugMode(const std::string& arg);
    bool printCopyright(const std::string& arg);
    bool printOptions(const std::string& arg);
    bool printCompileInfo(const std::string& arg);
    bool setLogFile(const std::string& arg);
    bool setSyslog(const std::string& arg);
    bool setRotatelog(const std::string& arg);
    bool createConfig(const std::string& arg);
    bool createExampleConfig(const std::string& arg);
    bool setHome(const std::string& arg);
    // are we requested to drop privs?
    bool setUser(const std::string& arg);
    // are we requested to daemonize?
    bool runAsDeamon(const std::string& arg);
    bool setOfflineMode(const std::string& arg);
    // are we requested to write a pidfile ?
    bool createPidFile(const std::string& arg);
    bool setConfigFile(const std::string& arg);
    bool setConfigDir(const std::string& arg);

    bool handleOptionArgs(const std::string& arg);

    bool addFile(const std::string& arg);

    bool printConfig();
    bool generateConfig();
    bool checkDirs();

    static void logCopyright();
    static std::optional<std::string> getEnv(const std::string& envVar);

    void cleanUp();
    std::vector<std::pair<bool, HandleCallback>> executeOptions(const std::vector<ArgumentHandler>& callbacks);
    void finalizeOptions(std::vector<std::pair<bool, HandleCallback>> finalHandlers);
    void handleAdditionalArgs(const std::vector<ConfigOptionArgs>& additionalArgs);
};

#endif // __GRB_RUNTIME_H__
