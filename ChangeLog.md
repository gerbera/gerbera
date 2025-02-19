## Gerbera - UPnP AV Mediaserver.

### HEAD

- Add Options to Scripts
- Add Run Scan action in Web UI
- Autoscan: Add missing properties to web UI and database
- Build correct Autoscan Type
- Build with fmt 11.1.3
- Cleanup tree and autoscan commands
- Debian Build for arm64
- Downgrade docker builder as well
- Extract Pixelformat for videos
- Fix album art in builtin layout
- Fix config load from database
- Fix cors host without ipv6 address
- Fix management auf autoscans in list
- Fix update of playlists
- Fix UpnpMap logic
- Focal: Freeze more aws-sdk versions
- Focal: freeze ruby aws-sdk versions used for package upload
- Force Reread of unknown files
- Generate Comment from Metadata During Scan
- Improve logging of string conversion
- JS: Fix config autoscan object in scripts
- Populate playlist item titles
- Refactor static code from ConfigDefinition
- Refactor WebRequests
- Remap metadata fields when parsing virtual-directories keys
- Rework autoscan list handling
- Set more metadata in builtin layout
- Sqlite: Exit after multiple exceptions
- Support Resources for External Items
- UI: Make details collapsable in items editor detail view
- Update Library Versions
- Update to googletest 1.16.0
- Update libexiv2 to 0.28.4
- Update wavpack
- work around build issue

### v2.4.1

- Fix Dockerfile

### v2.4.0

- Access permissions for files by client
- actions: clang-format 18
- Add variables to docker images
- Bump cookie, express and socket.io in /gerbera-web
- Clients: Support deleting entries
- Config: UI handler mimetypes and CSP
- Config: Unify path handling
- ConfigGenerator: Export defaults for more entries
- Debian: create changelog file
- Debian: pack postinst file
- deps: Fix build with ffmpeg 6.1
- Drop Build Support for Ubuntu 23.04
- Dynamic banner colour in Readme
- Enhance xsd
- Fix access to cache option if thumbnailer disabled
- Fix Artist Image in Virtual Layout
- Fix autoscan issue with empty filename
- Fix build without ffmpeg
- Fix CodeQL messages
- Fix config2.xsd: The QName value upnp-properties does not resolve to ...
- Fix container filters
- Fix creating containers in web ui
- Fix Handling command line options
- Fix handling of checkbox values on web ui
- Fix parsing enum config values
- Fix SIMPLE\_DATE
- Fix systemd detection
- Gerbera welcomes 2025
- Handle int parse error in config
- Improve compile info contents
- Improve handling multivalue tags in virtual paths
- Increase size of pupnp threadpool
- Mention systemd as optional package
- Resource directory configuration and cleanup collection
- Scripting: Clean up property handling
- Set correct image size for xx-large pictures
- Test: Fix CMake commands
- Update and refactor MySQL code
- Update Build Environment
- Update build for openSuSE
- Update docker settings
- Update js vendor files
- UPnP filters: provide defaults for filter properties
- UPnP Shortcuts feature
- UPnP: Add support for filters
- Validate XSD CI workflow
- Web UI: Add dark mode
- Web UI: Add Search page
- Web UI: Cleanup display of item number
- Web UI: Fix detail display

### v2.3.0

- Add build target for source documentation
- Add configuration option to block clients
- Add Splitter and Resize to items grid
- Bump webpack from 5.93.0 to 5.94.0 in /gerbera-web
- Clean up commented out code
- Clean up test code
- Clean up URL handling
- Config: Client specific DLNA profiles
- Config: Make UDN mandatory
- Config: Option to disable physical container types
- ConfigSetup: Allow multiple iterations for UI
- Configuration of additional target mimetype properties for transcoding
- Drop Apple Trailer support
- Extract image and video orientation to resource attributes
- Fix config2.xsd
- Fix ipv6 handling
- Fix NPUPnP version
- FIXME done: Remove fake resource
- Import: Fix renaming of file or folder
- Improve docker instructions
- Improve handling of thumbnail profiles
- Inotify: Don't handle 0 events
- Inotify: Fix deleting files
- Link to UPnP specification for search syntax
- Mention .m4a in compilation options
- Refactor StringConverter
- Refactor Upnp Services
- Restrict dynamic folders to browse flags
- Server logging for failed UpnpInit
- Stability fixes
- Upgrade cxxopts.hpp to 3.2.1
- Upgrade md5 to 2002 version
- Upnp Classes for containers in physical tree
- UPnP Search: Support integer and date comparison
- UPnP: Return error messages
- Use full client info as requestCookie
- Warn about empty script result
- Web security and upnp compatibility
- Web UI: Display tootip on containers in tree

### v2.2.0

