/*GRB*

    Gerbera - https://gerbera.io/

    config_val.h - this file is part of Gerbera.

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

    $Id$
*/

#ifndef __CONFIG_VAL_H__
#define __CONFIG_VAL_H__

#include "util/enum_iterator.h"

#include <upnpconfig.h>

#define CFG_MIN ConfigVal::SERVER_PORT
enum class ConfigVal {
    SERVER_PORT = 0,
    SERVER_IP,
    SERVER_NETWORK_INTERFACE,
    SERVER_NAME,
    SERVER_MANUFACTURER,
    SERVER_MANUFACTURER_URL,
    SERVER_MODEL_NAME,
    SERVER_MODEL_DESCRIPTION,
    SERVER_MODEL_NUMBER,
    SERVER_MODEL_URL,
    SERVER_SERIAL_NUMBER,
    SERVER_PRESENTATION_URL,
    SERVER_APPEND_PRESENTATION_URL_TO,
    SERVER_UDN,
    SERVER_HOME,
    SERVER_HOME_OVERRIDE,
    SERVER_TMPDIR,
    SERVER_WEBROOT,
    SERVER_ALIVE_INTERVAL,
    SERVER_HIDE_PC_DIRECTORY,
    SERVER_BOOKMARK_FILE,
    SERVER_UPNP_TITLE_AND_DESC_STRING_LIMIT,
    SERVER_UI_ENABLED,
    SERVER_UI_POLL_INTERVAL,
    SERVER_UI_POLL_WHEN_IDLE,
    SERVER_UI_ENABLE_NUMBERING,
    SERVER_UI_ENABLE_THUMBNAIL,
    SERVER_UI_ENABLE_VIDEO,
    SERVER_UI_ACCOUNTS_ENABLED,
    SERVER_UI_ACCOUNT_LIST,
    SERVER_UI_SESSION_TIMEOUT,
    SERVER_UI_DEFAULT_ITEMS_PER_PAGE,
    SERVER_UI_ITEMS_PER_PAGE_DROPDOWN,
    SERVER_UI_SHOW_TOOLTIPS,
    SERVER_UI_CONTENT_SECURITY_POLICY,
    SERVER_UI_EXTENSION_MIMETYPE_MAPPING,
    SERVER_UI_EXTENSION_MIMETYPE_DEFAULT,
    SERVER_UI_DOCUMENTATION_SOURCE,
    SERVER_UI_DOCUMENTATION_USER,
    SERVER_STORAGE,
    SERVER_STORAGE_MYSQL,
    SERVER_STORAGE_SQLITE,
    SERVER_STORAGE_DRIVER,
    SERVER_STORAGE_USE_TRANSACTIONS,
    SERVER_STORAGE_SQLITE_ENABLED,
    SERVER_STORAGE_SQLITE_DATABASE_FILE,
    SERVER_STORAGE_SQLITE_SYNCHRONOUS,
    SERVER_STORAGE_SQLITE_JOURNALMODE,
    SERVER_STORAGE_SQLITE_RESTORE,
    SERVER_STORAGE_SQLITE_BACKUP_ENABLED,
    SERVER_STORAGE_SQLITE_BACKUP_INTERVAL,
    SERVER_STORAGE_SQLITE_INIT_SQL_FILE,
    SERVER_STORAGE_SQLITE_UPGRADE_FILE,
    SERVER_STORAGE_MYSQL_ENABLED,
#ifdef HAVE_MYSQL
    SERVER_STORAGE_MYSQL_HOST,
    SERVER_STORAGE_MYSQL_PORT,
    SERVER_STORAGE_MYSQL_USERNAME,
    SERVER_STORAGE_MYSQL_SOCKET,
    SERVER_STORAGE_MYSQL_PASSWORD,
    SERVER_STORAGE_MYSQL_DATABASE,
    SERVER_STORAGE_MYSQL_INIT_SQL_FILE,
    SERVER_STORAGE_MYSQL_UPGRADE_FILE,
#endif
#ifdef HAVE_FFMPEGTHUMBNAILER
    SERVER_EXTOPTS_FFMPEGTHUMBNAILER_ENABLED,
    SERVER_EXTOPTS_FFMPEGTHUMBNAILER_VIDEO_ENABLED,
    SERVER_EXTOPTS_FFMPEGTHUMBNAILER_IMAGE_ENABLED,
    SERVER_EXTOPTS_FFMPEGTHUMBNAILER_THUMBSIZE,
    SERVER_EXTOPTS_FFMPEGTHUMBNAILER_SEEK_PERCENTAGE,
    SERVER_EXTOPTS_FFMPEGTHUMBNAILER_FILMSTRIP_OVERLAY,
    SERVER_EXTOPTS_FFMPEGTHUMBNAILER_IMAGE_QUALITY,
    SERVER_EXTOPTS_FFMPEGTHUMBNAILER_CACHE_DIR_ENABLED,
    SERVER_EXTOPTS_FFMPEGTHUMBNAILER_CACHE_DIR,
#endif
    SERVER_EXTOPTS_MARK_PLAYED_ITEMS_ENABLED,
    SERVER_EXTOPTS_MARK_PLAYED_ITEMS_STRING_MODE_PREPEND,
    SERVER_EXTOPTS_MARK_PLAYED_ITEMS_STRING,
    SERVER_EXTOPTS_MARK_PLAYED_ITEMS_SUPPRESS_CDS_UPDATES,
    SERVER_EXTOPTS_MARK_PLAYED_ITEMS_CONTENT_LIST,
#ifdef HAVE_LASTFMLIB
    SERVER_EXTOPTS_LASTFM_ENABLED,
    SERVER_EXTOPTS_LASTFM_USERNAME,
    SERVER_EXTOPTS_LASTFM_PASSWORD,
#endif
#ifdef UPNP_HAVE_TOOLS
    SERVER_UPNP_MAXJOBS,
#endif
    IMPORT_HIDDEN_FILES,
    IMPORT_FOLLOW_SYMLINKS,
    IMPORT_DEFAULT_DATE,
    IMPORT_LAYOUT_MODE,
    IMPORT_NOMEDIA_FILE,
    IMPORT_VIRTUAL_DIRECTORY_KEYS,
    IMPORT_FILESYSTEM_CHARSET,
    IMPORT_METADATA_CHARSET,
    IMPORT_PLAYLIST_CHARSET,
#ifdef HAVE_JS
    IMPORT_SCRIPTING_CHARSET,
    IMPORT_SCRIPTING_COMMON_SCRIPT,
    IMPORT_SCRIPTING_CUSTOM_SCRIPT,
    IMPORT_SCRIPTING_METAFILE_SCRIPT,
    IMPORT_SCRIPTING_PLAYLIST_SCRIPT,
    IMPORT_SCRIPTING_PLAYLIST_SCRIPT_LINK_OBJECTS,
    IMPORT_SCRIPTING_IMPORT_SCRIPT,
    IMPORT_SCRIPTING_IMPORT_SCRIPT_OPTIONS,
    IMPORT_SCRIPTING_IMPORT_LAYOUT_AUDIO,
    IMPORT_SCRIPTING_IMPORT_LAYOUT_VIDEO,
    IMPORT_SCRIPTING_IMPORT_LAYOUT_IMAGE,

