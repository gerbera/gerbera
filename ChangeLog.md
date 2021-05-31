## Gerbera - UPnP AV Mediaserver.

### v1.8.2

- Disable transactions by default
- Refactor sort parsing to make it testable
- Support configuration of structured layout
- Factor out handling of config definitions into separate class
- Display default config values on Web UI
- initialize several unique_ptrs
- move resize option to constructor
- string_view to string conversions
- pass SearchLexer by value
- return std::nullopt for std::optional
- remove pointless const_cast
- Improve handling of default config values
- several clang-tidy and manual cleanups
- Fix parseSortStatement
- mostly pass by value changes
- Revert last_write_time to seconds since 1970
- Encoding of web editor arguments
- clang-analyzer and misc
- Search query sign from beginning
- use C++ cast
- manual destructor changes
- Fix double truncation of playlist name
- clang: add missing moves
- clang: remove CTAD in some places
- Generate correct statement for metadata table
- clang fixes
- Make transactions for SQLite thread safe
- CI: Bump Clang version
- random stuff
- add support for Cygwin
- Enable tests for non-git-builds

### v1.8.1
- Mysql transactions
- Support creation_time on FFmpeg handler.
- Bump lodash from 4.17.19 to 4.17.21 in /gerbera-web
- Synchronize threads properly
- add missing optional header for GCC11
- Misc improvements
- remove several implicit fs::path conversions
- Concatenate multivalue field with entrySeparator
- Revert "replace find() end() comparisons with count()"
- remove push_back branch
- change some size_t to bool
- remove some pointless lambdas
- remove std::distance where empty is sufficient
- Implement SAMSUNG X_GetFeatureList
- More chrono optimizations
- CI: FreeBSD: Add Sqlite3 package
- algorithm cleanups
- Added documentation how to build the docker container
- Determine container art image from files
- massive std::chrono conversion
- string_view conversions
- Add support for Windows Media Player
- Conan: Tidy up CMake conan integration
- Implement UPnP SortCriteria
- Avoid needless process elevation requirement on illumos
- cmake: Remove unused vars
- Only use music tracks as container art source
- Work around undefined syscalls on solaroid systems
- Documentation for exiv2 meta data usage
- Clean compilation and errno fix
- Enable ContainerArt for PC Directory
- auto&& conversions
- Docker.md: add docker-compose instructions
- fs::path conversions
- Fix debian buster compilation
- Cleanup path handling
- add -d, -u and -P commandline options to docs
- Fixed required versions of libupnp and libnpupnp.
- fix UPNP_USING_CHUNKED breaks npupnp compatibility
- clang-tidy: replace typedef with using
- Add cmdline options -u, -d and -P
- Fix transcoded media ends before file is complete
- Documentation still points to bintray.com
- Bump master post release

### v1.8.0
- Significant improvement in import speeds with large collections
- Handle Samsung X_SetBookmark action, for saving position of viewed media, when enabled by flag
- New search support for containers, i.e. Albums, Artists and Genres
- Extract metadata information from mp3 files with ID3v1 tags, not just v2
- Added a retry when binding to interfaces, should make Gerbera much more reliable to start with slow network startup.
- Hack around broken libstdc++ large file support on Debian 10 with 32-bit OS (mostly older ARM devices)
- Fix build with GCC11
- Fix Debian/Ubuntu packages to work out of the box
- Beautify titles which are generated from file names

### v1.7.0

- You can now use libnupnp instead of libupnp as the underlying UPnP library
- Multiple disk albums are now sorted correctly and are disks distinguished by part number field
- Container images are now possible on containers without files
- Subtitle resources should now work on Samsung clients
- Thumbnails, album art and container images now shown in the Web UI Database view
- Set additional import options per directory on filesystem page
- You can now edit configuration (most of it) in the Web UI!
- Charset can be specific to import library (section: import/library options), e g. if your pictures use different encoding than your music
- Resource configuration can be used for container images (section: import/resources): define image name filters and strategy for container hierarchy
- Additional metadata stored from import scan
- Modification time for autoscan directories now reflects sub directories and reduces startup footprint
- Container images are stored as resources so browsing can return them directly
- All scripting functions have been moved to common js
- A custom script file can be set in `config xml`. It may contain additional functions or override the existing.
- A new builtin script function `addContainerTree` can be used to set container properties
- Most entries from configuration can be accessed via new global variable `config`.
- The script file import structured js has been dropped. The layout can now be selected with the configuration attribute audio layout in virtual layout element.
- Depending on your previous changes script changes adjustments are recommended - although no breaking changes have been made.
- Internal refactoring and code improvements
- Many bug fixes

