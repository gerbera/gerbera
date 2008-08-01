/*MT*
    
    MediaTomb - http://www.mediatomb.cc/
    
    dvd_read.cc - this file is part of MediaTomb.
    
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
    
    $Id: dvd_read.cc 1698 2008-02-23 20:48:30Z lww $
*/

/*
    Significant amounts of code were derived from the play_title.c example
    program which is part of libdvdread.

    play_title.c is (C) 2001 Billy Biggs <vektor@dumbterm.net> and licensed
    under GPL version 2.

    The angle block checks were taken from stream_dvd.c which is part of
    MPlayer, not sure about (C) since their sources have no header, licensed
    under GPL version 2.

    The dvdtime2msec() function as well as some other parts which are marked
    by comments were taken from lsdvd, (C)  2003 by Chris Phillips, 
    Henk Vergonet, licensed under GPL version 2.
 */

/// \file dvd_read.cc 


#ifdef HAVE_CONFIG_H
    #include "autoconfig.h"
#endif

#ifdef HAVE_LIBDVDREAD

#include "dvd_read.h"
#include <dvdread/nav_read.h>
#include <assert.h>

#include "tools.h"

using namespace zmm;

static double frames_per_s[4] = {-1.0, 25.00, -1.0, 29.97};

static struct { char code[3]; char name[20]; }
// from lsdvd, ISO-639
language[] = {
    { "  ", "Not Specified"     }, { "aa", "Afar"           },       
    { "ab", "Abkhazian"         }, { "af", "Afrikaans"      },      
    { "am", "Amharic"           }, { "ar", "Arabic"         }, 
    { "as", "Assamese"          }, { "ay", "Aymara"         },
    { "az", "Azerbaijani"       }, { "ba", "Bashkir"        },
    { "be", "Byelorussian"      }, { "bg", "Bulgarian"      }, 
    { "bh", "Bihari"            }, { "bi", "Bislama"        }, 
    { "bn", "Bengali; Bangla"   }, { "bo", "Tibetan"        }, 
    { "br", "Breton"            }, { "ca", "Catalan"        }, 
    { "co", "Corsican"          }, { "cs", "Czech"          },
    { "cy", "Welsh"             }, { "da", "Dansk"          }, 
    { "de", "Deutsch"           }, { "dz", "Bhutani"        }, 
    { "el", "Greek"             }, { "en", "English"        },
    { "eo", "Esperanto"         }, { "es", "Espanol"        }, 
    { "et", "Estonian"          }, { "eu", "Basque"         }, 
    { "fa", "Persian"           }, { "fi", "Suomi"          }, 
    { "fj", "Fiji"              }, { "fo", "Faroese"        }, 
    { "fr", "Francais"          }, { "fy", "Frisian"        }, 
    { "ga", "Gaelic"            }, { "gd", "Scots Gaelic"   }, 
    { "gl", "Galician"          }, { "gn", "Guarani"        }, 
    { "gu", "Gujarati"          }, { "ha", "Hausa"          },
    { "he", "Hebrew"            }, { "hi", "Hindi"          }, 
    { "hr", "Hrvatski"          }, { "hu", "Magyar"         }, 
    { "hy", "Armenian"          }, { "ia", "Interlingua"    }, 
    { "id", "Indonesian"        }, { "ie", "Interlingue"    }, 
    { "ik", "Inupiak"           }, { "in", "Indonesian"     },
    { "is", "Islenska"          }, { "it", "Italiano"       }, 
    { "iu", "Inuktitut"         }, { "iw", "Hebrew"         }, 
    { "ja", "Japanese"          }, { "ji", "Yiddish"        }, 
    { "jw", "Javanese"          }, { "ka", "Georgian"       }, 
    { "kk", "Kazakh"            }, { "kl", "Greenlandic"    },
    { "km", "Cambodian"         }, { "kn", "Kannada"        }, 
    { "ko", "Korean"            }, { "ks", "Kashmiri"       }, 
    { "ku", "Kurdish"           }, { "ky", "Kirghiz"        }, 
    { "la", "Latin"             }, { "ln", "Lingala"        }, 
    { "lo", "Laothian"          }, { "lt", "Lithuanian"     },
    { "lv", "Latvian, Lettish"  }, { "mg", "Malagasy"       }, 
    { "mi", "Maori"             }, { "mk", "Macedonian"     }, 
    { "ml", "Malayalam"         }, { "mn", "Mongolian"      }, 
    { "mo", "Moldavian"         }, { "mr", "Marathi"        }, 
    { "ms", "Malay"             }, { "mt", "Maltese"        },
    { "my", "Burmese"           }, { "na", "Nauru"          }, 
    { "ne", "Nepali"            }, { "nl", "Nederlands"     }, 
    { "no", "Norsk"             }, { "oc", "Occitan"        },
    { "om", "Oromo"             }, { "or", "Oriya"          }, 
    { "pa", "Punjabi"           }, { "pl", "Polish"         }, 
    { "ps", "Pashto, Pushto"    }, { "pt", "Portugues"      }, 
    { "qu", "Quechua"           }, { "rm", "Rhaeto-Romance" }, 
    { "rn", "Kirundi"           }, { "ro", "Romanian"       },
    { "ru", "Russian"           }, { "rw", "Kinyarwanda"    }, 
    { "sa", "Sanskrit"          }, { "sd", "Sindhi"         }, 
    { "sg", "Sangho"            }, { "sh", "Serbo-Croatian" }, 
    { "si", "Sinhalese"         }, { "sk", "Slovak"         }, 
    { "sl", "Slovenian"         }, { "sm", "Samoan"         },
    { "sn", "Shona"             }, { "so", "Somali"         }, 
    { "sq", "Albanian"          }, { "sr", "Serbian"        }, 
    { "ss", "Siswati"           }, { "st", "Sesotho"        }, 
    { "su", "Sundanese"         }, { "sv", "Svenska"        }, 
    { "sw", "Swahili"           }, { "ta", "Tamil"          },
    { "te", "Telugu"            }, { "tg", "Tajik"          }, 
    { "th", "Thai"              }, { "ti", "Tigrinya"       }, 
    { "tk", "Turkmen"           }, { "tl", "Tagalog"        },
    { "tn", "Setswana"          }, { "to", "Tonga"          }, 
    { "tr", "Turkish"           }, { "ts", "Tsonga"         }, 
    { "tt", "Tatar"             }, { "tw", "Twi"            },
    { "ug", "Uighur"            }, { "uk", "Ukrainian"      }, 
    { "ur", "Urdu"              }, { "uz", "Uzbek"          }, 
    { "vi", "Vietnamese"        }, { "vo", "Volapuk"        }, 
    { "wo", "Wolof"             }, { "xh", "Xhosa"          }, 
    { "yi", "Yiddish"           }, { "yo", "Yoruba"         }, 
    { "za", "Zhuang"            }, { "zh", "Chinese"        }, 
    { "zu", "Zulu"              }, { "xx", "Unknown"        }, 
    { "\0", "Unknown" } 
};

