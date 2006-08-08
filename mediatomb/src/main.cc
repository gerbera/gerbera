/*  main.cc - this file is part of MediaTomb.
                                                                                
    Copyright (C) 2005 Gena Batyan <bgeradz@deadlock.dhs.org>,
                       Sergey Bostandzhyan <jin@deadlock.dhs.org>
                                                                                
    MediaTomb is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.
                                                                                
    MediaTomb is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.
                                                                                
    You should have received a copy of the GNU General Public License
    along with MediaTomb; if not, write to the Free Software
    Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
*/

#include "common.h"
#include "server.h"
#include "process.h"
#include <getopt.h>
#include "config_manager.h"
#include "content_manager.h"
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
#include <signal.h>
#include <pwd.h>
#include <grp.h>

#define OPTSTR "i:p:c:u:g:a:l:P:dh"

using namespace zmm;

int shutdown_flag = 0;
void signal_handler(int signal);

int main(int argc, char **argv, char **envp)
{
    int      opt_index = 0;
    int      o;
    char     *ip = NULL;
    char     * err = NULL;
    unsigned short  port = 0;
    bool     daemon = false;

    struct   passwd *pwd;
    struct   group  *grp;

    static struct option long_options[] = {
                   {"ip", 1, 0, 'i'},
                   {"port", 1, 0, 'p'},
                   {"config", 1, 0, 'c'},
                   {"user", 1, 0, 'u'},
                   {"group", 1, 0, 'g'},
                   {"daemon", 0, 0, 'd'},
                   {"pid", 0, 0, 'P'},
                   {"add", 1, 0, 'a'},
                   {"logfile", 1, 0, 'l'},
                   {"help", 0, 0, 'h'},
                   {0, 0, 0, 0}
               };

    String config_file;
    String home;
    String user;
    String group;
    String pid_file;

    // before we do anything we will check if MediaTomb and the
    // UPnP SDK were both compiled with largefile support.
    // I had at least two cases where AC_SYS_LARGEFILE enabled
    // 64bit offsets for the SDK part but did not enable them
    // for MediaTomb. Adaptions to the confgiure script are made
    // to catch that problem during compile time; still we want
    // to be absolutely sure that the server was compiled without
    // this problem.

    if (sizeof(off_t) != UpnpGetOff_tSize())
    {
        log_error(("off_t size mismatch!!! Aborting,"));
        log_error(("Due to a compilation problem the MediaTomb binary contains an error.\n"));
        log_error(("Please take a look at the README file for more information.\n"));
    }

    Ref<Array<StringBase> > addFile(new Array<StringBase>());
    
    while (1)
    {
        o = getopt_long(argc, argv, OPTSTR, long_options, &opt_index);
        if (o == -1) break;

        switch (o)
        {
            case 'i':
//                log_info(("Option IP with param %s\n", optarg));
                ip = optarg;
                break;

            case 'p':
//                log_info(("Option Port with param %s\n", optarg));
                errno = 0;
                port = strtol(optarg, &err, 10);
                if ((port == 0) && (*err))
                {
                    log_error(("Invalid port argument: %s\n", optarg));
                    exit(EXIT_FAILURE);
                }
                log_info(("port set to: %d\n", port));
                break;

            case 'c':
//                log_info(("Option config with param %s\n", optarg));
                config_file = String(optarg);
                break;

            case 'd':
//                log_info(("Starting in deamon mode..."));
                daemon = true;
                break;

            case 'u':
//                log_info(("Running as user: %s\n", optarg));
                user = String(optarg);
                break;

            case 'g':
//                log_info(("Running as group: %s\n", optarg));
                group = String(optarg);
                break;
                
            case 'a':
//                log_info(("adding file/directory:: %s\n", optarg));
                printf("Adding file/directory: %s\n", optarg);
                addFile->append(String(optarg));
                break;

            case 'P':
                printf("Pid file: %s\n", optarg);
                pid_file = String(optarg);
                break;
                
            case 'l':
                log_open(optarg);
                break;
                
            case '?':
            case 'h':
                printf("Usage: mediatomb [options]\n\
                        \n\
Supported options:\n\
    --ip or -i         ip address\n\
    --port or -p       server port (the SDK only permits values => 49152)\n\
    --config or -c     configuration file to use\n\
    --daemon or -d     run server in background\n\
    --pidfile or -P    file to hold the process id\n\
    --user or -u       run server under specified username\n\
    --group or -g      run server unser specified group\n\
    --add or -a        add the given file/directory\n\
    --logfile or -l    log to specified file\n\
    --help or -h       this help message\n\
\n\
For more information visit http://mediatomb.sourceforge.net/\n\n");

                exit(EXIT_FAILURE);

            default:
                break;
         }
    }

    log_info(("======== MediaTomb UPnP Server version %s ========\n", VERSION));
    log_info(("http://www.mediatomb.org/\n\n"));

    // check if user and/or group parameter was specified and try to run the server
    // under the given user and/or group name
    if (group != nil)
    {
        grp = getgrnam(group.c_str());
        if (grp == NULL)
        {
            log_info(("Group %s not found!\n", group.c_str()));
            exit(EXIT_FAILURE);
        }

        if (setgid(grp->gr_gid) < 0)
        {
            log_info(("setgid failed %s\n", strerror(errno)));
            exit(EXIT_FAILURE);
        }
    }

    if (user != nil)
    {
        pwd = getpwnam(user.c_str()); 
        if (pwd == NULL)
        {
            log_info(("User %s not found!\n", user.c_str()));
            exit(EXIT_FAILURE);
        }

        if (setuid(pwd->pw_uid) < 0)
        {
            log_info(("setuid failed %s\n", strerror(errno)));
            exit(EXIT_FAILURE);
        }
        
    }
   
    
    // TODO: check if -c option is present, if not it is an error NOT to specify config file
    
    try
    {
#ifndef __CYGWIN__
        char *h = getenv("HOME");
        if (h != NULL)
            home = String(h);
#else
        char *h = getenv("HOMEPATH");
        char *d = getenv("HOMEDRIVE");
        if ((h != NULL)  && (d != NULL))
            home = String(d) + h;
#endif  // __CYGWIN__
            

/*        if ((config_file == nil) && (home == nil))
        {
            log_info(("No configuration specified and no user home directory set.\n"));
            exit(EXIT_FAILURE);
        }
        
*/

        ConfigManager::init(config_file, home);
    }
    catch (mxml::ParseException pe)
    {
        log_info(("Error parsing config file: %s line %d:\n%s\n",
               pe.context->location.c_str(),
               pe.context->line,
               pe.getMessage().c_str()));
        exit(EXIT_FAILURE);
    }
    catch (Exception e)
    {
        log_info(("%s\n", e.getMessage().c_str()));
        exit(EXIT_FAILURE);
    }    
    
    server = Ref<Server>(new Server());

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
        if (pid < 0) {
            exit(EXIT_FAILURE);
        }
        /* If we got a good PID, then
           we can exit the parent process. */
        if (pid > 0) {
            exit(EXIT_SUCCESS);
        }

        /* Change the file mode mask */
        umask(0);

        /* Open any logs here */        

        /* Create a new SID for the child process */
        sid = setsid();
        if (sid < 0) {
            /* Log the failure */
            exit(EXIT_FAILURE);
        }

        /* Change the current working directory */
        if ((chdir("/")) < 0) {
            /* Log the failure */
            exit(EXIT_FAILURE);
        }

        /* Close out the standard file descriptors */        
        close(STDIN_FILENO);

        /*
        close(STDOUT_FILENO);
        close(STDERR_FILENO);
        */

    }

    if (pid_file != nil)
    {
        pid_t cur_pid;
        FILE    *f;
        int     size;

        cur_pid = getpid();
        String pid = String::from(cur_pid);

        f = fopen(pid_file.c_str(), "w");
        if (f == NULL)
        {                    
            printf("Could not write pid file %s : %s\n", pid_file.c_str(), strerror(errno));
        }
        else
        {
            size = fwrite(pid.c_str(), sizeof(char), pid.length(), f);
            fclose(f);

            if (size < pid.length())
                printf("Error when writing pid file %s : %s\n", pid_file.c_str(), strerror(errno));
        }

    }

    // prepare to run processes
    init_process();
    
    try
    {
        server->init();
        server->upnp_init(String(ip), port);
    }
    catch(UpnpException upnp_e)
    {
        upnp_e.printStackTrace();
        log_info(("main: upnp error %d\n ", upnp_e.getErrorCode()));
        try
        {
            server->upnp_cleanup();
        }
        catch (Exception e)
        {
            e.printStackTrace();
        }
        exit(EXIT_FAILURE);
    }
    catch (Exception e)
    {
        e.printStackTrace();
        exit(EXIT_FAILURE);
    }

    if (addFile->size() > 0)
    {
        for (int i = 0; i < addFile->size(); i++)
        {
            try
            {   
                // add file/directory recursively and asynchronously
                log_info(("adding %s\n", String(addFile->get(i)).c_str()));
                ContentManager::getInstance()->addFile(String(addFile->get(i)), true, true);
            }
            catch (Exception e)
            {
                e.printStackTrace();
                exit(EXIT_FAILURE);
            }
        }
    }
    
    signal(SIGINT, signal_handler);
    signal(SIGTERM, signal_handler);
    
    // wait until signalled to terminate
    while (! shutdown_flag)
    {
        sleep(3600);
    }
   
    /* shutting down */
    int ret = EXIT_SUCCESS;
    try
    {
        server->upnp_cleanup();
    }
    catch(UpnpException upnp_e)
    {
        log_info(("main: upnp error %d\n ", upnp_e.getErrorCode()));
        ret = EXIT_FAILURE;
    }
    catch (Exception e)
    {
        e.printStackTrace();
        ret = EXIT_FAILURE;
    }

    log_info(("terminating\n"));
    log_close();

    return ret;
}

void signal_handler(int signal)
{
    shutdown_flag = 1;
}

