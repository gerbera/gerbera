/*GRB*

    Gerbera - https://gerbera.io/

    config_setup.h - this file is part of Gerbera.

    Copyright (C) 2020-2025 Gerbera Contributors

    Gerbera is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License version 2
    as published by the Free Software Foundation.

    Gerbera is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with Gerbera.  If not, see <http://www.gnu.org/licenses/>.

    $Id$
*/

/// \file config_definition.cc
///\brief Definitions of default values and setup for configuration.
#define GRB_LOG_FAC GrbLogFacility::config

#include "config_definition.h" // API

#include "cds/cds_enums.h"
#include "config/result/autoscan.h"
#include "config/result/box_layout.h"
#include "config/result/client_config.h"
#include "config/result/transcoding.h"
#include "config/setup/config_setup_array.h"
#include "config/setup/config_setup_autoscan.h"
#include "config/setup/config_setup_bool.h"
#include "config/setup/config_setup_boxlayout.h"
#include "config/setup/config_setup_client.h"
#include "config/setup/config_setup_dictionary.h"
#include "config/setup/config_setup_dynamic.h"
#include "config/setup/config_setup_enum.h"
#include "config/setup/config_setup_int.h"
#include "config/setup/config_setup_path.h"
#include "config/setup/config_setup_string.h"
#include "config/setup/config_setup_time.h"
#include "config/setup/config_setup_transcoding.h"
#include "config/setup/config_setup_tweak.h"
#include "config/setup/config_setup_vector.h"
#include "config_option_enum.h"
#include "config_options.h"
#include "config_setup.h"
#include "config_val.h"
#include "content/content.h"
#include "database/sqlite3/sqlite_config.h"
#include "metadata/metadata_enums.h"
#include "upnp/upnp_common.h"

#include <algorithm>
#include <upnpconfig.h>

#ifdef HAVE_CURL
#include <curl/curl.h>
#endif

// default values

#define DEFAULT_FOLLOW_SYMLINKS_VALUE YES
#define DEFAULT_RESOURCES_CASE_SENSITIVE YES
#define DEFAULT_UPNP_STRING_LIMIT (-1)

#define DEFAULT_LIBOPTS_ENTRY_SEPARATOR "; "

/// \brief default values for ConfigVal::IMPORT_SYSTEM_DIRECTORIES
static const std::vector<std::string> excludesFullpath {
    "/bin",
    "/boot",
    "/dev",
    "/etc",
    "/lib",
    "/lib32",
    "/lib64",
    "/libx32",
    "/proc",
    "/run",
    "/sbin",
    "/sys",
    "/tmp",
    "/usr",
    "/var",
};

/// \brief default values for ConfigVal::IMPORT_RESOURCES_FANART_FILE_LIST
static const std::vector<std::string> defaultFanArtFile {
    // "%title%.jpg",
    // "%filename%.jpg",
    // "folder.jpg",
    // "poster.jpg",
    // "cover.jpg",
    // "albumartsmall.jpg",
    // "%album%.jpg",
};

/// \brief default values for ConfigVal::IMPORT_RESOURCES_CONTAINERART_FILE_LIST
static const std::vector<std::string> defaultContainerArtFile {
    // "folder.jpg",
    // "poster.jpg",
    // "cover.jpg",
    // "albumartsmall.jpg",
};

/// \brief default values for ConfigVal::IMPORT_RESOURCES_SUBTITLE_FILE_LIST
static const std::vector<std::string> defaultSubtitleFile {
    // "%title%.srt",
    // "%filename%.srt"
};

/// \brief default values for ConfigVal::IMPORT_RESOURCES_METAFILE_FILE_LIST
static const std::vector<std::string> defaultMetadataFile {
    // "%filename%.nfo"
};

/// \brief default values for ConfigVal::IMPORT_RESOURCES_RESOURCE_FILE_LIST
static const std::vector<std::string> defaultResourceFile {
    // "%filename%.srt",
    // "cover.jpg",
    // "%album%.jpg",
};

/// \brief default values for ConfigVal::SERVER_UI_ITEMS_PER_PAGE_DROPDOWN
static const std::vector<std::string> defaultItemsPerPage {
    "10",
    "25",
    "50",
    "100",
};

/// \brief default values for ConfigVal::IMPORT_MAPPINGS_MIMETYPE_TO_CONTENTTYPE_LIST
static const std::map<std::string, std::string> mtCtDefaults {
    { "application/ogg", CONTENT_TYPE_OGG },
    { "audio/L16", CONTENT_TYPE_PCM },
    { "audio/flac", CONTENT_TYPE_FLAC },
    { "audio/mp4", CONTENT_TYPE_MP4 },
    { "audio/mpeg", CONTENT_TYPE_MP3 },
    { "audio/ogg", CONTENT_TYPE_OGG },
    { "audio/x-dsd", CONTENT_TYPE_DSD },
    { "audio/x-flac", CONTENT_TYPE_FLAC },
    { "audio/x-matroska", CONTENT_TYPE_MKA },
    { "audio/x-mpegurl", CONTENT_TYPE_PLAYLIST },
    { "audio/x-ms-wma", CONTENT_TYPE_WMA },
    { "audio/x-scpls", CONTENT_TYPE_PLAYLIST },
    { "audio/x-wav", CONTENT_TYPE_PCM },
    { "audio/x-wavpack", CONTENT_TYPE_WAVPACK },
    { "image/jpeg", CONTENT_TYPE_JPG },
    { "image/png", CONTENT_TYPE_PNG },
    { "video/mkv", CONTENT_TYPE_MKV },
    { "video/mp4", CONTENT_TYPE_MP4 },
    { "video/mpeg", CONTENT_TYPE_MPEG },
    { "video/x-matroska", CONTENT_TYPE_MKV },
    { "video/x-mkv", CONTENT_TYPE_MKV },
    { "video/x-ms-asf", CONTENT_TYPE_ASF },
    { MIME_TYPE_ASX_PLAYLIST, CONTENT_TYPE_PLAYLIST },
    { "video/x-msvideo", CONTENT_TYPE_AVI },
};

/// \brief default values for ConfigVal::IMPORT_MAPPINGS_CONTENTTYPE_TO_DLNAPROFILE_LIST
static const std::vector<std::vector<std::pair<std::string, std::string>>> ctDlnaDefaults {
    { { "from", CONTENT_TYPE_ASF }, { "to", "VC_ASF_AP_L2_WMA" } },
    { { "from", CONTENT_TYPE_AVI }, { "to", "AVI" } },
    { { "from", CONTENT_TYPE_DSD }, { "to", "DSF" } },
    { { "from", CONTENT_TYPE_FLAC }, { "to", "FLAC" } },
    { { "from", CONTENT_TYPE_JPG }, { "to", "JPEG_LRG" } },
    { { "from", CONTENT_TYPE_MKA }, { "to", "MKV" } },
    { { "from", CONTENT_TYPE_MKV }, { "to", "MKV" } },
    { { "from", CONTENT_TYPE_MP3 }, { "to", "MP3" } },
    { { "from", CONTENT_TYPE_MP4 }, { "to", "AVC_MP4_EU" } },
    { { "from", CONTENT_TYPE_MPEG }, { "to", "MPEG_PS_PAL" } },
    { { "from", CONTENT_TYPE_OGG }, { "to", "OGG" } },
    { { "from", CONTENT_TYPE_PCM }, { "to", "LPCM" } },
    { { "from", CONTENT_TYPE_PNG }, { "to", "PNG_LRG" } },
    { { "from", CONTENT_TYPE_WMA }, { "to", "WMAFULL" } },
};

/// \brief default values for ConfigVal::IMPORT_MAPPINGS_MIMETYPE_TO_UPNP_CLASS_LIST
static const std::map<std::string, std::string> mtUpnpDefaults {
    { "application/ogg", UPNP_CLASS_MUSIC_TRACK },
    { "audio/*", UPNP_CLASS_MUSIC_TRACK },
    { "image/*", UPNP_CLASS_IMAGE_ITEM },
    { "video/*", UPNP_CLASS_VIDEO_ITEM },
};

/// \brief default values for ConfigVal::IMPORT_MAPPINGS_CONTENTTYPE_TO_DLNATRANSFER_LIST
static const std::map<std::string, std::string> mtTransferDefaults {
    { "application/ogg", UPNP_DLNA_TRANSFER_MODE_STREAMING },
    { "audio/*", UPNP_DLNA_TRANSFER_MODE_STREAMING },
    { "image/*", UPNP_DLNA_TRANSFER_MODE_INTERACTIVE },
    { "video/*", UPNP_DLNA_TRANSFER_MODE_STREAMING },
    { "text/*", UPNP_DLNA_TRANSFER_MODE_BACKGROUND },
    { "application/x-srt", UPNP_DLNA_TRANSFER_MODE_BACKGROUND },
    { "srt", UPNP_DLNA_TRANSFER_MODE_BACKGROUND },
};

/// \brief default values for ConfigVal::IMPORT_MAPPINGS_EXTENSION_TO_MIMETYPE_LIST
static const std::map<std::string, std::string> extMtDefaults {
    { "asf", "video/x-ms-asf" },
    { "asx", MIME_TYPE_ASX_PLAYLIST }, // tweak to handle asx as playlist
    { "dff", "audio/x-dff" },
    { "dsd", "audio/x-dsd" },
    { "dsf", "audio/x-dsf" },
    { "flv", "video/x-flv" },
    { "m2ts", "video/mp2t" }, // LibMagic fails to identify MPEG2 Transport Streams
    { "m3u", "audio/x-mpegurl" },
    { "m3u8", "audio/x-mpegurl" },
    { "m4a", "audio/mp4" }, // LibMagic identifies this as audio/x-m4a, preventing TagLib from parsing it
    { "mka", "audio/x-matroska" },
    { "mkv", "video/x-matroska" },
    { "mp3", "audio/mpeg" },
    { "mts", "video/mp2t" }, // LibMagic fails to identify MPEG2 Transport Streams
    { "oga", "audio/ogg" },
    { "ogg", "audio/ogg" },
    { "ogm", "video/ogg" },
    { "ogv", "video/ogg" },
    { "ogx", "application/ogg" },
    { "pls", "audio/x-scpls" },
    { "ts", "video/mp2t" }, // LibMagic fails to identify MPEG2 Transport Streams
    { "tsa", "audio/mp2t" }, // LibMagic fails to identify MPEG2 Transport Streams
    { "tsv", "video/mp2t" }, // LibMagic fails to identify MPEG2 Transport Streams
    { "wax", "audio/x-ms-wax" },
    { "wm", "video/x-ms-wm" },
    { "wma", "audio/x-ms-wma" },
    { "wmv", "video/x-ms-wmv" },
    { "wmx", "video/x-ms-wmx" },
    { "wv", "audio/x-wavpack" },
    { "wvx", "video/x-ms-wvx" },
};

/// \brief default values for ConfigVal::SERVER_UI_EXTENSION_MIMETYPE_MAPPING
static const std::map<std::string, std::string> uiExtMtDefaults {
    { "css", "text/css" },
    { "html", "text/html" },
    { "js", "application/javascript" },
    { "json", "application/json" },
};

/// \brief default values for ConfigVal::IMPORT_MAPPINGS_IGNORED_EXTENSIONS
static const std::vector<std::string> ignoreDefaults {
    "part",
    "tmp",
};

/// \brief default values for ConfigVal::A_TRANSCODING_MIMETYPE_PROF_MAP
static const std::map<std::string, std::string> trMtDefaults {
    { "video/x-flv", "vlcmpeg" },
    { "application/ogg", "vlcmpeg" },
    { "audio/ogg", "ogg2mp3" },
};

/// \brief default values for ConfigVal::UPNP_SEARCH_SEGMENTS
static const std::vector<std::string> upnpSearchSegmentDefaults {
    "M_ARTIST",
    "M_TITLE",
};

/// \brief default values for ConfigVal::UPNP_ALBUM_PROPERTIES
static const std::map<std::string, std::string> upnpAlbumPropDefaults {
    { "dc:creator", "M_ALBUMARTIST" },
    { UPNP_SEARCH_ARTIST, "M_ALBUMARTIST" },
    { "upnp:albumArtist", "M_ALBUMARTIST" },
    { "upnp:composer", "M_COMPOSER" },
    { "upnp:conductor", "M_CONDUCTOR" },
    { "upnp:orchestra", "M_ORCHESTRA" },
    { "upnp:date", "M_UPNP_DATE" },
    { DC_DATE, "M_UPNP_DATE" },
    { "upnp:producer", "M_PRODUCER" },
    { "dc:publisher", "M_PUBLISHER" },
    { UPNP_SEARCH_GENRE, "M_GENRE" },
};

/// \brief default values for ConfigVal::UPNP_ARTIST_PROPERTIES
static const std::map<std::string, std::string> upnpArtistPropDefaults {
    { UPNP_SEARCH_ARTIST, "M_ALBUMARTIST" },
    { "upnp:albumArtist", "M_ALBUMARTIST" },
    { UPNP_SEARCH_GENRE, "M_GENRE" },
};

/// \brief default values for ConfigVal::UPNP_GENRE_PROPERTIES
static const std::map<std::string, std::string> upnpGenrePropDefaults {
    { UPNP_SEARCH_GENRE, "M_GENRE" },
};

/// \brief default values for ConfigVal::UPNP_PLAYLIST_PROPERTIES
static const std::map<std::string, std::string> upnpPlaylistPropDefaults {
    { DC_DATE, "M_UPNP_DATE" },
};

/// \brief default values for ConfigVal::IMPORT_LIBOPTS_ID3_METADATA_TAGS_LIST
static const std::map<std::string, std::string> id3SpecialPropertyMap {
    { "PERFORMER", UPNP_SEARCH_ARTIST "@role[Performer]" },
};

/// \brief default values for ConfigVal::IMPORT_LIBOPTS_FFMPEG_METADATA_TAGS_LIST
static const std::map<std::string, std::string> ffmpegSpecialPropertyMap {
    { "performer", UPNP_SEARCH_ARTIST "@role[Performer]" },
};

/// \brief default values for ConfigVal::IMPORT_VIRTUAL_DIRECTORY_KEYS
static const std::vector<std::vector<std::pair<std::string, std::string>>> virtualDirectoryKeys {
    { { "metadata", "M_ALBUMARTIST" }, { "class", UPNP_CLASS_MUSIC_ALBUM } },
    { { "metadata", "M_UPNP_DATE" }, { "class", UPNP_CLASS_MUSIC_ALBUM } },
};

/// \brief default values for ConfigVal::BOXLAYOUT_BOX
static const std::vector<BoxLayout> boxLayoutDefaults {
    // FOLDER_STRUCTURE

    BoxLayout(BoxKeys::audioAllAlbums, "Albums", UPNP_CLASS_CONTAINER, "MUSIC_ALBUMS"),
    BoxLayout(BoxKeys::audioAllArtists, "Artists", UPNP_CLASS_CONTAINER, "MUSIC_ARTISTS"),
    BoxLayout(BoxKeys::audioAll, "All Audio", UPNP_CLASS_CONTAINER),
    BoxLayout(BoxKeys::audioAllComposers, "Composers", UPNP_CLASS_CONTAINER),
    BoxLayout(BoxKeys::audioAllDirectories, "Directories", UPNP_CLASS_CONTAINER, "MUSIC_FOLDER_STRUCTURE"),
    BoxLayout(BoxKeys::audioAllGenres, "Genres", UPNP_CLASS_CONTAINER, "MUSIC_GENRES"),
    BoxLayout(BoxKeys::audioAllSongs, "All Songs", UPNP_CLASS_CONTAINER),
    BoxLayout(BoxKeys::audioAllTracks, "All - full name", UPNP_CLASS_CONTAINER, "MUSIC_ALL"),
    BoxLayout(BoxKeys::audioAllYears, "Year", UPNP_CLASS_CONTAINER),
    BoxLayout(BoxKeys::audioRoot, "Audio", UPNP_CLASS_CONTAINER),
    BoxLayout(BoxKeys::audioArtistChronology, "Album Chronology", UPNP_CLASS_CONTAINER),

    BoxLayout(BoxKeys::audioInitialAbc, "ABC", UPNP_CLASS_CONTAINER),
    BoxLayout(BoxKeys::audioInitialAllArtistTracks, "000 All", UPNP_CLASS_CONTAINER),
    BoxLayout(BoxKeys::audioInitialAllBooks, "Books", UPNP_CLASS_CONTAINER),
    BoxLayout(BoxKeys::audioInitialAudioBookRoot, "AudioBooks", UPNP_CLASS_CONTAINER, "MUSIC_AUDIOBOOKS"),

    BoxLayout(BoxKeys::audioStructuredAllAlbums, "-Album-", UPNP_CLASS_CONTAINER, "MUSIC_ALBUMS", true, 6),
    BoxLayout(BoxKeys::audioStructuredAllArtistTracks, "all", UPNP_CLASS_CONTAINER),
    BoxLayout(BoxKeys::audioStructuredAllArtists, "-Artist-", UPNP_CLASS_CONTAINER, "MUSIC_ARTISTS", true, 9),
    BoxLayout(BoxKeys::audioStructuredAllGenres, "-Genre-", UPNP_CLASS_CONTAINER, "MUSIC_GENRES", true, 6),
    BoxLayout(BoxKeys::audioStructuredAllTracks, "-Track-", UPNP_CLASS_CONTAINER, "MUSIC_ALL", true, 6),
    BoxLayout(BoxKeys::audioStructuredAllYears, "-Year-", UPNP_CLASS_CONTAINER),

    BoxLayout(BoxKeys::videoAllDates, "Date", UPNP_CLASS_CONTAINER, "VIDEOS_YEARS_MONTH"),
    BoxLayout(BoxKeys::videoAllDirectories, "Directories", UPNP_CLASS_CONTAINER, "VIDEOS_FOLDER_STRUCTURE"),
    BoxLayout(BoxKeys::videoAll, "All Video", UPNP_CLASS_CONTAINER, "VIDEOS_ALL"),
    BoxLayout(BoxKeys::videoAllYears, "Year", UPNP_CLASS_CONTAINER, "VIDEOS_YEARS"),
    BoxLayout(BoxKeys::videoUnknown, "Unknown", UPNP_CLASS_CONTAINER),
    BoxLayout(BoxKeys::videoRoot, "Video", UPNP_CLASS_CONTAINER, "VIDEOS"),
    // VIDEOS_GENRES
    // VIDEOS_ALBUM
    // VIDEOS_RECENTLY_ADDED
    // VIDEOS_LAST_PLAYED
    // VIDEOS_RECORDINGS

    // IMAGES_ALBUM
    // IMAGES_SLIDESHOWS
    // IMAGES_RECENTLY_ADDED
    // IMAGES_LAST_WATCHED
    BoxLayout(BoxKeys::imageAllDates, "Date", UPNP_CLASS_CONTAINER, "IMAGES_YEARS_MONTH"),
    BoxLayout(BoxKeys::imageAllDirectories, "Directories", UPNP_CLASS_CONTAINER, "IMAGES_FOLDER_STRUCTURE"),
    BoxLayout(BoxKeys::imageAll, "All Photos", UPNP_CLASS_CONTAINER, "IMAGES_ALL"),
    BoxLayout(BoxKeys::imageAllYears, "Year", UPNP_CLASS_CONTAINER, "IMAGES_YEARS"),
    BoxLayout(BoxKeys::imageRoot, "Photos", UPNP_CLASS_CONTAINER, "IMAGES"),
    BoxLayout(BoxKeys::imageUnknown, "Unknown", UPNP_CLASS_CONTAINER),

#ifdef ONLINE_SERVICES
    BoxLayout(BoxKeys::trailerRoot, "Online Services", UPNP_CLASS_CONTAINER),
    BoxLayout(BoxKeys::trailerAll, "All Trailers", UPNP_CLASS_CONTAINER),
    BoxLayout(BoxKeys::trailerAllGenres, "Genres", UPNP_CLASS_CONTAINER),
    BoxLayout(BoxKeys::trailerRelDate, "Release Date", UPNP_CLASS_CONTAINER),
    BoxLayout(BoxKeys::trailerPostDate, "Post Date", UPNP_CLASS_CONTAINER),
    BoxLayout(BoxKeys::trailerUnknown, "Unknown", UPNP_CLASS_CONTAINER),
#endif
#ifdef HAVE_JS
    BoxLayout(BoxKeys::playlistRoot, "Playlists", UPNP_CLASS_CONTAINER),
    BoxLayout(BoxKeys::playlistAll, "All Playlists", UPNP_CLASS_CONTAINER),
    BoxLayout(BoxKeys::playlistAllDirectories, "Directories", UPNP_CLASS_CONTAINER, "MUSIC_PLAYLISTS", true, 1),
#endif
};