In order to benefit from all these improvements it is recommended to clear your database and rescan your media library.

### v1.6.4
- Fix regression introduced in 1.6.2 in SQL generation

### v1.6.3
- Fix a regression introduced in 1.6.2 when adding resources

### v1.6.2
- Fixed a regression where some files could be removed from the library on a restart due to a race
- Fixed a crash in ffmpeg hander where metadata date field was non-numeric
- Add Samsung X_SetBookmark stub
- Bump jimp (fixes vuln jpeg-js)
- UI: Dont update mimetype to empty
- Fix tests with npupnp
- fix compilation with libcxx
- libevix2 fixes
- npupnp changes
- doc: add OpenWrt section
- clang-tidy fixes
- cxxopts: update to 2.2.1
- Update README badges


### v1.6.1
- fixed mime type retrieval for symlinks with libmagic
- Drop travis
- Actions: Run docker build on tags too
- Bump js stuff to fix security warnings
- Color folders with children instead of badges saying true
- Improve Resource Handling
- Add subtitle as resource and update entries with resources attached, improve ContentHandler
- Bump elliptic from 6.5.2 to 6.5.3 in /gerbera-web
- fix upnp header include
- Build fixes
- std::algorithm conversions


### v1.6.0
- Allow configuration of separator for multi-valued tags
- Show duk script error message on load
- Show all object details on UI
- Updated config.xml XSD
- Added support for Conan package manager
- Add friendly messages when finding spdlog as library
- Refactored caching of ffmpeg thumbnails.
- lots more!

### v1.5.0
- Client Auto detection and DLNA quirks always enabled.
- C++ Standardisation
- UI Enhancements
- Transcoding Enhancements
- Expat has been replaced with pugixml
- Spdlog is now used for logging
- libfmt is now used for string formatting
- The latest 1.12.1 version of libupnp is now required.

### v1.4.0
- Metadata MKV support via libmatroska
- SQLite: Add migration to v5 schema
- Update web modules
- Dockerfile: Add MKV support
- README update, minor doc tidy
- Docs: Scripting - tidy
- install-pupnp: Bump to 1.8.6, fix #534
- remove not used variables
- Docs: Scripting: remove docs for removed code, fix Note formatting
- Minor UI improvements

### v1.3.4/5
- Fix the build with LibUPnP 1.10
- Build against libupnp 1.8.5
- Fedora installation.
- UI e2e: Bump chromedriver

### v1.3.3
- Remove Storage Cache
- my_bool is not defined with mysql-connector-c 8.0
- Update config.xml
- Support inotify kernel driver on illumos/OmniOS
- Fix up docs and start script now that -P/--pidfile is removed
- Set language to support xenial & latest chrome
- Correct add file docs
- Update Web Development Dependencies
- Dockerfile created
- SQLite: Turn on foreign key support at runtime
- Fix error in sqlite schema
- Add DeviceDescriptionHandler
- Update lodash
- Rename handlers to util
- Convert Gerbera UI to ES6

### v1.3.2
- Allow to set the manufacturer and the modelURL via config file
- Fix find_program taglib-config when cross-compiling
- Update config-import.rst
- Add Gerbera version to UI
- cmake/FindFFMPEG: do not quieten messages when using pkg-config
- Fixture upgrade
- Update js cookie
- Add virtualUrl to AlbumArt resource for consistent resource URIs
- Use exiv2 header that include all headers
- Update vendor dependencies
- Docs: Bump version
- feat: provide a way to toggle DLNA-seeking with a config
- main: bugfix `interface` CLI option
- Generate config.xml with XML Declaration


### v1.3.1
- Build system improvements
- Fixes for DLNA Headers handling
- Add support for TXXX AuxData extraction from MP3
- Fix External URL resource generation
- Latest NPM Updates

