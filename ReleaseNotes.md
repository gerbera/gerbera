# Gerbera - UPnP AV Mediaserver.

## HEAD

Changed default for layout to `js` if built with JavaScript support.

### NEW Features

- All configuration options for autoscan directories are available in the web UI now, including scripts and storage in the database.
- New configuration option for autoscan directories to rescan files that ended up as plain object in the first run.
- Several script options to control audio layout
- Harmonisation of metadata handling for wavpack and matroska media handlers
- Fabricated comment on import in media handlers (incl. configuration)
- Resource attribute `pixelFormat` for videos.
- Additional Resources (thumbnail, subtitle) for External Urls
- New UI command "Scan Now" and minor improvement

### FIXES

- Album art and structure in builtin layout
- Autoscan objects in scripts
- Metadata in builtin layout, album artist in default js layout
- Commands in UI show descriptive tooltips
- Child items of autoscan folder in UI

### Code Improvements

- Removed static from ConfigDefinition
- Update versions of pugixml (1.15), libexif (v0.6.25), wavpack (5.8.1), fmt (11.1.3), spdlog (1.15.1), wavpack (1.8.1), googletest (1.16.0), libexiv2 (0.28.4)

## 2.4.1

### FIXES

- Fix Dockerfile

## v2.4.0

Rerun `gerbera --create-config="Boxlayout|DynamicContainer"` and merge changes to the `boxlayout` and `containers` the get support for upnp shortcut list feature.

There are some noteworthy new features in this release:
- the search page: Query your database with UPnP search statements in Web UI,
- the dark mode for the Web UI,
- access permissions for files: Restrict access to directories via client groups,
- support for UPnP filters in requests,
- support for UPnP CONTAINER\_SHORTCUTS feature.

### NEW Features

- Configuration of ui handler to allow hosting of docs and doxygen output
- Support for UPnP filters
- Support for UPnP shortcuts
- Search page in web UI
- Command line options `--create-config` and `--create-example-config` support arguments
- Provide packages for Ubuntu-24.10
- Allow deleting of client entries immediately

### FIXES

- Logic exceptions are reported
- Handling of logging command line options
- Handling of checkbox values on web ui
- Access to cache option if thumbnailer is disabled
- Container creation in web ui
- Handling of multi-valued tags in virtual paths
- Increase of pupnp threadpool size
- Drop Build Support for Ubuntu 23.04 and 23.10

### Code Improvements

- Build without ffmpeg leaves option unset
- Compatibility with ffmpeg > 6.0
- Logging of build info with `--compile-info`
- Update versions of libpupnp (1.14.20), spdlog (1.15.0)
- Update Build Environment
- Update versions of js vendor files jquery-ui (v1.13.2 -> v1.14.1), js-cookie (v3.0.1 -> v3.0.5), @popperjs/core (v2.11.6 -> v2.11.8)

## v2.3.0

Because of the extension of configuration of transcoding profiles, those producing PCM output have to be updated:
`<mimetype>audio/L16</mimetype>` must become
```
<mimetype value="audio/L16">
  <mime-property key="rate" resource="sampleFrequency"/>
  <mime-property key="channels" resource="nrAudioChannels"/>
</mimetype>
```
After many years Apple discontinued their trailers app and we had drop
Apple Trailer support as the last of Mediatombs online services. Thus online
service support is disabled in our builds, also, leading to parts of the
config being unused or triggering warnings.

### NEW Features

- Higher cross site scripting security in web ui
- Styles for xml documents when shown in browser
- Configuration of additional target mimetype properties for transcoding (e.g. needed for PCM output)
- Configuration of DLNA profile mappings for a client
- Display media details in web ui
- Image and thumbnail profiles support PNG images
- Page layout of items page with splitter and resize button
- Orientation for images and videos in resource attributes
- Additional client filtering options with friendlyName, modelName, manufacturer
- Configuration option to block clients
- Upnp classes are assigned to containers in physical tree (PC Directory) depending on the majority of children (only in grb-mode)
- Nested iterations are now supported in Config UI
- UPnP Search support for integer and date comparison
- Failed UPnP requests return error messages

### FIXES

- Warning in case import function does not return new ids.
- Config values `<online-content fetch-buffer-size="262144" fetch-buffer-fill-size="0">` are implemented now.
- Renaming of files or folders in grb-mode
- Nested arrays can be shown in config ui.
- Made UDN mandatory for UPnP announcements to work

### Code Improvements