/// \brief default values for ConfigVal::UPNP_RESOURCE_PROPERTY_DEFAULTS
static const std::map<std::string, std::string> upnpResourceDefaultPropDefaults {
    { "@allowedUse", "UNKNOWN" },
    { "@bitrate", "0" },
    { "@bitsPerSample", "0" },
    { "@colorDepth", "1" },
    { "@contentInfoURI", "" },
    { "@daylightSaving", "UNKNOWN" },
    { "@duration", "0" },
    { "@framerate", "30p" },
    { "@importUri", "" },
    { "@nrAudioChannels", "0" },
    { "@protection", "UNKNOWN" },
    { "@protocolInfo", "http:*:*:*" },
    { "@recordQuality", "" },
    { "@reliability", "100" },
    { "@remainingTime", "" },
    { "@resolution", "1x1" },
    { "@rightsInfoURI", "" },
    { "@sampleFrequency", "0" },
    { "@size", "0" },
    { "@tspec", "UNKNOWN" },
    { "@usageInfo", "This file is hosted by Gerbera" },
    { "@validityEnd", "1900-01-01T00:00:00" },
    { "@validityStart", "1900-01-01T00:00:00" },
};

/// \brief default values for ConfigVal::UPNP_OBJECT_PROPERTY_DEFAULTS
static const std::map<std::string, std::string> upnpObjectDefaultPropDefaults {
    { "@id", "0" },
    { "@parentID", "0" },
    { "dc:creator", "UNKNOWN" },
    { "dc:date", "1900-01-01T00:00:00" },
    { "upnp:actor", "UNKNOWN" },
    { "upnp:album", "UNKNOWN" },
    { "upnp:artist", "UNKNOWN" },
    { "upnp:artistDiscographyURI", "" },
    { "upnp:author", "UNKNOWN" },
    { "upnp:contributor", "UNKNOWN" },
    { "upnp:director", "UNKNOWN" },
    { "upnp:genre", "UNKNOWN" },
    { "upnp:language", "en-US" },
    { "upnp:lastPlaybackPosition", "0" },
    { "upnp:lastPlaybackTime", "1900-01-01T00:00:00" },
    { "upnp:lyricsURI", "" },
    { "upnp:originalTrackNumber", "0" },
    { "upnp:playbackCount", "0" },
    { "upnp:playlist", "UNKNOWN" },
    { "upnp:producer", "UNKNOWN" },
    { "upnp:publisher", "UNKNOWN" },
    { "upnp:rating", "0" },
    { "upnp:recordable", "1" },
    { "upnp:relation", "UNKNOWN" },
};

/// \brief default values for ConfigVal::UPNP_CONTAINER_PROPERTY_DEFAULTS
static const std::map<std::string, std::string> upnpContainerDefaultPropDefaults {
    { "@childContainerCount", "0" },
    { "@childCount", "0" },
    { "@id", "0" },
    { "@parentID", "0" },
    { "@restricted", "1" },
    { "@searchable", "1" },
    { "upnp:storageFree", "0" },
    { "upnp:storageMaxPartition", "0" },
    { "upnp:storageMedium", "UNKNOWN" },
    { "upnp:storageTotal", "0" },
    { "upnp:storageUsed", "0" },
};

/// \brief default values for ConfigVal::IMPORT_LIBOPTS_EXIV2_COMMENT_LIST
static const std::map<std::string, std::string> exiv2CommentDefaults {
    { "Taken with", "Exif.Image.Model" },
    { "Flash setting", "Exif.Photo.Flash" },
    { "Focal length", "Exif.Photo.FocalLength" },
    { "Focal length 35 mm equivalent", "Exif.Photo.FocalLengthIn35mmFilm" },
};