static char *audio_format[7] = 
                        {"ac3", "?", "mpeg1", "mpeg2", "lpcm ", "sdds ", "dts"};
static int   audio_id[7]     = {0x80, 0, 0xC0, 0xC0, 0xA0, 0, 0x88};

DVDReader::DVDReader(String path)
{
    vts = NULL;
    title = NULL;
    pgc = NULL;
    vmg = NULL;
    title_index = 0;
    chapter_index = 0;
    angle_index = 0;
    current_cell = 0;
    current_pack = 0;
    start_cell = 0;
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

    mutex = Ref<Mutex>(new Mutex(true));
}

DVDReader::~DVDReader()
{
    if (title)
        DVDCloseFile(title);

    if (vts)
        ifoClose(vts);

    if (vmg)
        ifoClose(vmg);

    if (dvd)
        DVDClose(dvd);
    log_debug("Closing DVD %s\n", dvd_path.c_str());
}

int DVDReader::titleCount()
{
    return vmg->tt_srpt->nr_of_srpts;
}

int DVDReader::chapterCount(int title_idx)
{
    if ((title_idx < 0) || (title_idx >= titleCount()))
        throw _Exception(_("Requested title number exceeds available titles "
                    "for DVD ") + dvd_path);

    return vmg->tt_srpt->title[title_idx].nr_of_ptts;
}