- Add log sinks for rotating log files and syslog
- Add missing define on ubuntu 18
- Add sample-frequency to config2.xsd
- Adding consistent album artist support
- Build against fmt 11.0.0
- Build support for libnpupnp 6.1.2
- Build: Add install scripts for alpine/docker
- Build: Add install scripts for OpenSuSE
- Build: add older debian
- Build: Allow build of ffmpegthumbnailer without ffmpeg
- Build: rework dependency handling
- Build: Scripts fail again with build
- Build: split debian modules
- Build: Upload for bionic
- Build: Upload for focal
- Build: Use Ruby2.6 for bionic
- Build: Use Ruby2.7 for bionic
- Bump braces from 3.0.2 to 3.0.3 in /gerbera-web
- Bump ws, engine.io and socket.io-adapter in /gerbera-web
- CI: Drop excluded ubuntu versions as CMake PPA now supports those combinations
- CI: Pass publish-deb secrets via env instead
- CI: Upload debs to pkg.gerbera.io
- clang-tidy: don't cast through void
- config2.xsd fixes
- Config: Add flag to disable dynamic descriptions
- Config: Separate config values
- docs: Update Arch Linux installs
- Docs: Update Ubuntu/Debian repo instructions
- Documentation: Update with new WebUI
- Extract command line argument handling to Runtime class
- Extract Content interface
- FFMpeg: add custom logger
- FFMpeg: Reduce logger noise
- Fix crashes on freebsd 14.1
- Fix creation of mr_reg.xml
- Fix database calls for browsing dynamic containers
- Fix discovery
- Fix logger.cc without DEBUG
- Handle broken Systemd on some debian
- Implement updating virtual entries
- Import: Add option case sensitive tags
- Import: fix single file update
- Improve documentation
- Inotify: Handle Inotify access failed
- Insecure download due to ubuntu-20.04/armhf failing
- Libexif: add custom logger
- Logging: Separate description requests
- Metadata: Unify handling and allow to disable
- Playlist: Allow configuration of directory depth
- Search: Add playstatus details to search properties
- Server: Handle all HTTP via virtualdir
- Startup: Always check dirs
- TagLib: Use DebugListener to suppress messages
- UI: Fix auto logout
- UI: Tidy up login and homepage
- UPnP description: Allow dynamic capability values
- UPnP specification files reflect client quirks
- Xml2Json: improve encoding for special chars

### v2.1.0

- Add ctypes include
- Add issue template
- Autoscan: avoid errors from parent directories
- Avoid crash when parent was not created yet.
- Bring back custom headers as client specific headers
- BUGFIX: common.js boxSetup check Video/AllDates for Dates
- Build Support for NPUPNP
- Bump express from 4.18.2 to 4.19.2 in /gerbera-web
- Bump follow-redirects from 1.15.3 to 1.15.4 in /gerbera-web
- Bump follow-redirects from 1.15.4 to 1.15.6 in /gerbera-web
- Bump xml2js and parse-bmfont-xml in /gerbera-web
- clang-tidy and cppcheck fixes
- clang-tidy fixes
- Cleanup FileRequestHandler
- CMake: Use presets
- Config: Refactor handling of integer types
- Correct two typos.
- debian12: fix script error
- Docker image usability
- Docker: update alpine version
- Enhances default javascript files to check config file options
- Expand firewall advice in docs
- Fix build-deb.sh for releases
- Fix findAutoscanDirectory
- Gerbera welcomes 2024
- Gerbera-web: update chromedriver for tests
- github workflows: make cmake stuff consistent
- Import: Safely handle second scan
- Import: Safely handle second scan - 2
- matroska_handler: update API calls to work with libebml/libmatroska 2.0
- MetadataHandler: Refactor static methods
- Metafile: handle instance
- Minor fixes to RST documentation
- Provide BoxKeys also for scripts
- Refactor config and enums to reduce nesting
- Refactor parser files
- Safely handle suppressed file types
- Scripting: print2 function with log level support
- some fixes
- Sqlite: Harden delete
- Take configuration into account when creating virtual-layout (built-in + JS)
- Update Conan to v2
- Update screenshots to latest version
- Update various library versions
- Update workflow actions
- WebUI: Add Item View
- Webui: update vendor libraries