std::vector<std::shared_ptr<ConfigSetup>> ConfigDefinition::getServerOptions()
{
    return {
        // Core Server options
        std::make_shared<ConfigUIntSetup>(ConfigVal::SERVER_PORT,
            "/server/port", "config-server.html#port",
            0, CheckPortValue),
        std::make_shared<ConfigStringSetup>(ConfigVal::SERVER_IP,
            "/server/ip", "config-server.html#ip",
            ""),
        std::make_shared<ConfigStringSetup>(ConfigVal::SERVER_NETWORK_INTERFACE,
            "/server/interface", "config-server.html#interface",
            ""),
        std::make_shared<ConfigStringSetup>(ConfigVal::SERVER_NAME,
            "/server/name", "config-server.html#name",
            "Gerbera"),
        std::make_shared<ConfigStringSetup>(ConfigVal::SERVER_MANUFACTURER,
            "/server/manufacturer", "config-server.html#manufacturer",
            "Gerbera Contributors"),
        std::make_shared<ConfigStringSetup>(ConfigVal::SERVER_MANUFACTURER_URL,
            "/server/manufacturerURL", "config-server.html#manufacturerurl",
            DESC_MANUFACTURER_URL),
        std::make_shared<ConfigStringSetup>(ConfigVal::VIRTUAL_URL,
            "/server/virtualURL", "config-server.html#virtualURL",
            ""),
        std::make_shared<ConfigStringSetup>(ConfigVal::EXTERNAL_URL,
            "/server/externalURL", "config-server.html#externalURL",
            ""),
        std::make_shared<ConfigStringSetup>(ConfigVal::SERVER_MODEL_NAME,
            "/server/modelName", "config-server.html#modelname",
            "Gerbera"),
        std::make_shared<ConfigStringSetup>(ConfigVal::SERVER_MODEL_DESCRIPTION,
            "/server/modelDescription", "config-server.html#modeldescription",
            "Free UPnP AV MediaServer, GNU GPL"),
        std::make_shared<ConfigStringSetup>(ConfigVal::SERVER_MODEL_NUMBER,
            "/server/modelNumber", "config-server.html#modelnumber",
            GERBERA_VERSION),
        std::make_shared<ConfigStringSetup>(ConfigVal::SERVER_MODEL_URL,
            "/server/modelURL", "config-server.html#modelurl",
            ""),
        std::make_shared<ConfigStringSetup>(ConfigVal::SERVER_SERIAL_NUMBER,
            "/server/serialNumber", "config-server.html#serialnumber",
            "1"),
        std::make_shared<ConfigStringSetup>(ConfigVal::SERVER_PRESENTATION_URL,
            "/server/presentationURL", "config-server.html#presentationurl", ""),
        std::make_shared<ConfigEnumSetup<UrlAppendMode>>(ConfigVal::SERVER_APPEND_PRESENTATION_URL_TO,
            "/server/presentationURL/attribute::append-to", "config-server.html#presentationurl",
            UrlAppendMode::none,
            std::map<std::string, UrlAppendMode>({ { "none", UrlAppendMode::none }, { "ip", UrlAppendMode::ip }, { "port", UrlAppendMode::port } })),
        std::make_shared<ConfigStringSetup>(ConfigVal::SERVER_UDN,
            "/server/udn", "config-server.html#udn", GRB_UDN_AUTO),
        std::make_shared<ConfigStringSetup>(ConfigVal::SERVER_HOME,
            "/server/home", "config-server.html#home"),
        std::make_shared<ConfigBoolSetup>(ConfigVal::SERVER_HOME_OVERRIDE,
            "/server/home/attribute::override", "config-server.html#home",
            NO),
        std::make_shared<ConfigPathSetup>(ConfigVal::SERVER_TMPDIR,
            "/server/tmpdir", "config-server.html#tmpdir",
            "/tmp/"),
        std::make_shared<ConfigPathSetup>(ConfigVal::SERVER_WEBROOT,
            "/server/webroot", "config-server.html#webroot"),
        std::make_shared<ConfigIntSetup>(ConfigVal::SERVER_ALIVE_INTERVAL,
            "/server/alive", "config-server.html#alive",
            180, ALIVE_INTERVAL_MIN, ConfigIntSetup::CheckMinValue),
        std::make_shared<ConfigBoolSetup>(ConfigVal::SERVER_HIDE_PC_DIRECTORY,
            "/server/pc-directory/attribute::upnp-hide", "config-server.html#pc-directory",
            NO),
        std::make_shared<ConfigPathSetup>(ConfigVal::SERVER_BOOKMARK_FILE,
            "/server/bookmark", "config-server.html#bookmark",
            "gerbera.html", ConfigPathArguments::isFile | ConfigPathArguments::resolveEmpty),
        std::make_shared<ConfigIntSetup>(ConfigVal::SERVER_UPNP_TITLE_AND_DESC_STRING_LIMIT,
            "/server/upnp-string-limit", "config-server.html#upnp-string-limit",
            DEFAULT_UPNP_STRING_LIMIT, CheckUpnpStringLimitValue),

        // Database setup
        std::make_shared<ConfigStringSetup>(ConfigVal::SERVER_STORAGE,
            "/server/storage", "config-server.html#storage",
            true),
        std::make_shared<ConfigBoolSetup>(ConfigVal::SERVER_STORAGE_USE_TRANSACTIONS,
            "/server/storage/attribute::use-transactions", "config-server.html#storage",
            NO),
        std::make_shared<ConfigStringSetup>(ConfigVal::SERVER_STORAGE_MYSQL,
            "/server/storage/mysql", "config-server.html#storage"),
#ifdef HAVE_MYSQL
        std::make_shared<ConfigBoolSetup>(ConfigVal::SERVER_STORAGE_MYSQL_ENABLED,
            "/server/storage/mysql/attribute::enabled", "config-server.html#storage",
            NO),
        std::make_shared<ConfigStringSetup>(ConfigVal::SERVER_STORAGE_MYSQL_HOST,
            "/server/storage/mysql/host", "config-server.html#storage",
            "localhost"),
        std::make_shared<ConfigIntSetup>(ConfigVal::SERVER_STORAGE_MYSQL_PORT,
            "/server/storage/mysql/port", "config-server.html#storage",
            0, CheckPortValue),
        std::make_shared<ConfigStringSetup>(ConfigVal::SERVER_STORAGE_MYSQL_USERNAME,
            "/server/storage/mysql/username", "config-server.html#storage",
            "gerbera"),
        std::make_shared<ConfigStringSetup>(ConfigVal::SERVER_STORAGE_MYSQL_SOCKET,
            "/server/storage/mysql/socket", "config-server.html#storage",
            ""),
        std::make_shared<ConfigStringSetup>(ConfigVal::SERVER_STORAGE_MYSQL_PASSWORD,
            "/server/storage/mysql/password", "config-server.html#storage",
            ""),
        std::make_shared<ConfigStringSetup>(ConfigVal::SERVER_STORAGE_MYSQL_DATABASE,
            "/server/storage/mysql/database", "config-server.html#storage",
            "gerbera"),
        std::make_shared<ConfigPathSetup>(ConfigVal::SERVER_STORAGE_MYSQL_INIT_SQL_FILE,
            "/server/storage/mysql/init-sql-file", "config-server.html#storage",
            "", ConfigPathArguments::isFile | ConfigPathArguments::mustExist | ConfigPathArguments::resolveEmpty), // This should really be "dataDir / mysql.sql"
        std::make_shared<ConfigPathSetup>(ConfigVal::SERVER_STORAGE_MYSQL_UPGRADE_FILE,
            "/server/storage/mysql/upgrade-file", "config-server.html#storage",
            "", ConfigPathArguments::isFile | ConfigPathArguments::mustExist | ConfigPathArguments::resolveEmpty),
#else
        std::make_shared<ConfigBoolSetup>(ConfigVal::SERVER_STORAGE_MYSQL_ENABLED,
            "/server/storage/mysql/attribute::enabled", "config-server.html#storage",
            NO),
#endif
        std::make_shared<ConfigStringSetup>(ConfigVal::SERVER_STORAGE_SQLITE,
            "/server/storage/sqlite3", "config-server.html#storage"),
        std::make_shared<ConfigStringSetup>(ConfigVal::SERVER_STORAGE_DRIVER,
            "/server/storage/driver", "config-server.html#storage"),
        std::make_shared<ConfigBoolSetup>(ConfigVal::SERVER_STORAGE_SQLITE_ENABLED,
            "/server/storage/sqlite3/attribute::enabled", "config-server.html#storage",
            YES),
        std::make_shared<ConfigPathSetup>(ConfigVal::SERVER_STORAGE_SQLITE_DATABASE_FILE,
            "/server/storage/sqlite3/database-file", "config-server.html#storage",
            "gerbera.db", ConfigPathArguments::isFile | ConfigPathArguments::resolveEmpty),
        std::make_shared<ConfigIntSetup>(ConfigVal::SERVER_STORAGE_SQLITE_SYNCHRONOUS,
            "/server/storage/sqlite3/synchronous", "config-server.html#storage",
            "off", SqliteConfig::CheckSqlLiteSyncValue, SqliteConfig::PrintSqlLiteSyncValue),
        std::make_shared<ConfigStringSetup>(ConfigVal::SERVER_STORAGE_SQLITE_JOURNALMODE,
            "/server/storage/sqlite3/journal-mode", "config-server.html#storage",
            "WAL", StringCheckFunction(SqliteConfig::CheckSqlJournalMode)),
        std::make_shared<ConfigBoolSetup>(ConfigVal::SERVER_STORAGE_SQLITE_RESTORE,
            "/server/storage/sqlite3/on-error", "config-server.html#storage",
            "restore", StringCheckFunction(SqliteConfig::CheckSqlLiteRestoreValue)),
        std::make_shared<ConfigBoolSetup>(ConfigVal::SERVER_STORAGE_SQLITE_BACKUP_ENABLED,
            "/server/storage/sqlite3/backup/attribute::enabled", "config-server.html#storage",
#ifdef SQLITE_BACKUP_ENABLED
            YES
#else
            NO
#endif
            ),
        std::make_shared<ConfigTimeSetup>(ConfigVal::SERVER_STORAGE_SQLITE_BACKUP_INTERVAL,
            "/server/storage/sqlite3/backup/attribute::interval", "config-server.html#storage",
            GrbTimeType::Seconds, 600, 1),

        std::make_shared<ConfigPathSetup>(ConfigVal::SERVER_STORAGE_SQLITE_INIT_SQL_FILE,
            "/server/storage/sqlite3/init-sql-file", "config-server.html#storage",
            "", ConfigPathArguments::isFile | ConfigPathArguments::mustExist | ConfigPathArguments::resolveEmpty), // This should really be "dataDir / sqlite3.sql"
        std::make_shared<ConfigPathSetup>(ConfigVal::SERVER_STORAGE_SQLITE_UPGRADE_FILE,
            "/server/storage/sqlite3/upgrade-file", "config-server.html#storage",
            "", ConfigPathArguments::isFile | ConfigPathArguments::mustExist | ConfigPathArguments::resolveEmpty),
        std::make_shared<ConfigIntSetup>(ConfigVal::SERVER_STORAGE_SQLITE_SHUTDOWN_ATTEMPTS,
            "/server/storage/sqlite3/attribute::shutdown-attempts", "config-server.html#storage",
            5, 2, ConfigIntSetup::CheckMinValue),

        // Web User Interface
        std::make_shared<ConfigBoolSetup>(ConfigVal::SERVER_UI_ENABLED,
            "/server/ui/attribute::enabled", "config-server.html#ui",
            YES),
        std::make_shared<ConfigTimeSetup>(ConfigVal::SERVER_UI_POLL_INTERVAL,
            "/server/ui/attribute::poll-interval", "config-server.html#ui",
            GrbTimeType::Seconds, 2, 1),
        std::make_shared<ConfigBoolSetup>(ConfigVal::SERVER_UI_POLL_WHEN_IDLE,
            "/server/ui/attribute::poll-when-idle", "config-server.html#ui",
            NO),
        std::make_shared<ConfigBoolSetup>(ConfigVal::SERVER_UI_ENABLE_NUMBERING,
            "/server/ui/attribute::show-numbering", "config-server.html#ui",
            YES),
        std::make_shared<ConfigBoolSetup>(ConfigVal::SERVER_UI_ENABLE_THUMBNAIL,
            "/server/ui/attribute::show-thumbnail", "config-server.html#ui",
            YES),
        std::make_shared<ConfigBoolSetup>(ConfigVal::SERVER_UI_ENABLE_VIDEO,
            "/server/ui/attribute::show-video", "config-server.html#ui",
            NO),
        std::make_shared<ConfigBoolSetup>(ConfigVal::SERVER_UI_ACCOUNTS_ENABLED,
            "/server/ui/accounts/attribute::enabled", "config-server.html#ui",
            NO),
        std::make_shared<ConfigDictionarySetup>(ConfigVal::SERVER_UI_ACCOUNT_LIST,
            "/server/ui/accounts", "config-server.html#ui",
            ConfigVal::A_SERVER_UI_ACCOUNT_LIST_ACCOUNT, ConfigVal::A_SERVER_UI_ACCOUNT_LIST_USER, ConfigVal::A_SERVER_UI_ACCOUNT_LIST_PASSWORD),
        std::make_shared<ConfigTimeSetup>(ConfigVal::SERVER_UI_SESSION_TIMEOUT,
            "/server/ui/accounts/attribute::session-timeout", "config-server.html#ui",
            GrbTimeType::Minutes, 30, 1),
        std::make_shared<ConfigIntSetup>(ConfigVal::SERVER_UI_DEFAULT_ITEMS_PER_PAGE,
            "/server/ui/items-per-page/attribute::default", "config-server.html#ui",
            25, 1, ConfigIntSetup::CheckMinValue),
        std::make_shared<ConfigArraySetup>(ConfigVal::SERVER_UI_ITEMS_PER_PAGE_DROPDOWN,
            "/server/ui/items-per-page", "config-server.html#ui",
            ConfigVal::A_SERVER_UI_ITEMS_PER_PAGE_DROPDOWN_OPTION, ConfigArraySetup::InitItemsPerPage, true,
            defaultItemsPerPage),
        std::make_shared<ConfigBoolSetup>(ConfigVal::SERVER_UI_SHOW_TOOLTIPS,
            "/server/ui/attribute::show-tooltips", "config-server.html#ui",
            YES),
        std::make_shared<ConfigStringSetup>(ConfigVal::SERVER_UI_CONTENT_SECURITY_POLICY,
            "/server/ui/content-security-policy", "config-server.html#ui",
            "default-src %HOSTS% 'unsafe-eval' 'unsafe-inline'; img-src *; media-src *; child-src 'none';",
            ConfigStringSetup::MergeContentSecurityPolicy),
        std::make_shared<ConfigDictionarySetup>(ConfigVal::SERVER_UI_EXTENSION_MIMETYPE_MAPPING,
            "/server/ui/extension-mimetype", "config-import.html#extension-mimetype",
            ConfigVal::A_IMPORT_MAPPINGS_MIMETYPE_MAP, ConfigVal::A_IMPORT_MAPPINGS_MIMETYPE_FROM, ConfigVal::A_IMPORT_MAPPINGS_MIMETYPE_TO,
            false, false, true, uiExtMtDefaults),
        std::make_shared<ConfigStringSetup>(ConfigVal::SERVER_UI_EXTENSION_MIMETYPE_DEFAULT,
            "/server/ui/extension-mimetype/attribute::default", "config-server.html#ui",
            "application/octet-stream"),
        std::make_shared<ConfigStringSetup>(ConfigVal::SERVER_UI_DOCUMENTATION_SOURCE,
            "/server/ui/source-docs-link", "config-server.html#ui",
            ""),
        std::make_shared<ConfigStringSetup>(ConfigVal::SERVER_UI_DOCUMENTATION_USER,
            "/server/ui/user-docs-link", "config-server.html#ui",
#ifdef GERBERA_RELEASE
            "https://docs.gerbera.io/en/stable/"
#else
            "https://docs.gerbera.io/en/latest/"
#endif
            ),

    // Logging and debugging
#ifdef GRBDEBUG
        std::make_shared<ConfigIntSetup>(ConfigVal::SERVER_LOG_DEBUG_MODE,
            "/server/attribute::debug-mode", "config-server.html#debug-mode",
            0, GrbLogger::makeFacility, GrbLogger::printFacility),
#endif
        std::make_shared<ConfigUIntSetup>(ConfigVal::SERVER_LOG_ROTATE_SIZE,
            "/server/logging/attribute::rotate-file-size", "config-server.html#logging",
            1024 * 1024 * 5),
        std::make_shared<ConfigUIntSetup>(ConfigVal::SERVER_LOG_ROTATE_COUNT,
            "/server/logging/attribute::rotate-file-count", "config-server.html#logging",
            10),
#ifdef UPNP_HAVE_TOOLS
        std::make_shared<ConfigUIntSetup>(ConfigVal::SERVER_UPNP_MAXJOBS,
            "/server/attribute::upnp-max-jobs", "config-server.html#upnp-max-jobs",
            500),
#endif

    // Thumbnails for images and videos
#ifdef HAVE_FFMPEGTHUMBNAILER
        std::make_shared<ConfigBoolSetup>(ConfigVal::SERVER_EXTOPTS_FFMPEGTHUMBNAILER_ENABLED,
            "/server/extended-runtime-options/ffmpegthumbnailer/attribute::enabled", "config-extended.html#ffmpegthumbnailer",
            YES),
        std::make_shared<ConfigBoolSetup>(ConfigVal::SERVER_EXTOPTS_FFMPEGTHUMBNAILER_VIDEO_ENABLED,
            "/server/extended-runtime-options/ffmpegthumbnailer/attribute::video-enabled", "config-extended.html#ffmpegthumbnailer",
            YES),
        std::make_shared<ConfigBoolSetup>(ConfigVal::SERVER_EXTOPTS_FFMPEGTHUMBNAILER_IMAGE_ENABLED,
            "/server/extended-runtime-options/ffmpegthumbnailer/attribute::image-enabled", "config-extended.html#ffmpegthumbnailer",
            YES),
        std::make_shared<ConfigIntSetup>(ConfigVal::SERVER_EXTOPTS_FFMPEGTHUMBNAILER_THUMBSIZE,
            "/server/extended-runtime-options/ffmpegthumbnailer/thumbnail-size", "config-extended.html#ffmpegthumbnailer",
            160, 1, ConfigIntSetup::CheckMinValue),
        std::make_shared<ConfigIntSetup>(ConfigVal::SERVER_EXTOPTS_FFMPEGTHUMBNAILER_SEEK_PERCENTAGE,
            "/server/extended-runtime-options/ffmpegthumbnailer/seek-percentage", "config-extended.html#ffmpegthumbnailer",
            5, 0, ConfigIntSetup::CheckMinValue),
        std::make_shared<ConfigBoolSetup>(ConfigVal::SERVER_EXTOPTS_FFMPEGTHUMBNAILER_FILMSTRIP_OVERLAY,
            "/server/extended-runtime-options/ffmpegthumbnailer/filmstrip-overlay", "config-extended.html#ffmpegthumbnailer",
            NO),
        std::make_shared<ConfigIntSetup>(ConfigVal::SERVER_EXTOPTS_FFMPEGTHUMBNAILER_IMAGE_QUALITY,
            "/server/extended-runtime-options/ffmpegthumbnailer/image-quality", "config-extended.html#ffmpegthumbnailer",
            8, CheckImageQualityValue),
        std::make_shared<ConfigBoolSetup>(ConfigVal::SERVER_EXTOPTS_FFMPEGTHUMBNAILER_CACHE_DIR_ENABLED,
            "/server/extended-runtime-options/ffmpegthumbnailer/cache-dir/attribute::enabled", "config-extended.html#ffmpegthumbnailer",
            YES),
        std::make_shared<ConfigStringSetup>(ConfigVal::SERVER_EXTOPTS_FFMPEGTHUMBNAILER_CACHE_DIR, // ConfigPathSetup
            "/server/extended-runtime-options/ffmpegthumbnailer/cache-dir", "config-extended.html#ffmpegthumbnailer",
            ""),
#endif

        // Playmarks
        std::make_shared<ConfigBoolSetup>(ConfigVal::SERVER_EXTOPTS_MARK_PLAYED_ITEMS_ENABLED,
            "/server/extended-runtime-options/mark-played-items/attribute::enabled", "config-extended.html#extended-runtime-options",
            NO),
        std::make_shared<ConfigBoolSetup>(ConfigVal::SERVER_EXTOPTS_MARK_PLAYED_ITEMS_STRING_MODE_PREPEND,
            "/server/extended-runtime-options/mark-played-items/string/attribute::mode", "config-extended.html#extended-runtime-options",
            DEFAULT_MARK_PLAYED_ITEMS_STRING_MODE, StringCheckFunction(ConfigBoolSetup::CheckMarkPlayedValue)),
        std::make_shared<ConfigStringSetup>(ConfigVal::SERVER_EXTOPTS_MARK_PLAYED_ITEMS_STRING,
            "/server/extended-runtime-options/mark-played-items/string", "config-extended.html#extended-runtime-options",
            false, "*", true),
        std::make_shared<ConfigBoolSetup>(ConfigVal::SERVER_EXTOPTS_MARK_PLAYED_ITEMS_SUPPRESS_CDS_UPDATES,
            "/server/extended-runtime-options/mark-played-items/attribute::suppress-cds-updates", "config-extended.html#extended-runtime-options",
            YES),
        std::make_shared<ConfigArraySetup>(ConfigVal::SERVER_EXTOPTS_MARK_PLAYED_ITEMS_CONTENT_LIST,
            "/server/extended-runtime-options/mark-played-items/mark", "config-extended.html#extended-runtime-options",
            ConfigVal::A_SERVER_EXTOPTS_MARK_PLAYED_ITEMS_CONTENT, ConfigArraySetup::InitPlayedItemsMark),
#ifdef HAVE_LASTFMLIB
        std::make_shared<ConfigBoolSetup>(ConfigVal::SERVER_EXTOPTS_LASTFM_ENABLED,
            "/server/extended-runtime-options/lastfm/attribute::enabled", "config-extended.html#lastfm",
            NO),
        std::make_shared<ConfigStringSetup>(ConfigVal::SERVER_EXTOPTS_LASTFM_USERNAME,
            "/server/extended-runtime-options/lastfm/username", "config-extended.html#lastfm",
            false, "lastfmuser", true),
        std::make_shared<ConfigStringSetup>(ConfigVal::SERVER_EXTOPTS_LASTFM_PASSWORD,
            "/server/extended-runtime-options/lastfm/password", "config-extended.html#lastfm",
            false, "lastfmpass", true),
#endif
#ifdef HAVE_CURL
        std::make_shared<ConfigIntSetup>(ConfigVal::URL_REQUEST_CURL_BUFFER_SIZE,
            "/server/online-content/attribute::fetch-buffer-size", "config-online.html#online-content",
            1024 * 1024, CURL_MAX_WRITE_SIZE, ConfigIntSetup::CheckMinValue),
        std::make_shared<ConfigIntSetup>(ConfigVal::URL_REQUEST_CURL_FILL_SIZE,
            "/server/online-content/attribute::fetch-buffer-fill-size", "config-online.html#online-content",
            0, 0, ConfigIntSetup::CheckMinValue),
#endif // HAVE_CURL

        // UPNP control
        std::make_shared<ConfigBoolSetup>(ConfigVal::UPNP_LITERAL_HOST_REDIRECTION,
            "/server/upnp/attribute::literal-host-redirection", "config-server.html#upnp",
            NO),
        std::make_shared<ConfigBoolSetup>(ConfigVal::UPNP_DYNAMIC_DESCRIPTION,
            "/server/upnp/attribute::dynamic-descriptions", "config-server.html#dynamic-descriptions",
            YES),
        std::make_shared<ConfigBoolSetup>(ConfigVal::UPNP_MULTI_VALUES_ENABLED,
            "/server/upnp/attribute::multi-value", "config-server.html#upnp",
            YES),
        std::make_shared<ConfigBoolSetup>(ConfigVal::UPNP_SEARCH_CONTAINER_FLAG,
            "/server/upnp/attribute::searchable-container-flag", "config-server.html#upnp",
            NO),
        std::make_shared<ConfigStringSetup>(ConfigVal::UPNP_SEARCH_SEPARATOR,
            "/server/upnp/attribute::search-result-separator", "config-server.html#upnp",
            " - "),
        std::make_shared<ConfigBoolSetup>(ConfigVal::UPNP_SEARCH_FILENAME,
            "/server/upnp/attribute::search-filename", "config-server.html#upnp",
            NO),
        std::make_shared<ConfigIntSetup>(ConfigVal::UPNP_CAPTION_COUNT,
            "/server/upnp/attribute::caption-info-count", "config-server.html#upnp",
            1, 0, ConfigIntSetup::CheckMinValue),
        std::make_shared<ConfigArraySetup>(ConfigVal::UPNP_SEARCH_ITEM_SEGMENTS,
            "/server/upnp/search-item-result", "config-server.html#upnpf",
            ConfigVal::A_IMPORT_LIBOPTS_AUXDATA_DATA, ConfigVal::A_IMPORT_LIBOPTS_AUXDATA_TAG,
            false, false, upnpSearchSegmentDefaults),
        std::make_shared<ConfigDictionarySetup>(ConfigVal::UPNP_ALBUM_PROPERTIES,
            "/server/upnp/album-properties", "config-server.html#upnp",
            ConfigVal::A_UPNP_PROPERTIES_PROPERTY, ConfigVal::A_UPNP_PROPERTIES_UPNPTAG, ConfigVal::A_UPNP_PROPERTIES_METADATA,
            false, false, true, upnpAlbumPropDefaults),
        std::make_shared<ConfigDictionarySetup>(ConfigVal::UPNP_ARTIST_PROPERTIES,
            "/server/upnp/artist-properties", "config-server.html#upnp",
            ConfigVal::A_UPNP_PROPERTIES_PROPERTY, ConfigVal::A_UPNP_PROPERTIES_UPNPTAG, ConfigVal::A_UPNP_PROPERTIES_METADATA,
            false, false, true, upnpArtistPropDefaults),
        std::make_shared<ConfigDictionarySetup>(ConfigVal::UPNP_GENRE_PROPERTIES,
            "/server/upnp/genre-properties", "config-server.html#upnp",
            ConfigVal::A_UPNP_PROPERTIES_PROPERTY, ConfigVal::A_UPNP_PROPERTIES_UPNPTAG, ConfigVal::A_UPNP_PROPERTIES_METADATA,
            false, false, true, upnpGenrePropDefaults),
        std::make_shared<ConfigDictionarySetup>(ConfigVal::UPNP_PLAYLIST_PROPERTIES,
            "/server/upnp/playlist-properties", "config-server.html#upnp",
            ConfigVal::A_UPNP_PROPERTIES_PROPERTY, ConfigVal::A_UPNP_PROPERTIES_UPNPTAG, ConfigVal::A_UPNP_PROPERTIES_METADATA,
            false, false, true, upnpPlaylistPropDefaults),
        std::make_shared<ConfigDictionarySetup>(ConfigVal::UPNP_TITLE_PROPERTIES,
            "/server/upnp/title-properties", "config-server.html#upnp",
            ConfigVal::A_UPNP_PROPERTIES_PROPERTY, ConfigVal::A_UPNP_PROPERTIES_UPNPTAG, ConfigVal::A_UPNP_PROPERTIES_METADATA,
            false, false, false),
        std::make_shared<ConfigSetup>(ConfigVal::A_UPNP_PROPERTIES_PROPERTY,
            "upnp-property", ""),
        std::make_shared<ConfigSetup>(ConfigVal::A_UPNP_NAMESPACE_PROPERTY,
            "upnp-namespace", ""),

        std::make_shared<ConfigDictionarySetup>(ConfigVal::UPNP_ALBUM_NAMESPACES,
            "/server/upnp/album-properties", "config-server.html#upnp",
            ConfigVal::A_UPNP_NAMESPACE_PROPERTY, ConfigVal::A_UPNP_NAMESPACE_KEY, ConfigVal::A_UPNP_NAMESPACE_URI,
            false, false, false),
        std::make_shared<ConfigDictionarySetup>(ConfigVal::UPNP_ARTIST_NAMESPACES,
            "/server/upnp/artist-properties", "config-server.html#upnp",
            ConfigVal::A_UPNP_NAMESPACE_PROPERTY, ConfigVal::A_UPNP_NAMESPACE_KEY, ConfigVal::A_UPNP_NAMESPACE_URI,
            false, false, false),
        std::make_shared<ConfigDictionarySetup>(ConfigVal::UPNP_GENRE_NAMESPACES,
            "/server/upnp/genre-properties", "config-server.html#upnp",
            ConfigVal::A_UPNP_NAMESPACE_PROPERTY, ConfigVal::A_UPNP_NAMESPACE_KEY, ConfigVal::A_UPNP_NAMESPACE_URI,
            false, false, false),
        std::make_shared<ConfigDictionarySetup>(ConfigVal::UPNP_PLAYLIST_NAMESPACES,
            "/server/upnp/playlist-properties", "config-server.html#upnp",
            ConfigVal::A_UPNP_NAMESPACE_PROPERTY, ConfigVal::A_UPNP_NAMESPACE_KEY, ConfigVal::A_UPNP_NAMESPACE_URI,
            false, false, false),
        std::make_shared<ConfigDictionarySetup>(ConfigVal::UPNP_TITLE_NAMESPACES,
            "/server/upnp/title-properties", "config-server.html#upnp",
            ConfigVal::A_UPNP_NAMESPACE_PROPERTY, ConfigVal::A_UPNP_NAMESPACE_KEY, ConfigVal::A_UPNP_NAMESPACE_URI,
            false, false, false),
        std::make_shared<ConfigStringSetup>(ConfigVal::A_UPNP_NAMESPACE_KEY,
            "attribute::xmlns", "config-server.html#upnp"),
        std::make_shared<ConfigStringSetup>(ConfigVal::A_UPNP_NAMESPACE_URI,
            "attribute::uri", "config-server.html#upnp"),

        std::make_shared<ConfigStringSetup>(ConfigVal::A_UPNP_PROPERTIES_UPNPTAG,
            "attribute::upnp-tag", "config-server.html#upnp"),
        std::make_shared<ConfigStringSetup>(ConfigVal::A_UPNP_PROPERTIES_METADATA,
            "attribute::meta-data", "config-server.html#upnp"),

        std::make_shared<ConfigDictionarySetup>(ConfigVal::UPNP_RESOURCE_PROPERTY_DEFAULTS,
            "/server/upnp/resource-defaults", "config-server.html#upnp",
            ConfigVal::A_UPNP_DEFAULT_PROPERTY_PROPERTY, ConfigVal::A_UPNP_DEFAULT_PROPERTY_TAG, ConfigVal::A_UPNP_DEFAULT_PROPERTY_VALUE,
            false, false, false, upnpResourceDefaultPropDefaults),
        std::make_shared<ConfigDictionarySetup>(ConfigVal::UPNP_OBJECT_PROPERTY_DEFAULTS,
            "/server/upnp/object-defaults", "config-server.html#upnp",
            ConfigVal::A_UPNP_DEFAULT_PROPERTY_PROPERTY, ConfigVal::A_UPNP_DEFAULT_PROPERTY_TAG, ConfigVal::A_UPNP_DEFAULT_PROPERTY_VALUE,
            false, false, false, upnpObjectDefaultPropDefaults),
        std::make_shared<ConfigDictionarySetup>(ConfigVal::UPNP_CONTAINER_PROPERTY_DEFAULTS,
            "/server/upnp/container-defaults", "config-server.html#upnp",
            ConfigVal::A_UPNP_DEFAULT_PROPERTY_PROPERTY, ConfigVal::A_UPNP_DEFAULT_PROPERTY_TAG, ConfigVal::A_UPNP_DEFAULT_PROPERTY_VALUE,
            false, false, false, upnpContainerDefaultPropDefaults),
        std::make_shared<ConfigSetup>(ConfigVal::A_UPNP_DEFAULT_PROPERTY_PROPERTY,
            "property-default", ""),
        std::make_shared<ConfigStringSetup>(ConfigVal::A_UPNP_DEFAULT_PROPERTY_TAG,
            "attribute::tag", "config-server.html#upnp"),
        std::make_shared<ConfigStringSetup>(ConfigVal::A_UPNP_DEFAULT_PROPERTY_VALUE,
            "attribute::value", "config-server.html#upnp"),

        // Dynamic (aka search) containers
        std::make_shared<ConfigDynamicContentSetup>(ConfigVal::SERVER_DYNAMIC_CONTENT_LIST,
            "/server/containers", "config-server.html#containers"),
        std::make_shared<ConfigBoolSetup>(ConfigVal::SERVER_DYNAMIC_CONTENT_LIST_ENABLED,
            "/server/containers/attribute::enabled", "config-server.html#containers",
            true, false),
        std::make_shared<ConfigPathSetup>(ConfigVal::A_DYNAMIC_CONTAINER_LOCATION,
            "attribute::location", "config-server.html#location",
            "/Auto", ConfigPathArguments::isFile | ConfigPathArguments::notEmpty | ConfigPathArguments::resolveEmpty),
        std::make_shared<ConfigPathSetup>(ConfigVal::A_DYNAMIC_CONTAINER_IMAGE,
            "attribute::image", "config-server.html#image",
            "", ConfigPathArguments::isFile | ConfigPathArguments::resolveEmpty),
        std::make_shared<ConfigStringSetup>(ConfigVal::A_DYNAMIC_CONTAINER_TITLE,
            "attribute::title", "config-server.html#title",
            ""),
        std::make_shared<ConfigStringSetup>(ConfigVal::A_DYNAMIC_CONTAINER_FILTER,
            "filter", "config-server.html#filter",
            true, "last_updated > \"@last31\""),
        std::make_shared<ConfigStringSetup>(ConfigVal::A_DYNAMIC_CONTAINER_UPNP_SHORTCUT,
            "attribute::upnp-shortcut", "config-server.html#upnp-shortcut",
            ""),
        std::make_shared<ConfigStringSetup>(ConfigVal::A_DYNAMIC_CONTAINER_SORT,
            "attribute::sort", "config-server.html#sort",
            ""),
        std::make_shared<ConfigIntSetup>(ConfigVal::A_DYNAMIC_CONTAINER_MAXCOUNT,
            "attribute::max-count", "config-server.html#max-count",
            500, 0, ConfigIntSetup::CheckMinValue),
    };
}