- Build target for source documentation
- Update versions of libexiv2 (0.28.3), fmt (11.0.2), googletest (1.15.2), npupnp (6.2.0), taglib (2.0.2), ffmpegthumbnailer (2.2.3)
- Upgrade contrib code cxxopts (3.2.1), md5 (2002)
- Extract code for npupnp and pupnp specific handling
- Refactoring of UPnP services
- Cleanup of several todos, FIXMES, code smells etc.

## v2.2.0

This update brings a new design for the start page, better logging and a new repository for ubuntu and debian getting rid of jfrog.

Also virtual items are detected during import and cleaned up as required instead of deleting and recreating them all.
This requires a change in js scripts: All import functions have to return a list of created object ids.

### NEW Features

- Configuration for case sensitive media tags (allow making them insensitive)
- Configuration options for playlist layout
- Configuration option to activate `IN_ATTRIB` event for, e.g. permission changes on disk incl. retry if that fails
- WebUI: New design for home page and login screen
- Allow metadata handler to be disabled, add charset support
- UPnP specification files (description.xml, cds.xml) now reflect client quirks
- Search for `upnp:lastPlaybackTime`, `upnp:playbackCount` and `play_group`
- Additional command line options for logging (`--syslog` and `--rotatelog`)
- TagLib messages are logged with gerbera now. They also show up with `debug-mode="taglib"`.
- LibExiv2 messages are logged with gerbera now. They also show up with `debug-mode="exiv2"`.
- LibExif messages are logged with gerbera now. They also show up with `debug-mode="exif"`.
- ffmpeg messages are logged with gerbera now. They also show up with `debug-mode="ffmpeg"`.

### FIXES

- Album artist handling in default js layout
- Build with ffmpegthumbnailer but without ffmpeg
- Changed repository for ubuntu and debian to https://pkg.gerbera.io/
- Autologout from UI
- Finally SIGHUP can be used to reload gerbera without restarting

### Code Improvements

- Rework of server mechanism for file and data requests
- Update versions of pupnp (1.14.19), npupnp (6.1.2), spdlog (1.14.1), taglib (2.0.1), ffmpegthumbnailer(2.2.2-60-g1b5a779), fmt (11.0.0)
- Allow building of libexif (up to v0.6.24-90-g2ed252d)
- Refactoring of property handling in scripts
- Refactoring of Inotify code and command line handling
- Refactoring of Config and Content code
- Reduce header nesting

## v2.1.0

This release started out as a mere bugfix release but gathered some nice features along the way.
This includes the full build support for NPUPNP, the update of the conan build system to V2 and the return of
custom headers for particular clients.

### NEW Features

- new JavaScript function `print2` to allow setting log type (info, debug, trace)
- configuration of client specific headers (brings back old custom-headers in new place)
- wider use of `box-layout` settings in javascript and builtin layout

### FIXES

- Fix loading of playlists
- Fix multiple crashes

### Code Improvements

- Restructuring files and refactoring classes
- Update versions of fmt (10.2.1), spdlog (1.13.0), libexiv2 (0.28.2), npupnp (6.1.1), pugixml (1.14), wavpack (5.7.0)
- WebUI: Update versions of jquery (3.7.1) and tether (2.0.0)
- Docker: Update Alpine version (3.19)
- Build System: Use cmake presets
- Build System: Update Conan to V2
- Build System: Support build with NPUPNP

## v2.0.0

This release is a new major release and contains two noteworthy changes.

The JavaScript integration has been overhauled to simplify the process of providing additional layout scripts.
If you created copies and modified that code to you purpose, you may have to update your code to the new interfaces for import functions.
In case you activate the new JavaScript folder loading mechanism by setting the respective config options,
ensure that no older scripts are still in the script folders.

This release also introduces the configuration of virtual layout, you can translate the container titles or hide containers you don't use.
Of course such a change requires a rescan of the library. The options for structured audio layout have been redesigned, so you need to migrate
them to the new `<box-layout ../>`. Run gerbera with `--create-config` to get the defaults for the new section.

The second major change is the new staged import mode (`grb`) which is not activated by default. The default import mode (`mt`) handles each file completely,
i.e. the physical file is read and the virtual layout is created in one go. The new grb-mode first reads all files, second creates the phyiscal
structure and finally runs the layout functions on the physical items. The benefit is that the after an update to the file the original object can be updated
instead of deleted and recreated like before.

### NEW Features

- Staged importing that allows updating the virtual layout instead of deleting and recreating it each time (`import-mode="grb"`)
- Config options as command line arguments (`--set-option OPT=VAL` with `--print-options`)
- Allow configuration of `follow-symlinks` per autoscan directory
- Configuration of containers in virtual layout: title can be changed, some nodes can be disabled
- New mode of loading Javascript plugins with cleanup of global variables
- Generation of example configuration via command line option `--create-example-config`
- Case insensitive sorting for databases
- New config options for URL handling and host redirection
- Use `.nomedia` to hide directory, incl. config option
- Support for UPnP commands GetFeatureList and GetSortExtensionCapabilities
- Build for Ubuntu 23.04 and 23.10

