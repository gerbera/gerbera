/*MT*
    
    MediaTomb - http://www.mediatomb.cc/
    
    config_manager.h - this file is part of MediaTomb.
    
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

/// \file config_manager.h
///\brief Definitions of the ConfigManager class.

#ifndef __CONFIG_MANAGER_H__
#define __CONFIG_MANAGER_H__

#include "autoscan.h"
#include "common.h"
#include "config_options.h"
#include "dictionary.h"
#include "mxml/mxml.h"
#include "object_dictionary.h"
#include "singleton.h"
#include "xpath.h"

#include "transcoding/transcoding.h"
#ifdef ONLINE_SERVICES
#include "online_service.h"
#endif
typedef enum {
    CFG_SERVER_PORT = 0,
    CFG_SERVER_IP,
    CFG_SERVER_NETWORK_INTERFACE,
    CFG_SERVER_NAME,
    CFG_SERVER_MANUFACTURER_URL,
    CFG_SERVER_MODEL_NAME,
    CFG_SERVER_MODEL_DESCRIPTION,
    CFG_SERVER_MODEL_NUMBER,
    CFG_SERVER_SERIAL_NUMBER,
    CFG_SERVER_PRESENTATION_URL,
    CFG_SERVER_APPEND_PRESENTATION_URL_TO,
    CFG_SERVER_UDN,
    CFG_SERVER_HOME,
    CFG_SERVER_TMPDIR,
    CFG_SERVER_WEBROOT,
    CFG_SERVER_SERVEDIR,
    CFG_SERVER_ALIVE_INTERVAL,
    CFG_SERVER_EXTEND_PROTOCOLINFO,
    CFG_SERVER_EXTEND_PROTOCOLINFO_CL_HACK,
    CFG_SERVER_EXTEND_PROTOCOLINFO_SM_HACK,
    CFG_SERVER_EXTEND_PROTOCOLINFO_DLNA_SEEK,
    CFG_SERVER_HIDE_PC_DIRECTORY,
    CFG_SERVER_BOOKMARK_FILE,
    CFG_SERVER_CUSTOM_HTTP_HEADERS,
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
    CFG_SERVER_STORAGE_CACHING_ENABLED,
#ifdef HAVE_SQLITE3
    CFG_SERVER_STORAGE_SQLITE_DATABASE_FILE,
    CFG_SERVER_STORAGE_SQLITE_SYNCHRONOUS,
    CFG_SERVER_STORAGE_SQLITE_RESTORE,
    CFG_SERVER_STORAGE_SQLITE_BACKUP_ENABLED,
    CFG_SERVER_STORAGE_SQLITE_BACKUP_INTERVAL,
#endif
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
    CFG_MAX
} config_option_t;

class ConfigManager : public Singleton<ConfigManager, std::mutex> {
public:
    ConfigManager();

    void init() override;
    zmm::String getName() override { return _("Config Manager"); }

    virtual ~ConfigManager();

    /// \brief Returns the name of the config file that was used to launch the server.
    inline zmm::String getConfigFilename() { return filename; }

    void load(zmm::String filename);

    /// \brief returns a config option of type String
    /// \param option option to retrieve.
    zmm::String getOption(config_option_t option);

    /// \brief returns a config option of type int
    /// \param option option to retrieve.
    int getIntOption(config_option_t option);

    /// \brief returns a config option of type bool
    /// \param option option to retrieve.
    bool getBoolOption(config_option_t option);

    /// \brief returns a config option of type Dictionary
    /// \param option option to retrieve.
    zmm::Ref<Dictionary> getDictionaryOption(config_option_t option);

    /// \brief returns a config option of type Array of StringBase
    /// \param option option to retrieve.
    zmm::Ref<zmm::Array<zmm::StringBase>> getStringArrayOption(config_option_t option);

    zmm::Ref<ObjectDictionary<zmm::Object>> getObjectDictionaryOption(config_option_t option);
#ifdef ONLINE_SERVICES
    /// \brief returns a config option of type Array of Object
    /// \param option option to retrieve.
    zmm::Ref<zmm::Array<zmm::Object>> getObjectArrayOption(config_option_t option);
#endif

    /// \brief returns a config option of type AutoscanList
    /// \param option to retrieve
    zmm::Ref<AutoscanList> getAutoscanListOption(config_option_t option);

    /// \brief returns a config option of type TranscodingProfileList
    /// \param option to retrieve
    zmm::Ref<TranscodingProfileList> getTranscodingProfileListOption(config_option_t option);

    /// \brief sets static configuration parameters that will be used by
    /// when the ConfigManager class initializes
    static void setStaticArgs(zmm::String _filename, zmm::String _userhome,
        zmm::String _config_dir = _(DEFAULT_CONFIG_HOME),
        zmm::String _prefix_dir = _(PACKAGE_DATADIR),
        zmm::String _magic = nullptr,
        bool _debug_logging = false,
        zmm::String _ip = nullptr, zmm::String _interface = nullptr, int _port = 0);

    static bool isDebugLogging() { return debug_logging; };

    /// \brief Creates a html file that is a redirector to the current server i
    /// instance
    void writeBookmark(zmm::String ip, zmm::String port);
    void emptyBookmark();

protected:
    void validate(zmm::String serverhome);
    zmm::String construct_path(zmm::String path);
    void prepare_path(zmm::String path, bool needDir = false, bool existenceUnneeded = false);

    static zmm::String filename;
    static zmm::String userhome;
    static zmm::String config_dir;
    static zmm::String prefix_dir;
    static zmm::String magic;
    static bool debug_logging;
    static zmm::String ip;
    static zmm::String interface;
    static int port;

    zmm::Ref<mxml::Document> rootDoc;
    zmm::Ref<mxml::Element> root;

    zmm::Ref<Dictionary> mime_content;

    zmm::Ref<zmm::Array<ConfigOption>> options;

    /// \brief Returns a config option with the given path, if option does not exist a default value is returned.
    /// \param xpath option xpath
    /// \param def default value if option not found
    ///
    /// the name parameter has the XPath syntax.
    /// Currently only two xpath constructs are supported:
    /// "/path/to/option" will return the text value of the given "option" element
    /// "/path/to/option/attribute::attr" will return the value of the attribute "attr"
    zmm::String getOption(zmm::String xpath, zmm::String def);

    /// \brief same as getOption but returns an integer value of the option
    int getIntOption(zmm::String xpath, int def);

    /// \brief Returns a config option with the given path, an exception is raised if option does not exist.
    /// \param xpath option xpath.
    ///
    /// The xpath parameter has XPath syntax.
    /// Currently only two xpath constructs are supported:
    /// "/path/to/option" will return the text value of the given "option" element
    /// "/path/to/option/attribute::attr" will return the value of the attribute "attr"
    zmm::String getOption(zmm::String xpath);

    /// \brief same as getOption but returns an integer value of the option
    int getIntOption(zmm::String xpath);

    /// \brief Returns a config XML element with the given path, an exception is raised if element does not exist.
    /// \param xpath option xpath.
    ///
    /// The xpath parameter has XPath syntax.
    /// "/path/to/element" will return the text value of the given "element" element
    zmm::Ref<mxml::Element> getElement(zmm::String xpath);

    /// \brief Checks if the string returned by getOption is valid.
    /// \param xpath xpath expression to the XML node
    zmm::String checkOptionString(zmm::String xpath);

    /// \brief Creates a dictionary from an XML nodeset.
    /// \param element starting element of the nodeset.
    /// \param nodeName name of each node in the set
    /// \param keyAttr attribute name to be used as a key
    /// \param valAttr attribute name to be used as value
    ///
    /// The basic idea is the following:
    /// You have a piece of XML that looks like this
    /// <some-section>
    ///    <map from="1" to="2"/>
    ///    <map from="3" to="4"/>
    /// </some-section>
    ///
    /// This function will create a dictionary with the following
    /// key:value paris: "1":"2", "3":"4"
    zmm::Ref<Dictionary> createDictionaryFromNodeset(zmm::Ref<mxml::Element> element, zmm::String nodeName, zmm::String keyAttr, zmm::String valAttr, bool tolower = false);

    /// \brief Creates an array of AutoscanDirectory objects from a XML nodeset.
    /// \param element starting element of the nodeset.
    /// \param scanmode add only directories with the specified scanmode to the array
    zmm::Ref<AutoscanList> createAutoscanListFromNodeset(zmm::Ref<mxml::Element> element, ScanMode scanmode);

    /// \brief Creates ab aray of TranscodingProfile objects from an XML
    /// nodeset.
    /// \param element starting element of the nodeset.
    zmm::Ref<TranscodingProfileList> createTranscodingProfileListFromNodeset(zmm::Ref<mxml::Element> element);

    /// \brief Creates an array of strings from an XML nodeset.
    /// \param element starting element of the nodeset.
    /// \param nodeName name of each node in the set
    /// \param attrName name of the attribute, the value of which shouldbe extracted
    ///
    /// Similar to \fn createDictionaryFromNodeset() this one extracts
    /// data from the following XML:
    /// <some-section>
    ///     <tag attr="data"/>
    ///     <tag attr="otherdata"/>
    /// <some-section>
    ///
    /// This function will create an array like that: ["data", "otherdata"]
    zmm::Ref<zmm::Array<zmm::StringBase>> createArrayFromNodeset(zmm::Ref<mxml::Element> element, zmm::String nodeName, zmm::String attrName);

    void dumpOptions();

#ifdef ONLINE_SERVICES
    /// \brief This functions activates the YouTube class and retrieves
    /// lets it parse the options.
    ///
    /// Note that usually the config manager does all the parsing, however
    /// in the case of online services the tasklist is something very generic
    /// and only the service class knows how to organize it. Since an
    /// online service is controlled by an external authority, the API and
    /// thus the options can change, for that reason we prefer keeping
    /// absolutely all functionality related to the service inside the
    /// service class.
    zmm::Ref<zmm::Array<zmm::Object>> createServiceTaskList(service_type_t service, zmm::Ref<mxml::Element> element);
#endif
};

#endif // __CONFIG_MANAGER_H__