int DVDReader::angleCount(int title_idx)
{
    if ((title_idx < 0) || (title_idx >= titleCount()))
        throw _Exception(_("Requested title number exceeds available titles "
                    "for DVD ") + dvd_path);

    return vmg->tt_srpt->title[title_idx].nr_of_angles;
}

void DVDReader::selectPGC(int title_idx, int chapter_idx, int angle_idx)
{
    if ((title_idx < 0) || (title_idx >= titleCount()))
        throw _Exception(_("Attmpted to select invalid title!"));

    if ((chapter_idx < 0) || (chapter_idx >= chapterCount(title_idx)))
        throw _Exception(_("Attmpted to select invalid chapter!"));

    if ((angle_idx < 0) || (angle_idx >= angleCount(title_idx)))
        throw _Exception(_("Attmpted to select invalid angle!"));

    AUTOLOCK(mutex);

    // check if the current vts is valid, otherwise close the old one and load 
    // the vts info for the title set of our requested title
    if ((title_idx != this->title_index) && vts)
    {
        ifoClose(vts);
        vts = NULL;
    }

    if (!vts)
        vts = ifoOpen(dvd, vmg->tt_srpt->title[title_idx].title_set_nr);

    if (!vts)
        throw _Exception(_("Could not open info file for title ") + title_idx);


    this->title_index = title_idx;
    this->chapter_index = chapter_idx;
    this->angle_index = angle_idx;

    // find our program chain
    int title_num = vmg->tt_srpt->title[title_index].vts_ttn;
    vts_ptt_srpt_t *vts_chapter_table = vts->vts_ptt_srpt; 
    int pgc_id = vts_chapter_table->title[title_num-1].ptt[chapter_idx].pgcn;
    int pgn = vts_chapter_table->title[title_num-1].ptt[chapter_idx].pgn;
    this->pgc = vts->vts_pgcit->pgci_srp[pgc_id-1].pgc;
    this->start_cell = this->pgc->program_map[pgn-1]-1;
    this->next_cell = this->start_cell;
    this->current_cell = this->start_cell;

    this->cont_sector = false;

    if ((title_idx != this->title_index) && (this->title))
    {
        DVDCloseFile(this->title);
        this->title = NULL;
    }

    if (!this->title)
        this->title = DVDOpenFile(dvd, 
                                  vmg->tt_srpt->title[title_idx].title_set_nr,
                                  DVD_READ_TITLE_VOBS);

    if (!this->title)
        throw _Exception(_("Could not open title VOBS for title ") + title_index);

    this->title_index = title_idx;
    this->chapter_index = chapter_idx;
    this->angle_index = angle_idx;

    // we do not know when the length() function will be called, however we
    // always want it to return the full length for the stream of the 
    // currently selected PGC; so we will have to save the initial values
    // that are otherwise altered by read.
    nc_len = this->next_cell;
    cc_len = this->current_cell;
    cp_len = this->current_pack;
    cs_len = this->cont_sector;

}

off_t DVDReader::length(bool precise)
{
    if (precise)
        return calculateExactLength();
    else
        return calculateApproxLength();
}