std::vector<std::shared_ptr<ConfigSetup>> ConfigDefinition::getClientOptions()
{
    return {
        std::make_shared<ConfigClientSetup>(ConfigVal::CLIENTS_LIST,
            "/clients", "config-clients.html#clients"),
        std::make_shared<ConfigBoolSetup>(ConfigVal::CLIENTS_LIST_ENABLED,
            "/clients/attribute::enabled", "config-clients.html#clients",
            NO),
        std::make_shared<ConfigTimeSetup>(ConfigVal::CLIENTS_CACHE_THRESHOLD,
            "/clients/attribute::cache-threshold", "config-clients.html#clients",
            GrbTimeType::Hours, 6, 1),
        std::make_shared<ConfigTimeSetup>(ConfigVal::CLIENTS_BOOKMARK_OFFSET,
            "/clients/attribute::bookmark-offset", "config-clients.html#clients",
            GrbTimeType::Seconds, 10, 0),

        std::make_shared<ConfigSetup>(ConfigVal::A_CLIENTS_CLIENT,
            "/clients/client", "config-clients.html#clients",
            ""),
        std::make_shared<ConfigIntSetup>(ConfigVal::A_CLIENTS_CLIENT_FLAGS,
            "flags", "config-clients.html#client",
            0, ClientConfig::makeFlags, ClientConfig::mapFlags),
        std::make_shared<ConfigStringSetup>(ConfigVal::A_CLIENTS_CLIENT_IP,
            "ip", "config-clients.html#client",
            ""),
        std::make_shared<ConfigStringSetup>(ConfigVal::A_CLIENTS_CLIENT_GROUP,
            "group", "config-clients.html#client",
            DEFAULT_CLIENT_GROUP),
        std::make_shared<ConfigStringSetup>(ConfigVal::A_CLIENTS_CLIENT_USERAGENT,
            "userAgent", "config-clients.html#client",
            ""),
        std::make_shared<ConfigBoolSetup>(ConfigVal::A_CLIENTS_CLIENT_ALLOWED,
            "allowed", "config-clients.html#client",
            YES),

        std::make_shared<ConfigSetup>(ConfigVal::A_CLIENTS_GROUP,
            "/clients/group", "config-clients.html#group"),
        std::make_shared<ConfigStringSetup>(ConfigVal::A_CLIENTS_GROUP_NAME,
            "name", "config-clients.html#group",
            DEFAULT_CLIENT_GROUP),
        std::make_shared<ConfigArraySetup>(ConfigVal::A_CLIENTS_GROUP_HIDDEN_LIST,
            "/clients/group", "config-clients.html#group",
            ConfigVal::A_CLIENTS_GROUP_HIDE, ConfigVal::A_CLIENTS_GROUP_LOCATION,
            false, false),
        std::make_shared<ConfigSetup>(ConfigVal::A_CLIENTS_GROUP_HIDE,
            "hide", "config-clients.html#group"),
        std::make_shared<ConfigPathSetup>(ConfigVal::A_CLIENTS_GROUP_LOCATION,
            "attribute::location", "config-clients.html#group",
            ""),

        std::make_shared<ConfigDictionarySetup>(ConfigVal::A_CLIENTS_UPNP_MAP_MIMETYPE,
            "/clients/client", "config-import.html#map",
            ConfigVal::A_IMPORT_MAPPINGS_MIMETYPE_MAP,
            ConfigVal::A_IMPORT_MAPPINGS_MIMETYPE_FROM,
            ConfigVal::A_IMPORT_MAPPINGS_MIMETYPE_TO,
            false, false, false),
        std::make_shared<ConfigVectorSetup>(ConfigVal::A_CLIENTS_UPNP_MAP_DLNAPROFILE,
            "/clients/client", "config-import.html#contenttype-dlnaprofile",
            ConfigVal::A_CLIENTS_UPNP_MAP_DLNAPROFILE_PROFILE,
            std::vector<ConfigVal> { ConfigVal::A_IMPORT_MAPPINGS_MIMETYPE_FROM, ConfigVal::A_IMPORT_MAPPINGS_MIMETYPE_TO },
            false, false, true),
        std::make_shared<ConfigIntSetup>(ConfigVal::A_CLIENTS_UPNP_CAPTION_COUNT,
            "attribute::caption-info-count", "config-clients.html#caption-info-count",
            1, 0, ConfigIntSetup::CheckMinValue),
        std::make_shared<ConfigIntSetup>(ConfigVal::A_CLIENTS_UPNP_STRING_LIMIT,
            "attribute::upnp-string-limit", "config-clients.html#upnp-string-limit",
            DEFAULT_UPNP_STRING_LIMIT, CheckUpnpStringLimitValue),
        std::make_shared<ConfigBoolSetup>(ConfigVal::A_CLIENTS_UPNP_MULTI_VALUE,
            "attribute::multi-value", "config-clients.html#multi-value",
            YES),
        std::make_shared<ConfigBoolSetup>(ConfigVal::A_CLIENTS_UPNP_FILTER_FULL,
            "attribute::full-filter", "config-clients.html#full-filter",
            NO),
        std::make_shared<ConfigDictionarySetup>(ConfigVal::A_CLIENTS_UPNP_HEADERS,
            "/clients/client", "config-import.html#headers",
            ConfigVal::A_CLIENTS_UPNP_HEADERS_HEADER,
            ConfigVal::A_CLIENTS_UPNP_HEADERS_KEY,
            ConfigVal::A_CLIENTS_UPNP_HEADERS_VALUE,
            false, false, false),

        std::make_shared<ConfigStringSetup>(ConfigVal::A_CLIENTS_UPNP_HEADERS_KEY,
            "attribute::key", "config-import.html#headers", ""),
        std::make_shared<ConfigStringSetup>(ConfigVal::A_CLIENTS_UPNP_HEADERS_VALUE,
            "attribute::value", "config-import.html#headers", ""),
    };
}