### v2.0.0

 - Add #include to fix building with gcc 14
 - Add artist chronology container
 - Add audio to year container
 - Add cleanup of missing entries to grb import mode
 - Add option for external URL to be used in web page.
 - Add permissions on HV transcoding devices in docker container
 - Add Support for Configuration of Virtual Layout
 - Add support for nomedia file / add options to block default M_DATE
 - Add support for UPnP commands GetFeatureList and GetSortExtensionCapabilities
 - Allow overriding home in config
 - Autoscan: Fix inotify without autoscan
 - Autoscan: Handle renamed directory
 - Autoscan: Use path for handling of non-existing
 - Build for Ubuntu 23.10
 - buildfix: support fmtlib 10
 - Bump @babel/traverse from 7.23.0 to 7.23.2 in /gerbera-web
 - Bump chromedriver from 117.0.3 to 119.0.1 in /gerbera-web
 - Bump engine.io and socket.io in /gerbera-web
 - Bump socket.io-parser from 4.2.2 to 4.2.3 in /gerbera-web
 - Bump ua-parser-js from 0.7.32 to 0.7.33 in /gerbera-web
 - Bump webpack from 5.75.0 to 5.76.0 in /gerbera-web
 - Bump word-wrap from 1.2.3 to 1.2.4 in /gerbera-web
 - Clean up physical entries in subdirectories
 - Clean up unreferenced items
 - Clients: Add detection for FSL radios
 - Clients: Support hiding resource types
 - Config WebUI: Catch up with all config changes
 - Config: Add follow-symlinks for autoscan
 - Config: Add required BoxLayout values from default
 - Config: Add support for time specifications
 - Config: Generate Example Configuration
 - Database items sorting case insensitive
 - DB rework playstatus save
 - DB: Don't fail on uncritical operations.
 - debian: bookworm is now stable
 - Display message on home screen when database is empty
 - Doc: Compile libupnp --disable-blocking-tcp-connections
 - Docker: Add JPEG libs
 - Docker: git badge update
 - Document dependency installation on Debian 12
 - Fix "virtual-directories" heading level in documentation
 - Fix conan
 - Fix for empty path
 - Fix handling transcoding requests
 - Fix import and documentation links
 - Fix processing for M_DATE and M_CREATION_DATE on FFmpeg handler.
 - Fix spelling errors reported by lintian
 - Fix troff warning
 - Fully implement Thumbnail handling for grb mode
 - Gerbera-Web: Update npm packages
 - Import: Add staged import process
 - Import: Fix LastModified for grb-mode
 - Import: Handle really short file names correctly
 - Import: Icon handling and other leftovers
 - Import: Improve handling of thumbnails in mt-mode
 - Keep our unique_ptrs for the xml in scope until we are finished with …
 - Playlist: Add support for boxlayout
 - Quirks: Check for clientInfo
 - README: fix CI badge
 - Restore duktape 2.3 support
 - Rework javascript mechanism
 - Samsung: Handle browse for content class correctly
 - Scripting docs: Fix importFile parameter name
 - Scripting: Log stacktrace on errors
 - Scripting: Remove debugging leftover
 - Set defaults for autoscan settings
 - Transcode: Wildcards for mime type filter
 - Transcoding: Improve docs and examples
 - Transcoding: option to filter mime types with wildcard
 - Update build for libexiv2
 - Update Documentation
 - Update Library Versions and Documentation
 - Update README.Docker.md -Add docker volume section
 - Update supported-devices.rst
 - Update to latest npm packages
 - Update Ubuntu Version
 - Update version of libfmt
 - Update versions of exiv2, fmt, spdlog, googletest and taglib
 - Upnp: Add client flag to send simple date only
 - Use new ffmpeg channel layout API
 - WebUI: Database View - don't allow deleting dynamic containers
 - WebUI: Fix display of time values
 - WebUI: Update JS libs
 - WebUI: Update popper to 2.11.6  / Update MD5 to 2.19.0

### v1.12.1

- Actions: Update deprecated replace-string-action
- Bump engine.io from 6.2.0 to 6.2.1 in /gerbera-web
- Bump loader-utils from 2.0.2 to 2.0.3 in /gerbera-web
- Clarify integer types handling
- Debug Options: Allow separated facility debugging messages.
- Docker: Bring back ffmpeg in transcoding image
- Docker: fix ffmpeg-lib
- Docker: Update to alpine 3.17
- Fix file encoding
- Fix Windows Explorer browsing
- Gerbera 2023
- libraries: add latest
- MetadataHandler: Use resource instead of id
- Minimal: fix test fixture
- openSuSE: Fix test and inits
- Reduce Header Nesting
- Samsung: allow configuring bookmark offset
- Server: Don't call handler if null
- Update npm packages
- XML: Allow escaping ticks for some clients