off_t DVDReader::chapterLength(int chapter_idx, bool precise)
{
    if (chapter_idx >= chapterCount(this->title_index))
        throw _Exception(_("Attempted to select invalid chapter!"));

    off_t len = 0;

    AUTOLOCK(mutex);

    if (precise)
    {
        // honestly, this sucks.. I need to find a better way to do this
        unsigned char buf[DVD_VIDEO_LB_LEN+1];
        int nc = this->next_cell;
        int cc = this->current_cell;
        int cp = this->current_pack;
        bool cs = this->cont_sector;
        int sc = this->start_cell;
        int ti = this->title_index;
        int ci = this->chapter_index;
        int ai = this->angle_index;
        pgc_t *pgc = this->pgc;

        int ncl = nc_len;
        int ccl = cc_len;
        int cpl = cp_len;
        int csl = cs_len;


        selectPGC(this->title_index, chapter_idx, this->angle_index);

        size_t readlen = 0;
        do
        {
            readlen = readSectorInternal(buf, DVD_VIDEO_LB_LEN+1, false, false);
            len += readlen;
        } while (readlen > 0);

        this->next_cell = nc;
        this->current_cell = cc;
        this->current_pack = cp;
        this->cont_sector = cs;
        this->start_cell = sc;
        this->title_index = ti;
        this->chapter_index = ci;
        this->angle_index = ai;
        this->pgc = pgc;

        nc_len = ncl;
        cc_len = ccl;
        cp_len = cpl;
        cs_len = csl;
    }
    else
    {
        len = (pgc->cell_playback[chapter_idx].last_sector - 
                pgc->cell_playback[chapter_idx].first_sector)
            *DVD_VIDEO_LB_LEN;

        log_debug("Chapter %d approx len %lld (first sec %x - last sec %x)\n", 
                this->chapter_index,
                (long long)len, 
                (int)pgc->cell_playback[chapter_idx].first_sector,
                (int)pgc->cell_playback[chapter_idx].last_sector);
        log_debug("Chapter %d approx len %lld (first sec %d - last sec %d)\n", 
                this->chapter_index,
                (long long)len, 
                (int)pgc->cell_playback[chapter_idx].first_sector,
                (int)pgc->cell_playback[chapter_idx].last_sector);
    }

#ifdef TOMBDEBUG
    dumpGlobalVars();
#endif

    return len;
}

#ifdef TOMBDEBUG
void DVDReader::dumpGlobalVars()
{
    log_debug("Next Cell:       %d\n", this->next_cell);
    log_debug("Current Cell:    %d\n", this->current_cell);
    log_debug("Current Pack:    %d\n", this->current_pack);
    log_debug("Cont Sector:     %d\n", this->cont_sector);
    log_debug("Start Cell:      %d\n", this->start_cell);
    log_debug("Title Index:     %d\n", this->title_index);
    log_debug("Chapter Index:   %d\n", this->chapter_index);
    log_debug("Angle Index:     %d\n", this->angle_index);
    log_debug("PGC              %p\n", this->pgc);
}
#endif
off_t DVDReader::calculateApproxLength()
{
    off_t len = 0;

    AUTOLOCK(mutex);

    int curcell = cc_len = this->current_cell;
    do
    {
        len += (this->pgc->cell_playback[curcell].last_sector -
                this->pgc->cell_playback[curcell].first_sector + 1)
            *DVD_VIDEO_LB_LEN;
        curcell++;
    } while (curcell < pgc->nr_of_cells);

    return len;
}

off_t DVDReader::calculateExactLength()
{
    off_t len = 0;

    AUTOLOCK(mutex);

    // save all global variables
    int nc = this->next_cell;
    int cc = this->current_cell;
    int cp = this->current_pack;
    bool cs = this->cont_sector;
    unsigned char buf[DVD_VIDEO_LB_LEN+1];

    this->next_cell = nc_len;
    this->current_cell = cc_len;
    this->current_pack = cp_len;
    this->cont_sector = cs_len;

    size_t readlen = 0;
    do
    {
        readlen = readSectorInternal(buf, DVD_VIDEO_LB_LEN+1, false);
        len += readlen;
    } while (readlen > 0);

    this->next_cell = nc;
    this->current_cell = cc;
    this->current_pack = cp;
    this->cont_sector = cs;

    return len;
}

size_t DVDReader::readSector(unsigned char *buffer, size_t length)
{
    AUTOLOCK(mutex);
    return readSectorInternal(buffer, length, true);
}