### FIXES

- Autoscan: Keep track of renamed directories
- Docker: add JPEG and update description
- Runtime issues in request handling
- Configurable handling of HOME directory
- Transcoding: parsing issue of requests
- Stability for sqlite database access
- Browsing on Samsung devices

### Code Improvements

- Update Javascript libraries
- Update versions of googletest (1.14), pupnp (1.14.18), libexiv2 (v0.28.1), libebml (1.4.5), fmt (10.2.0), pugixml (1.14), spdlog (1.12.0) and taglib (1.13.1)
- Compatibility with gcc14

## v1.12.1

### NEW Features

- Debug options for specialized messages
- Configure offset of lastPlayedPosition (aka Samsung bookmark)

### FIXES

- Calling null IOHandler
- Encoding of ticks ' as &apos; for Bose
- ffmpeg and transcoding in docker container
- Tests failing on openSuSE > 15.3 and others
- Update docker images to alpine 3.17

### Code Improvements

- Reduced header nesting
- Build with latest versions of pugixml (1.13), spdlog (1.11.0), taglib (1.13), wavpack (5.6.0)

## v1.12.0

### NEW Features

- Support for NFO files as additional resources: Set up in `resources` and place nfo-files (https://kodi.wiki/view/NFO_files/Templates) next to your media files.
- Tweaking mimetypes for clients
- Editing Flags in web UI
- More statistics on web UI
- Add support for ip subnets in client config
- Defaults for virtual container upnp class
- Configuration for SQLite database modes
- Offline mode for initial scan large libraries

### FIXES
- Database update on autoscan table
- Transcoding for external items
- Sqlite errors because of deleted objects
- Sorting by certain keys
- Broken path comparison (skipped renaming, adding files)
- Update docker images to alpine 3.16

### Code Improvements
- Xml2Json rework
- build with latest versions of pupnp (1.14.14), wavpack (5.5.0), ebml (1.4.4), matroska (1.7.1), exiv2 (0.27.5), fmt (9.1.0) and spdlog (1.10.0)
- Further Cleanups

### General
If you activated nfo-metafile resources you have to reimport your media files.

## v1.11.0

### NEW Features
- Database: Clients and statistics are stored database so restart does not empty client list. Client grouping for play statistics.
- Search: Support searching playlists containers
- Search: Respect ContainerID when performing search
- Import: item class filtering and mapping by file properties allows more sophisticated virtual structure
- Transcoding: Support filtering transcoding profiles by resource properties (like codecs) avoids transcoding if client can play files
- DLNA: Detect DNLA profiles by resource attributes to specify more detailled profile for handling in client
- File type support for WavPack improved: More metadata read with special library if compiled in.
- Support Ubuntu 22.04

### FIXES
- Playlist: Fix parser error
- Playlist: Handle end of file properly
- Browsing: Sort containers first
- Search: search result is sort by title now
- Import: Timestamps in future are not stored for containers

### Code Improvements
- ContentHandler to enum
- ResourceContentType to enum
- ResourceAttribute new style enum
- Config: Autoscan list to plain vector
- ContentManager: Single autoscan list
- Update Duktape version to 2.7.0
- Server: Clean up virtualURL handling
- Add WavPack as library
- Further Cleanups

### General
To benefit from changes a rescan of all media files is recommended

## v1.10.0

### NEW Features
- Add all metadata is seachable
- Add support for ASX playlists
- Drop Ubuntu 20.10, add 21.10
- Improve support for Samsung UPnP extension X_GetFeatureList
- Support for multiple entries in metadata
- Taglib: Handle OGG containing Opus, Speex or FLAC
- WebUI: Display status details on home page
- WebUI: Thumbnails for images and grid view for items

### FIXES
- Block negative track numbers
- Improve matroska parsing speed

### Code Improvements
- Update fmt version

## v1.9.2

- Titles of search results can be configured
- Containers in virtual layout can be defined as search result, so, e.g. albums, located in several places are only found once
- Metadata, like artist, appearing multiple times are now stored in that way and can be sent to UPnP clients as separate entries as well or addressed in layout scripts. If you have a custom js import script which updates metadata you have to modify it using the new properties (see doc on scripting)
- DLNA profile can be configured using video and audio codec, allow devices to pick supported streams
- DLNA profile can be set for transcoding