    IMPORT_SCRIPTING_COMMON_FOLDER,
    IMPORT_SCRIPTING_CUSTOM_FOLDER,
    IMPORT_SCRIPTING_IMPORT_FUNCTION_PLAYLIST,
    IMPORT_SCRIPTING_PLAYLIST_LINK_OBJECTS,
    IMPORT_SCRIPTING_IMPORT_FUNCTION_METAFILE,
    IMPORT_SCRIPTING_IMPORT_FUNCTION_AUDIOFILE,
    IMPORT_SCRIPTING_IMPORT_FUNCTION_VIDEOFILE,
    IMPORT_SCRIPTING_IMPORT_FUNCTION_IMAGEFILE,
#ifdef ONLINE_SERVICES
    IMPORT_SCRIPTING_IMPORT_FUNCTION_TRAILER,
#endif

    IMPORT_SCRIPTING_STRUCTURED_LAYOUT_SKIPCHARS,
    IMPORT_SCRIPTING_STRUCTURED_LAYOUT_DIVCHAR,
#endif // HAVE_JS
    IMPORT_SCRIPTING_VIRTUAL_LAYOUT_TYPE,
    IMPORT_SCRIPTING_IMPORT_GENRE_MAP,
#ifdef HAVE_MAGIC
    IMPORT_MAGIC_FILE,
#endif
    IMPORT_AUTOSCAN_TIMED_LIST,
#ifdef HAVE_INOTIFY
    IMPORT_AUTOSCAN_USE_INOTIFY,
    IMPORT_AUTOSCAN_INOTIFY_ATTRIB,
    IMPORT_AUTOSCAN_INOTIFY_LIST,
#endif
    IMPORT_MAPPINGS_IGNORE_UNKNOWN_EXTENSIONS,
    IMPORT_MAPPINGS_EXTENSION_TO_MIMETYPE_CASE_SENSITIVE,
    IMPORT_MAPPINGS_EXTENSION_TO_MIMETYPE_LIST,
    IMPORT_MAPPINGS_MIMETYPE_TO_UPNP_CLASS_LIST,
    IMPORT_MAPPINGS_CONTENTTYPE_TO_DLNATRANSFER_LIST,
    IMPORT_MAPPINGS_MIMETYPE_TO_CONTENTTYPE_LIST,
    IMPORT_MAPPINGS_CONTENTTYPE_TO_DLNAPROFILE_LIST,
    IMPORT_MAPPINGS_IGNORED_EXTENSIONS,
#ifdef HAVE_LIBEXIF
    IMPORT_LIBOPTS_EXIF_AUXDATA_TAGS_LIST,
    IMPORT_LIBOPTS_EXIF_METADATA_TAGS_LIST,
    IMPORT_LIBOPTS_EXIF_CHARSET,
    IMPORT_LIBOPTS_EXIF_ENABLED,
#endif
#ifdef HAVE_EXIV2
    IMPORT_LIBOPTS_EXIV2_AUXDATA_TAGS_LIST,
    IMPORT_LIBOPTS_EXIV2_METADATA_TAGS_LIST,
    IMPORT_LIBOPTS_EXIV2_CHARSET,
    IMPORT_LIBOPTS_EXIV2_ENABLED,
#endif
#ifdef HAVE_TAGLIB
    IMPORT_LIBOPTS_ID3_AUXDATA_TAGS_LIST,
    IMPORT_LIBOPTS_ID3_METADATA_TAGS_LIST,
    IMPORT_LIBOPTS_ID3_CHARSET,
    IMPORT_LIBOPTS_ID3_ENABLED,
#endif
    TRANSCODING_TRANSCODING_ENABLED,
    TRANSCODING_PROFILE_LIST,
#ifdef HAVE_CURL
    EXTERNAL_TRANSCODING_CURL_BUFFER_SIZE,
    EXTERNAL_TRANSCODING_CURL_FILL_SIZE,
#endif
#ifdef HAVE_CURL
    URL_REQUEST_CURL_BUFFER_SIZE,
    URL_REQUEST_CURL_FILL_SIZE,
#endif
#ifdef HAVE_FFMPEG
    IMPORT_LIBOPTS_FFMPEG_AUXDATA_TAGS_LIST,
    IMPORT_LIBOPTS_FFMPEG_METADATA_TAGS_LIST,
    IMPORT_LIBOPTS_FFMPEG_CHARSET,
    IMPORT_LIBOPTS_FFMPEG_ENABLED,
#endif
#ifdef HAVE_MATROSKA
    IMPORT_LIBOPTS_MKV_CHARSET,
    IMPORT_LIBOPTS_MKV_ENABLED,
#endif
#ifdef HAVE_WAVPACK
    IMPORT_LIBOPTS_WVC_CHARSET,
    IMPORT_LIBOPTS_WVC_ENABLED,
#endif
    CLIENTS_LIST,
    CLIENTS_LIST_ENABLED,
    CLIENTS_CACHE_THRESHOLD,
    CLIENTS_BOOKMARK_OFFSET,
    BOXLAYOUT_BOX,
    IMPORT_LAYOUT_PARENT_PATH,
    IMPORT_LAYOUT_MAPPING,
    IMPORT_LIBOPTS_ENTRY_SEP,
    IMPORT_LIBOPTS_ENTRY_LEGACY_SEP,
    IMPORT_DIRECTORIES_LIST,
    IMPORT_RESOURCES_CASE_SENSITIVE,
    IMPORT_RESOURCES_FANART_FILE_LIST,
    IMPORT_RESOURCES_SUBTITLE_FILE_LIST,
    IMPORT_RESOURCES_METAFILE_FILE_LIST,
    IMPORT_RESOURCES_RESOURCE_FILE_LIST,
    IMPORT_RESOURCES_CONTAINERART_FILE_LIST,
    IMPORT_RESOURCES_CONTAINERART_LOCATION,
    IMPORT_RESOURCES_CONTAINERART_PARENTCOUNT,
    IMPORT_RESOURCES_CONTAINERART_MINDEPTH,
    IMPORT_RESOURCES_FANART_DIR_LIST,
    IMPORT_RESOURCES_SUBTITLE_DIR_LIST,
    IMPORT_RESOURCES_METAFILE_DIR_LIST,
    IMPORT_RESOURCES_RESOURCE_DIR_LIST,
    IMPORT_RESOURCES_CONTAINERART_DIR_LIST,
    TRANSCODING_MIMETYPE_PROF_MAP_ALLOW_UNUSED,
    TRANSCODING_PROFILES_PROFILE_ALLOW_UNUSED,
    VIRTUAL_URL,
    EXTERNAL_URL,
    IMPORT_SYSTEM_DIRECTORIES,
    IMPORT_VISIBLE_DIRECTORIES,
    UPNP_LITERAL_HOST_REDIRECTION,
    UPNP_MULTI_VALUES_ENABLED,
    UPNP_DYNAMIC_DESCRIPTION,
    UPNP_SEARCH_SEPARATOR,
    UPNP_SEARCH_FILENAME,
    UPNP_SEARCH_ITEM_SEGMENTS,
    UPNP_SEARCH_CONTAINER_FLAG,
    UPNP_ALBUM_PROPERTIES,
    UPNP_ARTIST_PROPERTIES,
    UPNP_GENRE_PROPERTIES,
    UPNP_PLAYLIST_PROPERTIES,
    UPNP_TITLE_PROPERTIES,
    UPNP_ALBUM_NAMESPACES,
    UPNP_ARTIST_NAMESPACES,
    UPNP_GENRE_NAMESPACES,
    UPNP_PLAYLIST_NAMESPACES,
    UPNP_TITLE_NAMESPACES,
    UPNP_CAPTION_COUNT,
    IMPORT_READABLE_NAMES,
    IMPORT_CASE_SENSITIVE_TAGS,
    SERVER_DYNAMIC_CONTENT_LIST_ENABLED,
    SERVER_DYNAMIC_CONTENT_LIST,
    IMPORT_RESOURCES_ORDER,
#ifdef GRBDEBUG
    SERVER_LOG_DEBUG_MODE,
#endif
    SERVER_LOG_ROTATE_SIZE,
    SERVER_LOG_ROTATE_COUNT,