size_t DVDReader::readSectorInternal(unsigned char *buffer, size_t length, 
        bool read, bool till_eof)
{
    int last_pack; 

    if (!this->pgc || !this->title || !this->vts || !this->dvd)
        throw _Exception(_("DVD Reader class not initialized"));

    // for dummy reads we need at least the size of DVD_VIDEO_LB_LEN for the
    // NAV packet
    if (length < ((size_t)(DVD_VIDEO_LB_LEN)))
        throw _Exception(_("Insufficient buffer for read operation"));


    while (this->next_cell <= this->pgc->nr_of_cells)
    {
        if (!this->cont_sector)
        {
            current_cell = next_cell;

            // Check if we're entering an angle block.
            // This part of the angle block code was taken from MPlayer,
            // stream_dvd.c
            if (this->pgc->cell_playback[this->current_cell].block_type
                    == BLOCK_TYPE_ANGLE_BLOCK)
            {
                while (this->next_cell < this->pgc->nr_of_cells)
                {
                    if (this->pgc->cell_playback[this->next_cell].block_mode
                            == BLOCK_MODE_LAST_CELL)
                        break;
                    this->next_cell++;
                }
            }

            this->next_cell++;
            if (this->next_cell > this->pgc->nr_of_cells)
                return 0;

            if (this->pgc->cell_playback[this->next_cell].block_type
                    == BLOCK_TYPE_ANGLE_BLOCK)
            {
                this->next_cell += this->angle_index;
                if (this->next_cell > this->pgc->nr_of_cells)
                    return 0;
            }

            // We loop until we're out of this cell.
            this->current_pack = 
                this->pgc->cell_playback[this->current_cell].first_sector;
        }

            
        last_pack = (int)this->pgc->cell_playback[this->current_cell].last_sector;

        while (this->current_pack < last_pack) 
        {
            dsi_t dsi_pack;
            unsigned int next_vobu, next_ilvu_start, cur_output_size;

            // Read NAV packet.
            int len = DVDReadBlocks(this->title, (int)this->current_pack, 1, 
                    buffer);
            if (len != 1)
                throw _Exception(_("Failed to read block!"));

            // the values that come in addition to playe_title.c sample
            // were taken from MPlayer sources.
            if(!(buffer[38]   == 0 && buffer[39]   == 0 && 
                        buffer[40]   == 1 && buffer[41]   == 0xbf &&
                        buffer[1024] == 0 && buffer[1025] == 0 && 
                        buffer[1026] == 1 && buffer[1027] == 0xbf))
                throw _Exception(_("Not a nav packet!"));

            //  Parse the contained dsi packet.
            navRead_DSI(&dsi_pack, &(buffer[DSI_START_BYTE]));

            if (this->current_pack != (int)dsi_pack.dsi_gi.nv_pck_lbn)
                throw _Exception(_("Failed to parse dsi pack!"));

            // Determine where we go next.  These values are the ones we mostly
            // care about.
            next_ilvu_start = this->current_pack
                + dsi_pack.sml_agli.data[this->angle_index].address;
            cur_output_size = dsi_pack.dsi_gi.vobu_ea;

            // If we're not at the end of this cell, we can determine the next
            // VOBU to display using the VOBU_SRI information section of the
            // DSI.  Using this value correctly follows the current angle,
            // avoiding the doubled scenes in The Matrix, and makes our life
            // really happy.
            //
            // Otherwise, we set our next address past the end of this cell to
            // force the code above to go to the next cell in the program.
            if (dsi_pack.vobu_sri.next_vobu != SRI_END_OF_CELL)
            {
                next_vobu = this->current_pack
                    + (dsi_pack.vobu_sri.next_vobu & 0x7fffffff);
            }
            else
                next_vobu = this->current_pack + cur_output_size + 1;
#ifdef TOMBDEBUG
            if (cur_output_size > 1024)
                throw _Exception(_("cur_output_size > 1024"));
#endif
            this->current_pack++;

            // Read in and output cursize packs.
            if (read)
            {
                if (length < ((size_t)(cur_output_size*DVD_VIDEO_LB_LEN)))
                    throw _Exception(_("Insufficient buffer for read operation"));
                len = DVDReadBlocks(this->title, this->current_pack, 
                        (size_t)cur_output_size, buffer);
                if (len != (int)cur_output_size)
                {
                    throw _Exception(_("Failed to read ") + cur_output_size + 
                            _("blocks at ") + this->current_pack);
                }
            }
            this->current_pack = next_vobu;
            this->cont_sector = true;

            return (size_t)(cur_output_size*DVD_VIDEO_LB_LEN);

        } // while (this->current_pack < ...
        this->cont_sector = false;
        if (!till_eof)
            return 0;
    } //  while (this->next_cell < this->pgc->nr_of_cells)

    return 0;
}

int DVDReader::audioTrackCount()
{
    AUTOLOCK(mutex);
    return vts->vtsi_mat->nr_of_vts_audio_streams;
}

