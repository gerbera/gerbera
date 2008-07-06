/*MT*
    
    MediaTomb - http://www.mediatomb.cc/
    
    file_io_handler.h - this file is part of MediaTomb.
    
    Copyright (C) 2005 Gena Batyan <bgeradz@mediatomb.cc>,
                       Sergey 'Jin' Bostandzhyan <jin@mediatomb.cc>
    
    Copyright (C) 2006-2008 Gena Batyan <bgeradz@mediatomb.cc>,
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
    
    $Id: file_io_handler.h 1698 2008-02-23 20:48:30Z lww $
*/

/// \file dvd_read.cc 


#ifdef HAVE_CONFIG_H
    #include "autoconfig.h"
#endif

#ifdef HAVE_LIBDVDREAD

#include "dvd_read.h"

using namespace zmm;

DVDReader::DVDReader(String path)
{
    title_block_start = 0;
    title_block_end = 0;
    title_blocks = 0;
    title_offset = 0;
    title_cell_start = 0;
    title_cell_end = 0;
    current_cell = 0;
    next_cell = 0;

/*
 * Threads: this function uses chdir() and getcwd().
 * The current working directory is global to all threads,
 * so using chdir/getcwd in another thread could give unexpected results.
*/
    /// \todo check the implications of the above comment, do we use chdir()
    /// somewhere?
    dvd = DVDOpen(path.c_str());
    if (!dvd)
    {
        throw _Exception(_("Could not open DVD ") + path);
    }

    dvd_path = path;

    // read the video manager ifo file
    vmg = ifoOpen(dvd, 0);
    if (!vmg)
    {
        DVDClose(dvd);
        throw _Exception(_("Could not read VMG IFO for DVD ") + dvd_path);
    }

    /// \todo supposedly this is needed to reduce a memleak, check if this is
    /// still valid or if the leak has been fixed in the library
    DVDUDFCacheLevel(dvd, 0);

    log_debug("Opened DVD %s\n", dvd_path.c_str());

}

DVDReader::~DVDReader()
{
    if (vmg)
        ifoClose(vmg);

    if (dvd)
        DVDClose(dvd);
    log_debug("Closing DVD %s\n", dvd_path.c_str());
}

int DVDReader::titleCount()
{
    if (!vmg)
        throw _Exception(_("Invalid VMG for DVD ") + dvd_path);

    return vmg->tt_srpt->nr_of_srpts;
}

int DVDReader::chapterCount(int titleID)
{
    if (!vmg)
        throw _Exception(_("Invalid VMG for DVD ") + dvd_path);

    if (titleID > titleCount())
        throw _Exception(_("Requested title number exceeds available titles for DVD ") + dvd_path);

    return vmg->tt_srpt->title[titleID].nr_of_ptts;
}

void DVDReader::dumpVMG()
{
#ifdef DEBUG_LOG_ENABLED
    if (!vmg)
        log_debug("Invalid VMG for DVD %s\n", dvd_path.c_str());


    log_debug("txtdt_mgi_t: %s\n", vmg->txtdt_mgi->disc_name);
#endif
}

void DVDReader::selectPGC(int title, int chapter, int angle)
{
    if ((title < 0) || (title > titleCount()))
        throw _Exception(_("Attmpted to select invalid title!"));

}

#endif