std::vector<std::shared_ptr<ConfigSetup>> ConfigDefinition::getImportOptions()
{
    return {
        // boxlayout provides modification options like translation of tree items
        std::make_shared<ConfigBoxLayoutSetup>(ConfigVal::BOXLAYOUT_BOX,
            "/import/scripting/virtual-layout/boxlayout/box", "config-import.html#boxlayout",
            boxLayoutDefaults),
        std::make_shared<ConfigStringSetup>(ConfigVal::A_BOXLAYOUT_BOX_KEY,
            "attribute::key", "config-import.html#boxlayout",
            ""),
        std::make_shared<ConfigStringSetup>(ConfigVal::A_BOXLAYOUT_BOX_TITLE,
            "attribute::title", "config-import.html#boxlayout",
            ""),
        std::make_shared<ConfigStringSetup>(ConfigVal::A_BOXLAYOUT_BOX_CLASS,
            "attribute::class", "config-import.html#boxlayout",
            UPNP_CLASS_CONTAINER),
        std::make_shared<ConfigStringSetup>(ConfigVal::A_BOXLAYOUT_BOX_UPNP_SHORTCUT,
            "attribute::upnp-shortcut", "config-import.html#boxlayout",
            ""),
        std::make_shared<ConfigIntSetup>(ConfigVal::A_BOXLAYOUT_BOX_SIZE,
            "attribute::size", "config-import.html#boxlayout",
            1, -10, ConfigIntSetup::CheckMinValue),
        std::make_shared<ConfigBoolSetup>(ConfigVal::A_BOXLAYOUT_BOX_ENABLED,
            "attribute::enabled", "config-import.html#boxlayout",
            YES),

        // default file settings, can be overwritten in autoscan
        std::make_shared<ConfigBoolSetup>(ConfigVal::IMPORT_HIDDEN_FILES,
            "/import/attribute::hidden-files", "config-import.html#import",
            NO),
        std::make_shared<ConfigBoolSetup>(ConfigVal::IMPORT_FOLLOW_SYMLINKS,
            "/import/attribute::follow-symlinks", "config-import.html#import",
            DEFAULT_FOLLOW_SYMLINKS_VALUE),
        std::make_shared<ConfigBoolSetup>(ConfigVal::IMPORT_DEFAULT_DATE,
            "/import/attribute::default-date", "config-import.html#import",
            YES),
        std::make_shared<ConfigStringSetup>(ConfigVal::IMPORT_NOMEDIA_FILE,
            "/import/attribute::nomedia-file", "config-import.html#import",
            ".nomedia"),
        std::make_shared<ConfigEnumSetup<ImportMode>>(ConfigVal::IMPORT_LAYOUT_MODE,
            "/import/attribute::import-mode", "config-import.html#import",
            ImportMode::MediaTomb,
            std::map<std::string, ImportMode>({ { "mt", ImportMode::MediaTomb }, { "grb", ImportMode::Gerbera } })),
        std::make_shared<ConfigVectorSetup>(ConfigVal::IMPORT_VIRTUAL_DIRECTORY_KEYS,
            "/import/virtual-directories", "config-import.html#virtual-directories",
            ConfigVal::A_IMPORT_VIRT_DIR_KEY,
            std::vector<ConfigVal> { ConfigVal::A_IMPORT_VIRT_DIR_METADATA },
            false, false, true,
            virtualDirectoryKeys),
        std::make_shared<ConfigStringSetup>(ConfigVal::A_IMPORT_VIRT_DIR_KEY,
            "key", "config-import.html#virtual-directories",
            ""),
        std::make_shared<ConfigStringSetup>(ConfigVal::A_IMPORT_VIRT_DIR_METADATA,
            "metadata", "config-import.html#virtual-directories",
            ""),

        // mappings extension -> mimetype -> upnpclass / contenttype / transfermode -> dlnaprofile
        std::make_shared<ConfigBoolSetup>(ConfigVal::IMPORT_READABLE_NAMES,
            "/import/attribute::readable-names", "config-import.html#import",
            YES),
        std::make_shared<ConfigBoolSetup>(ConfigVal::IMPORT_CASE_SENSITIVE_TAGS,
            "/import/attribute::case-sensitive-tags", "config-import.html#import",
            YES),
        std::make_shared<ConfigDictionarySetup>(ConfigVal::IMPORT_MAPPINGS_EXTENSION_TO_MIMETYPE_LIST,
            "/import/mappings/extension-mimetype", "config-import.html#extension-mimetype",
            ConfigVal::A_IMPORT_MAPPINGS_MIMETYPE_MAP, ConfigVal::A_IMPORT_MAPPINGS_MIMETYPE_FROM, ConfigVal::A_IMPORT_MAPPINGS_MIMETYPE_TO,
            false, false, true, extMtDefaults),
        std::make_shared<ConfigBoolSetup>(ConfigVal::IMPORT_MAPPINGS_IGNORE_UNKNOWN_EXTENSIONS,
            "/import/mappings/extension-mimetype/attribute::ignore-unknown", "config-import.html#extension-mimetype",
            NO),
        std::make_shared<ConfigBoolSetup>(ConfigVal::IMPORT_MAPPINGS_EXTENSION_TO_MIMETYPE_CASE_SENSITIVE,
            "/import/mappings/extension-mimetype/attribute::case-sensitive", "config-import.html#extension-mimetype",
            NO),
        std::make_shared<ConfigDictionarySetup>(ConfigVal::IMPORT_MAPPINGS_MIMETYPE_TO_UPNP_CLASS_LIST,
            "/import/mappings/mimetype-upnpclass", "config-import.html#mimetype-upnpclass",
            ConfigVal::A_IMPORT_MAPPINGS_MIMETYPE_MAP, ConfigVal::A_IMPORT_MAPPINGS_MIMETYPE_FROM, ConfigVal::A_IMPORT_MAPPINGS_MIMETYPE_TO,
            false, false, true, mtUpnpDefaults),
        std::make_shared<ConfigDictionarySetup>(ConfigVal::IMPORT_MAPPINGS_CONTENTTYPE_TO_DLNATRANSFER_LIST,
            "/import/mappings/mimetype-dlnatransfermode", "config-import.html#mimetype-dlnatransfermode",
            ConfigVal::A_IMPORT_MAPPINGS_MIMETYPE_MAP, ConfigVal::A_IMPORT_MAPPINGS_MIMETYPE_FROM, ConfigVal::A_IMPORT_MAPPINGS_MIMETYPE_TO,
            false, false, true, mtTransferDefaults),
        std::make_shared<ConfigDictionarySetup>(ConfigVal::IMPORT_MAPPINGS_MIMETYPE_TO_CONTENTTYPE_LIST,
            "/import/mappings/mimetype-contenttype", "config-import.html#mimetype-contenttype",
            ConfigVal::A_IMPORT_MAPPINGS_M2CTYPE_LIST_TREAT, ConfigVal::A_IMPORT_MAPPINGS_M2CTYPE_LIST_MIMETYPE, ConfigVal::A_IMPORT_MAPPINGS_M2CTYPE_LIST_AS,
            false, false, true, mtCtDefaults),
        std::make_shared<ConfigVectorSetup>(ConfigVal::IMPORT_MAPPINGS_CONTENTTYPE_TO_DLNAPROFILE_LIST,
            "/import/mappings/contenttype-dlnaprofile", "config-import.html#contenttype-dlnaprofile",
            ConfigVal::A_IMPORT_MAPPINGS_MIMETYPE_MAP,
            std::vector<ConfigVal> { ConfigVal::A_IMPORT_MAPPINGS_MIMETYPE_FROM, ConfigVal::A_IMPORT_MAPPINGS_MIMETYPE_TO },
            false, false, true,
            ctDlnaDefaults),
        std::make_shared<ConfigArraySetup>(ConfigVal::IMPORT_MAPPINGS_IGNORED_EXTENSIONS,
            "/import/mappings/ignore-extensions", "config-import.html#ignore-extensions",
            ConfigVal::A_IMPORT_RESOURCES_ADD_FILE, ConfigVal::A_IMPORT_RESOURCES_NAME,
            false, false, ignoreDefaults),
        std::make_shared<ConfigDictionarySetup>(ConfigVal::IMPORT_LAYOUT_MAPPING,
            "/import/layout", "config-import.html#layout",
            ConfigVal::A_IMPORT_LAYOUT_MAPPING_PATH, ConfigVal::A_IMPORT_LAYOUT_MAPPING_FROM, ConfigVal::A_IMPORT_LAYOUT_MAPPING_TO),
        std::make_shared<ConfigBoolSetup>(ConfigVal::IMPORT_LAYOUT_PARENT_PATH,
            "/import/layout/attribute::parent-path", "config-import.html#layout",
            NO),

    // Scripting
#ifdef HAVE_JS
        std::make_shared<ConfigStringSetup>(ConfigVal::IMPORT_SCRIPTING_CHARSET,
            "/import/scripting/attribute::script-charset", "config-import.html#scripting",
            "UTF-8", ConfigStringSetup::CheckCharset),
        std::make_shared<ConfigPathSetup>(ConfigVal::IMPORT_SCRIPTING_COMMON_SCRIPT,
            "/import/scripting/common-script", "config-import.html#common-script",
            "", ConfigPathArguments::isFile | ConfigPathArguments::mustExist),
        std::make_shared<ConfigPathSetup>(ConfigVal::IMPORT_SCRIPTING_CUSTOM_SCRIPT,
            "/import/scripting/custom-script", "config-import.html#custom-script",
            "", ConfigPathArguments::isFile | ConfigPathArguments::mustExist),
        std::make_shared<ConfigPathSetup>(ConfigVal::IMPORT_SCRIPTING_PLAYLIST_SCRIPT,
            "/import/scripting/playlist-script", "config-import.html#playlist-script",
            "", ConfigPathArguments::isFile | ConfigPathArguments::mustExist),
        std::make_shared<ConfigPathSetup>(ConfigVal::IMPORT_SCRIPTING_METAFILE_SCRIPT,
            "/import/scripting/metafile-script", "config-import.html#metafile-script",
            "", ConfigPathArguments::isFile | ConfigPathArguments::mustExist),
        std::make_shared<ConfigBoolSetup>(ConfigVal::IMPORT_SCRIPTING_PLAYLIST_SCRIPT_LINK_OBJECTS,
            "/import/scripting/playlist-script/attribute::create-link", "config-import.html#playlist-script",
            NO),
        std::make_shared<ConfigPathSetup>(ConfigVal::IMPORT_SCRIPTING_IMPORT_SCRIPT,
            "/import/scripting/virtual-layout/import-script", "config-import.html#scripting",
            "", ConfigPathArguments::isFile | ConfigPathArguments::mustExist),
        std::make_shared<ConfigStringSetup>(ConfigVal::IMPORT_SCRIPTING_IMPORT_LAYOUT_AUDIO,
            "/import/scripting/virtual-layout/attribute::audio-layout", "config-import.html#scripting",
            ""),
        std::make_shared<ConfigStringSetup>(ConfigVal::IMPORT_SCRIPTING_IMPORT_LAYOUT_VIDEO,
            "/import/scripting/virtual-layout/attribute::video-layout", "config-import.html#scripting",
            ""),
        std::make_shared<ConfigStringSetup>(ConfigVal::IMPORT_SCRIPTING_IMPORT_LAYOUT_IMAGE,
            "/import/scripting/virtual-layout/attribute::image-layout", "config-import.html#scripting",
            ""),

        std::make_shared<ConfigPathSetup>(ConfigVal::IMPORT_SCRIPTING_COMMON_FOLDER,
            "/import/scripting/script-folder/common", "config-import.html#script-folder",
            "", ConfigPathArguments::resolveEmpty),
        std::make_shared<ConfigPathSetup>(ConfigVal::IMPORT_SCRIPTING_CUSTOM_FOLDER,
            "/import/scripting/script-folder/custom", "config-import.html#script-folder",
            "", ConfigPathArguments::none),
        std::make_shared<ConfigStringSetup>(ConfigVal::IMPORT_SCRIPTING_IMPORT_FUNCTION_PLAYLIST,
            "/import/scripting/import-function/playlist", "config-import.html#import-function",
            "importPlaylist"),
        std::make_shared<ConfigBoolSetup>(ConfigVal::IMPORT_SCRIPTING_PLAYLIST_LINK_OBJECTS,
            "/import/scripting/import-function/playlist/attribute::create-link", "config-import.html#import-function",
            YES),
        std::make_shared<ConfigStringSetup>(ConfigVal::IMPORT_SCRIPTING_IMPORT_FUNCTION_METAFILE,
            "/import/scripting/import-function/meta-file", "config-import.html#import-function",
            "importMetadata"),
        std::make_shared<ConfigStringSetup>(ConfigVal::IMPORT_SCRIPTING_IMPORT_FUNCTION_AUDIOFILE,
            "/import/scripting/import-function/audio-file", "config-import.html#import-function",
            "importAudio"),
        std::make_shared<ConfigStringSetup>(ConfigVal::IMPORT_SCRIPTING_IMPORT_FUNCTION_VIDEOFILE,
            "/import/scripting/import-function/video-file", "config-import.html#import-function",
            "importVideo"),
        std::make_shared<ConfigStringSetup>(ConfigVal::IMPORT_SCRIPTING_IMPORT_FUNCTION_IMAGEFILE,
            "/import/scripting/import-function/image-file", "config-import.html#import-function",
            "importImage"),
#ifdef ONLINE_SERVICES
        std::make_shared<ConfigStringSetup>(ConfigVal::IMPORT_SCRIPTING_IMPORT_FUNCTION_TRAILER,
            "/import/scripting/import-function/online-item", "config-import.html#import-function",
            "importOnlineItem"),
#endif

        std::make_shared<ConfigDictionarySetup>(ConfigVal::IMPORT_SCRIPTING_IMPORT_SCRIPT_OPTIONS,
            "/import/scripting/virtual-layout/script-options", "config-import.html#layout",
            ConfigVal::A_IMPORT_LAYOUT_SCRIPT_OPTION, ConfigVal::A_IMPORT_LAYOUT_SCRIPT_OPTION_NAME, ConfigVal::A_IMPORT_LAYOUT_SCRIPT_OPTION_VALUE),

        std::make_shared<ConfigStringSetup>(ConfigVal::IMPORT_SCRIPTING_STRUCTURED_LAYOUT_SKIPCHARS,
            "/import/scripting/virtual-layout/structured-layout/attribute::skip-chars", "config-import.html#scripting",
            ""),
        std::make_shared<ConfigStringSetup>(ConfigVal::IMPORT_SCRIPTING_STRUCTURED_LAYOUT_DIVCHAR,
            "/import/scripting/virtual-layout/structured-layout/attribute::div-char", "config-import.html#scripting",
            ""),
#endif // HAVE_JS

        // character conversion
        std::make_shared<ConfigDictionarySetup>(ConfigVal::IMPORT_SCRIPTING_IMPORT_GENRE_MAP,
            "/import/scripting/virtual-layout/genre-map", "config-import.html#layout",
            ConfigVal::A_IMPORT_LAYOUT_GENRE, ConfigVal::A_IMPORT_LAYOUT_MAPPING_FROM, ConfigVal::A_IMPORT_LAYOUT_MAPPING_TO),
        std::make_shared<ConfigStringSetup>(ConfigVal::IMPORT_FILESYSTEM_CHARSET,
            "/import/filesystem-charset", "config-import.html#filesystem-charset",
            DEFAULT_FILESYSTEM_CHARSET, ConfigStringSetup::CheckCharset),
        std::make_shared<ConfigStringSetup>(ConfigVal::IMPORT_METADATA_CHARSET,
            "/import/metadata-charset", "config-import.html#metadata-charset",
            DEFAULT_FILESYSTEM_CHARSET, ConfigStringSetup::CheckCharset),
        std::make_shared<ConfigStringSetup>(ConfigVal::IMPORT_PLAYLIST_CHARSET,
            "/import/playlist-charset", "config-import.html#playlist-charset",
            DEFAULT_FILESYSTEM_CHARSET, ConfigStringSetup::CheckCharset),
        std::make_shared<ConfigEnumSetup<LayoutType>>(ConfigVal::IMPORT_SCRIPTING_VIRTUAL_LAYOUT_TYPE,
            "/import/scripting/virtual-layout/attribute::type", "config-import.html#scripting",
#ifdef HAVE_JS
            LayoutType::Js,
#else
            LayoutType::Builtin,
#endif
            std::map<std::string, LayoutType>({ { "js", LayoutType::Js }, { "builtin", LayoutType::Builtin }, { "disabled", LayoutType::Disabled } })),

        // tweaks
        std::make_shared<ConfigStringSetup>(ConfigVal::IMPORT_LIBOPTS_ENTRY_SEP,
            "/import/library-options/attribute::multi-value-separator", "config-import.html#library-options",
            DEFAULT_LIBOPTS_ENTRY_SEPARATOR),
        std::make_shared<ConfigStringSetup>(ConfigVal::IMPORT_LIBOPTS_ENTRY_LEGACY_SEP,
            "/import/library-options/attribute::legacy-value-separator", "config-import.html#library-options",
            ""),

        std::make_shared<ConfigBoolSetup>(ConfigVal::IMPORT_RESOURCES_CASE_SENSITIVE,
            "/import/resources/attribute::case-sensitive", "config-import.html#resources",
            DEFAULT_RESOURCES_CASE_SENSITIVE),
        std::make_shared<ConfigArraySetup>(ConfigVal::IMPORT_RESOURCES_FANART_FILE_LIST,
            "/import/resources/fanart", "config-import.html#resources",
            ConfigVal::A_IMPORT_RESOURCES_ADD_FILE, ConfigVal::A_IMPORT_RESOURCES_NAME,
            false, false, defaultFanArtFile),
        std::make_shared<ConfigVectorSetup>(ConfigVal::IMPORT_RESOURCES_FANART_DIR_LIST,
            "/import/resources/fanart", "config-import.html#resources",
            ConfigVal::A_IMPORT_RESOURCES_ADD_DIR,
            std::vector<ConfigVal> { ConfigVal::A_IMPORT_RESOURCES_NAME, ConfigVal::A_IMPORT_RESOURCES_PTT, ConfigVal::A_IMPORT_RESOURCES_EXT },
            false, false, false),

        std::make_shared<ConfigArraySetup>(ConfigVal::IMPORT_RESOURCES_CONTAINERART_FILE_LIST,
            "/import/resources/container", "config-import.html#container",
            ConfigVal::A_IMPORT_RESOURCES_ADD_FILE, ConfigVal::A_IMPORT_RESOURCES_NAME,
            false, false, defaultContainerArtFile),
        std::make_shared<ConfigVectorSetup>(ConfigVal::IMPORT_RESOURCES_CONTAINERART_DIR_LIST,
            "/import/resources/container", "config-import.html#container",
            ConfigVal::A_IMPORT_RESOURCES_ADD_DIR,
            std::vector<ConfigVal> { ConfigVal::A_IMPORT_RESOURCES_NAME, ConfigVal::A_IMPORT_RESOURCES_PTT, ConfigVal::A_IMPORT_RESOURCES_EXT },
            false, false, false),
        std::make_shared<ConfigPathSetup>(ConfigVal::IMPORT_RESOURCES_CONTAINERART_LOCATION,
            "/import/resources/container/attribute::location", "config-import.html#container",
            ""),
        std::make_shared<ConfigIntSetup>(ConfigVal::IMPORT_RESOURCES_CONTAINERART_PARENTCOUNT,
            "/import/resources/container/attribute::parentCount", "config-import.html#container",
            2, 0, ConfigIntSetup::CheckMinValue),
        std::make_shared<ConfigIntSetup>(ConfigVal::IMPORT_RESOURCES_CONTAINERART_MINDEPTH,
            "/import/resources/container/attribute::minDepth", "config-import.html#container",
            2, 0, ConfigIntSetup::CheckMinValue),

        std::make_shared<ConfigArraySetup>(ConfigVal::IMPORT_RESOURCES_SUBTITLE_FILE_LIST,
            "/import/resources/subtitle", "config-import.html#resources",
            ConfigVal::A_IMPORT_RESOURCES_ADD_FILE, ConfigVal::A_IMPORT_RESOURCES_NAME,
            false, false, defaultSubtitleFile),
        std::make_shared<ConfigVectorSetup>(ConfigVal::IMPORT_RESOURCES_SUBTITLE_DIR_LIST,
            "/import/resources/subtitle", "config-import.html#resources",
            ConfigVal::A_IMPORT_RESOURCES_ADD_DIR,
            std::vector<ConfigVal> { ConfigVal::A_IMPORT_RESOURCES_NAME, ConfigVal::A_IMPORT_RESOURCES_PTT, ConfigVal::A_IMPORT_RESOURCES_EXT },
            false, false, false),

        std::make_shared<ConfigArraySetup>(ConfigVal::IMPORT_RESOURCES_METAFILE_FILE_LIST,
            "/import/resources/metafile", "config-import.html#resources",
            ConfigVal::A_IMPORT_RESOURCES_ADD_FILE, ConfigVal::A_IMPORT_RESOURCES_NAME,
            false, false, defaultMetadataFile),
        std::make_shared<ConfigVectorSetup>(ConfigVal::IMPORT_RESOURCES_METAFILE_DIR_LIST,
            "/import/resources/metafile", "config-import.html#resources",
            ConfigVal::A_IMPORT_RESOURCES_ADD_DIR,
            std::vector<ConfigVal> { ConfigVal::A_IMPORT_RESOURCES_NAME, ConfigVal::A_IMPORT_RESOURCES_PTT, ConfigVal::A_IMPORT_RESOURCES_EXT },
            false, false, false),

        std::make_shared<ConfigArraySetup>(ConfigVal::IMPORT_RESOURCES_RESOURCE_FILE_LIST,
            "/import/resources/resource", "config-import.html#resources",
            ConfigVal::A_IMPORT_RESOURCES_ADD_FILE, ConfigVal::A_IMPORT_RESOURCES_NAME,
            false, false, defaultResourceFile),
        std::make_shared<ConfigVectorSetup>(ConfigVal::IMPORT_RESOURCES_RESOURCE_DIR_LIST,
            "/import/resources/resource", "config-import.html#resources",
            ConfigVal::A_IMPORT_RESOURCES_ADD_DIR,
            std::vector<ConfigVal> { ConfigVal::A_IMPORT_RESOURCES_NAME, ConfigVal::A_IMPORT_RESOURCES_PTT, ConfigVal::A_IMPORT_RESOURCES_EXT },
            false, false, false),

        std::make_shared<ConfigArraySetup>(ConfigVal::IMPORT_RESOURCES_ORDER,
            "/import/resources/order", "config-import.html#resources-order",
            ConfigVal::A_IMPORT_RESOURCES_HANDLER, ConfigVal::A_IMPORT_RESOURCES_NAME,
            EnumMapper::checkContentHandler),

        std::make_shared<ConfigArraySetup>(ConfigVal::IMPORT_SYSTEM_DIRECTORIES,
            "/import/system-directories", "config-import.html#system-directories",
            ConfigVal::A_IMPORT_SYSTEM_DIR_ADD_PATH, ConfigVal::A_IMPORT_RESOURCES_NAME, true, true,
            excludesFullpath),
        std::make_shared<ConfigArraySetup>(ConfigVal::IMPORT_VISIBLE_DIRECTORIES,
            "/import/visible-directories", "config-import.html#visible-directories",
            ConfigVal::A_IMPORT_SYSTEM_DIR_ADD_PATH, ConfigVal::A_IMPORT_RESOURCES_NAME, false, false),

    // Autoscan settings
#ifdef HAVE_INOTIFY
        std::make_shared<ConfigBoolSetup>(ConfigVal::IMPORT_AUTOSCAN_USE_INOTIFY,
            "/import/autoscan/attribute::use-inotify", "config-import.html#autoscan",
            "auto", StringCheckFunction(ConfigBoolSetup::CheckInotifyValue)),
        std::make_shared<ConfigBoolSetup>(ConfigVal::IMPORT_AUTOSCAN_INOTIFY_ATTRIB,
            "/import/autoscan/attribute::inotify-attrib", "config-import.html#autoscan",
            NO),
        std::make_shared<ConfigAutoscanSetup>(ConfigVal::IMPORT_AUTOSCAN_INOTIFY_LIST,
            "/import/autoscan", "config-import.html#autoscan",
            AutoscanScanMode::INotify),
#endif
        std::make_shared<ConfigSetup>(ConfigVal::A_AUTOSCAN_DIRECTORY,
            "directory", "config-import.html#autoscan",
            ""),
        std::make_shared<ConfigAutoscanSetup>(ConfigVal::IMPORT_AUTOSCAN_TIMED_LIST,
            "/import/autoscan", "config-import.html#autoscan",
            AutoscanScanMode::Timed),
        std::make_shared<ConfigPathSetup>(ConfigVal::A_AUTOSCAN_DIRECTORY_LOCATION,
            "attribute::location", "config-import.html#autoscan",
            "", ConfigPathArguments::notEmpty | ConfigPathArguments::resolveEmpty),
        std::make_shared<ConfigEnumSetup<AutoscanScanMode>>(ConfigVal::A_AUTOSCAN_DIRECTORY_MODE,
            "attribute::mode", "config-import.html#autoscan",
            std::map<std::string, AutoscanScanMode>({ { AUTOSCAN_TIMED, AutoscanScanMode::Timed }, { AUTOSCAN_INOTIFY, AutoscanScanMode::INotify } })),
        std::make_shared<ConfigTimeSetup>(ConfigVal::A_AUTOSCAN_DIRECTORY_INTERVAL,
            "attribute::interval", "config-import.html#autoscan",
            GrbTimeType::Seconds, -1, 0),
        std::make_shared<ConfigBoolSetup>(ConfigVal::A_AUTOSCAN_DIRECTORY_RECURSIVE,
            "attribute::recursive", "config-import.html#autoscan",
            false, true),
        std::make_shared<ConfigBoolSetup>(ConfigVal::A_AUTOSCAN_DIRECTORY_DIRTYPES,
            "attribute::dirtypes", "config-import.html#autoscan",
            true, false),
        std::make_shared<ConfigIntSetup>(ConfigVal::A_AUTOSCAN_DIRECTORY_MEDIATYPE,
            "attribute::media-type", "config-import.html#autoscan",
            to_underlying(AutoscanDirectory::MediaType::Any), AutoscanDirectory::makeMediaType),
        std::make_shared<ConfigBoolSetup>(ConfigVal::A_AUTOSCAN_DIRECTORY_HIDDENFILES,
            "attribute::hidden-files", "config-import.html#autoscan"),
        std::make_shared<ConfigBoolSetup>(ConfigVal::A_AUTOSCAN_DIRECTORY_FOLLOWSYMLINKS,
            "attribute::follow-symlinks", "config-import.html#autoscan"),
        std::make_shared<ConfigIntSetup>(ConfigVal::A_AUTOSCAN_DIRECTORY_TASKCOUNT,
            "attribute::task-count", "config-import.html#autoscan"),
        std::make_shared<ConfigIntSetup>(ConfigVal::A_AUTOSCAN_DIRECTORY_SCANCOUNT,
            "attribute::scan-count", "config-import.html#autoscan"),
        std::make_shared<ConfigUIntSetup>(ConfigVal::A_AUTOSCAN_DIRECTORY_RETRYCOUNT,
            "attribute::retry-count", "config-import.html#autoscan",
            0),
        std::make_shared<ConfigStringSetup>(ConfigVal::A_AUTOSCAN_DIRECTORY_LMT,
            "attribute::last-modified", "config-import.html#autoscan"),
        std::make_shared<ConfigBoolSetup>(ConfigVal::A_AUTOSCAN_DIRECTORY_FORCE_REREAD_UNKNOWN,
            "attribute::force-reread-unknown", "config-import.html#autoscan",
            NO),

        // Directory Tweaks
        std::make_shared<ConfigDirectorySetup>(ConfigVal::IMPORT_DIRECTORIES_LIST,
            "/import/directories", "config-import.html#autoscan"),
        std::make_shared<ConfigSetup>(ConfigVal::A_DIRECTORIES_TWEAK,
            "/import/directories/tweak", "config-import.html#autoscan",
            ""),
        std::make_shared<ConfigPathSetup>(ConfigVal::A_DIRECTORIES_TWEAK_LOCATION,
            "attribute::location", "config-import.html#autoscan",
            "", ConfigPathArguments::mustExist | ConfigPathArguments::notEmpty | ConfigPathArguments::resolveEmpty),
        std::make_shared<ConfigBoolSetup>(ConfigVal::A_DIRECTORIES_TWEAK_INHERIT,
            "attribute::inherit", "config-import.html#autoscan",
            false, true),
        std::make_shared<ConfigBoolSetup>(ConfigVal::A_DIRECTORIES_TWEAK_RECURSIVE,
            "attribute::recursive", "config-import.html#autoscan",
            false, true),
        std::make_shared<ConfigBoolSetup>(ConfigVal::A_DIRECTORIES_TWEAK_HIDDEN,
            "attribute::hidden-files", "config-import.html#autoscan"),
        std::make_shared<ConfigBoolSetup>(ConfigVal::A_DIRECTORIES_TWEAK_CASE_SENSITIVE,
            "attribute::case-sensitive", "config-import.html#autoscan",
            DEFAULT_RESOURCES_CASE_SENSITIVE),
        std::make_shared<ConfigBoolSetup>(ConfigVal::A_DIRECTORIES_TWEAK_FOLLOW_SYMLINKS,
            "attribute::follow-symlinks", "config-import.html#autoscan",
            DEFAULT_FOLLOW_SYMLINKS_VALUE),
        std::make_shared<ConfigStringSetup>(ConfigVal::A_DIRECTORIES_TWEAK_META_CHARSET,
            "attribute::meta-charset", "config-import.html#charset",
            "", ConfigStringSetup::CheckCharset),
        std::make_shared<ConfigStringSetup>(ConfigVal::A_DIRECTORIES_TWEAK_FANART_FILE,
            "attribute::fanart-file", "config-import.html#resources",
            ""),
        std::make_shared<ConfigStringSetup>(ConfigVal::A_DIRECTORIES_TWEAK_SUBTITLE_FILE,
            "attribute::subtitle-file", "config-import.html#resources",
            ""),
        std::make_shared<ConfigStringSetup>(ConfigVal::A_DIRECTORIES_TWEAK_METAFILE_FILE,
            "attribute::metadata-file", "config-import.html#resources",
            ""),
        std::make_shared<ConfigStringSetup>(ConfigVal::A_DIRECTORIES_TWEAK_RESOURCE_FILE,
            "attribute::resource-file", "config-import.html#resources",
            ""),

        std::make_shared<ConfigStringSetup>(ConfigVal::A_IMPORT_MAPPINGS_M2CTYPE_LIST_TREAT,
            "treat", "config-import.html#mappings",
            ""),
        std::make_shared<ConfigStringSetup>(ConfigVal::A_IMPORT_MAPPINGS_M2CTYPE_LIST_MIMETYPE,
            "attribute::mimetype", "config-import.html#mappings",
            ""),
        std::make_shared<ConfigStringSetup>(ConfigVal::A_IMPORT_MAPPINGS_M2CTYPE_LIST_AS,
            "attribute::as", "config-import.html#mappings",
            ""),
        std::make_shared<ConfigStringSetup>(ConfigVal::A_IMPORT_MAPPINGS_MIMETYPE_FROM,
            "attribute::from", "config-import.html#mappings",
            ""),
        std::make_shared<ConfigStringSetup>(ConfigVal::A_IMPORT_MAPPINGS_MIMETYPE_TO,
            "attribute::to", "config-import.html#mappings",
            ""),

        std::make_shared<ConfigStringSetup>(ConfigVal::A_IMPORT_RESOURCES_NAME,
            "attribute::name", "config-import.html#resources",
            ""),
        std::make_shared<ConfigStringSetup>(ConfigVal::A_IMPORT_RESOURCES_PTT,
            "attribute::pattern", "config-import.html#resources",
            ""),
        std::make_shared<ConfigStringSetup>(ConfigVal::A_IMPORT_RESOURCES_EXT,
            "attribute::ext", "config-import.html#resources",
            ""),

        std::make_shared<ConfigStringSetup>(ConfigVal::A_IMPORT_LAYOUT_MAPPING_FROM,
            "attribute::from", "config-import.html#layout",
            ""),
        std::make_shared<ConfigStringSetup>(ConfigVal::A_IMPORT_LAYOUT_MAPPING_TO,
            "attribute::to", "config-import.html#layout",
            ""),

        std::make_shared<ConfigStringSetup>(ConfigVal::A_AUTOSCAN_CONTAINER_TYPE_AUDIO,
            "container-type-audio", "config-import.html#autoscan",
            UPNP_CLASS_MUSIC_ALBUM),
        std::make_shared<ConfigStringSetup>(ConfigVal::A_AUTOSCAN_CONTAINER_TYPE_IMAGE,
            "container-type-image", "config-import.html#autoscan",
            UPNP_CLASS_PHOTO_ALBUM),
        std::make_shared<ConfigStringSetup>(ConfigVal::A_AUTOSCAN_CONTAINER_TYPE_VIDEO,
            "container-type-video", "config-import.html#autoscan",
            UPNP_CLASS_CONTAINER),

        std::make_shared<ConfigStringSetup>(ConfigVal::A_IMPORT_LIBOPTS_AUXDATA_TAG,
            "attribute::tag", "config-import.html#auxdata",
            ""),
        std::make_shared<ConfigStringSetup>(ConfigVal::A_IMPORT_LIBOPTS_AUXDATA_KEY,
            "attribute::key", "config-import.html#auxdata",
            ""),
        std::make_shared<ConfigStringSetup>(ConfigVal::A_IMPORT_LIBOPTS_COMMENT_TAG,
            "attribute::tag", "config-import.html#comment",
            ""),
        std::make_shared<ConfigStringSetup>(ConfigVal::A_IMPORT_LIBOPTS_COMMENT_LABEL,
            "attribute::label", "config-import.html#comment",
            ""),
        std::make_shared<ConfigStringSetup>(ConfigVal::A_SERVER_UI_ACCOUNT_LIST_PASSWORD,
            "attribute::password", "config-server.html#ui",
            ""),
        std::make_shared<ConfigStringSetup>(ConfigVal::A_SERVER_UI_ACCOUNT_LIST_USER,
            "attribute::user", "config-server.html#ui",
            ""),
    };
}

