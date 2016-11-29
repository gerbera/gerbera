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

#include "common.h"
#include "singleton.h"
#include "server.h"
#include "process.h"

#include <getopt.h>

#include "config_manager.h"
#include "content_manager.h"
#include "timer.h"
#include "common.h"

#include <sys/types.h>
#include <sys/stat.h>
#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <errno.h>
#include <unistd.h>
#include <syslog.h>
#include <string.h>
#include <pthread.h>
#include <signal.h>
#include <pwd.h>
#include <grp.h>
#include <limits.h>

#define OPTSTR "i:e:p:c:m:f:u:g:a:l:P:dhD"

using namespace zmm;

int shutdown_flag = 0;
int restart_flag = 0;
pthread_t main_thread_id;
Ref<Timer> timer = nil;

void print_copyright()
{
    printf("\nMediaTomb UPnP Server version %s - %s\n\n", VERSION, 
            DESC_MANUFACTURER_URL);
    printf("===============================================================================\n");
    printf("Copyright 2005-2010 Gena Batsyan, Sergey Bostandzhyan, Leonhard Wimmer.\n");
    printf("MediaTomb is free software, covered by the GNU General Public License version 2\n\n");
}
void signal_handler(int signum);

int main(int argc, char **argv, char **envp)
{
    char     * err = NULL;
    int      port = -1;
    bool     daemon = false;
    struct   sigaction action;
    sigset_t mask_set;

    int      devnull = -1;
    int      redirect1 = -1;
    int      redirect2 = -1;
    struct   passwd *pwd;
    struct   group  *grp;

    int      opt_index = 0;
    int      o;
    static struct option long_options[] = {
        {"ip", 1, 0, 'i'},          // 0
        {"interface", 1, 0, 'e'},   // 1
        {"port", 1, 0, 'p'},        // 2
        {"config", 1, 0, 'c'},      // 3
        {"home", 1, 0, 'm'},        // 4
        {"cfgdir", 1, 0, 'f'},      // 5
        {"user", 1, 0, 'u'},        // 6
        {"group", 1, 0, 'g'},       // 7
        {"daemon", 0, 0, 'd'},      // 8
        {"pidfile", 1, 0, 'P'},     // 9
        {"add", 1, 0, 'a'},         // 10
        {"logfile", 1, 0, 'l'},     // 11
        {"debug", 0, 0, 'D'},       // 12
        {"compile-info", 0, 0, 0},  // 13
        {"version", 0, 0, 0},       // 14
        {"help", 0, 0, 'h'},        // 15
        {0, 0, 0, 0}                
    };

    String config_file;
    String home;
    String confdir;
    String user;
    String group;
    String pid_file;
    FILE* pid_fd = NULL;
    String interface;
    String ip;
    String prefix;
    String magic;
    bool debug_logging = false;
    bool print_version = false;

    Ref<Array<StringBase> > addFile(new Array<StringBase>());

#ifdef SOLARIS
    String ld_preload;
    char *preload = getenv("LD_PRELOAD");
    if (preload != NULL)
        ld_preload = String(preload);

    if ((preload == NULL) || (ld_preload.find("0@0") == -1))
    {
        printf("MediaTomb: Solaris check failed!\n");
        printf("Please set the environment to match glibc behaviour!\n");
        printf("LD_PRELOAD=/usr/lib/0@0.so.1\n");
        exit(EXIT_FAILURE);
    }
#endif

    while (1)
    {
        o = getopt_long(argc, argv, OPTSTR, long_options, &opt_index);
        if (o == -1) break;

        switch (o)
        {
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
                if ((port == 0) && (*err))
                {
                    log_error("Invalid port argument: %s\n", optarg);
                    exit(EXIT_FAILURE);
                }

                if (port > USHRT_MAX)
                {
                    log_error("Invalid port value %d. Maximum allowed port value is %d\n", port, USHRT_MAX);
                }
                log_debug("port set to: %d\n", port);
                break;

            case 'c':
                log_debug("Option config with param %s\n", optarg);
                config_file = optarg;
                break;

            case 'd':
                log_debug("Starting in daemon mode...");
                daemon = true;
                break;

            case 'u':
                log_debug("Running as user: %s\n", optarg);
                user = optarg;
                break;

            case 'g':
                log_debug("Running as group: %s\n", optarg);
                group = optarg;
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
                
#ifndef DEBUG_LOG_ENABLED
                print_copyright();
                printf("ERROR: MediaTomb wasn't compiled with debug output, but was run with -D/--debug.\n");
                exit(EXIT_FAILURE);
#endif
                log_debug("enabling debug output\n");
                debug_logging = true;
                break;
            case '?':
            case 'h':
                print_copyright();
                printf("Usage: mediatomb [options]\n\
                        \n\
Supported options:\n\
    --ip or -i         ip address to bind to\n\
    --interface or -e  network interface to bind to\n\
    --port or -p       server port (the SDK only permits values >= 49152)\n\
    --config or -c     configuration file to use\n\
    --daemon or -d     run server in background\n\
    --home or -m       define the home directory\n\
    --cfgdir or -f     name of the directory that is holding the configuration\n\
    --pidfile or -P    file to hold the process id\n\
    --user or -u       run server under specified username\n\
    --group or -g      run server under specified group\n\
    --add or -a        add the given file/directory\n\
    --logfile or -l    log to specified file\n\
    --debug or -D      enable debug output\n\
    --compile-info     print configuration summary and exit\n\
    --version          print version information and exit\n\
    --help or -h       this help message\n\
\n\
For more information visit " DESC_MANUFACTURER_URL "\n\n");

                exit(EXIT_FAILURE);
                break;
            case 0:
                switch (opt_index)
                {
                case 13: // --compile-info
                    print_copyright();
                    printf("Compile info:\n");
                    printf("-------------\n");
                    printf("%s\n\n", COMPILE_INFO);
                    exit(EXIT_SUCCESS);
                    break;
                case 14: // --version
                    print_version = true;
                    break;
                }
                break;
            default:
                break;
         }
    }

    if (print_version || ! daemon)
    {
        print_copyright();
       
        if (print_version)
            exit(EXIT_FAILURE);
    }

   // create pid file before dropping privileges
    if (pid_file != nil)
    {
        pid_fd = fopen(pid_file.c_str(), "w");
        if (pid_fd == NULL)
        {
            log_error("Could not write pid file %s : %s\n",
                       pid_file.c_str(), strerror(errno));
        }
    }

    // check if user and/or group parameter was specified and try to run the server
    // under the given user and/or group name
    if (group != nil)
    {
        grp = getgrnam(group.c_str());
        if (grp == NULL)
        {
            log_error("Group %s not found!\n", group.c_str());
            exit(EXIT_FAILURE);
        }
        
        if (setgid(grp->gr_gid) < 0)
        {
            log_error("setgid failed %s\n", strerror(errno));
            exit(EXIT_FAILURE);
        }
        
        // remove supplementary groups
        if (setgroups(0, NULL) < 0)
        {
            log_error("setgroups failed %s\n", strerror(errno));
            exit(EXIT_FAILURE);
        }
    }
    
    if (user != nil)
    {
        pwd = getpwnam(user.c_str());
        if (pwd == NULL)
        {
            log_error("User %s not found!\n", user.c_str());
            exit(EXIT_FAILURE);
        }
        
        // set supplementary groups
        if (initgroups(user.c_str(), getegid()) < 0)
        {
            log_error("initgroups failed %s\n", strerror(errno));
            exit(EXIT_FAILURE);
        }
        
        if (setuid(pwd->pw_uid) < 0)
        {
            log_error("setuid failed %s\n", strerror(errno));
            exit(EXIT_FAILURE);
        }
        
    }

    try
    {
        // if home is not given by the user, get it from the environment
        if (!string_ok(home) && (!string_ok(config_file)))
        {
#ifndef __CYGWIN__
            char *h = getenv("HOME");
            if (h != NULL)
                home = h;

#else
            char *h = getenv("HOMEPATH");
            char *d = getenv("HOMEDRIVE");
            if ((h != NULL)  && (d != NULL))
                home = d + h;

#endif  // __CYGWIN__
        }

        if (!string_ok(home) && (!string_ok(config_file)))
        {
            log_error("Could not determine users home directory\n");
            exit(EXIT_FAILURE);
        }

        if (!string_ok(confdir))
            confdir = _(DEFAULT_CONFIG_HOME);

        char *pref = getenv("MEDIATOMB_DATADIR");
        if (pref != NULL)
            prefix = pref;

        if (!string_ok(prefix))
            prefix = _(PACKAGE_DATADIR);

        char *mgc = getenv("MEDIATOMB_MAGIC_FILE");
        if (mgc != NULL)
            magic = mgc;

        if (!string_ok(magic))
            magic = nil;

        ConfigManager::setStaticArgs(config_file, home, confdir, prefix, magic, debug_logging);
        ConfigManager::getInstance();
    }
    catch (mxml::ParseException pe)
    {
        log_error("Error parsing config file: %s line %d:\n%s\n",
               pe.context->location.c_str(),
               pe.context->line,
               pe.getMessage().c_str());
        exit(EXIT_FAILURE);
    }
    catch (Exception e)
    {
        log_error("%s\n", e.getMessage().c_str());
        exit(EXIT_FAILURE);
    }    
    
    // starting as daemon if applicable
    if (daemon)
    {
        // The daemon startup code was taken from the Linux Daemon Writing HOWTO
        // written by Devin Watson <dmwatson@comcast.net>
        // The HOWTO is Copyright by Devin Watson, under the terms of the BSD License.
        
        /* Our process ID and Session ID */
        pid_t pid, sid; 
        

        /* Fork off the parent process */
        pid = fork();
        if (pid < 0) 
        {
            log_error("Failed to fork: %s\n", strerror(errno));
            exit(EXIT_FAILURE);
        }
        /* If we got a good PID, then
           we can exit the parent process. */
        if (pid > 0) 
        {
            exit(EXIT_SUCCESS);
        }

        /* Change the file mode mask */
        umask(0133);

        /* Open any logs here */        

        /* Create a new SID for the child process */
        sid = setsid();
        if (sid < 0) 
        {
            log_error("setsid failed: %s\n", strerror(errno));
            exit(EXIT_FAILURE);
        }

        /* Change the current working directory */
        if ((chdir("/")) < 0) 
        {
            log_error("Failed to chdir to / : %s\n", strerror(errno));
            exit(EXIT_FAILURE);
        }

        /* Close out the standard file descriptors */        
        close(STDIN_FILENO);
        close(STDOUT_FILENO);
        close(STDERR_FILENO);

        devnull = open("/dev/null", O_RDWR);
        if (devnull == -1)
        {
            log_error("Failed to open /dev/null: %s\n", strerror(errno));
            exit(EXIT_FAILURE);
        }

        redirect1 = dup(devnull);
        if (redirect1 == -1)
        {
            log_error("Failed to redirect output: %s\n", strerror(errno));
            close(devnull);
            exit(EXIT_FAILURE);
        }

        redirect2 = dup(devnull);
        if (redirect2 == -1)
        {
            log_error("Failed to redirect output: %s\n", strerror(errno));
            close(devnull);
            close(redirect1);
            exit(EXIT_FAILURE);
        }

    }

    if (pid_fd != NULL)
    {
        pid_t cur_pid;
        int     size;

        cur_pid = getpid();
        String pid = String::from(cur_pid);

        size = fwrite(pid.c_str(), sizeof(char), pid.length(), pid_fd);
        fclose(pid_fd);

        if (size < pid.length())
            log_error("Error when writing pid file %s : %s\n",
                      pid_file.c_str(), strerror(errno));
    }

    main_thread_id = pthread_self();
    // install signal handlers
    sigfillset(&mask_set);
    pthread_sigmask(SIG_SETMASK, &mask_set, NULL);

    memset(&action, 0, sizeof(action));
    action.sa_handler = signal_handler;
    action.sa_flags = 0;
    sigfillset(&action.sa_mask);
    if (sigaction(SIGINT, &action, NULL) < 0)
    {
        log_error("Could not register SIGINT handler!\n");
    }

    if (sigaction(SIGTERM, &action, NULL) < 0)
    {
        log_error("Could not register SIGTERM handler!\n");
    }

    if (sigaction(SIGHUP, &action, NULL) < 0)
    {
        log_error("Could not register SIGHUP handler!\n");
    }

    if (sigaction(SIGPIPE, &action, NULL) < 0)
    {
        log_error("Could not register SIGPIPE handler!\n");
    }

    Ref<SingletonManager> singletonManager = SingletonManager::getInstance();
    Ref<Server> server;
    try
    {
        server = Server::getInstance();
        server->upnp_init(interface, ip, port);
    }
    catch(UpnpException upnp_e)
    {

        sigemptyset(&mask_set);
        pthread_sigmask(SIG_SETMASK, &mask_set, NULL);

        upnp_e.printStackTrace();
        log_error("main: upnp error %d\n", upnp_e.getErrorCode());
        if (upnp_e.getErrorCode() == UPNP_E_SOCKET_BIND)
        {
            log_error("Could not bind to socket.\n");
            log_info("Please check if another instance of MediaTomb or\n");
            log_info("another application is running on port %d.\n", port);
        }
        else if (upnp_e.getErrorCode() == UPNP_E_SOCKET_ERROR)
        {
            log_error("Socket error.\n");
            log_info("Please check if your network interface was configured for multicast!\n");
            log_info("Refer to the README file for more information.\n");
        }
        
        try
        {
            singletonManager->shutdown(true);
        }
        catch (Exception e)
        {
            log_error("%s\n", e.getMessage().c_str());
            e.printStackTrace();
        }
        if (daemon)
        {
            close(devnull);
            close(redirect1);
            close(redirect2);
        }
        exit(EXIT_FAILURE);
    }
    catch (Exception e)
    {
        log_error("%s\n", e.getMessage().c_str());
        e.printStackTrace();
        if (daemon)
            close(devnull);
        exit(EXIT_FAILURE);
    }

    if (addFile->size() > 0)
    {
        for (int i = 0; i < addFile->size(); i++)
        {
            try
            {   
                // add file/directory recursively and asynchronously
                log_info("Adding %s\n", String(addFile->get(i)).c_str());
                ContentManager::getInstance()->addFile(String(addFile->get(i)),
                    true, true, 
                    ConfigManager::getInstance()->getBoolOption(CFG_IMPORT_HIDDEN_FILES));
            }
            catch (Exception e)
            {
                e.printStackTrace();
                if (daemon)
                {
                    close(devnull);
                    close(redirect1);
                    close(redirect2);
                }
                exit(EXIT_FAILURE);
            }
        }
    }
    
    sigemptyset(&mask_set);
    pthread_sigmask(SIG_SETMASK, &mask_set, NULL);
    
    // wait until signalled to terminate
    while (!shutdown_flag)
    {
        //pause();
        //sleep(timer->getNotifyInterval());
        
        if (timer == nil)
            timer = Timer::getInstance();
        
        timer->triggerWait();
        
        if (restart_flag != 0)
        {
            log_info("Restarting MediaTomb!\n");
            try
            {
                server = nil;
                timer = nil;
                
                singletonManager->shutdown(true);
                singletonManager = nil;
                singletonManager = SingletonManager::getInstance();
                
                try
                {
                    ConfigManager::setStaticArgs(config_file, home, confdir, 
                                                 prefix, magic);
                    ConfigManager::getInstance();
                }
                catch (mxml::ParseException pe)
                {
                    log_error("Error parsing config file: %s line %d:\n%s\n",
                            pe.context->location.c_str(),
                            pe.context->line,
                            pe.getMessage().c_str());
                    log_error("Could not restart MediaTomb\n");
                    // at this point upnp shutdown has already been called
                    // so it is safe to exit
                    exit(EXIT_FAILURE);
                }
                catch (Exception e)
                {
                    log_error("Error reloading configuration: %s\n", 
                              e.getMessage().c_str());
                    e.printStackTrace();
                    if (daemon)
                    {
                        close(devnull);
                        close(redirect1);
                        close(redirect2);
                    }
                    exit(EXIT_FAILURE);
                }
                
                ///  \todo fix this for SIGHUP
                server = Server::getInstance();
                server->upnp_init(interface, ip, port);
                
                restart_flag = 0;
            }
           catch(Exception e)
            {
                restart_flag = 0;
                shutdown_flag = 1;
                sigemptyset(&mask_set);
                pthread_sigmask(SIG_SETMASK, &mask_set, NULL);
                log_error("Could not restart MediaTomb\n");
            }
        }
    }

    // shutting down 
    int ret = EXIT_SUCCESS;
    try
    {
        singletonManager->shutdown(true);
    }
    catch(UpnpException upnp_e)
    {
        log_error("main: upnp error %d\n", upnp_e.getErrorCode());
        ret = EXIT_FAILURE;
    }
    catch (Exception e)
    {
        e.printStackTrace();
        ret = EXIT_FAILURE;
    }

    log_info("Server terminating\n");
    log_close();

    if (daemon)
    {
        close(devnull);
        close(redirect1);
        close(redirect2);
    }

    exit(ret);
}

void signal_handler(int signum)
{
    
    if (main_thread_id != pthread_self())
    {
        return;
    }
    
    if ((signum == SIGINT) || (signum == SIGTERM))
    {
        shutdown_flag++;
        if (shutdown_flag == 1)
            log_info("MediaTomb shutting down. Please wait...\n");
        else if (shutdown_flag == 2)
            log_info("Mediatomb still shutting down, signal again to kill.\n");
        else if (shutdown_flag > 2)
        {
            log_error("Clean shutdown failed, killing MediaTomb!\n");
            exit(1);
        }
        if (timer != nil)
            timer->signal();
    }
    else if (signum == SIGHUP)
    {
#if defined(ENABLE_SIGHUP)
        restart_flag = 1;
        if (timer != nil)
            timer->signal();
#else
        log_warning("SIGHUP handling was disabled during compilation.\n");
#endif
    }

    return;
}