### v1.12.0

 - actions: use alpine 3.16 container
 - Add cache for hostname
 - add deleted copy functions for WavPackHandler
 - Add edge tag to docker workflow
 - Add files sizes to server stats
 - Add support for ip subnets in client config
 - Autoscan: Support configuration of upnp class of virtual containers
 - Bump jquery-ui from 1.13.1 to 1.13.2 in /gerbera-web
 - Bump terser from 5.13.1 to 5.14.2 in /gerbera-web
 - clang-tidy: const ref to value
 - clang-tidy: const string ref conversion
 - clang-tidy: don't use else after return
 - clang-tidy: use data() instead of pointer magic
 - Cleanup tasks on removeObject
 - CodeQL: Update version
 - const member functions
 - Coverity fixes
 - database: clearFlags use bit operation
 - Debian: fix unstable build
 - Docker: Improve debugging build
 - Docker: Update to alpine 3.16
 - Drop scan_level on update to DB 19
 - Fix 2663 No album sub-containers per artist created in "builtin"
 - Fix build on systems that have const in second argument to iconv
 - Fix build with fmt 9.0
 - fix compilation with latest exiv2 master
 - Fix npm vulnerabilities
 - Fix removing flags in ui
 - Fix static init
 - Fix: Remove duplicate open breaking url transcoding
 - Handle deleted objects in sqlite queue
 - Handle Promise rejection
 - Import: fix layout and performance issues
 - Improve creating external links
 - Improve error handling for Resolution
 - Limit number of dynamic container entries
 - Minor fixes
 - more sonarlint
 - Path handling cleanup
 - Quirks: Avoid passing references
 - Read Metadata from external files (asx playlist and NFO)
 - remove const from member variables
 - Remove dropped ubuntu versions
 - replace some memcpy by std::copy_n
 - Saveguard Samsung Quirks
 - Simplify Xml2Json
 - sonarlint fixes
 - Sqlite: Configuration for database modes
 - Support tweaking mimetypes for client
 - Systemd: Add wants entry
 - This and that
 - Transcoding for external items
 - Update node packages
 - Update versions
 - use fmt 9.0.0
 - web: update jquery-ui to 1.13.2
 - Workflows: Update actions

### v1.11.0

- Add Clients to database
- Add item class filter and mapping to autoscan directory
- Add MetaData 'dc:data' to all Item and container
- add missing moves
- Add Quirk to block filename in item uri
- add throw_fmt_system_error define
- Allow agent to not exist if profile is disabled.
- Autoscan: fix out of range issue
- Autoscan: Tidy validation logic, nest enum
- bool simplifications
- Build Ubuntu 22.04
- Bump minimist from 1.2.5 to 1.2.6 in /gerbera-web
- change read/write functions to use std::byte
- Change ResourceContentType to Resource::purpose
- CI: build-deb: Pass API key to docker env
- CI: build-deb: Pass API key to docker env, again
- CI: Debian: whitelist /build
- CI: Docker local cache, only buld AMD64 for PRs
- clang-tidy
- clang-tidy fixes
- clang-tidy: add missing variable name in declaration
- clang-tidy: add special member functions
- clang-tidy: fix case style of class/structs
- clang-tidy: fix wrong erase
- clang-tidy: member function const
- clang-tidy: pass by value
- clang-tidy: redundant init
- clang-tidy: remove defaulted constructors
- clang-tidy: replace push with emplace_back
- clang-tidy: use at() instead of []
- clang-tidy: use braced init list
- clang-tidy: use move
- Config: Autoscan list to plain vector
- ContentHandler to enum
- ContentManager: Single autoscan list
- convert function to static
- doc/install.rst: add Buildroot
- doc/install.rst: drop Entware
- Don't store timestamps in future
- Ensure search sort by title
- FFMpegThumbnailer: Enable by default, improvements
- Find DNLA profiles by resource attributes
- fix bad URL
- Fix Bitrate formatting, populate bitsPerSample
- Fix built in JPEG resolution parsing.
- fix compilation
- Fix compilation of jpeg_resolution.cc
- fix compilation with npupnp
- fix declaration
- Fix playlist parser error
- Fix search-item-result regression.
- Fix sorting by composed keys
- Fix tag matches with ffmpeg_handler
- fix wrong variable name
- Fix: Repair broken Mysql statements
- get rid of make_pair
- Handle config sourced autoscans in DB more gracefully
- Improve readability of attributes
- Increase DukTape version
- manual move conversions
- manual unique_ptr removals
- Minimal: fix test
- move make_shared into function
- pass shared_ptr by value
- pass std::string by value
- Playlist: Handle end of file properly
- README: Bump required npupnp version
- Refactor handling of network addresses
- remove const from map
- Remove DB field from autoscan list
- remove IOHandler unique_ptrs
- remove pointless move
- remove pointless parentheses
- remove reference parameter
- remove unused includes
- remove unused upnp header
- rename to_seconds to toSeconds
- replace bzero with {}
- ResourceAttribute new style enum
- Respect ContainerID when performing search
- Restore friendly name in UI
- Revert "fix bad URL"
- Server: Clean up virtualURL handling
- simplify bool expression
- simplify some bools
- Sort containers first
- Split out URL handling methods to UrlUtils namespace
- Store Playback Status in new table
- string to string_view conversion
- Support filtering transcoding profiles
- Support parsing function for config values
- Support searching playlists containers
- ThreadRunner: Drop system-threads config
- Try to fix paths-ignore
- UI Prettier Modals, badges
- unused parameter for throw
- Update Duktape version for bookworm
- use empty()
- use to_integer
- value to ref conversions
- WavPack: Get metadata with original library
- webkit style for transcoding
- WebTests: Update node packages

### v1.10.0

