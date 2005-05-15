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

#define OPTSTR "i:p:c:hd"

using namespace zmm;



int main(int argc, char **argv, char **envp)
{
    char    c;
    int     opt_index = 0;
    int     o;
    char    *ip = NULL;
    unsigned short  port = 0;
    bool    daemon = false;

    static struct option long_options[] = {
                   {"ip", 1, 0, 'i'},
                   {"port", 1, 0, 'p'},
                   {"config", 1, 0, 'c'},
                   {"help", 0, 0, 'h'},
                   {"daemon", 0, 0, 'd'},
                   {0, 0, 0, 0}
               };

    String config_file;
    String home;

    printf("\nMediaTomb UPnP Server version %s\n\n", SERVER_VERSION);
    
    while (1)
    {
        o = getopt_long(argc, argv, OPTSTR, long_options, &opt_index);
        if (o == -1) break;

        switch (o)
        {
            case 'i':
                printf("Option IP with param %s\n", optarg);
                ip = optarg;
                break;

            case 'p':
                printf("Option Port with param %s\n", optarg);
                port = strtol(optarg, (char **)NULL, 10);
                printf("port set to: %d\n", port);
                break;
            case 'c':
                printf("Option config with param %s\n", optarg);

                config_file = optarg;
                    
                break;
            case 'd':
                printf("Starting in deamon mode...");
                daemon = true;
                break;
                
            case '?':
                printf("\n");
            case 'h':
                printf("Usage: mediatomb [options]\n\
                        \n\
Supported options:\n\
    --ip or -i         ip address\n\
    --port or -p       server port (the SDK only permits values => 49152)\n\
    --config or -c     configuration file to use\n\
    --daemon or -d     run server in background\n\
    --help or -h       this help message\n\
\n\
For more information visit http://mediatomb.sourceforge.net/\n\n");

                exit(EXIT_FAILURE);

            default:
                break;
         }
    }

    // TODO: check if -c option is present, if not it is an error NOT to specify config file
    
    try
    {
        char *h = getenv("HOME");
        if (h != NULL) home = h;

        if ((config_file == nil) && (home == nil))
        {
            printf("No configuration specified and no user home directory set.\n");
            exit(EXIT_FAILURE);
        }

        ConfigManager::init(config_file, home);
    }

    catch (mxml::ParseException pe)
    {
        printf("Error parsing config file: %s line %d:\n%s\n",
               pe.context->location.c_str(),
               pe.context->line,
               pe.getMessage().c_str());
        exit(EXIT_FAILURE);
    }
    catch (Exception e)
    {
        printf("%s\n", e.getMessage().c_str());
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
/*
        close(STDIN_FILENO);
        close(STDOUT_FILENO);
        close(STDERR_FILENO);
*/
        /* Daemon-specific initialization goes here */

    }


    // prepare to run processes
    init_process();
    
    try
    {
        server->init();
        server->upnp_init(ip, port);
 //       String dump = ConfigManager::getInstance()->getElement("/")->print();
 //       printf("Modified config dump:\n%s\n", dump.c_str());
    }
    catch(UpnpException upnp_e)
    {
        upnp_e.printStackTrace();
        printf("main: upnp error %d\n ", upnp_e.getErrorCode());
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
   
    // endless sleep
    while (1)
    {
        sleep(3600);
    }

    /* RUN THIS UPON SIGTERM */
    try
    {
        server->upnp_cleanup();
    }
    catch(UpnpException upnp_e)
    {
        printf("main: upnp error %d\n ", upnp_e.getErrorCode());
        exit(EXIT_FAILURE);
    }
    catch (Exception e)
    {
        e.printStackTrace();
        exit(EXIT_FAILURE);
    }
    /************************/
    
    return EXIT_SUCCESS;
}
