/*GRB*

    Gerbera - https://gerbera.io/
    
    config.h - this file is part of Gerbera.
    
    Copyright (C) 2020-2020 Gerbera Contributors
    
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

#ifndef __CONFIG_H__
#define __CONFIG_H__

#include <filesystem>
#include <map>
#include <memory>
#include <vector>
namespace fs = std::filesystem;

// forward declaration
class AutoscanList;
class ClientConfigList;
class AutoscanDirectory;
class TranscodingProfileList;

typedef enum {
    CFG_SERVER_PORT = 0,
    CFG_SERVER_IP,
    CFG_SERVER_NETWORK_INTERFACE,
    CFG_SERVER_NAME,
    CFG_SERVER_MANUFACTURER,
    CFG_SERVER_MANUFACTURER_URL,
    CFG_SERVER_MODEL_NAME,
    CFG_SERVER_MODEL_DESCRIPTION,
    CFG_SERVER_MODEL_NUMBER,
    CFG_SERVER_MODEL_URL,
    CFG_SERVER_SERIAL_NUMBER,
    CFG_SERVER_PRESENTATION_URL,
    CFG_SERVER_APPEND_PRESENTATION_URL_TO,
    CFG_SERVER_UDN,
    CFG_SERVER_HOME,
    CFG_SERVER_TMPDIR,
    CFG_SERVER_WEBROOT,
    CFG_SERVER_SERVEDIR,
    CFG_SERVER_ALIVE_INTERVAL,
    CFG_SERVER_HIDE_PC_DIRECTORY,
    CFG_SERVER_BOOKMARK_FILE,
    CFG_SERVER_UPNP_TITLE_AND_DESC_STRING_LIMIT,
    CFG_SERVER_UI_ENABLED,
    CFG_SERVER_UI_POLL_INTERVAL,
    CFG_SERVER_UI_POLL_WHEN_IDLE,
    CFG_SERVER_UI_ACCOUNTS_ENABLED,
    CFG_SERVER_UI_ACCOUNT_LIST,
    CFG_SERVER_UI_SESSION_TIMEOUT,
    CFG_SERVER_UI_DEFAULT_ITEMS_PER_PAGE,
    CFG_SERVER_UI_ITEMS_PER_PAGE_DROPDOWN,
    CFG_SERVER_UI_SHOW_TOOLTIPS,
    CFG_SERVER_STORAGE_DRIVER,
    CFG_SERVER_STORAGE_SQLITE_DATABASE_FILE,
    CFG_SERVER_STORAGE_SQLITE_SYNCHRONOUS,
    CFG_SERVER_STORAGE_SQLITE_RESTORE,
    CFG_SERVER_STORAGE_SQLITE_BACKUP_ENABLED,
    CFG_SERVER_STORAGE_SQLITE_BACKUP_INTERVAL,
#ifdef HAVE_MYSQL
    CFG_SERVER_STORAGE_MYSQL_HOST,
    CFG_SERVER_STORAGE_MYSQL_PORT,
    CFG_SERVER_STORAGE_MYSQL_USERNAME,
    CFG_SERVER_STORAGE_MYSQL_SOCKET,
    CFG_SERVER_STORAGE_MYSQL_PASSWORD,
    CFG_SERVER_STORAGE_MYSQL_DATABASE,
#endif
#if defined(HAVE_FFMPEG) && defined(HAVE_FFMPEGTHUMBNAILER)
    CFG_SERVER_EXTOPTS_FFMPEGTHUMBNAILER_ENABLED,
    CFG_SERVER_EXTOPTS_FFMPEGTHUMBNAILER_THUMBSIZE,
    CFG_SERVER_EXTOPTS_FFMPEGTHUMBNAILER_SEEK_PERCENTAGE,
    CFG_SERVER_EXTOPTS_FFMPEGTHUMBNAILER_FILMSTRIP_OVERLAY,
    CFG_SERVER_EXTOPTS_FFMPEGTHUMBNAILER_WORKAROUND_BUGS,
    CFG_SERVER_EXTOPTS_FFMPEGTHUMBNAILER_IMAGE_QUALITY,
    CFG_SERVER_EXTOPTS_FFMPEGTHUMBNAILER_CACHE_DIR_ENABLED,
    CFG_SERVER_EXTOPTS_FFMPEGTHUMBNAILER_CACHE_DIR,
#endif
    CFG_SERVER_EXTOPTS_MARK_PLAYED_ITEMS_ENABLED,
    CFG_SERVER_EXTOPTS_MARK_PLAYED_ITEMS_STRING_MODE_PREPEND,
    CFG_SERVER_EXTOPTS_MARK_PLAYED_ITEMS_STRING,
    CFG_SERVER_EXTOPTS_MARK_PLAYED_ITEMS_SUPPRESS_CDS_UPDATES,
    CFG_SERVER_EXTOPTS_MARK_PLAYED_ITEMS_CONTENT_LIST,
#ifdef HAVE_LASTFMLIB
    CFG_SERVER_EXTOPTS_LASTFM_ENABLED,
    CFG_SERVER_EXTOPTS_LASTFM_USERNAME,
    CFG_SERVER_EXTOPTS_LASTFM_PASSWORD,
#endif
    CFG_IMPORT_HIDDEN_FILES,
    CFG_IMPORT_FOLLOW_SYMLINKS,
    CFG_IMPORT_FILESYSTEM_CHARSET,
    CFG_IMPORT_METADATA_CHARSET,
    CFG_IMPORT_PLAYLIST_CHARSET,
#ifdef HAVE_JS
    CFG_IMPORT_SCRIPTING_CHARSET,
    CFG_IMPORT_SCRIPTING_COMMON_SCRIPT,
    CFG_IMPORT_SCRIPTING_PLAYLIST_SCRIPT,
    CFG_IMPORT_SCRIPTING_PLAYLIST_SCRIPT_LINK_OBJECTS,
    CFG_IMPORT_SCRIPTING_IMPORT_SCRIPT,
#endif // JS
    CFG_IMPORT_SCRIPTING_VIRTUAL_LAYOUT_TYPE,
#ifdef HAVE_MAGIC
    CFG_IMPORT_MAGIC_FILE,
#endif
    CFG_IMPORT_AUTOSCAN_TIMED_LIST,
#ifdef HAVE_INOTIFY
    CFG_IMPORT_AUTOSCAN_USE_INOTIFY,
    CFG_IMPORT_AUTOSCAN_INOTIFY_LIST,
#endif
    CFG_IMPORT_MAPPINGS_IGNORE_UNKNOWN_EXTENSIONS,
    CFG_IMPORT_MAPPINGS_EXTENSION_TO_MIMETYPE_CASE_SENSITIVE,
    CFG_IMPORT_MAPPINGS_EXTENSION_TO_MIMETYPE_LIST,
    CFG_IMPORT_MAPPINGS_MIMETYPE_TO_UPNP_CLASS_LIST,
    CFG_IMPORT_MAPPINGS_MIMETYPE_TO_CONTENTTYPE_LIST,
#ifdef HAVE_LIBEXIF
    CFG_IMPORT_LIBOPTS_EXIF_AUXDATA_TAGS_LIST,
#endif
#ifdef HAVE_EXIV2
    CFG_IMPORT_LIBOPTS_EXIV2_AUXDATA_TAGS_LIST,
#endif
#if defined(HAVE_TAGLIB)
    CFG_IMPORT_LIBOPTS_ID3_AUXDATA_TAGS_LIST,
#endif
    CFG_TRANSCODING_PROFILE_LIST,
#ifdef HAVE_CURL
    CFG_EXTERNAL_TRANSCODING_CURL_BUFFER_SIZE,
    CFG_EXTERNAL_TRANSCODING_CURL_FILL_SIZE,
#endif //HAVE_CURL
#ifdef SOPCAST
    CFG_ONLINE_CONTENT_SOPCAST_ENABLED,
    CFG_ONLINE_CONTENT_SOPCAST_REFRESH,
    CFG_ONLINE_CONTENT_SOPCAST_UPDATE_AT_START,
    CFG_ONLINE_CONTENT_SOPCAST_PURGE_AFTER,
#endif
#ifdef ATRAILERS
    CFG_ONLINE_CONTENT_ATRAILERS_ENABLED,
    CFG_ONLINE_CONTENT_ATRAILERS_REFRESH,
    CFG_ONLINE_CONTENT_ATRAILERS_UPDATE_AT_START,
    CFG_ONLINE_CONTENT_ATRAILERS_PURGE_AFTER,
    CFG_ONLINE_CONTENT_ATRAILERS_RESOLUTION,
#endif
#if defined(HAVE_FFMPEG)
    CFG_IMPORT_LIBOPTS_FFMPEG_AUXDATA_TAGS_LIST,
#endif
    CFG_CLIENTS_LIST,
    CFG_CLIENTS_LIST_ENABLED,
    CFG_IMPORT_LAYOUT_PARENT_PATH,
    CFG_IMPORT_LAYOUT_MAPPING,
    CFG_IMPORT_LIBOPTS_ENTRY_SEP,
    CFG_IMPORT_LIBOPTS_ENTRY_LEGACY_SEP,
    CFG_IMPORT_RESOURCES_FANART_FILE_LIST,
    CFG_IMPORT_RESOURCES_SUBTITLE_FILE_LIST,
    CFG_IMPORT_RESOURCES_RESOURCE_FILE_LIST,

    CFG_MAX
} config_option_t;

class Config {
public:
    /// \brief Returns the name of the config file that was used to launch the server.
    virtual fs::path getConfigFilename() const = 0;

    /// \brief returns a config option of type std::string
    /// \param option option to retrieve.
    virtual std::string getOption(config_option_t option) = 0;

    /// \brief returns a config option of type int
    /// \param option option to retrieve.
    virtual int getIntOption(config_option_t option) = 0;

    /// \brief returns a config option of type bool
    /// \param option option to retrieve.
    virtual bool getBoolOption(config_option_t option) = 0;

    /// \brief returns a config option of type dictionary
    /// \param option option to retrieve.
    virtual std::map<std::string, std::string> getDictionaryOption(config_option_t option) = 0;

    /// \brief returns a config option of type array of string
    /// \param option option to retrieve.
    virtual std::vector<std::string> getArrayOption(config_option_t option) = 0;

    /// \brief returns a config option of type AutoscanList
    /// \param option to retrieve
    virtual std::shared_ptr<AutoscanList> getAutoscanListOption(config_option_t option) = 0;

    /// \brief returns a config option of type ClientConfigList
    /// \param option to retrieve
    virtual std::shared_ptr<ClientConfigList> getClientConfigListOption(config_option_t option) = 0;

    /// \brief returns a config option of type TranscodingProfileList
    /// \param option to retrieve
    virtual std::shared_ptr<TranscodingProfileList> getTranscodingProfileListOption(config_option_t option) = 0;
};

#endif // __CONFIG_H__