std::vector<std::shared_ptr<ConfigSetup>> ConfigDefinition::getLibraryOptions()
{
    return {
#ifdef HAVE_LIBEXIF
        // options for libexif
        std::make_shared<ConfigArraySetup>(ConfigVal::IMPORT_LIBOPTS_EXIF_AUXDATA_TAGS_LIST,
            "/import/library-options/libexif/auxdata", "config-import.html#libexif",
            ConfigVal::A_IMPORT_LIBOPTS_AUXDATA_DATA,
            ConfigVal::A_IMPORT_LIBOPTS_AUXDATA_TAG,
            false, false),
        std::make_shared<ConfigDictionarySetup>(ConfigVal::IMPORT_LIBOPTS_EXIF_METADATA_TAGS_LIST,
            "/import/library-options/libexif/metadata", "config-import.html#libexif",
            ConfigVal::A_IMPORT_LIBOPTS_AUXDATA_DATA,
            ConfigVal::A_IMPORT_LIBOPTS_AUXDATA_TAG,
            ConfigVal::A_IMPORT_LIBOPTS_AUXDATA_KEY,
            false, true, false),
        std::make_shared<ConfigStringSetup>(ConfigVal::IMPORT_LIBOPTS_EXIF_CHARSET,
            "/import/library-options/libexif/attribute::charset", "config-import.html#charset",
            ""),
        std::make_shared<ConfigBoolSetup>(ConfigVal::IMPORT_LIBOPTS_EXIF_ENABLED,
            "/import/library-options/libexif/attribute::enabled", "config-import.html#enabled",
            YES),
        std::make_shared<ConfigBoolSetup>(ConfigVal::IMPORT_LIBOPTS_EXIF_COMMENT_ENABLED,
            "/import/library-options/libexif/comment/attribute::enabled", "config-import.html#comment",
            NO),
        std::make_shared<ConfigDictionarySetup>(ConfigVal::IMPORT_LIBOPTS_EXIF_COMMENT_LIST,
            "/import/library-options/libexif/comment", "config-import.html#libexif",
            ConfigVal::A_IMPORT_LIBOPTS_COMMENT_DATA,
            ConfigVal::A_IMPORT_LIBOPTS_COMMENT_LABEL,
            ConfigVal::A_IMPORT_LIBOPTS_COMMENT_TAG,
            false, false, false),
#endif
#ifdef HAVE_EXIV2
        // options for exiv2
        std::make_shared<ConfigArraySetup>(ConfigVal::IMPORT_LIBOPTS_EXIV2_AUXDATA_TAGS_LIST,
            "/import/library-options/exiv2/auxdata", "config-import.html#exiv2",
            ConfigVal::A_IMPORT_LIBOPTS_AUXDATA_DATA,
            ConfigVal::A_IMPORT_LIBOPTS_AUXDATA_TAG,
            false, false),
        std::make_shared<ConfigDictionarySetup>(ConfigVal::IMPORT_LIBOPTS_EXIV2_METADATA_TAGS_LIST,
            "/import/library-options/exiv2/metadata", "config-import.html#exiv2",
            ConfigVal::A_IMPORT_LIBOPTS_AUXDATA_DATA,
            ConfigVal::A_IMPORT_LIBOPTS_AUXDATA_TAG,
            ConfigVal::A_IMPORT_LIBOPTS_AUXDATA_KEY,
            false, true, false),
        std::make_shared<ConfigStringSetup>(ConfigVal::IMPORT_LIBOPTS_EXIV2_CHARSET,
            "/import/library-options/exiv2/attribute::charset", "config-import.html#charset",
            ""),
        std::make_shared<ConfigBoolSetup>(ConfigVal::IMPORT_LIBOPTS_EXIV2_ENABLED,
            "/import/library-options/exiv2/attribute::enabled", "config-import.html#enabled",
            YES),
        std::make_shared<ConfigBoolSetup>(ConfigVal::IMPORT_LIBOPTS_EXIV2_COMMENT_ENABLED,
            "/import/library-options/exiv2/comment/attribute::enabled", "config-import.html#comment",
            YES),
        std::make_shared<ConfigDictionarySetup>(ConfigVal::IMPORT_LIBOPTS_EXIV2_COMMENT_LIST,
            "/import/library-options/exiv2/comment", "config-import.html#exiv2",
            ConfigVal::A_IMPORT_LIBOPTS_COMMENT_DATA,
            ConfigVal::A_IMPORT_LIBOPTS_COMMENT_LABEL,
            ConfigVal::A_IMPORT_LIBOPTS_COMMENT_TAG,
            false, false, false, exiv2CommentDefaults),
#endif
#ifdef HAVE_TAGLIB
        // options for taglib (id3)
        std::make_shared<ConfigArraySetup>(ConfigVal::IMPORT_LIBOPTS_ID3_AUXDATA_TAGS_LIST,
            "/import/library-options/id3/auxdata", "config-import.html#id3",
            ConfigVal::A_IMPORT_LIBOPTS_AUXDATA_DATA,
            ConfigVal::A_IMPORT_LIBOPTS_AUXDATA_TAG,
            false, false),
        std::make_shared<ConfigDictionarySetup>(ConfigVal::IMPORT_LIBOPTS_ID3_METADATA_TAGS_LIST,
            "/import/library-options/id3/metadata", "config-import.html#id3",
            ConfigVal::A_IMPORT_LIBOPTS_AUXDATA_DATA,
            ConfigVal::A_IMPORT_LIBOPTS_AUXDATA_TAG,
            ConfigVal::A_IMPORT_LIBOPTS_AUXDATA_KEY,
            false, true, false, id3SpecialPropertyMap),
        std::make_shared<ConfigStringSetup>(ConfigVal::IMPORT_LIBOPTS_ID3_CHARSET,
            "/import/library-options/id3/attribute::charset", "config-import.html#charset",
            ""),
        std::make_shared<ConfigBoolSetup>(ConfigVal::IMPORT_LIBOPTS_ID3_ENABLED,
            "/import/library-options/id3/attribute::enabled", "config-import.html#enabled",
            YES),
        std::make_shared<ConfigBoolSetup>(ConfigVal::IMPORT_LIBOPTS_ID3_COMMENT_ENABLED,
            "/import/library-options/id3/comment/attribute::enabled", "config-import.html#comment",
            NO),
        std::make_shared<ConfigDictionarySetup>(ConfigVal::IMPORT_LIBOPTS_ID3_COMMENT_LIST,
            "/import/library-options/id3/comment", "config-import.html#id3",
            ConfigVal::A_IMPORT_LIBOPTS_COMMENT_DATA,
            ConfigVal::A_IMPORT_LIBOPTS_COMMENT_LABEL,
            ConfigVal::A_IMPORT_LIBOPTS_COMMENT_TAG,
            false, false, false),
#endif
#ifdef HAVE_FFMPEG
        // options for ffmpeg (libav)
        std::make_shared<ConfigArraySetup>(ConfigVal::IMPORT_LIBOPTS_FFMPEG_AUXDATA_TAGS_LIST,
            "/import/library-options/ffmpeg/auxdata", "config-import.html#ffmpeg",
            ConfigVal::A_IMPORT_LIBOPTS_AUXDATA_DATA,
            ConfigVal::A_IMPORT_LIBOPTS_AUXDATA_TAG,
            false, false),
        std::make_shared<ConfigDictionarySetup>(ConfigVal::IMPORT_LIBOPTS_FFMPEG_METADATA_TAGS_LIST,
            "/import/library-options/ffmpeg/metadata", "config-import.html#ffmpeg",
            ConfigVal::A_IMPORT_LIBOPTS_AUXDATA_DATA,
            ConfigVal::A_IMPORT_LIBOPTS_AUXDATA_TAG,
            ConfigVal::A_IMPORT_LIBOPTS_AUXDATA_KEY,
            false, true, false, ffmpegSpecialPropertyMap),
        std::make_shared<ConfigStringSetup>(ConfigVal::IMPORT_LIBOPTS_FFMPEG_CHARSET,
            "/import/library-options/ffmpeg/attribute::charset", "config-import.html#charset",
            ""),
        std::make_shared<ConfigBoolSetup>(ConfigVal::IMPORT_LIBOPTS_FFMPEG_ENABLED,
            "/import/library-options/ffmpeg/attribute::enabled", "config-import.html#enabled",
            YES),
        std::make_shared<ConfigBoolSetup>(ConfigVal::IMPORT_LIBOPTS_FFMPEG_COMMENT_ENABLED,
            "/import/library-options/ffmpeg/comment/attribute::enabled", "config-import.html#comment",
            NO),
        std::make_shared<ConfigDictionarySetup>(ConfigVal::IMPORT_LIBOPTS_FFMPEG_COMMENT_LIST,
            "/import/library-options/ffmpeg/comment", "config-import.html#ffmpeg",
            ConfigVal::A_IMPORT_LIBOPTS_COMMENT_DATA,
            ConfigVal::A_IMPORT_LIBOPTS_COMMENT_LABEL,
            ConfigVal::A_IMPORT_LIBOPTS_COMMENT_TAG,
            false, false, false),
#endif
#ifdef HAVE_MATROSKA
        // options for matroska (mkv)
        std::make_shared<ConfigArraySetup>(ConfigVal::IMPORT_LIBOPTS_MKV_AUXDATA_TAGS_LIST,
            "/import/library-options/mkv/auxdata", "config-import.html#mkv",
            ConfigVal::A_IMPORT_LIBOPTS_AUXDATA_DATA,
            ConfigVal::A_IMPORT_LIBOPTS_AUXDATA_TAG,
            false, false),
        std::make_shared<ConfigDictionarySetup>(ConfigVal::IMPORT_LIBOPTS_MKV_METADATA_TAGS_LIST,
            "/import/library-options/mkv/metadata", "config-import.html#mkv",
            ConfigVal::A_IMPORT_LIBOPTS_AUXDATA_DATA,
            ConfigVal::A_IMPORT_LIBOPTS_AUXDATA_TAG,
            ConfigVal::A_IMPORT_LIBOPTS_AUXDATA_KEY,
            false, true, false),
        std::make_shared<ConfigStringSetup>(ConfigVal::IMPORT_LIBOPTS_MKV_CHARSET,
            "/import/library-options/mkv/attribute::charset", "config-import.html#charset",
            ""),
        std::make_shared<ConfigBoolSetup>(ConfigVal::IMPORT_LIBOPTS_MKV_ENABLED,
            "/import/library-options/mkv/attribute::enabled", "config-import.html#enabled",
            YES),
        std::make_shared<ConfigBoolSetup>(ConfigVal::IMPORT_LIBOPTS_MKV_COMMENT_ENABLED,
            "/import/library-options/mkv/comment/attribute::enabled", "config-import.html#comment",
            NO),
        std::make_shared<ConfigDictionarySetup>(ConfigVal::IMPORT_LIBOPTS_MKV_COMMENT_LIST,
            "/import/library-options/mkv/comment", "config-import.html#mkv",
            ConfigVal::A_IMPORT_LIBOPTS_COMMENT_DATA,
            ConfigVal::A_IMPORT_LIBOPTS_COMMENT_LABEL,
            ConfigVal::A_IMPORT_LIBOPTS_COMMENT_TAG,
            false, false, false),
#endif
#ifdef HAVE_WAVPACK
        // options for wavpack
        std::make_shared<ConfigArraySetup>(ConfigVal::IMPORT_LIBOPTS_WAVPACK_AUXDATA_TAGS_LIST,
            "/import/library-options/wavpack/auxdata", "config-import.html#wavpack",
            ConfigVal::A_IMPORT_LIBOPTS_AUXDATA_DATA,
            ConfigVal::A_IMPORT_LIBOPTS_AUXDATA_TAG,
            false, false),
        std::make_shared<ConfigDictionarySetup>(ConfigVal::IMPORT_LIBOPTS_WAVPACK_METADATA_TAGS_LIST,
            "/import/library-options/wavpack/metadata", "config-import.html#wavpack",
            ConfigVal::A_IMPORT_LIBOPTS_AUXDATA_DATA,
            ConfigVal::A_IMPORT_LIBOPTS_AUXDATA_TAG,
            ConfigVal::A_IMPORT_LIBOPTS_AUXDATA_KEY,
            false, true, false),
        std::make_shared<ConfigStringSetup>(ConfigVal::IMPORT_LIBOPTS_WAVPACK_CHARSET,
            "/import/library-options/wavpack/attribute::charset", "config-import.html#charset",
            ""),
        std::make_shared<ConfigBoolSetup>(ConfigVal::IMPORT_LIBOPTS_WAVPACK_ENABLED,
            "/import/library-options/wavpack/attribute::enabled", "config-import.html#enabled",
            YES),
        std::make_shared<ConfigBoolSetup>(ConfigVal::IMPORT_LIBOPTS_WAVPACK_COMMENT_ENABLED,
            "/import/library-options/wavpack/comment/attribute::enabled", "config-import.html#comment",
            NO),
        std::make_shared<ConfigDictionarySetup>(ConfigVal::IMPORT_LIBOPTS_WAVPACK_COMMENT_LIST,
            "/import/library-options/wavpack/comment", "config-import.html#wavpack",
            ConfigVal::A_IMPORT_LIBOPTS_COMMENT_DATA,
            ConfigVal::A_IMPORT_LIBOPTS_COMMENT_LABEL,
            ConfigVal::A_IMPORT_LIBOPTS_COMMENT_TAG,
            false, false, false),
#endif
#ifdef HAVE_MAGIC
        std::make_shared<ConfigPathSetup>(ConfigVal::IMPORT_MAGIC_FILE,
            "/import/magic-file", "config-import.html#magic-file",
            "", ConfigPathArguments::isFile | ConfigPathArguments::mustExist),
#endif
    };
}