- Add all metadata to search capabilities
- add another using declaration
- Add configuration for extensions to ignore on import
- add const for getter functions
- add const to get functions
- add const to various get functions
- add explicit
- Add flag to hide dynamic content on Samsung devices
- add mising header for size_t
- add missing defines
- Add mutext to lock access to layout
- add parentheses to macros
- Add screenshots for grid view
- Add support for ASX playlists
- Add two moves.
- Allow IPs in request validation
- Allow setting environment variables in transcoding sub processes
- Also display autoscan badge in filesystem view
- append with fmt
- Attempt to close leak
- Avoid locking mutex for too long
- avoid some copies
- avoid some shared_ptr copies
- avoid some shared_ptr copying
- Avoid storing reference to node
- avoid using new for a char array
- Batch-execute multiple getChildCount queries
- Bleeding edge
- Block negative track numbers
- Block XML Declaration for IRadio based devices
- Bring back selection of items per page on items page
- BufferedIOHandler: Fix random exceptions
- Build armv7/arm64 deb images
- Build libupnp in Dockerfile
- Bump follow-redirects from 1.14.7 to 1.14.8 in /gerbera-web
- Bump jquery-ui from 1.12.1 to 1.13.0 in /gerbera-web
- Bump JS libs
- Bump karma from 6.3.13 to 6.3.14 in /gerbera-web
- C++ cast
- cast to proper type
- CdsContainer: Comment out overriden function
- change checkResolution to return a pair
- change class to struct
- Change compare order for to IPV6 then IPV4
- CI: Bump CI to clang-format-13
- clang-tidy: pass by value
- clang-tidy: renames to WebKit style
- clang-tidy: run through readability checks
- clang-tidy: use WebKit style for ffmpeg variables
- clang-tidy: various renamings
- CMake: UUID target/MacOS fixes
- Code cleanup for #2033
- comment fixes
- comment out some unused code
- Conan: Dont install CMake
- Conan: Use taglib package from conan
- const auto ref conversion
- const exiv2 exception
- const pointer
- const ref conversion
- content manager changes
- convert all enum members to upper case
- convert all usages of toCString to to8Bit
- convert double pointer to pointer ref
- convert function to static
- convert map to simple array
- convert std::array to normal array
- convert string to fs::path
- convert string to fs::path
- copy shared pointer instead of passing reference
- cppcheck fixes
- deb: Drop Ubuntu 20.10, add 21.10
- default init some shared pointers
- default initialize some fields in the constructor
- Delay loading of status after login
- Delay release of ixmlDocument
- deque removal
- Display status details on home page
- Docker: Add transcoding tag
- Docker: Bump alpine to 3.14
- Docker: Enable github actions based layer caching
- Docs: sphinx 4.2, pin versions, fix warnings
- Document the special role of "PC Directory"
- Don't iterate on different copies
- don't use else after return
- Dont trigger play hooks for metadata resources
- Drop resourceHandler concept
- Drop servedir feature.
- Drop SopCast "support"
- early exit
- Enable colour log levels
- exiv2: silence messages without debug
- Feature/dockerfile
- ffmpeg: const additions
- FFMpegThumbnailer: Split into its own handler
- find_if conversion
- Fix bad static_pointer_cast transformation
- fix bugs in addFfmpegMetadataFields()
- fix definition of M_CONTENT_CLASS
- fix deprecated copy warning
- Fix detection of content type for thumbnails
- Fix display of container art
- Fix focal build
- Fix for fmt > 8.0
- Fix function to change items per page
- Fix join statement for multiple metadata search
- Fix library links in compilation docs
- fix memory leak
- fix mismatching declaration
- Fix npm audit messages
- Fix playlist numbering
- fix some implicit conversion warnings
- fix subtitle type value
- Fix transcoding regression
- fix variable names to WebKit style
- fix wrong case
- fix wrong cast
- fix wrong macro comments
- fmt c++20 fixes
- follow rule of zero with some classes
- Free EBML memory
- Free libMatroska memory
- Free XMP namespaces
- fs::path conversion
- get logfile from command line
- get rid of static_pointer_cast
- grammar fixes
- grb_fs: run through clang-tidy
- Handle path comparison correctly
- Happy New Year
- https
- Implement libupnp hostname validation
- Implement sort on metadata
- Import script hints
- Improve DLNA compatibility
- Improve matroska parsing speed
- Improve support for Samsung UPnP extension X_GetFeatureList
- Improve upgrade file checks and messages
- Initialize builtin layout properly
- JS: add array check to getArrayProperty
- lambda conversion
- Layout: remove unused profiling code
- make all mutexes mutable
- make maps and vectors const
- make startswith constexpr
- manual header removals
- matroska_handler: default init activeFlag
- Mention "gerbera --compile-info" on compilation docs
- merge some if statements
- merge various if statements
- Minor UI Improvements
- Misc fixes
- more unneeded headers
- Move DB defines out of common
- move filesystem declaration after headers
- Move filesystem function to grb_fs
- move several methods out of line
- move std::function stuff
- Multiply out sample rate reported by FFMpeg
- MySQL fix warnings with glibc 2.34, pass a real pointer
- mysql: rename variables based on WebKit style
- Only set DNLA headers for resources we know about
- Optimize Ffmpeg handler
- Optimize SQL Statements / result gathering
- Optimize the use of references
- pair conversion
- pass by value
- pass shared_ptr constructors by reference
- Path to map rework, handle empty values
- place it variable in lambda
- pointer to reference conversions
- Process: misc tidy ups
- ProcessExecutor: small std::array conversion
- Provide documentation for file types, metadata and playlists
- random sonarlint cleanups
- readability clang-tidy fixes
- Reduce Docker image size
- reduce lambda size slightly
- ref conversion
- Refactor file_request_handler
- Remove .c_str() for fmt-string arguments
- remove cast with make_shared
- remove const from several unique_ptrs
- remove custom destructor
- remove custom merge function
- remove dead assignment
- remove defaulted parameter.
- Remove duplicate logic from file_request_handler
- remove extra semicolon
- remove FileIOHandler destroctor
- remove get() calls and match types
- remove initializer_list usage
- remove mimetype parameter
- remove not really used bool
- remove old libupnp compatibility
- remove old PRETTY_FUNCTION define
- remove pointless move
- remove pointless reference
- remove pointless void cast
- remove random header
- remove redundant base class init
- Remove root from container path
- remove several unused functions
- remove some C arrays
- remove some chrono code duplication
- remove some chrono includes
- remove some more headers
- remove some uninitialized variables
- remove some unused macros
- remove some {}.
- remove stat include
- remove string_view quote
- remove time_t stuff
- Remove trailing slash from URL
- remove unique_ptr
- remove unique_ptr from tests
- remove unused class
- remove unused enum
- remove unused friend declaration
- remove unused optional header.
- remove unused parameter
- remove various shared_ptr copies
- rename const variables to WebKit style
- Rename session cookie to GerberaSID.
- replace all lock_guard with scoped_lock
- replace C function pointer with std::function
- replace callbacks with lambdas
- replace constructor with using
- replace constructors with using declarations
- replace copy with move
- Replace double-pointer out-param with return value
- replace fatalHandler with lambda
- replace findinotify with KDE's version
- replace for loop with while
- replace insert with emplace
- Replace removed app.add_stylesheet() by app.add_css_file()
- restore log level to debug
- return function directly
- return pair instead of by reference param
- return std::pair for stripLocationPrefix
- Reuse Request Handler
- Revert "replace all lock_guard with scoped_lock"
- Revert "silence exiv2 warnings"
- Rework creation of containers
- rework for loop
- run clang-format on test files
- Search title in metadata instead of file name
- Send alive messages every 60s by default
- set by move
- Set refId for playlist container
- Set video codec for artwork resource if media is an audio file
- several duktake type fixes
- shared to unique_ptr conversions
- Show numbering of items in UI
- Show subdir of recursive autoscan in directory tree
- silence exiv2 warnings
- simplify bool
- simplify loop
- Simplify makeFifo, unify with curl and tidy unused
- simplify shared_ptr assignment
- simplify some if statements
- simplify some static_pointer_cast
- small typo
- some stuff
- static function conversion
- static in front
- std::string_view conversions
- Support for multiple entries in genre
- Switch staticThreadProc to lambda
- Taglib: Handle OGG containing Opus, Speex or FLAC
- test: apply the same strlen removal as main.cc
- test: Increase gtest discovery timeout
- test: some more make_unique conversions
- test: Use correct option for test discovery timeout
- test: use make_unique instead of new
- tests: remove various using namespace
- test_upnp_headers: init changes
- Thumbnails for images and grid view for items
- Tools optimizations
- turn vector to array
- UI: Hide more when not logged in
- UI: Tidy homepage
- unique_ptr conversions
- unused enum
- unused includes
- Update fmt version
- Update man page and help text to reflect the current situation
- Update node modules to fix security issues
- use auto
- use auto with cast
- Use better primary key for grb_cds_resource
- use braced init list
- Use correct method to set attribute
- use dynamic cast for derived classes
- use lambda and fs::path
- use operator < instead of startswith
- use operator< instead of creating temporaries
- use some type deduction
- use startswith
- use std::byte
- use std::function
- use std::pair for checkFileAndSubtitle
- use to_string for bool
- use underlying_type_t
- Validate bound port
- various fixes
- Web logo link
- Web: remove unused, duplicate code. Rename check_request
- WebUI: bump bootstrap to 4.6.1, delete non min files

