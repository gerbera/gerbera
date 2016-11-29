/*MT*
    
    MediaTomb - http://www.mediatomb.cc/
    
    dvdnav_read.cc - this file is part of MediaTomb.
    
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

/*
    Significant amounts of code were derived from the menus.c example
    program which is part of libdvdnav.

    menus.c is (C) 2003 by the libdvdnav project under GPLv2 or later.

    The dvdtime2msec() function as well as some other parts which are marked
    by comments were taken from lsdvd, (C)  2003 by Chris Phillips, 
    Henk Vergonet, licensed under GPL version 2.
 */

/// \file dvdnav_read.cc


#ifdef HAVE_LIBDVDNAV

#include "dvdnav_read.h"
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

static const char *audio_format[7] = 
                        {"ac3", "?", "mpeg1", "mpeg2", "lpcm ", "sdds ", "dts"};
static int   audio_id[7]     = {0x80, 0, 0xC0, 0xC0, 0xA0, 0, 0x88};

DVDNavReader::DVDNavReader(String path)
{
    /*
     * Threads: this function uses chdir() and getcwd().
     * The current working directory is global to all threads,
     * so using chdir/getcwd in another thread could give unexpected results.
     */
    /// \todo check the implications of the above comment, do we use chdir()
    /// somewhere?
    if (dvdnav_open(&dvd, path.c_str()) != DVDNAV_STATUS_OK)
    {
        throw _Exception(_("Could not open DVD ") + path);
    }

    dvd_path = path;

    // set the PGC positioning flag to have position information relatively to 
    // the whole feature instead of just relatively to the current chapter 
    if (dvdnav_set_PGC_positioning_flag(dvd, 1) != DVDNAV_STATUS_OK)
    {
        throw _Exception(_("Failed to set PGC positioning flag on DVD ") + 
                          path);
    }

    log_debug("Opened DVD %s\n", dvd_path.c_str());

    mutex = Ref<Mutex>(new Mutex(true));

    EOT = true;
}

DVDNavReader::~DVDNavReader()
{
    if (dvd)
        dvdnav_close(dvd);
    log_debug("Closing DVD %s\n", dvd_path.c_str());
}

int DVDNavReader::titleCount()
{
   int32_t t;
    if (dvdnav_get_number_of_titles(dvd, &t) != DVDNAV_STATUS_OK)
        throw _Exception(_("Failed to get title count for DVD ") + dvd_path + 
                " : " + dvdnav_err_to_string(dvd));

    return t;
}

int DVDNavReader::chapterCount(int title_idx)
{
    int32_t c;

    title_idx++;

    if ((title_idx < 1) || (title_idx > titleCount()))
        throw _Exception(_("Requested title number exceeds available titles "
                    "for DVD ") + dvd_path);

    if (dvdnav_get_number_of_parts(dvd, title_idx, &c) != DVDNAV_STATUS_OK)
        throw _Exception(_("Failed to get chapter count for title ") 
                         + title_idx + " DVD " + dvd_path);

    return c;
}

void DVDNavReader::selectPGC(int title_idx, int chapter_idx)
{
    title_idx++;
    chapter_idx++;

    if ((title_idx < 1) || (title_idx > titleCount()))
        throw _Exception(_("Attmpted to select invalid title!"));

    if ((chapter_idx < 1) || (chapter_idx > chapterCount(title_idx-1)))
        throw _Exception(_("Attempted to select invalid chapter!"));

    AUTOLOCK(mutex);

   if (dvdnav_part_play(dvd, title_idx, chapter_idx) != DVDNAV_STATUS_OK)
   {
       throw _Exception(_("Failed to select PGC for DVD ") + dvd_path + " : " +
                         dvdnav_err_to_string(dvd));
   }

   EOT = false;
}