    MAX,

    // only attributes are allowed beyond MAX
    A_SERVER_EXTOPTS_MARK_PLAYED_ITEMS_CONTENT,
    A_SERVER_UI_ITEMS_PER_PAGE_DROPDOWN_OPTION,
    A_SERVER_UI_ACCOUNT_LIST_ACCOUNT,
    A_SERVER_UI_ACCOUNT_LIST_USER,
    A_SERVER_UI_ACCOUNT_LIST_PASSWORD,
    A_IMPORT_MAPPINGS_MIMETYPE_MAP,
    A_IMPORT_MAPPINGS_MIMETYPE_FROM,
    A_IMPORT_MAPPINGS_MIMETYPE_TO,
    A_IMPORT_MAPPINGS_M2CTYPE_LIST_TREAT,
    A_IMPORT_MAPPINGS_M2CTYPE_LIST_MIMETYPE,
    A_IMPORT_MAPPINGS_M2CTYPE_LIST_AS,
    A_IMPORT_LAYOUT_GENRE,
    A_IMPORT_LAYOUT_SCRIPT_OPTION,
    A_IMPORT_LAYOUT_SCRIPT_OPTION_NAME,
    A_IMPORT_LAYOUT_SCRIPT_OPTION_VALUE,
    A_IMPORT_LAYOUT_MAPPING_PATH,
    A_IMPORT_LAYOUT_MAPPING_FROM,
    A_IMPORT_LAYOUT_MAPPING_TO,
    A_IMPORT_RESOURCES_ADD_FILE,
    A_IMPORT_RESOURCES_ADD_DIR,
    A_IMPORT_RESOURCES_NAME,
    A_IMPORT_RESOURCES_PTT,
    A_IMPORT_RESOURCES_EXT,
    A_IMPORT_VIRT_DIR_KEY,
    A_IMPORT_VIRT_DIR_METADATA,
    A_IMPORT_LIBOPTS_AUXDATA_DATA,
    A_IMPORT_LIBOPTS_AUXDATA_KEY,
    A_IMPORT_LIBOPTS_AUXDATA_TAG,
    A_TRANSCODING_MIMETYPE_PROF_MAP,
    A_TRANSCODING_MIMETYPE_FILTER,
    A_TRANSCODING_MIMETYPE_PROF_MAP_TRANSCODE,
    A_TRANSCODING_MIMETYPE_PROF_MAP_MIMETYPE,
    A_TRANSCODING_MIMETYPE_PROF_MAP_USING,
    A_TRANSCODING_PROFILES_PROFLE,
    A_TRANSCODING_PROFILES_PROFLE_ENABLED,
    A_TRANSCODING_PROFILES_PROFLE_TYPE,
    A_TRANSCODING_PROFILES_PROFLE_NAME,
    A_TRANSCODING_PROFILES_PROFLE_CLIENTFLAGS,
    A_TRANSCODING_PROFILES_PROFLE_MIMETYPE,
    A_TRANSCODING_PROFILES_PROFLE_MIMETYPE_VALUE,
    A_TRANSCODING_PROFILES_PROFLE_MIMETYPE_PROPERTIES,
    A_TRANSCODING_PROFILES_PROFLE_MIMETYPE_PROPERTIES_KEY,
    A_TRANSCODING_PROFILES_PROFLE_MIMETYPE_PROPERTIES_RESOURCE,
    A_TRANSCODING_PROFILES_PROFLE_MIMETYPE_PROPERTIES_METADATA,
    A_TRANSCODING_PROFILES_PROFLE_RESOLUTION,
    A_TRANSCODING_PROFILES_PROFLE_AVI4CC,
    A_TRANSCODING_PROFILES_PROFLE_AVI4CC_MODE,
    A_TRANSCODING_PROFILES_PROFLE_AVI4CC_4CC,
    A_TRANSCODING_PROFILES_PROFLE_ACCURL,
    A_TRANSCODING_PROFILES_PROFLE_DLNAPROF,
    A_TRANSCODING_PROFILES_PROFLE_SRCDLNA,
    A_TRANSCODING_PROFILES_PROFLE_NOTRANSCODING,
    A_TRANSCODING_PROFILES_PROFLE_SAMPFREQ,
    A_TRANSCODING_PROFILES_PROFLE_NRCHAN,
    A_TRANSCODING_PROFILES_PROFLE_HIDEORIG,
    A_TRANSCODING_PROFILES_PROFLE_THUMB,
    A_TRANSCODING_PROFILES_PROFLE_FIRST,
    A_TRANSCODING_PROFILES_PROFLE_ACCOGG,
    A_TRANSCODING_PROFILES_PROFLE_AGENT,
    A_TRANSCODING_PROFILES_PROFLE_AGENT_COMMAND,
    A_TRANSCODING_PROFILES_PROFLE_AGENT_ARGS,
    A_TRANSCODING_PROFILES_PROFLE_AGENT_ENVIRON,
    A_TRANSCODING_PROFILES_PROFLE_AGENT_ENVIRON_KEY,
    A_TRANSCODING_PROFILES_PROFLE_AGENT_ENVIRON_NAME,
    A_TRANSCODING_PROFILES_PROFLE_AGENT_ENVIRON_VALUE,
    A_TRANSCODING_PROFILES_PROFLE_BUFFER,
    A_TRANSCODING_PROFILES_PROFLE_BUFFER_SIZE,
    A_TRANSCODING_PROFILES_PROFLE_BUFFER_CHUNK,
    A_TRANSCODING_PROFILES_PROFLE_BUFFER_FILL,
    A_AUTOSCAN_DIRECTORY,
    A_AUTOSCAN_DIRECTORY_LOCATION,
    A_AUTOSCAN_DIRECTORY_MODE,
    A_AUTOSCAN_DIRECTORY_INTERVAL,
    A_AUTOSCAN_DIRECTORY_RECURSIVE,
    A_AUTOSCAN_DIRECTORY_DIRTYPES,
    A_AUTOSCAN_DIRECTORY_MEDIATYPE,
    A_AUTOSCAN_DIRECTORY_HIDDENFILES,
    A_AUTOSCAN_DIRECTORY_FOLLOWSYMLINKS,
    A_AUTOSCAN_DIRECTORY_SCANCOUNT,
    A_AUTOSCAN_DIRECTORY_TASKCOUNT,
    A_AUTOSCAN_DIRECTORY_RETRYCOUNT,
    A_AUTOSCAN_DIRECTORY_LMT,
    A_AUTOSCAN_CONTAINER_TYPE_AUDIO,
    A_AUTOSCAN_CONTAINER_TYPE_IMAGE,
    A_AUTOSCAN_CONTAINER_TYPE_VIDEO,
    A_CLIENTS_CLIENT,
    A_CLIENTS_CLIENT_FLAGS,
    A_CLIENTS_CLIENT_IP,
    A_CLIENTS_CLIENT_GROUP,
    A_CLIENTS_CLIENT_USERAGENT,
    A_CLIENTS_CLIENT_ALLOWED,
    A_CLIENTS_UPNP_HEADERS,
    A_CLIENTS_UPNP_HEADERS_HEADER,
    A_CLIENTS_UPNP_HEADERS_KEY,
    A_CLIENTS_UPNP_HEADERS_VALUE,
    A_BOXLAYOUT_BOX_KEY,
    A_BOXLAYOUT_BOX_TITLE,
    A_BOXLAYOUT_BOX_CLASS,
    A_BOXLAYOUT_BOX_SIZE,
    A_BOXLAYOUT_BOX_ENABLED,
    A_DIRECTORIES_TWEAK,
    A_DIRECTORIES_TWEAK_LOCATION,
    A_DIRECTORIES_TWEAK_INHERIT,
    A_DIRECTORIES_TWEAK_RECURSIVE,
    A_DIRECTORIES_TWEAK_HIDDEN,
    A_DIRECTORIES_TWEAK_CASE_SENSITIVE,
    A_DIRECTORIES_TWEAK_FOLLOW_SYMLINKS,
    A_DIRECTORIES_TWEAK_META_CHARSET,
    A_DIRECTORIES_TWEAK_FANART_FILE,
    A_DIRECTORIES_TWEAK_SUBTITLE_FILE,
    A_DIRECTORIES_TWEAK_METAFILE_FILE,
    A_DIRECTORIES_TWEAK_RESOURCE_FILE,
    A_IMPORT_SYSTEM_DIR_ADD_PATH,
    A_UPNP_PROPERTIES_PROPERTY,
    A_UPNP_PROPERTIES_UPNPTAG,
    A_UPNP_PROPERTIES_METADATA,
    A_UPNP_NAMESPACE_PROPERTY,
    A_UPNP_NAMESPACE_KEY,
    A_UPNP_NAMESPACE_URI,
    A_CLIENTS_UPNP_MAP_MIMETYPE,
    A_CLIENTS_UPNP_CAPTION_COUNT,
    A_CLIENTS_UPNP_STRING_LIMIT,
    A_CLIENTS_UPNP_MULTI_VALUE,
    A_CLIENTS_UPNP_FILTER_FULL,
    A_CLIENTS_UPNP_MAP_DLNAPROFILE,
    A_CLIENTS_UPNP_MAP_DLNAPROFILE_PROFILE,
    A_IMPORT_RESOURCES_HANDLER,

    A_DYNAMIC_CONTAINER,
    A_DYNAMIC_CONTAINER_LOCATION,
    A_DYNAMIC_CONTAINER_IMAGE,
    A_DYNAMIC_CONTAINER_TITLE,
    A_DYNAMIC_CONTAINER_FILTER,
    A_DYNAMIC_CONTAINER_SORT,
    A_DYNAMIC_CONTAINER_MAXCOUNT,
};

enum class UrlAppendMode {
    none,
    ip,
    port
};

enum class LayoutType {
    Disabled,
    Builtin,
    Js,
};

using ConfigOptionIterator = EnumIterator<ConfigVal, CFG_MIN, ConfigVal::MAX>;

#define DEFAULT_MARK_PLAYED_CONTENT_AUDIO "audio"
#define DEFAULT_MARK_PLAYED_CONTENT_VIDEO "video"
#define DEFAULT_MARK_PLAYED_CONTENT_IMAGE "image"

#endif // __CONFIG_VAL_H__