### v1.9.2

- replace POSIX remove with fs::remove
- Don't erase from empty vector
- replace some {} with ()
- Make title of search result configurable + set searchable container flag
- fix lastfm compilation
- small fmt::format conversion
- clang-tidy
- Handle multi-valued meta data
- Refactor TranscodeExternalHandler
- Modernize parts of the request handling with C++17 and add UTs
- std::find conversion
- static_cast conversions
- Canonicalize path from playlist file
- Rewrite split string
- Update command generator method
- std mem conversions
- Unify simple SQL queries
- std file conversions
- capture several lambda refs by value
- Replace stdio with fmt calls
- Simplify AddUpdateTable vectors without shared_ptr
- replace several substr calls with startswith
- use std::int32_t
- Allow adding metadata from media file tags
- replace substr with startswith
- use std::exit
- use std::abort
- Fix regressions of 1.9.1
- Downgrade libspdlog dependency to 1.8.1
- Update instructions to match CMake requirements
- use std::system
- use std::strlen
- change size_t to std
- const ref conversion
- more emplace conversions
- taglib: demote warning to debug
- rvalue conversions
- move variables down
- clang-tidy: remove redundant c_str
- simplify it loop slightly
- remove dead assignments
- clang-tidy: performance fixes
- fix wrong value being assigned
- remove std::insert
- replace loop with std::find
- simplify loop
- Only count items with distinct IDs in TotalMatches search result.
- Optimize CdsObject creation/updates
- Ensure Root dir teminates with /
- Update build scripts to run on Raspbian
- fix some weird memory leak
- remove last std::list
- remove some lambdas
- std::shared_ptr<Executor> value conversions
- add std::move for some maps
- use auto for Timer constructors
- rename several shadowed variables
- use auto
- splitUrl: use std::string
- remove sort of redundant std::move
- use string::npos
- use C++ macro to check for starts_with
- trimString changes
- use C++ macro to check for to_underlying
- Add Composer Tag to Metacontent Handler
- Use lambda in UpnpXMLBuilder::renderObject to trim strings
- gitignore: Add buildconfig
- use auto with constexpr
- remove unused variables
- replace pointers with references
- remove unique_ptr casts to base class
- some matroska cleanups
- remove return after throw
- remove pointless size_t casts
- add maybe_unused for builds without DEBUG
- Refactor database schema to remove unused indices
- Remove unique_ptr objects where stack memory is sufficient
- Ensure EnumMapper creates its own copies
- don't pass string_view by reference
- default init mode
- don't assign nullptr to std::string
- replace find_if with any_of
- move getLocation call down
- Batch inserts to metadata table
- Revert "use targets for libmatroska and libebml"
- Update README.md
- update FindFilesystem.cmake
- convert reference to value
- use targets for libmatroska and libebml
- remove optional check
- use updated FindLibExiv2
- use normal libcurl search
- CamelCase changes
- Conan: Require CMake >=3.18
- Fix FreeBSD with libinotify, add to CI
- use updated FindTagLib.cmake
- const ref conversion
- Simplify or remove unnecessary fmt::format calls
- remove unused assigned values
- remove unused variable
- cppcheck: add const
- default init some members
- cppcheck: fix inconsistent declarations
- avoid nullptr assignment