int DVDReader::duration()
{
    int ms;
    int total = 0;
    int cell = this->chapter_index;

    for (int i=0; i<pgc->nr_of_programs; i++)
    {
        ms=0;
        int next = pgc->program_map[i+1];
        if (i == pgc->nr_of_programs - 1) next = pgc->nr_of_cells + 1;

        while (cell < next - 1)
        {
            ms = ms + dvdtime2msec(&pgc->cell_playback[cell].playback_time);

            cell++;
        }
        total = total + (int)(ms * 0.001);
    }
    return total;
}

int DVDReader::titleDuration()
{
    AUTOLOCK(mutex);
    return (int)(dvdtime2msec(&pgc->playback_time)/1000.0);
}

int DVDReader::chapterDuration(int chapter_idx)
{
    if ((chapter_idx < 0) || (chapter_idx >= chapterCount(this->title_index)))
        throw _Exception(_("Attmpted to select invalid chapter!"));

    AUTOLOCK(mutex);
    return (int)(dvdtime2msec(&pgc->cell_playback[chapter_idx].playback_time)*0.001);
}

// this function was taken from lsdvd
long DVDReader::dvdtime2msec(dvd_time_t *dt)
{
    double fps = frames_per_s[(dt->frame_u & 0xc0) >> 6];
    long   ms;
    ms  = (((dt->hour &   0xf0) >> 3) * 5 + (dt->hour   & 0x0f)) * 3600000;
    ms += (((dt->minute & 0xf0) >> 3) * 5 + (dt->minute & 0x0f)) * 60000;
    ms += (((dt->second & 0xf0) >> 3) * 5 + (dt->second & 0x0f)) * 1000;

    if(fps > 0)
    ms += (long)(((dt->frame_u & 0x30) >> 3) * 5 + (dt->frame_u & 0x0f) * 1000.0 / fps);

    return ms;
}

// from lsdvd
String DVDReader::getLanguage(char *code)
{
    int k = 0;
    while (memcmp(language[k].code, code, 2) && language[k].name[0] ) { k++; }
    return _(language[k].name);
}

String DVDReader::audioLanguage(int stream_idx)
{
    char code[3];
    audio_attr_t *audio_attr;

    if ((stream_idx < 0) || (stream_idx >= audioTrackCount()))
        throw _Exception(_("Attmpted to select invalid audio stream!"));

    AUTOLOCK(mutex);

    audio_attr = &vts->vtsi_mat->vts_audio_attr[stream_idx];
    sprintf(code, "%c%c", audio_attr->lang_code >> 8, 
                          audio_attr->lang_code & 0xff);
    
    if (!code[0])
    { 
        code[0] = 'x'; 
        code[1] = 'x'; 
    }

    return getLanguage(code);
}

int DVDReader::audioSampleFrequency(int stream_idx)
{
    if ((stream_idx < 0) || (stream_idx >= audioTrackCount()))
        throw _Exception(_("Attmpted to select invalid audio stream!"));

    audio_attr_t *audio_attr = &vts->vtsi_mat->vts_audio_attr[stream_idx];
    if (audio_attr->sample_frequency == 0)
        return 48000;
    else
        return 96000;
}

int DVDReader::audioChannels(int stream_idx)
{
    if ((stream_idx < 0) || (stream_idx >= audioTrackCount()))
        throw _Exception(_("Attmpted to select invalid audio stream!"));

    audio_attr_t *audio_attr = &vts->vtsi_mat->vts_audio_attr[stream_idx];

    return audio_attr->channels + 1;

}

String DVDReader::audioFormat(int stream_idx)
{
    if ((stream_idx < 0) || (stream_idx >= audioTrackCount()))
        throw _Exception(_("Attmpted to select invalid audio stream!"));

    audio_attr_t *audio_attr = &vts->vtsi_mat->vts_audio_attr[stream_idx];
    return _(audio_format[audio_attr->audio_format]);
}

int DVDReader::audioStreamID(int stream_idx)
{
    if ((stream_idx < 0) || (stream_idx >= audioTrackCount()))
        throw _Exception(_("Attmpted to select invalid audio stream!"));

    audio_attr_t *audio_attr = &vts->vtsi_mat->vts_audio_attr[stream_idx];
    return audio_id[audio_attr->audio_format]+stream_idx;
}

#endif//HAVE_LIBDVDREAD