### v1.3.0
- C++17 is now required to build (clang, gcc-7, gcc-8)
- Improved Samsung DTV Support (Still not entirely complete, but some more models may work)
- Added FLAC, Wavpack, DSD to default configuration
- Fixed Transcoding bugs with HTTP Protocol
- Properly handle upnp:date for Album sorting on UPNP devices
- Exposed resource options to import scripts (audio channels etc)
- Added support for Classical music tags: Composer, Conductor, Orchestra
- Fix UI update bug for macOS
- Add online-content examples
- Improve scripted installation
- Add configurable title for UI Home
- Fix SQL bugs
- Create Gerbera Homebrew Tap (for macOS High Sierra & Mojave)
- Various bug fixes and ongoing refactoring
- Add CentOS install instructions

### v1.2.0
- Amazing new web ui
- UPnP Search implemented
- Improved Docs: docs.gerbera.io, kindly hosted by Read the Docs.
- Broken Youtube support removed
- Fixed AUXDATA truncation with UTF-8 characters.
- Improved message when libupnp fails to bind correctly.
- Allow use of FFMpeg to extract AUXDATA
- Duktape JS scripting errors are now visible in log file
- Fixed a crash in EXIV2 image handler.
- Fixed "root path" sometimes missing for scripted layouts.

### v1.1.0
- Modern UI Preview
- Raspberry Pi / 32bit fixes
- Video thumbnail support
- Protocol Extensions
- BSD Fixes
- Album Artist support
- The --pidfile option has been removed, as we removed the --daemon option in the previous release retaining --pidfile option did not make sense
- This release supports <=libupnp 1.8.2 due to breaking changes in libupnp master branch, 1.2 will most likely require >=1.8.3.

### v1.0.0
- Rebranded as Gerbera, new Logo!
- Ported to CMake
- Removed bundled libupnp - Now requires upstream 1.8 version.
- Removed bundled libuuid.
- Enhanced cover art support for MP4, FLAC, Vorbis & WMA files (via TagLib).
- TagLib handler opens files read only: stops inotify rescanning the dir on access causing "Object not found" see here.
- Album folders have "creator" metadata (artist).
- Album folders have artwork: either from external files or the tracks embedded artwork.
- Per-track external art support: filename-of-track.jp\* (minus the audio file extension).
- WIP port to "Modern C++" / tidying.
- Removed libflac use/dep.
- Remove libmp4v2 use/dep.
- Remove id3lib use/dep.
- Removed broken DVD support (dvdnav).
- IPv6 Support
- Replaced SpiderMonkey (mozjs) dependency with the duktape engine.
- Lots of other stuff.

### MediaTomb v0.12.2
- Added mtime and sizeOnDisk to JS objects

### MediaTomb v0.12.1 08.04.2010
- fixed YouTube service support (got broken after they updated their
  website)
- fixed a problem in soap response http headers (solves "access error" 
  on Yamaha RX-V2065)
- turned out the change log for 0.12.0 was not complete (various 
  closed bugs were not mentioned)
- fixed automatic id3lib detection when taglib is not available

### MediaTomb v0.12.0 25.03.2010
- added video thumbnail generation using libffmpegthumbnailer
- added configure settings which allow to enable sqlite backups by 
  default
- added cross compile defaults for the inotify check to configure
- added configure check for broken libmagic on Slackware
- added libmp4v2 metadata handler to get tags and cover art from mp4
  files
- got rid of several compiler warnings
- added storage cache
- added storage insert buffering
- fixed mysql "threads didn't exit" issue
- implemented YouTube service support which allows to watch YouTube 
  videos on your UPnP player (in combination with transcoding)
- added fixes to allow PCM playback on the PS3 and other devices,
  thanks to mru for his findings. This allows streaming transcoded
  OGG and FLAC media to the PS3.
- added option to enable tooltips for the web UI, thanks to cweiske
  for the patch