### v1.9.1

- use const auto&
- replace for_each with copy_if
- avoid default nullptr initialization
- remove some string calls
- make shared_ptr reference const
- remove pointless empty line
- get rid of auto&& with structured bindings
- simplify bool
- some auto&& removals
- add missing std::vector
- string_view should not be taken by reference
- Build debian with arguments
- libfmt formatters for quoted SQL identifiers
- Read flac audio properties even if image does not exist
- Split CI validation jobs
- Fix linkage error in on aarch64 with g++-10
- match return type
- Add support for default values in config UI
- improve debian script
- Use correct codename for unstable debian
- debian::unstable uses libduktape206
- add PathBase constructor
- use some auto
- simplify while loop slightly
- avoid doing work in if statements
- manual const conversions
- clang-tidy: make member function const
- more move with push_back
- use more auto in taglib
- get rid of unused variable
- pass std::string by const reference
- add variout maybe_unused
- change to auto&&
- Add validation for DynamicContent::location
- pass ClientInfo by unique_ptr
- add ClientCacheEntry constructor
- use some auto and CTAD
- remove pointless to_string
- remove single argument std::string
- convert vector to deque
- replace several emplace_back with push_back
- convert Quirks to unique_ptr
- Refresh Clients config in ClientList after change of config in UI
- Code refactoring and performance enhancements in sql_database
- convert vector to deque
- rvalue reference conversions
- several auto conversions
- Add M3U8 support
- remove shared_ptr from vector
- test cleanups
- get rid of ClientCacheEntry pointer
- fix bad unique_ptr usage
- Make UpnpXMLBuilder::orderedHandler nonstatic
- Fix mapConfigOption return
- {} conversions
- prefer xml-node children over xpath
- add missing move
- basic clang tidy
- replace several inserts with std::copy
- clang-tidy: use emplace_back
- use std::string_view in if statements
- remove const char version of quote
- use auto and CTAD
- Update required versions for spdlog and libfmt
- Use fixed spdlog and fmt in all debian systems
- use newer spdlog and fmt for debian
- Add dynamic containers setup
- fmt disallows string_view as format specifier
- Change generation of SQL statements to format
- final unique_ptr removals
- Fix broken upgrade script
- const ref conversions
- move unique_ptr removals
- emplace_back conversions
- const ref conversions
- replace temp variable with returns
- Update Dockerfile
- return string instead of const char