size_t DVDNavReader::readSector(unsigned char *buffer, size_t length)
{
    AUTOLOCK(mutex);

    unsigned char *p = buffer;
    size_t consumed = 0;

    if (length < DVD_VIDEO_LB_LEN)
        throw _Exception(_("Buffer must be at least ") + DVD_VIDEO_LB_LEN);

    while (!EOT)
    {
        int result, event, len;

        result = dvdnav_get_next_block(dvd, (uint8_t *)p, &event, &len);
        if (result == DVDNAV_STATUS_ERR)
        {
            throw _Exception(_("Error getting next block for DVD ") + dvd_path +
                               " : " + dvdnav_err_to_string(dvd));
        }

        switch (event)
        {
            case DVDNAV_BLOCK_OK:
                consumed = consumed + len;
                if ((consumed + DVD_VIDEO_LB_LEN) > length)
                    return consumed;
                    
                p = p + len;
                break;
            case DVDNAV_STILL_FRAME:
                {
                    dvdnav_still_event_t *still_event;
                    still_event = (dvdnav_still_event_t *)p;
                    if (still_event->length == 0xff)
                        dvdnav_still_skip(dvd);
                }
                break;
            case DVDNAV_WAIT:
                dvdnav_wait_skip(dvd);
                break;
            case DVDNAV_CELL_CHANGE:
                {
                    int32_t tt = 0, ptt = 0;
                    dvdnav_current_title_info(dvd, &tt, &ptt);
                    if (tt == 0)
                    {
                        log_warning("Reached DVD menu, aborting.\n");
                        EOT = true;
                        return consumed;
                    }
                }
                break;
            case DVDNAV_STOP:
                EOT = true;
                return consumed;
                break;
            case DVDNAV_NAV_PACKET:
            case DVDNAV_NOP:
            case DVDNAV_SPU_CLUT_CHANGE:
            case DVDNAV_SPU_STREAM_CHANGE:
            case DVDNAV_AUDIO_STREAM_CHANGE:
            case DVDNAV_HIGHLIGHT:
            case DVDNAV_VTS_CHANGE:
            case DVDNAV_HOP_CHANNEL:
                break;
            default:
                log_error("Uknown event when playing DVD %s\n", dvd_path.c_str());
                EOT = true;
                return -1;
                break;
        }
    }
    return 0;
}

int DVDNavReader::audioTrackCount()
{
    AUTOLOCK(mutex);

    uint8_t count = 0;
    while (true)
    {
        if(dvdnav_get_audio_logical_stream(dvd, count) < 0)
            break;

        // afaik only 8 streams are supported?
        // \todo check the exact amount of supported audio streams in the DVD
        if (count > 10)
            break;

        count++;
    }

    return (int)count;
}

// from lsdvd
String DVDNavReader::getLanguage(char *code)
{
    int k = 0;
    while (memcmp(language[k].code, code, 2) && language[k].name[0] ) { k++; }
    return _(language[k].name);
}

String DVDNavReader::audioLanguage(int stream_idx)
{
    char code[3];
    audio_attr_t audio_attr;
    
    AUTOLOCK(mutex);

    if (dvdnav_get_audio_attr(dvd, stream_idx, &audio_attr) != DVDNAV_STATUS_OK)
        throw _Exception(_("Error error retrieving audio language from DVD ") +
                           dvd_path + " : " + 
                           dvdnav_err_to_string(dvd));

    sprintf(code, "%c%c", audio_attr.lang_code >> 8, 
                          audio_attr.lang_code & 0xff);
    
    if (!code[0])
    { 
        code[0] = 'x'; 
        code[1] = 'x'; 
    }

    return getLanguage(code);
}

int DVDNavReader::audioSampleFrequency(int stream_idx)
{
    audio_attr_t audio_attr;
    
    AUTOLOCK(mutex);
    
    if (dvdnav_get_audio_attr(dvd, stream_idx, &audio_attr) != DVDNAV_STATUS_OK)
        throw _Exception(_("Error error retrieving audio language from DVD ") +
                           dvd_path + " : " + 
                           dvdnav_err_to_string(dvd));

    if (audio_attr.sample_frequency == 0)
        return 48000;
    else
        return 96000;
}

int DVDNavReader::audioChannels(int stream_idx)
{
    audio_attr_t audio_attr;
   
    AUTOLOCK(mutex);

    if (dvdnav_get_audio_attr(dvd, stream_idx, &audio_attr) != DVDNAV_STATUS_OK)
        throw _Exception(_("Error error retrieving audio language from DVD ") +
                           dvd_path + " : " + 
                           dvdnav_err_to_string(dvd));

    return audio_attr.channels + 1;
}

String DVDNavReader::audioFormat(int stream_idx)
{
    audio_attr_t audio_attr;
    
    AUTOLOCK(mutex);
    
    if (dvdnav_get_audio_attr(dvd, stream_idx, &audio_attr) != DVDNAV_STATUS_OK)
        throw _Exception(_("Error error retrieving audio language from DVD ") +
                           dvd_path + " : " +
                           dvdnav_err_to_string(dvd));
    return _(audio_format[audio_attr.audio_format]);
}

int DVDNavReader::audioStreamID(int stream_idx)
{
    audio_attr_t audio_attr;

    AUTOLOCK(mutex);
    
    if (dvdnav_get_audio_attr(dvd, stream_idx, &audio_attr) != DVDNAV_STATUS_OK)
        throw _Exception(_("Error error retrieving audio language from DVD ") +
                           dvd_path + " : " +
                           dvdnav_err_to_string(dvd));

    return audio_id[audio_attr.audio_format]+stream_idx;
}

#endif//HAVE_LIBDVDNAV
