/*MT*
    
    MediaTomb - http://www.mediatomb.cc/
    
    process.cc - this file is part of MediaTomb.
    
    Copyright (C) 2005 Gena Batyan <bgeradz@mediatomb.cc>,
                       Sergey 'Jin' Bostandzhyan <jin@mediatomb.cc>
    
    Copyright (C) 2006-2007 Gena Batyan <bgeradz@mediatomb.cc>,
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

/// \file process.cc

#ifdef HAVE_CONFIG_H
    #include "autoconfig.h"
#endif

#include "process.h"

#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>

#include <string.h>
#include <errno.h>

using namespace zmm;

#define BUF_SIZE 256
#define MAX_TEMPLATE 64

char input_template[MAX_TEMPLATE];
char output_template[MAX_TEMPLATE];

void init_process()
{
    sprintf(input_template, "/tmp/mediaserver_in_%d_XXXXXX", getuid());
    sprintf(output_template, "/tmp/mediaserver_out_%d_XXXXXX", getuid());
}

String run_process(String prog, String param, String input)
{
    FILE *file;
    int fd;

    /* creating input file */
    char input_file[MAX_TEMPLATE];
    strcpy(input_file, input_template);
    fd = mkstemp(input_file);
    int ret = write(fd, input.c_str(), input.length());
    close(fd);
    
    /* touching output file */
    char output_file[MAX_TEMPLATE];
    strcpy(output_file, output_template);
    fd = mkstemp(output_file);
    close(fd);
   
    /* executing script */
    String command = prog + " " + param + " < " + input_file +
        " > " + output_file;
    log_debug("running %s\n", command.c_str());
    ret = system(command.c_str());

    /* reading output file */
    file = fopen(output_file, "r");
    Ref<StringBuffer> output(new StringBuffer());

    int bytesRead;
    char buf[BUF_SIZE];
    while (1)
    {
        bytesRead = fread(buf, 1, BUF_SIZE, file);
        if(bytesRead > 0)
            *output << String(buf, bytesRead);
        else
            break;
    }
    fclose(file);

    /* removing input and output files */
    unlink(input_file);
    unlink(output_file);

    return output->toString();
}