- fixed bug #1986789 - Error when renaming a playlist container
- added parameter -D/--debug (enable debug output)
- closed feature request #1934646 - added parameter --version 
  (prints version information
- added parameter --compile-info (prints compile information)
- fixed problem "Negative duration in .m3u files" (SF forum)
- fixed bug #2078017 - Playlist inital import fails in autoscan
  directories
- fixed bug #1890657 - transcoding tmp file using 2 //
- fixed bug #1978210 - compile error with newer libcurl
- fixed bug #1934649 - typo in --help text
- fixed bugs #1986709 and #1996046 - cannot rename item with & in name
- fixed bug #2122696 - build error with MySQL 5.1
- fixed bug #1973930 - Incorrect UPnP class assigned to Vorbis and
  Theora items
- fixed bug #1934659 - unspecific error message when db is not available
- fixed bug #2904448 - memset with number of bytes set to 0 in
  array.cc
- added a "Directories" view in the default layout for Photos and
  Video, the feature is still somewhat experimental
- added a feature that allows to mark played items
- improved inotify check which was failing with 2.4 kernels
- added support for lastfmlib, thanks to Dirk Vanden Boer vor the
  patch
- increased buffer length for the exif field following the request
  from one of our users
- fixed a problem where the upnp-string-limit function was not
  correctly truncating UTF-8 strings
- fixed a bug where --enable-id3lib did not turn off the automatic
  enabling of taglib, which then resulted in an error message saying, 
  that both libraries are enabled
- implemented feature request #2833402 - ability to change
  ffmpegthumbnailer param "image quallity"
- fixed bug #2783557 - Mediatomb always flags file as chunked output,
  thanks to Michael Guntsche
- fixed bug #2804342 - wrong namespace in <serviceId>
- added a fix for glibc 2.10, thanks to Honoome for the patch
- added patches for NetBSD, thanks to Jared D. McNeill
- fixed bug #2944701 Adding comment in config as last line yields
  segfault
- fixed bug #2779907 SQLITE3: (1) cannot start a transaction within a
  transaction
- fixed bug #2820213 build broken with libmp4v2-1.9.0
- fixed bug #2161155 inotify thread aborts 
- fixed bug #2011296 SVN rev 1844 doesn't show more than 10 files on
  PS3
- fixed bug #1988738 web ui docs miss note how to access web ui
- fixed bug #1929230 invalid XML in UPNP messages
- fixed bug #1865386 autoscan dir already exists error 
- implemented feature request #2313953 support for forked libmp4v2
  project
- implemented feature request #1955192 flag/mark watched video files
- implemented feature request #1928580 better logging/tracing support
- implemented feature request #1754010 m4a metadata extraction

### MediaTomb v0.11.0 01.03.2008  External transcoding support
- implemented transcoding support that allows to plug in an arbitrary
  transcoding application
- added fourcc detection for AVI files and transcoding options to 
  limit transcoding to certain fourcc's
- added new metadata extractor using ffmpeg, patch submitted by 
  Ingo Preiml
- added vorbis / theora detection for ogg containers, so video files
  should not end up in audio containers anymore
- fixed bug where database-file option was still checked even when
  MySQL was selected in the configuration
- fixed a bug where check of the home directory was enforced even if 
  the configuration file was specified on command line
- UTF-8 fix suggested by piman - taglib should handle UTF-8 correctly, 
  so we will request an UTF-8 string from taglib and do not do the 
  conversion ourselves
- UTF-8 fix for libextractor, basically same as with TagLib
- added default mapping for flv files since they are not correctly
  recognized by libmagic
- fixed a bug where we could get crashed by a missing URL parameter
  sent to the UI
- fixed 64bit related issues in the UPnP SDK
- fixed a problem where ID3 tags were not detected with id3lib
- fixed off by one errors in StringBuffer class, thanks to stigpo for
  the patch
- fixed bug #1824216 - encoded URLs were not treated properly
- made sure that log output is always flushed
- made temporary directory configurable
- using expat as XML parser
- new XML generator - support for comments
- changed layout of the storage configuration XML
- added migration for old config.xml
- speed up sqlite3 by setting synchronous=off by default
- adding automatic database backup for sqlite3
- adding automatic backup restore on sqlite3 database corruption
- fix permission problem - supplementary groups weren't set by
  initgroups()
- fixed js (spidermonkey) related crashes
- fix mysql reconnect issue - charset was lost after reconnect
- added check to avoid coredump when max number of inotify watches
  has been reached
- made sure that range requests specified as "bytes=0-" do not trigger
  a 416 response for media where the filesize is unknown
- added fix for chunked encoding that was posted on the maemo forums
- fixed configure to determine if iconv needs the const char cast or
  not (fixes OSX compilation problem)
- added album art support for the PS3
- fixed a bug where path used by add container chain was not converted
  to UTF-8
- added patch for author and director extraction from id3 tags, 
  submitted by Reinhard Enders
- init script for fedora now uses the -e option instead of grepping
  for the IP (old variant only worked on systems with english
  language)
- updated spec file with changes from Marc Wiriadisastra
- we now are also providing the filesize along with the other metadata
- added environment variables that can be used for additional server
  configuration (useful for directory independent NAS setups)
- added comments and examples to the config.xml that is generated by
  the server
- added patch from Gabriel Burca to extract keywords from id3 tags
- added runtime inotify detection
- added a workaround for the Noxon V1 which for some reasons sends 
  us a double encoded ampersand XML sequences in the URL
- implemented feature request #1771561, extension to mimetype mappings
  can now be case insensitive

### MediaTomb v0.10.0	11.07.2007  Playlist and inotify autoscan support.
- MySQL database version pumped to 3 (MediaTomb will automatically
  update the database during the first launch)
- Sqlite3 database version pumped to 2 (MediaTomb will automatically
  update the database during the first launch)
- added support for inotify based autoscan
- playlist support - parsing playlist via js is now possible
- added network interface option
- added workaround for a PS3 related problem, where sockets were left
  open (bug #1732412)
- improved iconv handling of illegal characters
- added character conversion safeguards to make sure that non UTF-8
  strings do not make it into the database from js scripts
- made character conversion functions available to js
- added option to hide PC Directory from UPnP renderers
- added album art support, album art is extracted from ID3 tags
  and offered to UPnP devices

### MediaTomb v0.9.1  28.05.2007  Playstation 3 support
- added support for Playstation 3
- added command line option that allows to tell the server where to
  put the configuration upon first startup
- fixed a fseeko-check related bug in configure
- the configure script now honors the LDFLAGS while checking for sqlite3
- fixed wrong message printout in configure
- PC Directory can now be renamed in the UI
- fixed a bug in configure.ac, the --with-extractor-libs= parameter
  didn't work
- fixed a MySQL related bug, if the path or filename contained non-UTF8
  characters, the inserted strings weren't complete
- circumvented a bug/feature of older MySQL versions, that caused the
  MySQL database creation script to fail
- MySQL database version pumped to 2 (MediaTomb will automatically
  update the database during the first launch)

### MediaTomb v0.9.0  25.03.2007  Major rework
- the UI was completely rewritten from scratch; it uses AJAX for all
  requests
- integrated libupnp (http://pupnp.sf.net/) into our source tree
- added largefile support
- code has undergone some performance optimizations
- fixed a bug where server did not shutdown while http download
  was in progress
- all sqlite3 queries are now handled by a single, dedicated
  thread to make MediaTomb work with SQLite3 compiled with
  "--enable-threadsafe"
- fixed a bug where setting -p 0 did not trigger automatic
  port selection if it had to override the value in config.xml
- fixed bug 1425424 - crash on a bad config.xml - we did not handle 
  the case where the <udn> tag was not present.
  Thanks to Nektarios K. Papadopoulos <npapadop@users.sourceforge.net>
  for the report and patch.
- added configure option and adapted the code to completely 
  supress all log output
- added configure option and adapted the code to supress
  debug output
- added taglib support, thanks to Benhur Stein <benhur.stein@gmail.com>
  for the patch. 
- bug 1524468 (startup in daemon mode fails) does no longer occur
  after the integration of libupnp 1.4.x sources
- fixed bug 1292295 (string conversion was failing on 64bit platforms)
- fixed issue with ContainerUpdateIDs (were not sent out along with
  accepted subscription)
- fixed issue where some directories could not be browsed (filesystem 
  browser). It turned out that we forgot to convert the filenames
  to UTF-8, as the result invalid characters made their way into
  the XML that was feeded to the browser.
- improved illegal character conversion handling - in case iconv
  fails we will pad the rest of the name string with "?" and print
  out the failed name in the console.
- we now try to determine the default import charsets by looking
  at the system locale.
- implemented track number metadata extraction (feature request
  1430799). track numbers are now extracted via id3lib/taglib and
  can also be used in the import script. import.js has been adapted
  to add tracknumbers to song names in the Album container.
- added option to supress hidden files when browsing the filesystem
- added X_MS_MediaReceiverRegistrar Service to allow future Xbox 360 
  support
- added workarounds to enable Telegent TG100 avi playback
- server can now be restarted by sending it a SIGHUP
- the current configuration file will never be displayed in the UI and
  will never be added to the database - we do not want the user to
  accidentally share it on the network since it may contain
  security sensitive data
- tomb-install is no longer needed, the server will automatically create
  a default ~/.mediatomb/config.xml file if there is none
- the database tables will be created automatically if they don't
  exist
- adapted configure script to correctly set flags for linkers that
  use the --as-needed option
- the "PC Directory" isn't changeable via the UI anymore to ensure a
  correct view of the "PC Directory"
- lookups in the database are now done with hashes, which should make
  many things faster, especially the adding
- changed the database field "location" from varchar(255) to text to
  allow urls and locations of unlimited size
- created a new theme for the UI
- added support for "blind" .srt requests - some renderers like the
  DSM320 will blindly request the url provided via browse, replacing
  and found file extension with .srt, we will look for the .srt file
  in the directory where the original media is located, using the
  same filename, but with the .srt extension
- created XML schema for "config.xml" to provide the possiblity of
  validating the configuration
- added fallback if js is not available - there is a builtin feature
  to create a default virtual layout now; config.xml has got an
  option to select the virtual-layout type, it can be builting, js or
  disabled.
- added charset option to the import script, it is not possible to
  specify the script encoding
- we now validate filesystem, metadata and scripting charsets upon 
  startup
- added a special option that allows limiting title and description
  lengths in UPnP browse replies to a specified length; this was
  necessary to work around a bug in the TG100 that has problems
  browsing items where title length exceeds 101 characters.
- import.js can now specify the upnp class of the last container in the
  chain
- all items within a container which upnp class is set to 
  object.container.album.musicAlbum will automatically be sorted by 
  track number.
- added option to configure the presentation URL to make it easier
  for NAS vendors to integrate the server into their web UI
- made model number configurable

### MediaTomb v0.8.1	07.09.2005
- added "serve" directory, any files there can be downloaded 
  like from a normal webserver
- implemented keyword extraction for auxdata from libexif
- implemented keyword extraction for auxdata from libextractor
- added Exif metadata support for images via libexif.
- added resolution attribute to image res tag
- added bitrate and duration attribute to mp3 res tag (via id3lib)
- added auxdata field for items, auxilary data can be extracted
  and used by import scripts.
- added configuration options to specify which aux fields should 
  be extracted and filled by the library.
- added mysql support
- refined configure script, almost all dependencies are now
  optional.
- changed database, (not backward compatible again)
- when an item is deleted all referenced items are deleted as well
- added option to specify an alternative magic file

### MediaTomb v0.8.0  15.06.2005  Scripting/Virtual Server Layout
- Server layout can now be defined using java script,
  default layout script is provided.
- Added ID3 tag support.
- Fixed various memory leaks.
- Files in the Filesystem Browser are now sorted alphabetically.
- Added extension to mime-type mappings to the configuration.
- Added option to limit import by file extensions.
- Added daemon mode as well as setuid/setgid options, init.d
  script included.
- Fixed mime-type recognition when importing media (filemagic output
  is now parsed using regular expressions)
- Introduced new log system.
- Adding/Browsing/Removing media in the UI is now handled
  asynchroniously.
- Added a command line option to import media upon server launch.
- Added target directory option to tomb-install, cleaned up a few
  things.

### MediaTomb v0.7.1  17.04.2005  D-Link DSM-320 fix
- Mappings of mime-types to upnp classes are now configurable.
  Actually this is how we solved the DSM-320 problem - they
  crashed if upnp class of items was different than what the
  DSM expected.

- tomb-install now creates the database and configuration file
  from templates (was hardcoded); also sets the name of the server
  individually for each user (host / username)

- Improved handling of configuration, if non critical options are
  missing we are now setting default values.

### MediaTomb v0.7    14.04.2005  First release
- The changelog does not start at day 0, updates will be made
  starting from this release.