std::vector<std::shared_ptr<ConfigSetup>> ConfigDefinition::getTranscodingOptions()
{
    return {
        std::make_shared<ConfigBoolSetup>(ConfigVal::TRANSCODING_TRANSCODING_ENABLED,
            "/transcoding/attribute::enabled", "config-transcode.html#transcoding",
            NO),
        std::make_shared<ConfigTranscodingSetup>(ConfigVal::TRANSCODING_PROFILE_LIST,
            "/transcoding", "config-transcode.html#transcoding"),

#ifdef HAVE_CURL
        std::make_shared<ConfigIntSetup>(ConfigVal::EXTERNAL_TRANSCODING_CURL_BUFFER_SIZE,
            "/transcoding/attribute::fetch-buffer-size", "config-transcode.html#transcoding",
            262144, CURL_MAX_WRITE_SIZE, ConfigIntSetup::CheckMinValue),
        std::make_shared<ConfigIntSetup>(ConfigVal::EXTERNAL_TRANSCODING_CURL_FILL_SIZE,
            "/transcoding/attribute::fetch-buffer-fill-size", "config-transcode.html#transcoding",
            0, 0, ConfigIntSetup::CheckMinValue),
#endif // HAVE_CURL

        // mimetype identification and media filtering
        std::make_shared<ConfigBoolSetup>(ConfigVal::TRANSCODING_MIMETYPE_PROF_MAP_ALLOW_UNUSED,
            "/transcoding/mimetype-profile-mappings/attribute::allow-unused", "config-transcode.html#mimetype-profile-mappings",
            NO),
        std::make_shared<ConfigDictionarySetup>(ConfigVal::A_TRANSCODING_MIMETYPE_PROF_MAP,
            "/transcoding/mimetype-profile-mappings", "config-transcode.html#mimetype-profile-mappings",
            ConfigVal::A_TRANSCODING_MIMETYPE_PROF_MAP_TRANSCODE,
            ConfigVal::A_TRANSCODING_MIMETYPE_PROF_MAP_MIMETYPE,
            ConfigVal::A_TRANSCODING_MIMETYPE_PROF_MAP_USING,
            false, false, false, trMtDefaults),
        std::make_shared<ConfigSetup>(ConfigVal::A_TRANSCODING_MIMETYPE_FILTER,
            "/transcoding/mimetype-profile-mappings/transcode", "config-transcode.html#mimetype-profile-mappings",
            ""),
        std::make_shared<ConfigStringSetup>(ConfigVal::A_TRANSCODING_MIMETYPE_PROF_MAP_USING,
            "attribute::using", "config-transcode.html#mimetype-profile-mappings",
            ""),
        std::make_shared<ConfigUIntSetup>(ConfigVal::A_TRANSCODING_PROFILES_PROFLE_CLIENTFLAGS,
            "attribute::client-flags", "config-transcode.html#mimetype-profile-mappings",
            0, ClientConfig::makeFlags),
        std::make_shared<ConfigStringSetup>(ConfigVal::A_TRANSCODING_PROFILES_PROFLE_SRCDLNA,
            "attribute::source-profile", "config-transcode.html#mimetype-profile-mappings",
            ""),

        // Transcoding Profiles
        std::make_shared<ConfigBoolSetup>(ConfigVal::TRANSCODING_PROFILES_PROFILE_ALLOW_UNUSED,
            "/transcoding/profiles/attribute::allow-unused", "config-transcode.html#profiles",
            NO),
        std::make_shared<ConfigEnumSetup<TranscodingType>>(ConfigVal::A_TRANSCODING_PROFILES_PROFLE_TYPE,
            "attribute::type", "config-transcode.html#profiles",
            std::map<std::string, TranscodingType>({ { "none", TranscodingType::None }, { "external", TranscodingType::External }, /* for the future...{"remote", TranscodingType::Remote}*/ })),
        std::make_shared<ConfigEnumSetup<AviFourccListmode>>(ConfigVal::A_TRANSCODING_PROFILES_PROFLE_AVI4CC_MODE,
            "mode", "config-transcode.html#profiles",
            std::map<std::string, AviFourccListmode>({ { "ignore", AviFourccListmode::Ignore }, { "process", AviFourccListmode::Process }, { "disabled", AviFourccListmode::None } })),
        std::make_shared<ConfigStringSetup>(ConfigVal::A_TRANSCODING_PROFILES_PROFLE_AVI4CC_4CC,
            "fourcc", "config-transcode.html#profiles",
            ""),
        std::make_shared<ConfigStringSetup>(ConfigVal::A_TRANSCODING_PROFILES_PROFLE_DLNAPROF,
            "attribute::dlna-profile", "config-transcode.html#profiles",
            ""),
        std::make_shared<ConfigStringSetup>(ConfigVal::A_TRANSCODING_PROFILES_PROFLE_NOTRANSCODING,
            "attribute::no-transcoding", "config-transcode.html#profiles",
            ""),
        std::make_shared<ConfigBoolSetup>(ConfigVal::A_TRANSCODING_PROFILES_PROFLE_ENABLED,
            "attribute::enabled", "config-transcode.html#profiles"),
        std::make_shared<ConfigBoolSetup>(ConfigVal::A_TRANSCODING_PROFILES_PROFLE_ACCURL,
            "accept-url", "config-transcode.html#profiles"),
        std::make_shared<ConfigBoolSetup>(ConfigVal::A_TRANSCODING_PROFILES_PROFLE_HIDEORIG,
            "hide-original-resource", "config-transcode.html#profiles"),
        std::make_shared<ConfigBoolSetup>(ConfigVal::A_TRANSCODING_PROFILES_PROFLE_THUMB,
            "thumbnail", "config-transcode.html#profiles"),
        std::make_shared<ConfigBoolSetup>(ConfigVal::A_TRANSCODING_PROFILES_PROFLE_FIRST,
            "first-resource", "config-transcode.html#profiles"),
        std::make_shared<ConfigBoolSetup>(ConfigVal::A_TRANSCODING_PROFILES_PROFLE_ACCOGG,
            "accept-ogg-theora", "config-transcode.html#profiles"),
        std::make_shared<ConfigArraySetup>(ConfigVal::A_TRANSCODING_PROFILES_PROFLE_AVI4CC,
            "avi-fourcc-list", "config-transcode.html#profiles",
            ConfigVal::A_TRANSCODING_PROFILES_PROFLE_AVI4CC_4CC, ConfigVal::MAX,
            true, true),
        std::make_shared<ConfigIntSetup>(ConfigVal::A_TRANSCODING_PROFILES_PROFLE_SAMPFREQ,
            "sample-frequency", "config-transcode.html#profiles",
            "-1", CheckProfileNumberValue),
        std::make_shared<ConfigIntSetup>(ConfigVal::A_TRANSCODING_PROFILES_PROFLE_NRCHAN,
            "audio-channels", "config-transcode.html#profiles",
            "-1", CheckProfileNumberValue),
        std::make_shared<ConfigSetup>(ConfigVal::A_TRANSCODING_PROFILES_PROFLE,
            "/transcoding/profiles/profile", "config-transcode.html#profiles",
            ""),
        std::make_shared<ConfigStringSetup>(ConfigVal::A_TRANSCODING_PROFILES_PROFLE_NAME,
            "attribute::name", "config-transcode.html#profiles",
            true, "", true),
        std::make_shared<ConfigStringSetup>(ConfigVal::A_TRANSCODING_PROFILES_PROFLE_MIMETYPE,
            "mimetype", "config-transcode.html#profiles",
            true, "", true),

        // Buffer
        std::make_shared<ConfigIntSetup>(ConfigVal::A_TRANSCODING_PROFILES_PROFLE_BUFFER_SIZE,
            "attribute::size", "config-transcode.html#profiles",
            0, 0, ConfigIntSetup::CheckMinValue),
        std::make_shared<ConfigIntSetup>(ConfigVal::A_TRANSCODING_PROFILES_PROFLE_BUFFER_CHUNK,
            "attribute::chunk-size", "config-transcode.html#profiles",
            0, 0, ConfigIntSetup::CheckMinValue),
        std::make_shared<ConfigIntSetup>(ConfigVal::A_TRANSCODING_PROFILES_PROFLE_BUFFER_FILL,
            "attribute::fill-size", "config-transcode.html#profiles",
            0, 0, ConfigIntSetup::CheckMinValue),

        // Agent
        std::make_shared<ConfigPathSetup>(ConfigVal::A_TRANSCODING_PROFILES_PROFLE_AGENT_COMMAND,
            "attribute::command", "config-transcode.html#profiles",
            "",
            ConfigPathArguments::isFile | ConfigPathArguments::mustExist | ConfigPathArguments::notEmpty | ConfigPathArguments::isExe | ConfigPathArguments::resolveEmpty),
        std::make_shared<ConfigStringSetup>(ConfigVal::A_TRANSCODING_PROFILES_PROFLE_AGENT_ARGS,
            "attribute::arguments", "config-transcode.html#profiles",
            true, "", true),
        std::make_shared<ConfigDictionarySetup>(ConfigVal::A_TRANSCODING_PROFILES_PROFLE_AGENT_ENVIRON,
            "agent", "config-transcode.html#profiles",
            ConfigVal::A_TRANSCODING_PROFILES_PROFLE_AGENT_ENVIRON_KEY,
            ConfigVal::A_TRANSCODING_PROFILES_PROFLE_AGENT_ENVIRON_NAME,
            ConfigVal::A_TRANSCODING_PROFILES_PROFLE_AGENT_ENVIRON_VALUE,
            false, false, false),
        std::make_shared<ConfigStringSetup>(ConfigVal::A_TRANSCODING_PROFILES_PROFLE_RESOLUTION,
            "resolution", "config-transcode.html#profiles",
            false),
        std::make_shared<ConfigSetup>(ConfigVal::A_TRANSCODING_PROFILES_PROFLE_AGENT,
            "agent", "config-transcode.html#profiles",
            true),
        std::make_shared<ConfigStringSetup>(ConfigVal::A_TRANSCODING_PROFILES_PROFLE_BUFFER,
            "buffer", "config-transcode.html#profiles",
            true),
        std::make_shared<ConfigStringSetup>(ConfigVal::A_TRANSCODING_MIMETYPE_PROF_MAP_MIMETYPE,
            "attribute::mimetype", "config-transcode.html#profiles",
            ""),
        std::make_shared<ConfigStringSetup>(ConfigVal::A_TRANSCODING_PROFILES_PROFLE_MIMETYPE_VALUE,
            "attribute::value", "config-transcode.html#profiles",
            ""),
        std::make_shared<ConfigSetup>(ConfigVal::A_TRANSCODING_PROFILES_PROFLE_MIMETYPE_PROPERTIES,
            "mime-property", "config-transcode.html#profiles",
            ""),
        std::make_shared<ConfigStringSetup>(ConfigVal::A_TRANSCODING_PROFILES_PROFLE_MIMETYPE_PROPERTIES_KEY,
            "attribute::key", "config-transcode.html#profiles",
            ""),
        std::make_shared<ConfigEnumSetup<ResourceAttribute>>(ConfigVal::A_TRANSCODING_PROFILES_PROFLE_MIMETYPE_PROPERTIES_RESOURCE,
            "attribute::resource", "config-transcode.html#profiles",
            ResourceAttribute::MAX,
            EnumMapper::mapAttributeName, EnumMapper::getAttributeName),
        std::make_shared<ConfigEnumSetup<MetadataFields>>(ConfigVal::A_TRANSCODING_PROFILES_PROFLE_MIMETYPE_PROPERTIES_METADATA,
            "attribute::metadata", "config-transcode.html#profiles",
            MetadataFields::M_MAX,
            MetaEnumMapper::remapMetaDataField, MetaEnumMapper::getMetaFieldName),
    };
}

std::vector<std::shared_ptr<ConfigSetup>> ConfigDefinition::getSimpleOptions()
{
    return {
        std::make_shared<ConfigSetup>(ConfigVal::A_TRANSCODING_PROFILES_PROFLE_AGENT_ENVIRON_KEY,
            "environ", ""),
        std::make_shared<ConfigSetup>(ConfigVal::A_TRANSCODING_PROFILES_PROFLE_AGENT_ENVIRON_NAME,
            "name", ""),
        std::make_shared<ConfigStringSetup>(ConfigVal::A_TRANSCODING_PROFILES_PROFLE_AGENT_ENVIRON_VALUE,
            "value", ""),
        std::make_shared<ConfigSetup>(ConfigVal::A_SERVER_UI_ITEMS_PER_PAGE_DROPDOWN_OPTION,
            "option", ""),
        std::make_shared<ConfigSetup>(ConfigVal::A_SERVER_EXTOPTS_MARK_PLAYED_ITEMS_CONTENT,
            "content", ""),
        std::make_shared<ConfigSetup>(ConfigVal::A_SERVER_UI_ACCOUNT_LIST_ACCOUNT,
            "account", ""),
        std::make_shared<ConfigSetup>(ConfigVal::A_IMPORT_MAPPINGS_MIMETYPE_MAP,
            "map", ""),
        std::make_shared<ConfigSetup>(ConfigVal::A_IMPORT_RESOURCES_ADD_FILE,
            "add-file", ""),
        std::make_shared<ConfigSetup>(ConfigVal::A_IMPORT_RESOURCES_ADD_DIR,
            "add-dir", ""),
        std::make_shared<ConfigSetup>(ConfigVal::A_IMPORT_LIBOPTS_AUXDATA_DATA,
            "add-data", ""),
        std::make_shared<ConfigSetup>(ConfigVal::A_IMPORT_LIBOPTS_COMMENT_DATA,
            "add-comment", ""),
        std::make_shared<ConfigSetup>(ConfigVal::A_TRANSCODING_MIMETYPE_PROF_MAP_TRANSCODE,
            "transcode", ""),
        std::make_shared<ConfigSetup>(ConfigVal::A_IMPORT_LAYOUT_MAPPING_PATH,
            "path", ""),
        std::make_shared<ConfigSetup>(ConfigVal::A_IMPORT_LAYOUT_SCRIPT_OPTION,
            "script-option", ""),
        std::make_shared<ConfigStringSetup>(ConfigVal::A_IMPORT_LAYOUT_SCRIPT_OPTION_NAME,
            "name", ""),
        std::make_shared<ConfigStringSetup>(ConfigVal::A_IMPORT_LAYOUT_SCRIPT_OPTION_VALUE,
            "value", ""),
        std::make_shared<ConfigSetup>(ConfigVal::A_IMPORT_LAYOUT_GENRE,
            "genre", ""),
        std::make_shared<ConfigSetup>(ConfigVal::A_IMPORT_SYSTEM_DIR_ADD_PATH,
            "add-path", ""),
        std::make_shared<ConfigSetup>(ConfigVal::A_DYNAMIC_CONTAINER,
            "container", ""),
        std::make_shared<ConfigSetup>(ConfigVal::A_IMPORT_RESOURCES_HANDLER,
            "handler", ""),
        std::make_shared<ConfigSetup>(ConfigVal::A_CLIENTS_UPNP_HEADERS_HEADER,
            "header", ""),
        std::make_shared<ConfigSetup>(ConfigVal::A_CLIENTS_UPNP_MAP_DLNAPROFILE_PROFILE,
            "dlna", ""),
        std::make_shared<ConfigBoolSetup>(ConfigVal::A_LIST_EXTEND,
            "attribute::extend", "config-overview.html#extend",
            NO),
    };
}

void ConfigDefinition::initOptions(const std::shared_ptr<ConfigDefinition>& self)
{
    auto serverOptions = getServerOptions();
    auto clientOptions = getClientOptions();
    auto importOptions = getImportOptions();
    auto libraryOptions = getLibraryOptions();
    auto transcodingOptions = getTranscodingOptions();
    auto simpleOptions = getSimpleOptions();

    for (auto list : { serverOptions, clientOptions, importOptions, libraryOptions, transcodingOptions, simpleOptions }) {
        complexOptions.reserve(complexOptions.size() + list.size());
        complexOptions.insert(complexOptions.end(), list.begin(), list.end());
    }

    for (auto&& option : complexOptions) {
        option->setDefinition(self);
    }
}

