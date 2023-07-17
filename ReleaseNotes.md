# Gerbera - UPnP AV Mediaserver.

## MAIN

This release contains changes to some Javascript functions. If you created copies and modified that code to you purpose, you may have to update your code.
If you activate the new javascript folder loading mechanism ensure that no older script are still in the script folders.

This release also introduced the configuration of virtual layout, you can translate the container titles or hide containers you don't use.
Of course such a change requires a rescan of the library. The options for structured audio layout have been redesigned, so you need to migrate
them to the new `<box-layout ../>`. Run gerbera with `--create-config` to get the default for the new section.

### NEW Features

- Case insensitive sorting for databases
- Config options as command line arguments
- Staged importing that allows updating the virtual layout instead of deleting and recreating it each time
- Configuration of containers in virtual layout: title can be changed, some nodes can be disabled
- New mode of loading Javascript plugins with cleanup of global variables
- New config options for URL handling and host redirection
- Use `.nomedia` to hide directory, incl. config option

### FIXES

- Docker: add JPEG and update description
- Runtime issues in request handling
- Configurable handling of HOME directory
- Transcoding: parsing issue of requests

### Code Improvements

- Update Javascript libraries
- Update versions of libexiv2, fmt, spdlog and taglib

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
