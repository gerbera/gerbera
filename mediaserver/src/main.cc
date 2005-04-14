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

#define OPTSTR "i:p:c:h"

using namespace zmm;



int main(int argc, char **argv, char **envp)
{
    int     ret=0;  // general purpose error code
    char    c;      //
    int     opt_index = 0;
    int     o;
    char    *ip = NULL;
    unsigned short  port = 0;
    static struct option long_options[] = {
                   {"ip", 1, 0, 'i'},
                   {"port", 1, 0, 'p'},
                   {"config", 1, 0, 'c'},
                   {"help", 0, 0, 'h'},
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

            case '?':
                printf("\n");
            case 'h':
                printf("Usage: mediatomb [options]\n\
                        \n\
Supported options:\n\
    --ip or -i         ip address\n\
    --port or -p       server port (the SDK only permits values => 49152)\n\
    --config or -c     configuration file to use\n\
    --help or -h       this help message\n\
\n\
For more information visit http://mediatomb.sourceforge.net/\n\n");

                exit(0);

//            case '?':
//                break;

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
            exit(1);
        }

        ConfigManager::init(config_file, home);
    }

    catch (mxml::ParseException pe)
    {
        printf("Error parsing config file: %s line %d:\n%s\n",
               pe.context->location.c_str(),
               pe.context->line,
               pe.getMessage().c_str());
        exit(1);
    }
    catch (Exception e)
    {
        printf("%s\n", e.getMessage().c_str());
        exit(1);
    }

    server = Ref<Server>(new Server());

    
    try
    {
        server->upnp_init(ip, port);
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
        exit(1);
    }
    catch (Exception e)
    {
        e.printStackTrace();
        exit(1);
    }


    // prepare to run processes
    init_process();
    
    // we are now initialised
    // any keypress will cause us to quit
    printf("\nPRESS ANY KEY TO QUIT!\n\n");
    c = getchar();

    try
    {
        server->upnp_cleanup();
    }
    catch(UpnpException upnp_e)
    {
        printf("main: upnp error %d\n ", upnp_e.getErrorCode());
        exit(1);
    }
    catch (Exception e)
    {
        e.printStackTrace();
        exit(1);
    }
   
    printf("main: end\n");

    return 0;
}