/// \brief define parent options for path search
void ConfigDefinition::initHierarchy()
{
    parentOptions = {
        { ConfigVal::A_TRANSCODING_PROFILES_PROFLE_ENABLED, { ConfigVal::TRANSCODING_PROFILE_LIST } },
        { ConfigVal::A_TRANSCODING_PROFILES_PROFLE_CLIENTFLAGS, { ConfigVal::TRANSCODING_PROFILE_LIST } },
        { ConfigVal::A_TRANSCODING_PROFILES_PROFLE_ACCURL, { ConfigVal::TRANSCODING_PROFILE_LIST } },
        { ConfigVal::A_TRANSCODING_PROFILES_PROFLE_TYPE, { ConfigVal::TRANSCODING_PROFILE_LIST } },

        { ConfigVal::A_CLIENTS_UPNP_FILTER_FULL, { ConfigVal::A_CLIENTS_CLIENT, ConfigVal::CLIENTS_LIST } },
        { ConfigVal::A_CLIENTS_UPNP_MULTI_VALUE, { ConfigVal::A_CLIENTS_CLIENT, ConfigVal::CLIENTS_LIST } },

        { ConfigVal::A_UPNP_PROPERTIES_PROPERTY, { ConfigVal::UPNP_ALBUM_PROPERTIES, ConfigVal::UPNP_GENRE_PROPERTIES, ConfigVal::UPNP_ARTIST_PROPERTIES, ConfigVal::UPNP_TITLE_PROPERTIES, ConfigVal::UPNP_PLAYLIST_PROPERTIES } },
        { ConfigVal::A_UPNP_PROPERTIES_UPNPTAG, { ConfigVal::UPNP_ALBUM_PROPERTIES, ConfigVal::UPNP_GENRE_PROPERTIES, ConfigVal::UPNP_ARTIST_PROPERTIES, ConfigVal::UPNP_TITLE_PROPERTIES, ConfigVal::UPNP_PLAYLIST_PROPERTIES } },
        { ConfigVal::A_UPNP_PROPERTIES_METADATA, { ConfigVal::UPNP_ALBUM_PROPERTIES, ConfigVal::UPNP_GENRE_PROPERTIES, ConfigVal::UPNP_ARTIST_PROPERTIES, ConfigVal::UPNP_TITLE_PROPERTIES, ConfigVal::UPNP_PLAYLIST_PROPERTIES } },

        { ConfigVal::A_UPNP_DEFAULT_PROPERTY_PROPERTY, { ConfigVal::UPNP_RESOURCE_PROPERTY_DEFAULTS, ConfigVal::UPNP_OBJECT_PROPERTY_DEFAULTS, ConfigVal::UPNP_CONTAINER_PROPERTY_DEFAULTS } },
        { ConfigVal::A_UPNP_DEFAULT_PROPERTY_TAG, { ConfigVal::UPNP_RESOURCE_PROPERTY_DEFAULTS, ConfigVal::UPNP_OBJECT_PROPERTY_DEFAULTS, ConfigVal::UPNP_CONTAINER_PROPERTY_DEFAULTS } },
        { ConfigVal::A_UPNP_DEFAULT_PROPERTY_VALUE, { ConfigVal::UPNP_RESOURCE_PROPERTY_DEFAULTS, ConfigVal::UPNP_OBJECT_PROPERTY_DEFAULTS, ConfigVal::UPNP_CONTAINER_PROPERTY_DEFAULTS } },

        { ConfigVal::A_UPNP_NAMESPACE_PROPERTY, { ConfigVal::UPNP_ALBUM_NAMESPACES, ConfigVal::UPNP_GENRE_NAMESPACES, ConfigVal::UPNP_ARTIST_NAMESPACES, ConfigVal::UPNP_TITLE_NAMESPACES, ConfigVal::UPNP_PLAYLIST_NAMESPACES } },
        { ConfigVal::A_UPNP_NAMESPACE_KEY, { ConfigVal::UPNP_ALBUM_NAMESPACES, ConfigVal::UPNP_GENRE_NAMESPACES, ConfigVal::UPNP_ARTIST_NAMESPACES, ConfigVal::UPNP_TITLE_NAMESPACES, ConfigVal::UPNP_PLAYLIST_NAMESPACES } },
        { ConfigVal::A_UPNP_NAMESPACE_URI, { ConfigVal::UPNP_ALBUM_NAMESPACES, ConfigVal::UPNP_GENRE_NAMESPACES, ConfigVal::UPNP_ARTIST_NAMESPACES, ConfigVal::UPNP_TITLE_NAMESPACES, ConfigVal::UPNP_PLAYLIST_NAMESPACES } },

        { ConfigVal::A_AUTOSCAN_DIRECTORY_LOCATION, { ConfigVal::IMPORT_AUTOSCAN_TIMED_LIST,
#ifdef HAVE_INOTIFY
                                                        ConfigVal::IMPORT_AUTOSCAN_INOTIFY_LIST
#endif
                                                    } },
        { ConfigVal::A_AUTOSCAN_DIRECTORY_MODE, { ConfigVal::IMPORT_AUTOSCAN_TIMED_LIST,
#ifdef HAVE_INOTIFY
                                                    ConfigVal::IMPORT_AUTOSCAN_INOTIFY_LIST
#endif
                                                } },
        { ConfigVal::A_AUTOSCAN_DIRECTORY_RECURSIVE, { ConfigVal::IMPORT_AUTOSCAN_TIMED_LIST,
#ifdef HAVE_INOTIFY
                                                         ConfigVal::IMPORT_AUTOSCAN_INOTIFY_LIST
#endif
                                                     } },
        { ConfigVal::A_AUTOSCAN_DIRECTORY_MEDIATYPE, { ConfigVal::IMPORT_AUTOSCAN_TIMED_LIST,
#ifdef HAVE_INOTIFY
                                                         ConfigVal::IMPORT_AUTOSCAN_INOTIFY_LIST
#endif
                                                     } },
        { ConfigVal::A_AUTOSCAN_DIRECTORY_HIDDENFILES, { ConfigVal::IMPORT_AUTOSCAN_TIMED_LIST,
#ifdef HAVE_INOTIFY
                                                           ConfigVal::IMPORT_AUTOSCAN_INOTIFY_LIST
#endif
                                                       } },
        { ConfigVal::A_AUTOSCAN_DIRECTORY_FOLLOWSYMLINKS, { ConfigVal::IMPORT_AUTOSCAN_TIMED_LIST,
#ifdef HAVE_INOTIFY
                                                              ConfigVal::IMPORT_AUTOSCAN_INOTIFY_LIST
#endif
                                                          } },
        { ConfigVal::A_AUTOSCAN_DIRECTORY_SCANCOUNT, { ConfigVal::IMPORT_AUTOSCAN_TIMED_LIST,
#ifdef HAVE_INOTIFY
                                                         ConfigVal::IMPORT_AUTOSCAN_INOTIFY_LIST
#endif
                                                     } },
        { ConfigVal::A_AUTOSCAN_DIRECTORY_RETRYCOUNT, { ConfigVal::IMPORT_AUTOSCAN_TIMED_LIST,
#ifdef HAVE_INOTIFY
                                                          ConfigVal::IMPORT_AUTOSCAN_INOTIFY_LIST
#endif
                                                      } },
        { ConfigVal::A_AUTOSCAN_DIRECTORY_LMT, { ConfigVal::IMPORT_AUTOSCAN_TIMED_LIST,
#ifdef HAVE_INOTIFY
                                                   ConfigVal::IMPORT_AUTOSCAN_INOTIFY_LIST
#endif
                                               } },

        { ConfigVal::A_DIRECTORIES_TWEAK_LOCATION, { ConfigVal::IMPORT_DIRECTORIES_LIST } },
        { ConfigVal::A_DIRECTORIES_TWEAK_RECURSIVE, { ConfigVal::IMPORT_DIRECTORIES_LIST } },
        { ConfigVal::A_DIRECTORIES_TWEAK_HIDDEN, { ConfigVal::IMPORT_DIRECTORIES_LIST } },
        { ConfigVal::A_DIRECTORIES_TWEAK_CASE_SENSITIVE, { ConfigVal::IMPORT_DIRECTORIES_LIST } },
        { ConfigVal::A_DIRECTORIES_TWEAK_FOLLOW_SYMLINKS, { ConfigVal::IMPORT_DIRECTORIES_LIST } },

        { ConfigVal::A_DYNAMIC_CONTAINER, { ConfigVal::SERVER_DYNAMIC_CONTENT_LIST } },
        { ConfigVal::A_DYNAMIC_CONTAINER_LOCATION, { ConfigVal::SERVER_DYNAMIC_CONTENT_LIST } },
        { ConfigVal::A_DYNAMIC_CONTAINER_IMAGE, { ConfigVal::SERVER_DYNAMIC_CONTENT_LIST } },
        { ConfigVal::A_DYNAMIC_CONTAINER_TITLE, { ConfigVal::SERVER_DYNAMIC_CONTENT_LIST } },
        { ConfigVal::A_DYNAMIC_CONTAINER_FILTER, { ConfigVal::SERVER_DYNAMIC_CONTENT_LIST } },
        { ConfigVal::A_DYNAMIC_CONTAINER_SORT, { ConfigVal::SERVER_DYNAMIC_CONTENT_LIST } },

        { ConfigVal::A_TRANSCODING_MIMETYPE_PROF_MAP_MIMETYPE, {} },

        { ConfigVal::A_IMPORT_MAPPINGS_M2CTYPE_LIST_MIMETYPE, { ConfigVal::IMPORT_MAPPINGS_MIMETYPE_TO_CONTENTTYPE_LIST, ConfigVal::IMPORT_MAPPINGS_CONTENTTYPE_TO_DLNAPROFILE_LIST } },
        { ConfigVal::A_IMPORT_MAPPINGS_M2CTYPE_LIST_TREAT, { ConfigVal::IMPORT_MAPPINGS_MIMETYPE_TO_CONTENTTYPE_LIST, ConfigVal::IMPORT_MAPPINGS_CONTENTTYPE_TO_DLNAPROFILE_LIST } },
        { ConfigVal::A_IMPORT_MAPPINGS_M2CTYPE_LIST_AS, { ConfigVal::IMPORT_MAPPINGS_MIMETYPE_TO_CONTENTTYPE_LIST, ConfigVal::IMPORT_MAPPINGS_CONTENTTYPE_TO_DLNAPROFILE_LIST } },
        { ConfigVal::A_IMPORT_LIBOPTS_COMMENT_LABEL, {
#ifdef HAVE_LIBEXIF
                                                         ConfigVal::IMPORT_LIBOPTS_EXIF_COMMENT_LIST,
#endif
#ifdef HAVE_EXIV2
                                                         ConfigVal::IMPORT_LIBOPTS_EXIV2_COMMENT_LIST,
#endif
#ifdef HAVE_TAGLIB
                                                         ConfigVal::IMPORT_LIBOPTS_ID3_COMMENT_LIST,
#endif
#ifdef HAVE_FFMPEG
                                                         ConfigVal::IMPORT_LIBOPTS_FFMPEG_COMMENT_LIST,
#endif
#ifdef HAVE_MATROSKA
                                                         ConfigVal::IMPORT_LIBOPTS_MKV_COMMENT_LIST,
#endif
#ifdef HAVE_WAVPACK
                                                         ConfigVal::IMPORT_LIBOPTS_WAVPACK_COMMENT_LIST,
#endif
                                                     } },
        { ConfigVal::A_IMPORT_LIBOPTS_COMMENT_TAG, {
#ifdef HAVE_LIBEXIF
                                                       ConfigVal::IMPORT_LIBOPTS_EXIF_COMMENT_LIST,
#endif
#ifdef HAVE_EXIV2
                                                       ConfigVal::IMPORT_LIBOPTS_EXIV2_COMMENT_LIST,
#endif
#ifdef HAVE_TAGLIB
                                                       ConfigVal::IMPORT_LIBOPTS_ID3_COMMENT_LIST,
#endif
#ifdef HAVE_FFMPEG
                                                       ConfigVal::IMPORT_LIBOPTS_FFMPEG_COMMENT_LIST,
#endif
#ifdef HAVE_MATROSKA
                                                       ConfigVal::IMPORT_LIBOPTS_MKV_COMMENT_LIST,
#endif
#ifdef HAVE_WAVPACK
                                                       ConfigVal::IMPORT_LIBOPTS_WAVPACK_COMMENT_LIST,
#endif
                                                   } },
        { ConfigVal::A_IMPORT_LIBOPTS_AUXDATA_TAG, {
#ifdef HAVE_LIBEXIF
                                                       ConfigVal::IMPORT_LIBOPTS_EXIF_METADATA_TAGS_LIST,
                                                       ConfigVal::IMPORT_LIBOPTS_EXIF_AUXDATA_TAGS_LIST,
#endif
#ifdef HAVE_EXIV2
                                                       ConfigVal::IMPORT_LIBOPTS_EXIV2_METADATA_TAGS_LIST,
                                                       ConfigVal::IMPORT_LIBOPTS_EXIV2_AUXDATA_TAGS_LIST,
#endif
#ifdef HAVE_TAGLIB
                                                       ConfigVal::IMPORT_LIBOPTS_ID3_METADATA_TAGS_LIST,
                                                       ConfigVal::IMPORT_LIBOPTS_ID3_AUXDATA_TAGS_LIST,
#endif
#ifdef HAVE_FFMPEG
                                                       ConfigVal::IMPORT_LIBOPTS_FFMPEG_METADATA_TAGS_LIST,
                                                       ConfigVal::IMPORT_LIBOPTS_FFMPEG_AUXDATA_TAGS_LIST,
#endif
#ifdef HAVE_MATROSKA
                                                       ConfigVal::IMPORT_LIBOPTS_MKV_METADATA_TAGS_LIST,
                                                       ConfigVal::IMPORT_LIBOPTS_MKV_AUXDATA_TAGS_LIST,
#endif
#ifdef HAVE_WAVPACK
                                                       ConfigVal::IMPORT_LIBOPTS_WAVPACK_METADATA_TAGS_LIST,
                                                       ConfigVal::IMPORT_LIBOPTS_WAVPACK_AUXDATA_TAGS_LIST,
#endif
                                                   } },
        { ConfigVal::A_IMPORT_LIBOPTS_AUXDATA_KEY, {
#ifdef HAVE_LIBEXIF
                                                       ConfigVal::IMPORT_LIBOPTS_EXIF_METADATA_TAGS_LIST,
#endif
#ifdef HAVE_EXIV2
                                                       ConfigVal::IMPORT_LIBOPTS_EXIV2_METADATA_TAGS_LIST,
#endif
#ifdef HAVE_TAGLIB
                                                       ConfigVal::IMPORT_LIBOPTS_ID3_METADATA_TAGS_LIST,
#endif
#ifdef HAVE_FFMPEG
                                                       ConfigVal::IMPORT_LIBOPTS_FFMPEG_METADATA_TAGS_LIST,
#endif
#ifdef HAVE_MATROSKA
                                                       ConfigVal::IMPORT_LIBOPTS_MKV_METADATA_TAGS_LIST,
#endif
#ifdef HAVE_WAVPACK
                                                       ConfigVal::IMPORT_LIBOPTS_WAVPACK_METADATA_TAGS_LIST,
#endif
                                                   } },
        { ConfigVal::A_IMPORT_MAPPINGS_MIMETYPE_FROM, {
                                                          ConfigVal::IMPORT_MAPPINGS_EXTENSION_TO_MIMETYPE_LIST,
                                                          ConfigVal::IMPORT_MAPPINGS_MIMETYPE_TO_CONTENTTYPE_LIST,
                                                          ConfigVal::IMPORT_MAPPINGS_MIMETYPE_TO_UPNP_CLASS_LIST,
                                                          ConfigVal::IMPORT_MAPPINGS_CONTENTTYPE_TO_DLNATRANSFER_LIST,
                                                          ConfigVal::IMPORT_MAPPINGS_CONTENTTYPE_TO_DLNAPROFILE_LIST,
                                                          ConfigVal::SERVER_UI_EXTENSION_MIMETYPE_MAPPING,
                                                          ConfigVal::A_CLIENTS_UPNP_MAP_DLNAPROFILE,
                                                      } },
        { ConfigVal::A_IMPORT_MAPPINGS_MIMETYPE_TO, {
                                                        ConfigVal::IMPORT_MAPPINGS_EXTENSION_TO_MIMETYPE_LIST,
                                                        ConfigVal::IMPORT_MAPPINGS_MIMETYPE_TO_CONTENTTYPE_LIST,
                                                        ConfigVal::IMPORT_MAPPINGS_MIMETYPE_TO_UPNP_CLASS_LIST,
                                                        ConfigVal::IMPORT_MAPPINGS_CONTENTTYPE_TO_DLNATRANSFER_LIST,
                                                        ConfigVal::IMPORT_MAPPINGS_CONTENTTYPE_TO_DLNAPROFILE_LIST,
                                                        ConfigVal::SERVER_UI_EXTENSION_MIMETYPE_MAPPING,
                                                        ConfigVal::A_CLIENTS_UPNP_MAP_DLNAPROFILE,
                                                    } },

        { ConfigVal::A_IMPORT_RESOURCES_NAME, { ConfigVal::IMPORT_RESOURCES_FANART_FILE_LIST, ConfigVal::IMPORT_RESOURCES_CONTAINERART_FILE_LIST, //
                                                  ConfigVal::IMPORT_RESOURCES_RESOURCE_FILE_LIST, ConfigVal::IMPORT_RESOURCES_SUBTITLE_FILE_LIST, ConfigVal::IMPORT_RESOURCES_METAFILE_FILE_LIST, //
                                                  ConfigVal::IMPORT_RESOURCES_FANART_DIR_LIST, ConfigVal::IMPORT_RESOURCES_CONTAINERART_DIR_LIST, ConfigVal::IMPORT_RESOURCES_RESOURCE_DIR_LIST, //
                                                  ConfigVal::IMPORT_RESOURCES_SUBTITLE_DIR_LIST, ConfigVal::IMPORT_RESOURCES_METAFILE_DIR_LIST, //
                                                  ConfigVal::IMPORT_SYSTEM_DIRECTORIES, ConfigVal::IMPORT_RESOURCES_ORDER } },
        { ConfigVal::A_IMPORT_RESOURCES_EXT, { ConfigVal::IMPORT_RESOURCES_FANART_DIR_LIST, ConfigVal::IMPORT_RESOURCES_CONTAINERART_DIR_LIST, //
                                                 ConfigVal::IMPORT_RESOURCES_RESOURCE_DIR_LIST, ConfigVal::IMPORT_RESOURCES_SUBTITLE_DIR_LIST, ConfigVal::IMPORT_RESOURCES_METAFILE_DIR_LIST } },
        { ConfigVal::A_IMPORT_RESOURCES_PTT, { ConfigVal::IMPORT_RESOURCES_FANART_DIR_LIST, ConfigVal::IMPORT_RESOURCES_CONTAINERART_DIR_LIST, //
                                                 ConfigVal::IMPORT_RESOURCES_RESOURCE_DIR_LIST, ConfigVal::IMPORT_RESOURCES_SUBTITLE_DIR_LIST, ConfigVal::IMPORT_RESOURCES_METAFILE_DIR_LIST } },

        { ConfigVal::A_IMPORT_LAYOUT_MAPPING_FROM, { ConfigVal::IMPORT_LAYOUT_MAPPING, ConfigVal::IMPORT_SCRIPTING_IMPORT_GENRE_MAP } },
        { ConfigVal::A_IMPORT_LAYOUT_MAPPING_TO, { ConfigVal::IMPORT_LAYOUT_MAPPING, ConfigVal::IMPORT_SCRIPTING_IMPORT_GENRE_MAP } },
#ifdef HAVE_JS
        { ConfigVal::A_IMPORT_LAYOUT_SCRIPT_OPTION_NAME, { ConfigVal::IMPORT_SCRIPTING_IMPORT_SCRIPT_OPTIONS } },
        { ConfigVal::A_IMPORT_LAYOUT_SCRIPT_OPTION_VALUE, { ConfigVal::IMPORT_SCRIPTING_IMPORT_SCRIPT_OPTIONS } },
#endif
    };
}

/// \brief define option dependencies for automatic loading
void ConfigDefinition::initDependencies()
{
    dependencyMap = {
        { ConfigVal::SERVER_STORAGE_SQLITE_DATABASE_FILE, ConfigVal::SERVER_STORAGE_SQLITE_ENABLED },
        { ConfigVal::SERVER_STORAGE_SQLITE_SYNCHRONOUS, ConfigVal::SERVER_STORAGE_SQLITE_ENABLED },
        { ConfigVal::SERVER_STORAGE_SQLITE_JOURNALMODE, ConfigVal::SERVER_STORAGE_SQLITE_ENABLED },
        { ConfigVal::SERVER_STORAGE_SQLITE_RESTORE, ConfigVal::SERVER_STORAGE_SQLITE_ENABLED },
        { ConfigVal::SERVER_STORAGE_SQLITE_BACKUP_ENABLED, ConfigVal::SERVER_STORAGE_SQLITE_ENABLED },
        { ConfigVal::SERVER_STORAGE_SQLITE_BACKUP_INTERVAL, ConfigVal::SERVER_STORAGE_SQLITE_ENABLED },
        { ConfigVal::SERVER_STORAGE_SQLITE_INIT_SQL_FILE, ConfigVal::SERVER_STORAGE_SQLITE_ENABLED },
        { ConfigVal::SERVER_STORAGE_SQLITE_UPGRADE_FILE, ConfigVal::SERVER_STORAGE_SQLITE_ENABLED },
#ifdef HAVE_MYSQL
        { ConfigVal::SERVER_STORAGE_MYSQL_HOST, ConfigVal::SERVER_STORAGE_MYSQL },
        { ConfigVal::SERVER_STORAGE_MYSQL_DATABASE, ConfigVal::SERVER_STORAGE_MYSQL },
        { ConfigVal::SERVER_STORAGE_MYSQL_USERNAME, ConfigVal::SERVER_STORAGE_MYSQL },
        { ConfigVal::SERVER_STORAGE_MYSQL_PORT, ConfigVal::SERVER_STORAGE_MYSQL },
        { ConfigVal::SERVER_STORAGE_MYSQL_SOCKET, ConfigVal::SERVER_STORAGE_MYSQL },
        { ConfigVal::SERVER_STORAGE_MYSQL_PASSWORD, ConfigVal::SERVER_STORAGE_MYSQL },
        { ConfigVal::SERVER_STORAGE_MYSQL_INIT_SQL_FILE, ConfigVal::SERVER_STORAGE_MYSQL },
        { ConfigVal::SERVER_STORAGE_MYSQL_UPGRADE_FILE, ConfigVal::SERVER_STORAGE_MYSQL },
#endif
#ifdef HAVE_CURL
        { ConfigVal::EXTERNAL_TRANSCODING_CURL_BUFFER_SIZE, ConfigVal::TRANSCODING_TRANSCODING_ENABLED },
        { ConfigVal::EXTERNAL_TRANSCODING_CURL_FILL_SIZE, ConfigVal::TRANSCODING_TRANSCODING_ENABLED },
#endif
#ifdef HAVE_FFMPEGTHUMBNAILER
        { ConfigVal::SERVER_EXTOPTS_FFMPEGTHUMBNAILER_THUMBSIZE, ConfigVal::SERVER_EXTOPTS_FFMPEGTHUMBNAILER_ENABLED },
        { ConfigVal::SERVER_EXTOPTS_FFMPEGTHUMBNAILER_SEEK_PERCENTAGE, ConfigVal::SERVER_EXTOPTS_FFMPEGTHUMBNAILER_ENABLED },
        { ConfigVal::SERVER_EXTOPTS_FFMPEGTHUMBNAILER_FILMSTRIP_OVERLAY, ConfigVal::SERVER_EXTOPTS_FFMPEGTHUMBNAILER_ENABLED },
        { ConfigVal::SERVER_EXTOPTS_FFMPEGTHUMBNAILER_IMAGE_QUALITY, ConfigVal::SERVER_EXTOPTS_FFMPEGTHUMBNAILER_ENABLED },
        { ConfigVal::SERVER_EXTOPTS_FFMPEGTHUMBNAILER_CACHE_DIR_ENABLED, ConfigVal::SERVER_EXTOPTS_FFMPEGTHUMBNAILER_ENABLED },
        { ConfigVal::SERVER_EXTOPTS_FFMPEGTHUMBNAILER_CACHE_DIR, ConfigVal::SERVER_EXTOPTS_FFMPEGTHUMBNAILER_ENABLED },
#endif
#ifdef HAVE_LASTFMLIB
        { ConfigVal::SERVER_EXTOPTS_LASTFM_USERNAME, ConfigVal::SERVER_EXTOPTS_LASTFM_ENABLED },
        { ConfigVal::SERVER_EXTOPTS_LASTFM_PASSWORD, ConfigVal::SERVER_EXTOPTS_LASTFM_ENABLED },
#endif
    };
}

const char* ConfigDefinition::mapConfigOption(ConfigVal option) const
{
    auto co = std::find_if(complexOptions.begin(), complexOptions.end(), [=](auto&& c) { return c->option == option; });
    if (co != complexOptions.end()) {
        return (*co)->xpath;
    }
    return "";
}

bool ConfigDefinition::isDependent(ConfigVal option) const
{
    return (dependencyMap.find(option) != dependencyMap.end());
}

std::shared_ptr<ConfigSetup> ConfigDefinition::findConfigSetup(ConfigVal option, bool save) const
{
    auto co = std::find_if(complexOptions.begin(), complexOptions.end(), [=](auto&& s) { return s->option == option; });
    if (co != complexOptions.end()) {
        log_debug("Config: option found: '{}'", (*co)->xpath);
        return *co;
    }

    if (save)
        return nullptr;

    throw_std_runtime_error("Error in config code: {} tag not found. ConfigVal::MAX={}", option, ConfigVal::MAX);
}

std::shared_ptr<ConfigSetup> ConfigDefinition::findConfigSetupByPath(
    const std::string& key,
    bool save,
    const std::shared_ptr<ConfigSetup>& parent) const
{
    auto co = std::find_if(complexOptions.begin(), complexOptions.end(), [&](auto&& s) { return s->getUniquePath() == key; });
    if (co != complexOptions.end()) {
        log_debug("Config: option found: '{}'", (*co)->xpath);
        return *co;
    }

    if (parent) {
        auto attrKey = key.substr(parent->getUniquePath().length());
        if (attrKey.find_first_of(']') != std::string::npos) {
            attrKey = attrKey.substr(attrKey.find_first_of(']') + 1);
        }
        if (attrKey.find("attribute::") != std::string::npos) {
            attrKey = attrKey.substr(attrKey.find("attribute::") + 11);
        }

        co = std::find_if(complexOptions.begin(), complexOptions.end(), [&](auto&& s) {
            return s->getUniquePath() == attrKey //
                && (parentOptions.find(s->option) == parentOptions.end() //
                    || parentOptions.at(s->option).end() != std::find(parentOptions.at(s->option).begin(), parentOptions.at(s->option).end(), s->option));
        });
        if (co != complexOptions.end()) {
            log_debug("Config: attribute option found: '{}'", (*co)->xpath);
            return *co;
        }
    }

    if (save) {
        co = std::find_if(complexOptions.begin(), complexOptions.end(),
            [&](auto&& s) {
                auto uPath = s->getUniquePath();
                auto len = std::min(uPath.length(), key.length());
                return key.substr(0, len) == uPath.substr(0, len);
            });
        return (co != complexOptions.end()) ? *co : nullptr;
    }

    throw_std_runtime_error("Error in config code: {} tag not found", key);
}