### v1.9.0

- fix wrong define
- use auto instead of const char*
- move several variables down
- unique_ptr removals
- getTimespecAfterMillis: return ret instead of void
- Generate namespace attributes required for properties
- replace reference parameter with std::pair
- clang-tidy 8 fixes
- Allow setting resource order
- remove pointless {}
- use map.emplace
- remove some = {}
- use thread constructor
- Re-add CI check with clang
- Send container updates
- fixup lambda for C++20 compatibility
- lastfm: switch to C++ API
- move make_unique down
- use move with shared_ptr
- make uiUpdateIDs a unique_ptr
- remove pointless const_cast
- Factor out transactions to reduce overhead when disabled
- Fix X_GetFeatureList (Samsung)
- some move
- move make_shared outside of initializer list
- switch xmlDoc to unique_ptr
- remove ret variable
- lambda conversion
- more unique_ptr
- some CTAD
- remove pointless temporary
- small cleanups
- Add more documentation and cleanup SQL init code
- remove wrong static
- Move resources to separate table
- several string_view conversions
- convert expandName to string_view
- Add link to database doc
- Add migration hook to version update
- Automatically load options
- use make_pair in std::array
- remove std from std::next
- remove aslowercase
- pass by value
- Add DLNA profiles strings and visible file system directories to configuration
- don't default assign nullptr to smart pointers
- add missing const
- remove == nullptr
- Add ProductCap to please Samsung TVs
- simplify pidfile write
- Move db upgrade commands to config file
- clang-tidy: remove implicit bool conversions
- Add codec info to resource data
- Subtitle: Add resouce and CaptionInfoEx
- remove pointless constructor
- clang-tidy: don't use else after return
- several constructor changes
- static
- change length parameter to size_t
- add missing header
- Implement EnumIterator
- SQLDatabase: Refactor init and upgrade
- const member function conversions
- clang-tidy: add missing functions
- add a missing this->
- pass 0 to std::unordered_
- replace std::list with std::vector
- remove manual loop
- remove pointless find
- small lambda conversion
- Regression: Samsung Compatibility
- switch to C++ ffmpegthumbnailer API
- cosmetic map changes
- remove two pointless unique_ptrs
- declare AVFormatContext as struct
- default init some members
- Fix MySQL migration
- switch several for loops to use size_t
- replace POSIX file stuff with C
- Implemement dynamic containers
- small error handle
- replace several usages of format with to_string
- remove several {}
- don't throw in noexcept destructors
- Fix lastfm compilation
- use make_pair
- use a unique_ptr in lambda
- nppnp changes
- Close memory leak by duplicate call to ixmlCloneDOMString
- replace rand with std::random
- server: default init some variables
- fix std::accumulate
- add missing default initialization
- default initialize some io stuff
- add missing close
- add missing nullptr check
- default initialize members in mysql
- Simplify and cleanup
- string ref to string_view conversion
- Cleanup exifhandler
- Assign clientInfo in a default constructor
- pass by value and std::move
- use sizeof for snprintf
- Add configuration option for SopCast mimetypes
- fix replace string functions
- lambda conversion
- clang-tidy: remove redundant specifiers
- clang-tidy: use auto
- clang-tidy: simplify boolean expression
- Configuration of folders for resources
- remove const so move can be used
- replace const static with static const
- default member init
- use std::replace instead of replaceAllString
- replace stringHash with single accumulate call
- remove pointless blank lines
- match else and if blocks
- remove duplicate include
- use raw strings for multi line ones
- Dockerfile: add tzdata
- more make_unique changes
- make bultinClientInfo constexpr
- add m4a support for taglib
- add back several defaulted destructors
- avoid unused template warning
- clang: add missing move
- default init some members
- Cleanup legacy code
- Clean up a bit
- Cppcheck
- Add documentation and template for Apache/NGinx as reverse proxy
- Fix script syntax
- clang-tidy applied to tests
- Add build for ubuntu 21.04 hirsute hippo
- create-config: Reset config dir to empty if not in command line
- TagLib: Add support for aux data to all supported media types
- Haiku patches
- clangh-tidy: non const reference removal
- manual structured binding conversions
- clang-tidy: pass unique_ptr by value
- clang-tidy: get rid of some long and short
- clang-tidy: fix some narrowing conversions
- clang-tidy: use auto&& to avoid warning
- clang-tidy: C to C++ headers
- clang-tidy: initialize some members
- Refactor SearchHandler to use ColumnMapper instead of hard coded texts.
- mostly unique_ptr changes
- move some initializations up
- Revert "initialize several unique_ptrs"
- initialize several unique_ptrs

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
