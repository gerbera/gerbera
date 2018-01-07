.. gerbera documentation master file, created by
   sphinx-quickstart on Sat Dec 23 17:22:17 2017.
   You can adapt this file completely to your liking, but it should at least
   contain the root `toctree` directive.

Welcome to gerbera's documentation!
===================================

.. toctree::
   :maxdepth: 2
   :caption: Contents:



Indices and tables
==================

* :ref:`genindex`
* :ref:`modindex`
* :ref:`search`

Introduction
============

MediaTomb is an open source (GPL) UPnP MediaServer with a nice web user interface, it allows you to stream your digital media through your home network and listen to/watch it on a variety of UPnP compatible devices.

MediaTomb implements the UPnP MediaServer V 1.0 specification that can be found on http://www.upnp.org/. The current implementation focuses on parts that are required by the specification, however we look into extending the functionality to cover the optional parts of the spec as well.

MediaTomb should work with any UPnP compliant MediaRenderer, please tell us if you experience difficulties with particular models, also take a look at the Supported Devices list for more information.

**WARNING!**

The server has an integrated file system browser in the UI, that means that anyone who has access to the UI can browse your file system (with user permissions under which the server is running) and also download your data! If you want maximum security - disable the UI completely! Account authentication offers simple protection that might hold back your kids, but it is not secure enough for use in an untrusted environment!

Note:
    since the server is meant to be used in a home LAN environment the UI is enabled by default and accounts are deactivated, thus allowing anyone on your network to connect to the user interface.

Currently Supported Features
----------------------------

-  browse and playback your media via UPnP

-  metadata extraction from mp3, ogg, flac, jpeg, etc. files.

-  Exif thumbnail support

-  user defined server layout based on extracted metadata (scriptable virtual containers)

-  automatic directory rescans

-  sophisticated web UI with a tree view of the database and the file system, allowing to add/remove/edit/browse your media

-  highly flexible media format transcoding via plugins / scripts

-  allows to watch YouTube(tm) videos on your UPnP player device

-  supports last fm scrobbing using lastfmlib

-  on the fly video thumbnail generation with libffmpegthumbnailer

-  support for external URLs (create links to internet content and serve them via UPnP to your renderer)

-  support for ContentDirectoryService container updates

-  Active Items (experimental feature), allows execution of server side scripts upon HTTP GET requests to certain items

-  highly flexible configuration, allowing you to control the behavior of various features of the server

-  runs on Linux, FreeBSD, NetBSD, Mac OS X, eCS

-  runs on x86, Alpha, ARM, MIPS, Sparc, PowerPC

Requirements
============

Note:
    remember to install associated development packages, because development headers are needed for compilation!

Note:
    libupnp is now a part of MediaTomb and does not have to be installed separately. We base our heavily patched version on libupnp 1.4.1 from http://pupnp.sf.net/

Note:
    you need at least one database in order to compile and run MediaTomb - either sqlite or mysql.

In order to compile MediaTomb you will have to install the following packages.:

-  sqlite (version > 3.x) http://www.sqlite.org/ *REQUIRED (if mysql is not available)*

-  mysql client library (version > 4.0.x) http://mysql.org/ *REQUIRED (if sqlite is not available)*

-  expat http://expat.sourceforge.net/ *REQUIRED*

   Expat is a very good and robust XML parser, since most of UPnP is based on XML this package is a requirement.

-  zlib http://www.zlib.net/ *OPTIONAL, HIGHLY RECOMMENDED*

   Zlib is a compression library that is available on most systems, we need it for the database autocreation functionality. Make sure to install the zlib development package providing zlib.h, if it is not available you will need to create the MediaTomb sqlite3/MySQL database manually.

-  libmagic *OPTIONAL, RECOMMENDED*

   This is the 'file' package, it is used to determine the mime type of the media. If you don't have this you will have to enter file extension to mime type mappings manually in your config file.

-  js - SpiderMonkey JavaScript Engine http://www.mozilla.org/js/spidermonkey/ *OPTIONAL, RECOMMENDED, REQUIRED FOR PLAYLIST SUPPORT*

   This package is necessary to allow the user defined creation of virtual containers. The import.js script defines the layout of your media, the default import script will create a structure sorted by Audio/Photo/Video, it will make use of the gathered metadata (like ID3 tags) to sort your music by Artist/Album/Genre/Year , etc. The import script can be adjusted and modified - it allows you to create the layout that you want.

-  taglib http://developer.kde.org/~wheeler/taglib.html *OPTIONAL, RECOMMENDED*

   This library retrieves metadata from mp3, ogg and flac files. You will need it if you want to have virtual objects for those files (i.e. nice content layout).

   Note:
       It makes no sense to use taglib and id3lib at the same time, the configure script will first look for TagLib, if TagLib detection fails it will search for id3lib. You can also force the configure script to take the library of your choice, overriding the default setting.

-  id3lib http://id3lib.sourceforge.net/ (at least version 3.8.3) *OPTIONAL, RECOMMENDED (if TagLib is not available)*

   This library retrieves id3 tags from mp3 files.

-  libexif http://libexif.sourceforge.net/ *OPTIONAL, RECOMMENDED*

   You will need this library if you want to extract metadata from images, this will allow you to have virtual containers for your Photos, sorted by various attributes like Date, etc. It also enables thumbnail support: if EXIF thumbnails are present in your images they will also be offered via UPnP.

-  curl http://curl.haxx.se/ *OPTIONAL, REQUIRED FOR YOUTUBE AND SOPCAST SUPPORT*

   curl is a library that allows to easily fetch content from the web, if you want to compile MediaTomb with YouTube and/or SopCast support then curl is required.

-  ffmpeg http://ffmpeg.mplayerhq.hu/ *OPTIONAL*

   Currently ffmpeg is used to gather additional metadata from audio and video files.

-  libffmpegthumbnailer http://code.google.com/p/ffmpegthumbnailer/ *OPTIONAL*

   ffmpegthumbnailer is used to generate video thumbnails on the fly. If your device (like DSM-510 or PlayStation 3) supports video thumbnails it would be worth to compile MediaTomb with this library.

   Note:
       ffmpegthumbnailer support is only provided if MediaTomb is compiled with ffmpeg support.

-  lastfmlib http://code.google.com/p/lastfmlib/ *OPTIONAL*

   last.fm scrobbing

In order to use the web UI you will need to have javascript enabled in your web browser.

The UI has been tested and works with the recent versions of :

-  Firefox/Mozilla

-  Opera

Tested and does not work with the recent versions of:

-  Konqueror

-  Safari

Limited functionality with:

-  Internet Explorer 6 and 7

Compiling From Source
=====================

Standard Method
---------------

If you don't care about the details - make sure you have installed the required packages and the appropriate development headers and simply run

::

    $ ./configure
    $ make
    $ make install

This should compile and install MediaTomb, the resulting binary is ready to run.

Note:
    if you checked out the sources from SVN the configure script will not be available, you will have to create it with the following command:

::

    autoreconf -i

Configure Options
-----------------

The MediaTomb configure script provides a large variety of options, allowing you to specify the additional libraries that will be used, features that will be compiled or disabled, workarounds for known bugs in some distributions and so on. Some options are straightforward, some require deeper knowledge - make sure you know what you are doing :)

Install Location Of Architecture Independent Files
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

::

    --prefix=PREFIX

Default:
    /usr/local/

Install all architecture independent files - all .js files and .png images for the Web UI, the import.js script, the service description XML files - in the directory of your choice. This is especially useful if you do not want to perform a system-wide installation, but want to install MediaTomb only for your user.

Important:
    the prefix path will be compiled into the binary; the binary will still be relocatable, but you move those files you will have to point MediaTomb to the proper location by specifying it in the server configuration file.

Static Build
~~~~~~~~~~~~

::

    --enable-static

Default:
    disabled

Build a static binary. This may be useful if you plan to install a precompiled MediaTomb binary on a system that does not have all the required libraries and where installation of those libraries is not possible due to reasons beyond your control.

Note:
    if you enable this option, make sure that you have all static versions of the appropriate libraries installed on your system. The configure script may not detect that those are missing - in this case you will get linker errors. Some distributions, for example Fedora Core, do not ship static library versions.

Automatically Create Database
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

::

    --enable-db-autocreate

Default:
    enabled

Automatically create the database if it is missing (for example upon a first time launch). Disabling this will make the resulting binary a little smaller, however you will have to take care of the database creation yourself by invoking the appropriate .sql scripts that are provided with the package.

Note:
    the server configuration file has to be setup correctly. Either sqlite or MySQL has to be chosen in the storage section, for sqlite the database file has to point to a writable location, for MySQL the user has to be setup with a valid password and permissions and the database ”mediatomb” has to exist.

Debug Malloc/Realloc Of Zero bytes
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

::

    --enable-debug-malloc0

Default:
    disabled

This feature is only for debugging purposes, whenever a malloc or realloc with a value of zero bytes is encountered, the server will terminate with abort()

Force Linking With The Pthread Library
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

::

    --enable-pthread-lib

Default:
    disabled

We use the ACX\_PTHREAD macro from the autoconf archive to determine the way how to link against the pthread library. Usually it works fine, but it can fail when cross compiling. This configure option tells us to use -lpthread when linking, it seems to be needed when building MediaTomb under Optware. Note, that using --disable-pthread-lib will not prevent automatic checks against the pthread library.

Force Linking With The Iconv Library
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

::

    --enable-iconv-lib

Default:
    disabled

By default we will attempt to use iconv functionality provided with glibc, however under some circumstances it may make sense to link against a separate iconv library. This option will attempt to do that. Note, that using --disable-iconv-lib will not prevent automatic attempts to link ageinst the iconv library in the case where builtin glibc iconv functionality is not available.

Use Atomic Assembler Code For x86 Single CPU systems
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

::

    --enable-atomic-x86-single

Default:
    disabled

Use assembler code suited for single CPU x86 machines. This may improve performance, but your binary will not function properly on SMP systems. If you specify this for a non x86 architecture the binary will not run at all. If you wonder about the purpose of assembler code in a mediaserver application: we need it for atomic operations that are required for reference counting. The pthread library will be used as a fallback for other architectures, but can also be forced by a designated configure option. This however, will have the worst performance.

By default x86 SMP code will be used on x86 systems - it will reliably work on both SMP and single CPU systems, but will not be as fast as the atomic-x86-single option on uniprocessor machines.

Use Pthread Code For Atomic Operations
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

::

    --enable-atomic-pthread

Default:
    auto detect

This is the default setting for non x86 architectures, we may add assembler optimizations for other architectures as well, but currently only x86 optimizations are available. This option may also be safely used on x86 machines - the drawback is poor performance, compared to assembler optimized code.

Enable SIGHUP Handling
~~~~~~~~~~~~~~~~~~~~~~

::

    --enable-sighup

This option turns on SIGHUP handling, every time a SIGHUP is caught we will attempt to restart the server and reread the configuration file. By default this feature is enabled for x86 platforms, but is disabled for others. We discovered that MediaTomb will not cleanly restart on ARM based systems, investigations revealed that this is somehow related to an unclean libupnp shutdown. This will be fixed in a later release.

Default:
    auto detect

X\_MS\_MediaReceiverRegistrar Support
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

::

    --enable-mrreg-service

Default:
    disabled

This option will enable the compilation and support of the X\_MS\_MediaReceiverRegistrar UPnP service, this was implemented for future Xbox 360 support. If you have a renderer that requires this service, you can safely enable it. It will always return true to IsValidated and IsAuthorized requests.

Note:
    eventhough this service is implemented there is still no Xbox 360 support in MediaTomb, more work needs to be done.

Playstation 3 Support
~~~~~~~~~~~~~~~~~~~~~

::

    --enable-protocolinfo-extension

Default:
    enabled

This option allows to send additional information in the protocolInfo attribute, this will enable MP3 and MPEG4 playback for the Playstation 3, but may also be useful to some other renderers.

Note:
    allthough compiled in, this feature is disabled in configuration by default.

Fseeko Check
~~~~~~~~~~~~

::

    --disable-fseeko-check

Default:
    enabled

This is a workaround for a bug in some Debian distributions, disable this check if you know that your system has large file support, but configure fails to detect it.

Largefile Support
~~~~~~~~~~~~~~~~~

::

    --disable-largefile

Default:
    auto

By default largefile support will be auto detected by configure, however you can disable it if you do not want it or if you experience problems with it on your system.

Redefinition Of Malloc And Realloc
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

::

    --disable-rpl-malloc

Default:
    enabled

Autoconf may redefine malloc and realloc functions to rpl\_malloc and rpl\_realloc, usually this will happen if the autotools think that you are compiling against a non GNU C library. Since malloc and realloc may behave different on other systems, this gives us the opportunity to write wrapper functions to handle special cases. However, this redefinition may get triggered when cross compiling, even if you are compiling against the GNU C lib. If this is the case, you can use this option to disable the redefinition.

SQLite Support
~~~~~~~~~~~~~~

::

    --enable-sqlite3

Default:
    enabled

The SQLite database is very easy in installation and use, you do not have to setup any users, permissions, etc. A database file will be simply created as specified in the MediaTomb configuration. At least SQLite version 3 is required.

MySQL Support
~~~~~~~~~~~~~

::

    --enable-mysql

Default:
    enabled

MySQL is a very powerful database, however it requires some additional setup. You will find information on how to setup MediaTomb with MySQL in the Installation section.

SpiderMonkey LibJS Support
~~~~~~~~~~~~~~~~~~~~~~~~~~

::

    --enable-libjs

Default:
    enabled

SpiderMonkey is Mozilla's JavaScript engine, it plays a very important role in MediaTomb. We use it to create a nice virtual container layout based on the metadata that is extracted from your media. We also allow the user to create custom import scripts, so everyone has the possibility to adapt the layout to ones personal needs. Read more about this in the installation section.

The main problem with this library is, that it is called differently on various distributions and that it is installed in different locations. For example, it is called js on Fedora, but is available under the name of smjs on Debian. If configure fails to find your js headers and libraries you can point it to the desired locations (see options below).

Filemagic Support
~~~~~~~~~~~~~~~~~

::

    --enable-libmagic

Default:
    enabled

This library determines the file type and provides us with the appropriate mime type information. It is very important to correctly determine the mime type of your media - this information will be sent to your renderer. Based on the mime type information, the renderer will decide if it can play/display the particular file or not. If auto detection returns strange mime types, you may want to do a check using the 'file' command (the 'file' package must be installed on your system). Assuming that you want to check somefile.avi enter the following in your terminal:

::

    $ file -i somefile.avi

This will print the detected mime type, this is exactly the information that we use in MediaTomb. You can override auto detection by defining appropriate file extension to mime type mappings in your configuration file. You can also edit the mime type information of an imported object manually via the web UI.

Id3lib Support
~~~~~~~~~~~~~~

::

    --enable-id3lib

Default:
    disabled, used if taglib is not available

This library will parse id3 tags of your MP3 files, the gathered information will be saved in the database and provided via UPnP. Further, the gathered metadata will be used by the import script to create a nice container layout (Audio/Artist/Album, etc.)

Taglib Support
~~~~~~~~~~~~~~

::

    --enable-taglib

Default:
    enabled, preffered over id3lib

This library will parse id3 tags of your MP3 files as well as information provided with flac files. It claims to be faster than id3lib, but it also seems to have some drawbacks. We had some cases where it crashed when trying to parse tags of certain MP3 files on embedded systems, we had reports and observed that it had problems parsing the sample rates. We also did some valgrinding and detected memory leaks. Our feeling is, that you will have more stable results with id3lib, however it is up to you to enable or disable this library. By default id3lib will be taken if both libraries are present on the system.

Libexif Support
~~~~~~~~~~~~~~~

::

    --enable-libexif

Default:
    enabled

The exif library will gather metadata from your photos, it will also find exif thumbnails which are created automatically by most digital camera models. The gathered data will be used by the import script, the thumbnails will be offered as additional resources via UPnP.

Inotify Support
~~~~~~~~~~~~~~~

::

    --enable-inotify

Default:
    auto

Inotify is a kernel mechanism that allows monitoring of filesystem events. You need this if you want to use the Inotify Autoscan mode, contrary to the Timed mode which recsans given directories in specified intervals, Inotify mode will immedeately propagate changes in monitored directories on the filesystem to the database.

If you do not specify this option configure will check if inotify works on the build system and compile it in only if the check succeeds. If you specify this option, the functionality will be compiled in even if the build system does not support inotify - the availability of inotify will then be checked at server runtime.

YouTube Service Support
~~~~~~~~~~~~~~~~~~~~~~~

::

    --enable-youtube

Default:
    enabled

This option enabled support of the YouTube service, it allows to gather information about content on the YouTube site and offers the content via UPnP, thus enabling you to watch your favorite YouTube videos on your UPnP player device. The feature only makes sense in combination with transcoding, since most devices do not support playback of flv files natively.

External Transcoding
~~~~~~~~~~~~~~~~~~~~

::

    --enable-external-transcoding

Default:
    enabled

Sqlite Backup Defaults
~~~~~~~~~~~~~~~~~~~~~~

::

    --enable-sqlite-backup-defaults

Default:
    disabled

Enables backup option for sqlite as the default setting, might be useful for NAS builds.

Curl
~~~~

::

    --enable-curl

Default:
    enabled if external transcoding or YouTube features are turned on

It only makes sense to enable the curl library if YouTube and External Transcoding are turned on. YouTube requires curl, but it's optional for External Transcoding.

Ffmpeg Support
~~~~~~~~~~~~~~

::

    --enable-ffmpeg

Default:
    enabled

Currently the ffmpeg library is used to extract additional information from audio and video files. It is also capable of reading out the tag information from theora content. It is not yet used for transcoding, so this feature only gathers additional metadata.

Ffmpeg Thumbnailer Support
~~~~~~~~~~~~~~~~~~~~~~~~~~

::

    --enable-ffmpegthumbnailer

Default:
    enabled

Compiling with ffmpegthumbnailer support is only possible if you also compile with ffmpeg support. The library allows to generate thumbnails for the videos on the fly.

MediaTomb Debug Output
~~~~~~~~~~~~~~~~~~~~~~

::

    --enable-tombdebug

Default:
    disabled

This option enables debug output, the server will print out a lot of information which is mainly interesting to developers. Use this if you are trying to trace down a bug or a problem, the additional output may give you some clues.

UPnP Library Debug Output
~~~~~~~~~~~~~~~~~~~~~~~~~

::

    --enable-upnpdebug

Default:
    disabled

This option enables debug output of the UPnP SDK. You should not need it under normal circumstances.

Log Output
~~~~~~~~~~

::

    --disable-log

Default:
    enabled

This option allows you to suppress all log output from the server.

Debug Log Output
~~~~~~~~~~~~~~~~

Default:
    enabled

This option allows you to compile the server with debug messages. If enabled, switching between verbose and normal output during runtime becomes possible.

Package Search Directory
~~~~~~~~~~~~~~~~~~~~~~~~

::

    --with-search=DIR

Default:
    /opt/local/ on Darwin, /usr/local/ on all other systems

Some systems may have whole sets of packages installed in an alternative location, for example Darwinports on OSX get installed to /opt/local/. This option tells the configure script to additionally search for headers and libraries of various packages in DIR/include and DIR/lib.

Specifying Header And Library Locations Of Various Packages
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

You can specify the exact location of particular headers and libraries. Some packages use extra programs that tell us the appropriate flags that are needed for compilation - like mysql\_config. You can also specify the exact location of those programs. The parameters are self explanatory, in case of headers and libraries the DIR parameter is the directory where those headers and libraries are located.

::

    --with-sqlite3-h=DIR               search for sqlite3 headers only in DIR
    --with-sqlite3-libs=DIR            search for sqlite3 libraries only in DIR
    --with-mysql-cfg=mysql_config      absolute path/name of mysql_config
    --with-js-h=DIR                    search for js (spidermonkey) headers in DIR
    --with-js-libs=DIR                 search for js (spidermonkey) libraries in DIR
    --with-libmagic-h=DIR              search for filemagic headers in DIR
    --with-libmagic-libs=DIR           search for filemagic headers in DIR
    --with-libexif-h=DIR               search for libexif headers in DIR
    --with-libexif-libs=DIR            search for libexif libraries in DIR
    --with-expat-h=DIR                 search for expat headers in DIR
    --with-expat-libs=DIR              search for expat libraries in DIR
    --with-taglib-cfg=taglib-config    absolute path/name of taglib-config
    --with-id3lib-h=DIR                search for id3lib headers in DIR
    --with-id3lib-libs=DIR             search for id3lib libraries in DIR
    --with-zlib-h=DIR                  search for zlib headers in DIR
    --with-zlib-libs=DIR               search for zlib libraries in DIR
    --with-inotify-h=DIR               search for inotify header in DIR
    --with-iconv-h=DIR                 search for iconv headers in DIR/sys
    --with-iconv-libs=DIR              search for iconv libraries in DIR
    --with-avformat-h=DIR              search for avformat headers in DIR
    --with-avformat-libs=DIR           search for avformat libraries in DIR
    --with-avutil-libs=DIR             search for avutil libraries in DIR
    --with-ffmpegthumbnailer-h=DIR     search for ffmpegthumbnailer headers in DIR
    --with-ffmpegthumbnailer-libs=DIR  search for ffmpegthumbnailer libraries in DIR
    --with-curl-cfg=curl-config        absolute path/name of curl-config script
    --with-libmp4v2-h=DIR              search for libmp4v2 headers in DIR
    --with-libmp4v2-libs=DIR           search for libmp4v2 libraries in DIR
    --with-rt-libs=DIR                 search for rt libraries in DIR
    --with-pthread-libs=DIR            search for pthread libraries in DIR
    --with-lastfmlib-h=DIR             search for lastfmlib headers in DIR
    --with-lastfmlib-libs=DIR          search for lastfmlib libraries in DIR

The devconf Script
~~~~~~~~~~~~~~~~~~

If you are doing some development work and some debugging, you will probably want to compile with the -g flag and also disable optimization. The devconf script does exactly that. In addition, it accepts command line parameters that are passed to the configure script.

Initial Installation
====================

Network Setup
-------------

Some systems require a special setup on the network interface. If MediaTomb exits with UPnP Error -117, or if it does not respond to M-SEARCH requests from the renderer (i.e. MediaTomb is running, but your renderer device does not show it) you should try the following settings (the lines below assume that MediaTomb is running on a Linux machine, on network interface eth1):

::

    # route add -net 239.0.0.0 netmask 255.0.0.0 eth1
    # ifconfig eth1 allmulti

Those settings will be applied automatically by the init.d startup script.

You should also make sure that your firewall is not blocking port UDP port 1900 (required for SSDP) and UDP/TCP port of MediaTomb. By default MediaTomb will select a free port starting with 49152, however you can specify a port of your choice in the configuration file.

First Time Launch
-----------------

When starting MediaTomb for the first time, a .mediatomb directory will be created in your home. Further, a default server configuration file, called config.xml will be generated in that directory.

Using Sqlite Database
~~~~~~~~~~~~~~~~~~~~~

If you are using sqlite - you are ready to go, the database file will be created automatically and will be located ~/.mediatomb/mediatomb.db If needed you can adjust the database file name and location in the server configuration file.

Using MySQL Database
~~~~~~~~~~~~~~~~~~~~

If MediaTomb was compiled with support for both databases, sqlite will be chosen as default because the initial database can be created and used without any user interaction. If MediaTomb was compiled only with MySQL support, the appropriate config.xml file will be created in the ~/.mediatomb directory, but the server will then terminate, because user interaction is required.

MediaTomb has to be able to connect to the MySQL server and at least the (empty) database has to exist. To create the database and provide MediaTomb with the ability to connect to the MySQL server you need to have the appropriate permissions. Note that user names and passwords in MySQL have nothing to do with UNIX accounts, MySQL has it's own user names/passwords. Connect to the MySQL database as ”root” or any other user with the appropriate permissions:

::

    $ mysql [-u <username>] [-p]

(You'll probably need to use ”-u” to specify a different MySQL user and ”-p” to specify a password.)

Create a new database for MediaTomb: (substitute ”<database name>” with the name of the database)

::

    mysql> CREATE DATABASE <database name>;

(You can also use ”mysqladmin” instead.)

Give MediaTomb the permissions to access the database:

::

    mysql> GRANT ALL ON <database name>.*
           TO '<user name>'@'<hostname>'
           IDENTIFIED BY '<password>';

If you don't want to set a password, omit ”IDENTIFIED BY ..” completely. You could also use the MySQL ”root” user with MediaTomb directly, but this is not recommended.

To create a database and a user named ”mediatomb” (who is only able to connect via ”localhost”) without a password (the defaults) use:

::

    mysql> CREATE DATABASE mediatomb;
    mysql> GRANT ALL ON mediatomb.* TO 'mediatomb'@'localhost';

If MediaTomb was compiled with database auto creation the tables will be created automatically during the first startup. All table names have a ”mt\_” prefix, so you can theoretically share the database with a different application. However, this is not recommended.

If database auto creation wasn't compiled in (configure was run with the ”--disable-db-autocreate” or zlib.h was not available) you have to create the tables manually:

::

    $ mysql [-u <username>] [-p] \
      <database name> < \
      <install prefix>/share/mediatomb/mysql.sql

After creating the database and making the appropriate changes in your MediaTomb config file you are ready to go - launch the server, and everything should work.

Command Line Options
====================

There is a number of options that can be passed via command line upon server start up, for a short summary you can invoke MediaTomb with the following parameter:

::

    $ mediatomb --help

Note:
    the command line options override settings in the configuration file!

IP Address
----------

::

    --ip or -i

The server will bind to the given IP address, currently we can not bind to multiple interfaces so binding to 0.0.0.0 will not be possible.

Interface
---------

::

    --interface or -e

Interface to bind to, for example eth0, this can be specified instead of the ip address.

Port
----

::

    --port or -p

Specify the server port that will be used for the web user interface, for serving media and for UPnP requests, minimum allowed value is 49152. If this option is omitted a default port will be chosen, however, in this case it is possible that the port will change upon server restart.

Configuration File
------------------

::

     --config or -c

By default MediaTomb will search for a file named ”config.xml” in the ~/.mediatomb directory. This option allows you to specify a config file by the name and location of your choice. The file name must be absolute.

Daemon Mode
-----------

::

    --daemon or -d

Run the server in background, MediaTomb will shutdown on SIGTERM, SIGINT and restart on SIGHUP.

Home Directory
--------------

::

    --home or -m

Specify an alternative home directory. By default MediaTomb will try to retrieve the users home directory from the environment, then it will look for a .mediatomb directory in users home. If .mediatomb was found we will try to find the default configuration file (config.xml), if not found we will create both, the .mediatomb directory and the default config file.

This option is useful in two cases: when the home directory can not be retrieved from the environment (in this case you could also use -c to point MediaTomb to your configuration file or when you want to create a new configuration in a non standard location (for example, when setting up daemon mode). In the latter case you can combine this parameter with the parameter described in ?

Config Directory
----------------

::

    --cfgdir or -f

The default configuration directory is combined out of the users home and the default that equals to .mediatomb, this option allows you to override the default directory naming. This is useful when you want to setup the server in a nonstandard location, but want that the default configuration to be written by the server.

Write PID File
--------------

::

    --pidfile or -P

Specify a file that will hold the server process ID, the filename must be absolute.

Run Under Different User Name
-----------------------------

::

    --user or -u

Run MediaTomb under the specified user name, this is especially useful in combination with the daemon mode.

Run Under Different Group
-------------------------

::

    --group or -g

Run MediaTomb under the specified group, this is especially useful in combination with the daemon mode.

Add Content
-----------

::

    --add or -a

Add the specified directory or file name to the database without UI interaction. The path must be absolute, if path is a directory then it will be added recursively. If path is a file, then only the given file will be imported.

Log To File
-----------

::

    --logfile or -l

Do not output log messages to stdout, but redirect everything to a specified file.

Debug Output
------------

::

    --debug or -D

Enable debug log output.

Compile Info
------------

::

    --compile-info

Print the configuration summary (used libraries and enabled features) and exit.

Version Information
-------------------

::

    --version

Print version information and exit.

Display Command Line Summary
----------------------------

::

    --help or -h

Print a summary about the available command line options.

Configuration File
==================

MediaTomb is highly configurable and allows the user to set various options and preferences that define the servers behavior. Rather than enforcing certain features upon the user, we prefer to offer a number of choices where possible. The heart of MediaTomb configuration is the config.xml file, which is located in the ~/.mediatomb directory. If the configuration file is not found in the default location and no configuration was specified on the command line, MediaTomb will generate a default config.xml file in the ~/.mediatomb directory. The file is in the XML format and can be edited by a simple text editor, here is the list of all available options:

-  "*Required*\ " means that the server will not start if the tag is missing in the configuration.

-  "*Optional*\ " means that the tag can be left out of the configuration file.

The root tag of MediaTomb configuration is:

::

    <config>

Server Settings
---------------

These settings define the server configuration, this includes UPnP behavior, selection of database, accounts for the UI as well as installation locations of shared data.

::

    <server>

*Required*

This section defines the server configuration parameters.

Child tags:

-  ::

       <port>0</port>

   *Optional*

   *Default*:*0 (automatic)*

   Specifies the port where the server will be listening for HTTP requests. Note, that because of the implementation in the UPnP SDK only ports above 49152 are supported. The value of zero means, that a port will be automatically selected by the SDK.

-  ::

       <ip>192.168.0.23</ip>

   *Optional*

   *Default*:*ip of the first available interface.*

   Specifies the IP address to bind to, by default one of the available interfaces will be selected.

-  ::

       <interface>eth0</interface>

   *Optional*

   *Default*:*first available interface.*

   Specifies the interface to bind to, by default one of the available interfaces will be selected.

-  ::

       <name>MediaTomb</name>

   *Optional*

   *Default*:*MediaTomb*

   Server friendly name, you will see this on your devices that you use to access the server.

-  ::

       <manufacturerURL>http://mediatomb.org/</manufacturerURL>

   *Optional*

   *Default*:*http://mediatomb.cc/*

   This tag sets the manufacturer URL of a UPnP device, a custom setting may be necessary to trick some renderers in order to enable special features that otherwise are only active with the vendor implemented server.

-  ::

       <modelName>MediaTomb</modelName>

   *Optional*

   *Default: MediaTomb*

   This tag sets the model name of a UPnP device, a custom setting may be necessary to trick some renderers in order to enable special features that otherwise are only active with the vendor implemented server.

-  ::

       <modelNumber>0.9.0</modelNumber>

   *Optional*

   *Default: MediaTomb version*

   This tag sets the model number of a UPnP device, a custom setting may be necessary to trick some renderers in order to enable special features that otherwise are only active with the vendor implemented server.

-  ::

       <serialNumber>1</serialNumber>

   *Optional*

   *Default: 1*

   This tag sets the serial number of a UPnP device.

-  ::

       <presentationURL append-to="ip">80/index.html</presentationURL>

   *Optional*

   *Default*:*”/”*

   The presentation URL defines the location of the servers user interface, usually you do not need to change this however, vendors who want to ship our server along with their NAS devices may want to point to the main configuration page of the device.

   Attributes:

   -  ::

          append-to=...

      *Optional*

      *Default: ”none”*

      The append-to attribute defines how the text in the presentationURL tag should be treated.

      The allowed values are:

      ::

          append-to="none"

      Use the string exactly as it appears in the presentationURL tag.

      ::

          append-to="ip"

      Append the string specified in the presentationURL tag to the ip address of the server, this is useful in a dynamic ip environment where you do not know the ip but want to point the URL to the port of your web server.

      ::

          append-to="port"

      Append the string specified in the presentationURL tag to the server ip and port, this may be useful if you want to serve some static pages using the built in web server.

-  ::

       <udn/>

   *Required*

   *Default*:*automatically generated if the tag is empty*

   Unique Device Name, according to the UPnP spec it must be consistent throughout reboots. You can fill in something yourself, but we suggest that you leave this tag empty - it will be filled out and saved automatically after the first launch of the server.

-  ::

       <home>/home/your_user_name/.mediatomb</home>

   *Required*

   *Default: ~/.mediatomb*

   Server home - the server will search for the data that it needs relative to this directory - basically for the sqlite database file. The mediatomb.html bookmark file will also be generated in that directory.

-  ::

       <webroot>/usr/share/mediatomb/web</webroot>

   *Required*

   *Default: depends on the installation prefix that is passed to the configure script.*

   Root directory for the web server, this is the location where device description documents, UI html and js files, icons, etc. are stored.

-  ::

       <servedir>/home/myuser/mystuff</servedir>

   *Optional*

   *Default: empty (disabled)*

   Files from this directory will be served as from a regular web server. They do not need to be added to the database, but they are also not served via UPnP browse requests. Directory listing is not supported, you have to specify full paths.

   Example:
       the file something.jar is located in /home/myuser/mystuff/javasubdir/something.jar on your filesystem. Your ip address is 192.168.0.23, the server is running on port 50500. Assuming the above configuration you could download it by entering this link in your web browser: http://192.168.0.23:50500/content/serve/javasubdir/something.jar

-  ::

       <alive>180</alive>

   *Optional*

   *Default:* *180, this is according to the UPnP specification.*

   Interval for broadcasting SSDP:alive messages

-  ::

       <protocolInfo extend="no"/>

   *Optional*

   *Default:* *no*

   Adds specific tags to the protocolInfo attribute, this is required to enable MP3 and MPEG4 playback on Playstation 3.

-  ::

       <pc-directory upnp-hide="no"/>

   *Optional*

   *Default: no*

   Enabling this option will make the PC-Directory container invisible for UPnP devices.

   Note:
       independent of the above setting the container will be always visible in the web UI!

-  ::

       <tmpdir>/tmp/</tmpdir>

   *Optional*

   *Default: /tmp/*

   Selects the temporary directory that will be used by the server.

-  ::

       <bookmark>mediatomb.html</bookmark>

   *Optional*

   *Default: mediatomb.html*

   The bookmark file offers an easy way to access the user interface, it is especially helpful when the server is not configured to run on a fixed port. Each time the server is started, the bookmark file will be filled in with a redirect to the servers current IP address and port. To use it, simply bookmark this file in your browser, the default location is ~/.mediatomb/mediatomb.html

-  ::

       <custom-http-headers>

   *Optional*

   This section holds the user defined HTTP headers that will be added to all HTTP responses that come from the server.

   Child tags:

   -  ::

          <add header="..."/>
          <add header="..."/>
          ...

      *Optional*

      Specify a header to be added to the response. If you have a DSM-320 use <add header="X-User-Agent: redsonic"/> to fix the .AVI playback problem.

-  ::

       <upnp-string-limit>

   *Optional*

   *Default:* *disabled*

   This will limit title and description length of containers and items in UPnP browse replies, this feature was added a s workaround for the TG100 bug which can only handle titles no longer than 100 characters. A negative value will disable this feature, the minimum allowed value is "4" because three dots will be appended to the string if it has been cut off to indicate that limiting took place.

-  ::

       <ui enabled="yes" poll-interval="2" poll-when-idle="no"/>

   *Optional*

   This section defines various user interface settings.

   **WARNING!**

   The server has an integrated filesystem browser, that means that anyone who has access to the UI can browse your filesystem (with user permissions under which the server is running) and also download your data! If you want maximum security - disable the UI completely! Account authentication offers simple protection that might hold back your kids, but it is not secure enough for use in an untrusted environment!

   Note:
       since the server is meant to be used in a home LAN environment the UI is enabled by default and accounts are deactivated, thus allowing anyone on your network to connect to the user interface.

   Attributes:

   -  ::

          enabled=...

      *Optional*

      *Default: yes*

      Enables (”yes”) or disables (”no”) the web user interface.

   -  ::

          show-tooltips=...

      *Optional*

      *Default: yes*

      This setting specifies if icon tooltips should be shown in the web UI.

   -  ::

          poll-interval=...

      *Optional*

      *Default: 2*

      The poll-interval is an integer value which specifies how often the UI will poll for tasks. The interval is specified in seconds, only values greater than zero are allowed.

   -  ::

          poll-when-idle=...

      *Optional*

      *Default: no*

      The poll-when-idle attribute influences the behavior of displaying current tasks: - when the user does something in the UI (i.e. clicks around) we always poll for the current task and will display it - if a task is active, we will continue polling in the background and update the current task view accordingly - when there is no active task (i.e. the server is currently idle) we will stop the background polling and only request updates upon user actions, but not when the user is idle (i.e. does not click around in the UI)

      Setting poll-when-idle to "yes" will do background polling even when there are no current tasks; this may be useful if you defined multiple users and want to see the tasks the other user is queuing on the server while you are actually idle.

      The tasks that are monitored are:

      -  adding files or directories

      -  removing items or containers

      -  automatic rescans

   Child tags:

   -  ::

          <accounts enabled="yes" session-timeout="30"/>

      *Optional*

      This section holds various account settings.

      Attributes:

      -  ::

             enabled=...

         *Optional*

         *Default: yes*

         Specifies if accounts are enabled (”yes”) or disabled (”no”).

      -  ::

             session-timeout=...

         *Optional*

         *Default: 30*

         The session-timeout attribute specifies the timeout interval in minutes. The server checks every five minutes for sessions that have timed out, therefore in the worst case the session times out after session-timeout + 5 minutes.

      Accounts can be defined as shown below:

      -  ::

             <account user="name" password="password"/>
             <account user="name" password="password"/>
             ....

         *Optional*

         There can be multiple users, however this is mainly a feature for the future. Right now there are no per-user permissions.

   -  ::

          <items-per-page default="25">

      *Optional*

      *Default: 25*

      This sets the default number of items per page that will be shown when browsing the database in the web UI.

      The values for the items per page drop down menu can be defined in the following manner:

      -  ::

             <option>10</option>
             <option>25</option>
             <option>50</option>
             <option>100</option>

         *Default: 10, 25, 50, 100*

         Note:
             this list must contain the default value, i.e. if you define a default value of 25, then one of the <option> tags must also list this value.

-  ::

       <storage caching="yes">

   *Required*

   Defines the storage section - database selection is done here. Currently sqlite3 and mysql are supported. Each storage driver has it's own configuration parameters.

   -  ::

          caching="yes"

      *Optional*

      *Default: yes*

      Enables caching, this feature should improve the overall import speed.

   -  ::

          <sqlite enabled="yes>

      *Required if MySQL is not defined*

      Allowed values are ”sqlite3” or ”mysql”, the available options depend on the selected driver.

      -  ::

             enabled="yes"

         *Optional*

         *Default: yes*

      Below are the sqlite driver options:

      -  ::

             <database-file>mediatomb.db</database-file>

         *Optional*

         *Default:* *mediatomb.db*

         The database location is relative to the server's home, if the sqlite database does not exist it will be created automatically.

      -  ::

             <synchronous>off</synchronous>

         *Optional*

         *Default: off*

         Possible values are ”off”, ”normal” and ”full”.

         This option sets the SQLite pragma ”synchronous”. This setting will affect the performance of the database write operations. For more information about this option see the SQLite documentation: http://www.sqlite.org/pragma.html#pragma_synchronous

      -  ::

             <on-error>restore</on-error>

         *Optional*

         *Default: restore*

         Possible values are ”restore” and ”fail”.

         This option tells MediaTomb what to do if an SQLite error occurs (no database or a corrupt database). If it is set to ”restore” it will try to restore the database from a backup file (if one exists) or try to recreate a new database from scratch.

         If the option is set to ”fail”, MediaTomb will abort on an SQLite error.

      -  ::

             <backup enabled="no" interval="6000"/>

         *Optional*

         Backup parameters:

         -  ::

                enabled=...

            *Optional*

            *Default: no*

            Enables or disables database backup.

         -  ::

                interval=...

            *Optional*

            *Default: 600*

            Defines the backup interval in seconds.

   -  ::

          <mysql enabled="no"/>

      Defines the MySQL storage driver section.

      -  ::

             enabled=...

         *Optional*

         *Default: yes*

         Enables or disables the MySQL driver.

      Below are the child tags for MySQL:

      -  ::

             <host>localhost</host>

         *Optional*

         *Default: "localhost"*

         This specifies the host where your MySQL database is running.

      -  ::

             <port>0</port>

         *Optional*

         *Default: 0*

         This specifies the port where your MySQL database is running.

      -  ::

             <username>root</username>

         *Optional*

         *Default: "mediatomb"*

         This option sets the user name that will be used to connect to the database.

      -  ::

             <password></password>

         *Optional*

         *Default: no password*

         Defines the password for the MySQL user. If the tag doesn't exist MediaTomb will use no password, if the tag exists, but is empty MediaTomb will use an empty password. MySQL has a distinction between no password and an empty password.

      -  ::

             <database>mediatomb</database>

         *Optional*

         *Default: "mediatomb"*

         Name of the database that will be used by MediaTomb.

Extended Runtime Options
~~~~~~~~~~~~~~~~~~~~~~~~

::

    <extended-runtime-options>

These options reside under the server tag and allow to additinally control the so called "runtime options". The difference to the import options is:

Import options are only triggered when data is being imported into the database, this means that if you want new settings to be applied to already imported data, you will have to reimport it. Runtime options can be switched on and off without the need to reimport any media. Unfortunately you still need to restart the server so that these options take effect, however after that they are immediately active.

-  ::

       <ffmpegthumbnailer enabled="no">

   *Optional*

   *Default: no*

   Ffmpegthumbnailer is a nice, easy to use library that allows to generate thumbnails from video files. Some DLNA compliant devices support video thumbnails, if you think that your device may be one of those you can try enabling this option. It would also make sense to enable the protocolInfo option, since it will add specific DLNA tags to tell your device that a video thumbnail is being offered by the server.

   Note:
       thumbnails are not cached and not stored anywhere, they will be generated on the fly in memory and released afterwards. If your device supports video thumbnails and requests them from the server in large amounts, the performance when browsing video containers will depend on the speed of your machine. A feature to allow caching of thumbnails is planned for future releases.

   The following options allow to control the ffmpegthumbnailer library (these are basically the same options as the ones offered by the ffmpegthumbnailer command line application). All tags below are optional and have sane default values.

   -  ::

          <thumbnail-size>128</thumbnail-size>

      *Optional*

      *Default: 128*

      The thumbnail size should not exceed 160x160 pixels, higher values can be used but will mostprobably not be supported by DLNA devices. The value of zero or less is not allowed.

   -  ::

          <seek-percentage>5</seek-percentage>

      *Optional*

      *Default: 5*

      Time to seek to in the movie (percentage), values less than zero are not allowed.

   -  ::

          <filmstrip-overlay>yes</filmstrip-overlay>

      *Optional*

      *Default: yes*

      Creates a filmstrip like border around the image, turn this option off if you want pure images.

   -  ::

          <image-quality>8</image-quality>

      *Optional*

      *Default: 8*

      Sets the image quality of the generated thumbnails.

   -  ::

          <workaround-bugs>no</workaround-bugs>

      *Optional*

      *Default: no*

      Accodring to ffmpegthumbnailer documentation, this option will enable workarounds for bugs in older ffmpeg versions. You can try enabling it if you experience unexpected behaviour, like hangups during thumbnail generation, crashes and alike.

-  ::

       <lastfm enabled="no">

   *Optional*

   Support for the last.fm service.

   -  ::

          <username>login</username>

      *Required*

      Your last.fm user name.

   -  ::

          <password>pass</password>

      *Required*

      Your last.fm password.

-  ::

       <mark-played-items enabled="no" suppress-cds-updates="yes">

   *Optional*

   The attributes of the tag have the following meaning:

   -  ::

          enabled=...

      *Optional*

      *Default: no*

      Enables or disables the marking of played items, seto to "yes" to enable the feature.

   -  ::

          suppress-cds-updates=...

      *Optional*

      *Default: yes*

      This is an advanced feature, leave the default setting if unsure. Usually, when items are modified we send out container updates as specified in the Content Directory Service. This notifies the player that data in a particular container has changed, players that support CDS updates will rebrowse the container and refresh the view. However, in this case we probably do not want it (this actually depends on the particular player implementation). For example, if we update the list of currently playing items, the player could interrupt playback and rebrowse the current container - clearly an unwatned behaviour. Because of this, we provide an option to suppress and not send out container updates - only for the case where the item is marked as "played". In order to see the changes you will have to get out of the current container and enter it again - then the view on your player should get updated.

      Note:
          some players (i.e. PS3) cache a lot of data and do not react to container updates, for those players it may be necessary to leave the server view or restart the player in order to update content (same as when adding new data).

   The following tag defines how played items should be marked:

   -  ::

          <string mode="prepend">* </string>

      *Optional*

      *Default: \**

      Specifies what string should be appended or prepended to the title of the object that will be marked as "played".

      -  ::

             mode=...

         *Optional*

         *Default: prepend*

         Specifies how a string should be added to the object's title, allowed values are "append" and "prepend".

   -  ::

          <mark>

      *Optional*

      This subsection allows to list which type of content should get marked. We figured, that marking played items is mostly useful for videos, mainly for watching series. It could also be used with audio and image content, but otherwise it's probably useless. Thefore we decided to specify only three supported types that can get marked:

      -  ::

             <content>audio</content>
             <content>video</content>
             <content>image</content>

         You can specify any combination of the above tags to mark the items you want.

Import Settings
---------------

The import settings define various options on how to aggregate the content.

::

    <import hidden-files="no">

*Optional*

This tag defines the import section.

Attributes:

-  ::

       hidden-files=...

   *Optional*

   *Default: no*

   This attribute defines if files starting with a dot will be imported into the database (”yes”). Autoscan can override this attribute on a per directory basis.

Child tags:

-  ::

       <filesystem-charset>ISO-8859-1</filesystem-charset>

   *Optional*

   *Default:* *if nl\_langinfo() function is present, this setting will be auto detected based on your system locale, else set to ISO-8859-1*

   Defines the charset of the filesystem. For example, if you have file names in Cyrillic KOI8-R encoding, then you should specify that here. The server uses UTF-8 internally, this import parameter will help you to correctly import your data.

-  ::

       <metadata-charset>ISO-8859-1</metadata-charset>

   *Optional*

   *Default:* *if nl\_langinfo() function is present, this setting will be auto detected based on your system locale, else set to ISO-8859-1*

   Same as above, but defines the charset of the metadata (i.e. id3 tags, Exif information, etc.)

-  ::

       <scripting script-charset="UTF-8">

   *Optional*

   Defines the scripting section.

   -  ::

          script-charset=...

      *Optional*

      *Default: UTF-8*

   Below are the available scripting options:

   -  ::

          <virtual-layout type="builtin">

      *Optional*

      Defines options for the virtual container layout; the so called ”virtual container layout” is the way how the server organizes the media according to the extracted metadata. For example, it allows sorting audio files by Album, Artist, Year and so on.

      -  ::

             type=...

         *Optional*

         *Default: builtin*

         Specifies what will be used to create the virtual layout, possible values are:

         -  builtin: a default layout will be created by the server

         -  js: a user customizable javascript will be used (MediaTomb must be compiled with js support)

         -  disabled: only PC-Directory structure will be created, i.e. no virtual layout

      The virtual layout can be adjusted using an import script which is defined as follows:

      -  ::

             <import-script>/path/to/my/import-script.js</import-script>

         *Required if virtual layout type is ”js*\ ”

         *Default:* *${prefix}/share/mediatomb/js/import.js, where ${prefix} is your installation prefix directory.*

         Points to the script invoked upon media import. For more details read doc/scripting.txt

      -  ::

             <dvd-script>/path/to/my/import-dvd.js</dvd-script>

         *Optional, only has effect when layout type is ”js*\ ”\ *and if MediaTomb was compiled with libdvdread support.*

         *Default:* *${prefix}/share/mediatomb/js/import-dvd.js, where ${prefix} is your installation prefix directory.*

         Points to the script invoked upon import of DVD iso images. For more details read doc/scripting.txt

   -  ::

          <common-script>/path/to/my/common-script.js</common-script>

      *Optional*

      *Default:* *${prefix}/share/mediatomb/js/common.js, where ${prefix} is your installation prefix directory.*

      Points to the so called common script - think of it as a custom library of js helper functions, functions added there can be used in your import and in your playlist scripts. For more details read doc/scripting.txt

   -  ::

          <playlist-script create-link="yes">/path/to/my/playlist-script.js</playlist-script>

      *Optional*

      *Default:* *${prefix}/share/mediatomb/js/playlists.js, where ${prefix} is your installation prefix directory.*

      Points to the script that is parsing various playlists, by default parsing of pls and m3u playlists is implemented, however the script can be adapted to parse almost any kind of text based playlist. For more details read doc/scripting.txt

      -  ::

             create-link=...

         *Optional*

         *Default: yes*

         Links the playlist to the virtual container which contains the expanded playlist items. This means, that if the actual playlist file is removed from the database, the virtual container corresponding to the playlist will also be removed.

-  ::

       <magic-file>/path/to/my/magic-file</magic-file>

   *Optional*

   *Default: System default*

   Specifies an alternative file for filemagic, containing mime type information.

-  ::

       <autoscan use-inotify="auto">

   *Optional*

   Specifies a list of default autoscan directories.

   This section defines persistent autoscan directories. It is also possible to define autoscan directories in the UI, the difference is that autoscan directories that are defined via the config file can not be removed in the UI. Even if the directory gets removed on disk, the server will try to monitor the specified location and will re add the removed directory if it becomes available/gets created again.

   -  ::

          use-inotify=...

      *Optional*

      *Default: auto*

      Specifies if the inotify autoscan feature should be enabled. The default value is "auto", which means that availability of inotify support on the system will be detected automatically, it will then be used if available. Setting the option to 'no' will disable inotify even if it is available. Allowed values: "yes", "no", "auto"

   Child tags:

   -  ::

          <directory location="/media" mode="timed" interval="3600"
              level="full" recursive="no" hidden-files="no"/>
          <directory location="/audio" mode="inotify"
              recursive="yes" hidden-files="no"/>
          ...

      *Optional*

      Defines an autoscan directory and it's parameters.

      The attributes specify various autoscan options:

      -  ::

             location=...

         *Required*

         Absolute path to the directory that shall be monitored.

      -  ::

             mode=...

         *Required*

         Scan mode, currently "inotify" and "timed" are supported. Timed mode rescans the given directory in specified intervals, inotify mode uses the kernel inotify mechanism to watch for filesystem events.

      -  ::

             interval=...

         *Required for ”timed” mode*

         Scan interval in seconds.

      -  ::

             level=...

         *Required for ”timed” mode*

         Either "full" or "basic". Basic mode will only check if any files have been added or were deleted from the monitored directory, full mode will remember the last modification time and re add the media that has changed. Full mode might be useful when you want to monitor changes in the media, like id3 tags and alike.

      -  ::

             recursive=...

         *Required*

         Values of "yes" or "no" are allowed, specifies if autoscan shall monitor the given directory including all sub directories.

      -  ::

             hidden-files=...

         *Optional*

         *Default:* *value specified in <import hidden-files=””/>*

         Allowed values: "yes" or "no", process hidden files, overrides the hidden-files value in the <import/> tag.

-  ::

       <mappings>

   *Optional*

   Defines various mapping options for importing media, currently two subsections are supported.

   This section defines mime type and upnp:class mappings, it is vital if filemagic is not available - in this case media type auto detection will fail and you will have to set the mime types manually by matching the file extension. It is also helpful if you want to override auto detected mime types or simply skip filemagic processing for known file types.

   -  ::

          <extension-mimetype ignore-unknown="no" case-sensitive="no">

      *Optional*

      This section holds the file name extension to mime type mappings.

      Attributes:

      -  ::

             ignore-unknown=...

         *Optional*

         *Default:* *no*

         If ignore-unknown is set to "yes", then only the extensions that are listed in this section are imported.

      -  ::

             case-sensitive=...

         *Optional*

         *Default: no*

         Specifies if extensions listed in this section are case sensitive, allowed values are "yes" or "no".

      Child tags:

      -  ::

             <map from="mp3" to="audio/mpeg"/>

         *Optional*

         Specifies a mapping from a certain file name extension (everything after the last dot ".") to mime type.

         Note:
             this improves the import speed, because invoking libmagic to discover the right mime type of a file is omitted for files with extensions listed here.

         Note:
             extension is case sensitive, this will probably need to be fixed.

   -  ::

          <mimetype-upnpclass>

      *Optional*

      This section holds the mime type to upnp:class mappings.

      Child tags:

      -  ::

             <map from="audio/*" to="object.item.audioItem.musicTrack"/>

         *Optional*

         Specifies a mapping from a certain mime type to upnp:class in the Content Directory. The mime type can either be entered explicitly "audio/mpeg" or using a wildcard after the slash "audio/\*". The values of "from" and "to" attributes are case sensitive.

   -  ::

          <mimetype-contenttype>

      *Optional*

      This section makes sure that the server knows about remapped mimetypes and still extracts the metadata correctly. For example, we know that id3lib can only handle mp3 files, the default mimetype of mp3 content is audio/mpeg. If the user remaps mp3 files to a different mimetype, we must know about it so we can still pass this item to id3lib for metadata extraction.

      Note:
          if this section is not present in your config file, the defaults will be filled in automatically. However, if you add an empty tag, without defining the following <treat> tags, the server will assume that you want to have an empty list and no files will be process by the metadata handler.

      -  ::

             <treat mimetype="audio/mpeg" as="mp3"/>

         *Optional*

         Tells the server what content the specified mimetype actually is.

         Note:
             it makes no sense to define 'as' values that are not below, the server only needs to know the content type of the ones specified, otherwise it does not matter.

         The 'as' attribute can have following values:

         -  ::

                mp3

            *Default mimetype: audio/mpeg*

            The content is an mp3 file and should be processed by either id3lib or taglib (if available).

         -  ::

                ogg

            *Default mimetype: application/ogg*

            The content is an ogg file and should be processed by taglib (if available).

         -  ::

                flac

            *Default mimetype: audio/x-flac*

            The content is a flac file and should be processed by taglib (if available).

         -  ::

                jpg

            *Default mimetype: image/jpeg*

            The content is a jpeg image and should be processed by libexif (if available).

         -  ::

                playlist

            *Default mimetype: audio/x-mpegurl or audio/x-scpls*

            The content is a playlist and should be processed by the playlist parser script.

         -  ::

                pcm

            *Default mimetype: audio/L16 or audio/x-wav*

            The content is a PCM file.

         -  ::

                avi

            *Default mimetype: video/x-msvideo*

            The content is an AVI container, FourCC extraction will be attempted.

-  ::

       <library-options>

   *Optional*

   This section holds options for the various supported import libraries, it is useful in conjunction with virtual container scripting, but also allows to tune some other features as well.

   Currently the library-options allow additional extraction of the so called auxilary data (explained below) and provide control over the video thumbnail generation.

   Here is some information on the auxdata: UPnP defines certain tags to pass along metadata of the media (like title, artist, year, etc.), however some media provides more metadata and exceeds the scope of UPnP. This additional metadata can be used to fine tune the server layout, it allows the user to create a more complex container structure using a customized import script. The metadata that can be extracted depends on the library, currently we support libebexif which provides a default set of keys that can be passed in the options below. The data according to those keys will the be extracted from the media and imported into the database along with the item. When processing the item, the import script will have full access to the gathered metadata, thus allowing the user to organize the data with the use of the extracted information. A practical example would be: if have more than one digital camera in your family you could extract the camera model from the Exif tags and sort your photos in a structure of your choice, like:

   Photos/MyCamera1/All Photos

   Photos/MyCamera1/Date

   Photos/MyCamera2/All Photos

   Photos/MyCamera2/Date

   etc.

   Child tags:

   -  ::

          <libexif>

      *Optional*

      Options for the exif library.

      Child tags:

      -  ::

             <auxdata>

         *Optional*

         Currently only adding keywords to auxdata is supported. For a list of keywords/tags see the libexif documentation. Auxdata can be read by the import java script to gain more control over the media structure.

         Child tags:

         -  ::

                <add-data tag="keyword1"/>
                <add-data tag="keyword2"/>
                ...

            *Optional*

            If the library was able to extract the data according to the given keyword, it will be added to auxdata. You can then use that data in your import scripts.

      A sample configuration for the example described above would be:

      ::

          <libexif>
              <auxdata>
                  <add-data tag="EXIF_TAG_MODEL"/>
              </auxdata>
          </libexif>

   -  ::

          <id3>

      *Optional*

      These options apply to id3lib or taglib libraries.

      Child tags:

      -  ::

             <auxdata>

         *Optional*

         Currently only adding keywords to auxdata is supported. The keywords are those defined in the id3 specification, we do not perform any extra checking, so you could try to use any string as a keyword - if it does not exist in the tag nothing bad will happen.

         Here is a list of possible keywords:

         TALB, TBPM, TCOM, TCON, TCOP, TDAT, TDLY, TENC, TEXT, TFLT, TIME, TIT1, TIT2, TIT3, TKEY, TLAN, TLEN, TMED, TOAL, TOFN, TOLY, TOPE, TORY, TOWN, TPE1, TPE2, TPE3, TPE4, TPOS, TPUB, TRCK, TRDA, TRSN, TRSO, TSIZ, TSRC, TSSE, TYER, TXXX

         Child tags:

         -  ::

                <add-data tag="TCOM"/>
                <add-data tag="TENC"/>
                ...

            *Optional*

            If the library was able to extract the data according to the given keyword, it will be added to auxdata. You can then use that data in your import scripts.

      A sample configuration for the example described above would be:

      ::

          <id3>
              <auxdata>
                  <add-data tag="TENC"/>
              </auxdata>
          </id3>

Online Content Settings
~~~~~~~~~~~~~~~~~~~~~~~

This section resides under import and defines options for various supported online services.

::

    <online-content fetch-buffer-size="262144" fetch-buffer-fill-size="0">

*Optional*

This tag defines the online content section.

Attributes:

-  ::

       fetch-buffer-size=...

   *Optional*

   *Default: 262144*

   Often, online content can be directly accessed by the player - we will just give it the URL. However, sometimes it may be necessary to proxy the content through MediaTomb. This setting defines the buffer size in bytes, that will be used when fetching content from the web. The value must not be less than allowed by the curl library (usually 16384 bytes).

-  ::

       fetch-buffer-fill-size=...

   *Optional*

   *Default: 0 (disabled)*

   This setting allows to prebuffer a certain amount of data, given in bytes, before sending it to the player, this should ensure a constant data flow in case of slow connections. Usually this setting is not needed, because most players will anyway have some kind of buffering, however if the connection is particularly slow you may want to try enable this setting.

Below are the settings for supported services.

YouTube Service
~~~~~~~~~~~~~~~

MediaTomb allows you to watch YouTube videos using your UPnP device, you can specify what content you are interested in, we will import the meta data and present it in the database, so you can browse the content on your player just like you do it with your local data.

WANING:
    by using this feature you may be violating the YouTube service terms and conditions: ` <http://www.youtube.com/t/terms>`__

Note:
    we do not download/mirror the actual videos, we only get description of the content and present it in the database. The actual video is only streamed when you press play on your device. So, when we use the term ”retrieve videos” we actually mean retrieving the video name, description and associated meta data, not the actual flv file.

Note:
    so far we have not yet seen many devices that will natively support the .flv format, you will need to setup appropriate transcoding profiles to make use of the YouTube feature.

::

    <YouTube enabled="no" refresh="12600" update-at-start="yes" purge-after="1200000" racy-content="exclude">

*Optional*

Options and setting for YouTube service support, the attributes are:

-  ::

       enabled=...

   *Optional*

   *Default: no*

   Enables or disables the service, allowed values are ”yes” or ”no”.

-  ::

       refresh=...

   *Optional*

   *Default: 28800*

   Refresh interval in seconds, each time the interval elapses we will contact the YouTube server and fetch new meta data, presenting the changes in the database view. This allows you to stay up to date with what is available on YouTube, you probably should not need to update more than once per day, so specify a reasonably high value here. Setting the option to 0 (zero) disables automatic refreshing.

-  ::

       purge-after=...

   *Optional*

   *Default: 0 (disabled)*

   Time amount in seconds when data that has not been updated is considered old and can be purged from the database. This setting helps to prevent an endless filling of the database with YouTube meta data, the idea is the following: each video has a unique ID, during a refresh cycle the video meta data will be imported into the database and a time stamp will be added. Each refresh cycle we may encounter same content (i.e. nothing changed on the YouTube site), we will update the time stamp. However, if a particular video is no longer available on YouTube it will not be refreshed, once the time stamp exceeds the purge-after value we will remove the meta data for this particular video from the database.

-  ::

       update-at-start=...

   *Optional*

   *Default: no*

   This option defines if we will contact the service right after MediaTomb was started (about 10 seconds after start up) or if data should be fetched when the first refresh cycle kicks in. It may be useful if you frequently restart the server and do not want the content to be updated each time. Allowed values are ”yes” or ”no”.

-  ::

       format=...

   *Optional*

   *Default: mp4*

   Selects the format in which the videos should be retrieved from YouTube.

   -  ::

          mp4

      A while ago YouTube also offers the videos in the mp4 format which is playable by most players, it will also allow you to seek/pause.

   -  ::

          flv

      Flash video, obviously not a good choice if you want to play it on your UPnP device, only useful in combination with a properly set up transcoding profile.

-  ::

       hd=...

   *Optional*

   *Default: no*

   If set to ”yes”, then a HD video will be retrieved if available.

-  ::

       racy-content=...

   *Optional*

   *Default: exclude*

   Specifies if restricted content should be inclueded in the search results. The option will automatically be used with YouTube requests that support. By default restricted content is not included.

Below are various options that allow you to define what content you are interested in.

Note:
    all tags are optional and can occur more then once.

-  ::

       <standardfeed feed="top_rated" region-id="ru" time-range="today" start-index="1" amount="all"/>

   *Optional*

   This tag tells us to retrieve one of the standard video feeds. It can appear more than once with different options (i.e. different feed and region settings).

   -  ::

          feed=...

      *Required*

      Definition of the standard feed that you are interested in, the allowed values are:

      -  ::

             top_rated

      -  ::

             top_favorites

      -  ::

             most_viewed

      -  ::

             most_recent

      -  ::

             most_discussed

      -  ::

             most_linked

      -  ::

             most_responded

      -  ::

             recently_featured

      -  ::

             watch_on_mobile

   -  ::

          region-id=...

      *Optional*

      Allows to get a standard feed for a specific region, the option is ignored for the watch\_on\_mobile standard feed.

      Currently, the following region id's are supported:

      -  ::

             au

         Australia

      -  ::

             br

         Brazil

      -  ::

             ca

         Canada

      -  ::

             fr

         France

      -  ::

             de

         Germany

      -  ::

             gb

         Great Britain

      -  ::

             nl

         The Netherlands

      -  ::

             hk

         Hong Kong

      -  ::

             ie

         Ireland

      -  ::

             it

         Italy

      -  ::

             jp

         Japan

      -  ::

             mx

         Mexico

      -  ::

             nz

         New Zealand

      -  ::

             pl

         Poland

      -  ::

             ru

         Russia

      -  ::

             kr

         South Korea

      -  ::

             es

         Spain

      -  ::

             tw

         Taiwan

      -  ::

             us

         United States

   -  ::

          time-range=...

      *Optional*

      Allows to specify a certain time range when retrieving the feed content. The time-range option is only supported for the following standard feeds: top\_rated, top\_favorites, most\_viewed, most\_discussed, most\_linked and most\_responded.

      Following values are supported:

      -  ::

             all_time

      -  ::

             today

      -  ::

             this_week

      -  ::

             this_month

   -  ::

          start-index=...

      *Optional*

      Starting index of the item in the list, useful if you want to skip the first xxx results, must be an integer value >= 1.

   -  ::

          amount=...

      Amount of items to fetch, can be either an integer value or the keyword "all".

-  ::

       <favorites user="mediatomb"/>

   *Optional*

   Retrieves all favorites of the given user.

   -  ::

          user=...

      *Required*

      Name of the YouTube user whose favorites should be fetched.

-  ::

       <subscriptions user="mediatomb"/>

   *Optional*

   Retrieves all subscriptions of the given user.

   -  ::

          user=...

      *Required*

      Name of the YouTube user whose subscriptions should be fetched.

-  ::

       <playlists user="mediatomb"/>

   *Optional*

   Retrieve all playlists of the given user.

   -  ::

          user=...

      *Required*

      Name of the YouTube user whose playlists should be fetched.

-  ::

       <uploads user="mediatomb" start-index="1" amount="all"/>

   *Required*

   Retrieves videos that were uploaded by a specified user.

   -  ::

          user=...

      *Required*

      Name of the YouTube user whose playlists should be fetched.

   -  ::

          start-index=...

      *Optional*

      Starting index of the item in the list, useful if you want to skip the first xxx results, must be an integer value >= 1.

   -  ::

          amount=...

      Amount of items to fetch, can be either an integer value or the keyword "all".

Transcoding Settings
--------------------

The transcoding section allows to define ways on how to transcode content.

::

    <transcoding enabled="yes" fetch-buffer-size="262144" fetch-buffer-fill-size="0">

*Optional*

This tag defines the transcoding section.

Attributes:

-  ::

       enabled=...

   *Optional*

   *Default: yes*

   This attribute defines if transcoding is enabled as a whole, possible values are ”yes” or ”no”.

-  ::

       fetch-buffer-size=...

   *Optional*

   *Default: 262144*

   In case you have transcoders that can not handle online content directly (see the accept-url parameter below), it is possible to put the transcoder between two FIFOs, in this case MediaTomb will fetch the online content. This setting defines the buffer size in bytes, that will be used when fetching content from the web. The value must not be less than allowed by the curl library (usually 16384 bytes).

-  ::

       fetch-buffer-fill-size=...

   *Optional*

   *Default: 0 (disabled)*

   This setting allows to prebuffer a certain amount of data before sending it to the transcoder, this should ensure a constant data flow in case of slow connections. Usually this setting is not needed, because most transcoders will just patiently wait for data and we anyway buffer on the output end. However, we observed that ffmpeg will fail to transcode flv files if it encounters buffer underruns - this setting helps to avoid this situation.

Child tags:

-  ::

       <mimetype-profile-mappings>

   The mime type to profile mappings define which mime type is handled by which profile.

   Different mime types can map to the same profile in case that the transcoder in use supports various input formats. The same mime type can also map to several profiles, in this case multiple resources in the XML will be generated, allowing the player to decide which one to take.

   The mappings under mimetype-profile are defined in the following manner:

   -  ::

          <transcode mimetype="audio/x-flac" using="oggflac-pcm"/>

      *Optional*

      In this example we want to transcode our flac audio files (they have the mimetype audio/x-flac) using the ”oggflac-pcm” profile which is defined below.

      -  ::

             mimetype=...

         Selects the mime type of the source media that should be transcoded.

      -  ::

             using=...

         Selects the transcoding profile that will handle the mime type above. Information on how to define transcoding profiles can be found below.

-  ::

       <profiles>

   This section defines the various transcoding profiles.

   -  ::

          <profile name="oggflag-pcm" enabled="yes" type="external">

      *Optional*

      Definition of a transcoding profile.

      -  ::

             name=...

         *Required*

         Name of the transcoding profile, this is the name that is specified in the mime type to profile mappings.

      -  ::

             enabled=...

         *Required*

         Enables or disables the profile, allowed values are ”yes” or ”no”.

      -  ::

             type=...

         *Required*

         Defines the profile type, currently only ”external” is supported, this will change in the future.

      -  ::

             <mimetype>audio/x-wav</mimetype>

         *Required*

         Defines the mime type of the transcoding result (i.e. of the transcoded stream). In the above example we transcode to PCM.

      -  ::

             <accept-url>yes</accept-url>

         *Optional*

         *Default: yes*

         Some transcoders are able to handle non local content, i.e. instead giving a local file name you can pass an URL to the transcoder. However, some transcoders can only deal with local files, for this case set the value to ”no”.

      -  ::

             <first-resource>no</first-resource>

         *Optional*

         *Default: no*

         It is possible to offer more than one resource in the browse result, a good player implementation will go through all resources and pick the one that it can handle best. Unfortunately most players only look at the first resource and ignore the rest. When you add a transcoding profile for a particular media type it will show up as an additional resource in the browse result, using this parameter you can make sure that the transcoded resource appears first in the list.

         Note:
             if more than one transcoding profile is applied on one source media type (i.e. you transcode an OGG file to MP3 and to PCM), and the first-resource parameter is specified in both profiles, then the resource positions are undefined.

      -  ::

             <hide-original-resource>no</hide-original-resource>

         *Optional*

         *Default: no*

         This parameter will hide the resource of the original media when sending the browse result to the player, this can be useful if your device gets confused by multiple resources and allows you to send only the transcoded one.

      -  ::

             <accept-ogg-theora>no</accept-org-theora>

         *Optional*

         *Default: no*

         As you may know, OGG is just a container, the content could be Vorbis or Theora while the mime type is ”application/ogg”. For transcoding we need to identify if we are dealing with audio or video content, specifying yes in this tag in the profile will make sure that only OGG files containing Theora will be processed.

      -  ::

             <avi-fourcc-list mode="ignore">

         *Optional*

         *Default: disabled*

         This option allows to specify a particular list of AVI fourcc strings that can be either set to be ignored or processed by the profile.

         Note:
             this option has no effect on non AVI content.

         -  ::

                mode=...

            *Required*

            Specifies how the list should be handled by the transcoding engine, possible values are:

            -  ::

                   "disabled"

               The option is completely disabled, fourcc list is not being processed.

            -  ::

                   "process"

               Only the fourcc strings that are listed will be processed by the transcoding profile, AVI files with other fourcc strings will be ignored. Setting this is useful if you want to transcode only some specific fourcc's and not transcode the rest.

            -  ::

                   "ignore"

               The fourcc strings listed will not be transcoded, all other codecs will be transcoded. Setting this might be useful if you want to prevent a limited number of codecs from being transcoded, but want to apply transcoding on the rest (i.e. - do not transcode divx and xvid, but want to transcode mjpg and whatever else might be in the AVI container).

         The list of fourcc strings is enclosed in the avi-fourcc-list section:

         -  ::

                <fourcc>XVID</fourcc>
                <fourcc>DX50</fourcc>

            etc...

      -  ::

             <agent command="ogg123" arguments="-d wav -f %out %in/>

         *Required*

         Defines the transcoding agent and the parameters, in the example above we use ogg123 to convert ogg or flac to wav.

         -  ::

                command=...

            *Required*

            Defines the transcoder binary that will be executed by MediaTomb upon a transcode request, the binary must be in $PATH. It is very important that the transcoder is capable of writing the output to a FIFO, some applications, for example ffmpeg, have problems with that. The command line arguments are specified separately (see below).

         -  ::

                arguments=...

            *Required*

            Specifies the command line arguments that will be given to the transcoder application upon execution. There are two special tokens:

            ::

                %in
                %out

            Those tokens get substituted by the input file name and the output FIFO name before execution.

      -  ::

             <buffer size="1048576" chunk-size="131072" fill-size="262144"/>

         *Required*

         These settings help you to achieve a smooth playback of transcoded media. The actual values need to be tuned and depend on the speed of your system. The general idea is to buffer the data before sending it out to the player, it is also possible to delay first playback until the buffer is filled to a certain amount. The prefill should give you enough space to overcome some high bitrate scenes in case your system can not transcode them in real time.

         -  ::

                size=...

            *Required*

            Size of the buffer in bytes.

         -  ::

                chunk-size=...

            *Required*

            Size of chunks in bytes, that are read by the buffer from the transcoder. Smaller chunks will produce a more constant buffer fill ratio, however too small chunks may slow things down.

         -  ::

                fill-size=...

            *Required*

            Initial fill size - number of bytes that have to be in the buffer before the first read (i.e. before sending the data to the player for the first time). Set this to 0 (zero) if you want to disable prefilling.

      -  ::

             <resolution>320x240</resolution>

         *Optional*

         *Default: not specified*

         Allows you to tell the resolution of the transcoded media to your player. This may be helpful if you want to generate thumbnails for your photos, or if your player has the ability to pick video streams in a particular resolution. Of course the setting should match the real resolution of the transcoded media.

      -  ::

             <use-chunked-encoding>yes</use-chunked-encoding>

         *Optional*

         *Default: yes*

         Specifies that the content should be sent out using chunked HTTP encoding, this is the default setting for transcoded streams, because the content length of the data is not known.

      -  ::

             <sample-frequency>source</sample-frequency>

         *Optional*

         *Default: source*

         Specifies the sample frequency of the transcoded media, this information is passed to the player and is particularly important when streaming PCM data. Possible values are: *source*\ (automatically set the same frequency as the frequency of the source content, which is useful if you are not doing any resampling)\ *, off*\ (do not provide this information to the player)\ *, frequency*\ (specify a fixed value, where *frequency* is a numeric value > 0)

      -  ::

             <audio-channels>source</audio-channels>

         *Optional*

         *Default: source*

         Specifies the number of audio channels in the transcoded media, this information is passed to the player and is particularly important when streaming PCM data. Possible values are: *source*\ (automatically set the same number of audio channels as in the source content)\ *, off*\ (do not provide this information to the player)\ *, number*\ (specify a fixed value, where *number* is a numeric value > 0)

      -  ::

             <thumbnail>yes</thumbnail>

         *Optional*

         *Default: no*

         Note:
             this is an experimental option, the implementation will be refined in the future releases.

         This is a special option which was added for the PS3 users. If the resolution option (see above) was set, then, depending on the resolution an special DLNA tag will be added, marking the resource as a thumbnail. This is useful if you have a transcoding script that extracts an image out of the video and presents it as a thumbnail.

         Use the option with caution, no extra checking is being done if the resulting mimetype represents an image, also, it is will only work if the output of the profile is a JPG image.

Supported Devices
=================

**Attention Hardware Manufacturers:**

If you want to improve compatibility between MediaTomb and your renderer device or if you are interested in a port of MediaTomb for your NAS device please e-mail to: <contact at mediatomb dot cc>

MediaRenderers
--------------

MediaTomb supports all UPnP compliant MediaRenderers, however there can always be various problems that depend on the particular device implementation. We always try to implement workarounds to compensate for failures and limitations of various renderers.

This is the list of client devices that MediaTomb has been tested with and that are known to work. Please drop us a mail if you are using MediaTomb with a device that is not in the list, report any success and failure. We will try to fix the issues and will add the device to the list.

Acer
~~~~

-  AT3705-MGW

Asus
~~~~

-  O!Play

Conceptronic
~~~~~~~~~~~~

-  C54WMP

Currys UK
~~~~~~~~~

-  Logik IR100

Denon
~~~~~

-  AVR-3808

-  AVR-4306

-  AVR-4308

-  S-52

-  ASD-3N

D-Link
~~~~~~

-  DSM-320

-  DSM-320RD

-  DSM-510

-  DSM-520

Some additional settings in MediaTomb configuration are required to enable special features for the DSM renderers. If you have a DSM-320 and are experiencing problems during AVI playback, add the following to the server section of your config.xml:

::

    <custom-http-headers>
        <add header="X-User-Agent: redsonic"/>
    </custom-http-headers>

Further, the DSM-320 behaves differently if it thinks that it is dealing with the D-Link server. Add the following to the server section of your configuration to enable srt subtitle support:

::

    <manufacturerURL>redsonic.com</manufacturerURL>
    <modelNumber>105</modelNumber>

It is still being investigated, but we were able to get subtitles working with a U.S. DSM-320 unit running firmware version 1.09

Also, the DSM-510 (probably also valid for other models) will only play avi files if the mimetype is set to video/avi, you may want to add a mapping for that to the extension-mimetype section in your config.xml:

::

    <map from="avi" to="video/avi"/>

Freecom
~~~~~~~

-  MusicPal

Häger
~~~~~

-  OnAir (also known as BT Internet Radio)

HP
~~

-  MediaSmart TV

Users reported that after a firmwre upgrade the device stopped working properly. It seems that it does not sue the UPnP Browse action anymore, but now uses the optional Search action which is not implemented in MediaTomb.

Hifidelio
~~~~~~~~~

-  Hifidelio Pro-S

I-O Data
~~~~~~~~

-  AVeL LinkPlayer2 AVLP2/DVDLA

JVC
~~~

-  DD-3

-  DD-8

Kathrein
~~~~~~~~

-  UFS922

Kodak
~~~~~

-  EasyShare EX-1011

Linn
~~~~

-  Sneaky DS

Linksys
~~~~~~~

-  WMLS11B (Wireless-B Music System)

-  KiSS 1600

Medion
~~~~~~

-  MD 85651

NeoDigits
~~~~~~~~~

-  HELIOS X3000

Netgear
~~~~~~~

-  EVA700

-  MP101

Nokia
~~~~~

-  N-95

-  N-800

Odys
~~~~

-  i-net MusicBox

Philips
~~~~~~~

-  Streamium SL-300i

-  Streamium SL-400i

-  Streamium MX-6000i

-  Streamium NP1100

-  Streamium MCi900

-  WAS7500

-  WAK3300

-  WAC3500D

-  SLA-5500

-  SLA-5520

-  37PFL9603D

Pinnacle
~~~~~~~~

-  ShowCenter 200

-  SoundBridge

Pioneer
~~~~~~~

-  BDP-HD50-K

-  BDP-94HD

Raidsonic
~~~~~~~~~

-  IB-MP308HW-B

Revo
~~~~

-  Pico RadioStation

Roberts
~~~~~~~

-  WM201 WiFi Radio

Playing OGG audio files requres a custom mimetype, add the following to the <extension-mimetype> section and reimport your OGGs:

::

    <map from="ogg" to="audio/ogg"/>

Also, add this to the <mimetype-contenttype> section:

::

    <treat mimetype="audio/ogg" as="ogg"/>

Roku
~~~~

-  SoundBridge M1001

-  SoundBridge M2000

Sagem
~~~~~

-  My Dual Radio 700

Siemens
~~~~~~~

-  Gigaset M740AV

SMC
~~~

-  EZ Stream SMCWAA-G

Snazio
~~~~~~

-  Snazio\* Net DVD Cinema HD SZ1350

Sony
~~~~

-  Playstation 3

Firmware 1.80 introduces UPnP/DLNA support, add the following to the <server> section of your configuration file to enable MediaTomb PS3 compatibility:

::

    <protocolInfo extend="yes"/>

Syabas
~~~~~~

-  Popcorn Hour A110

T+A
~~~

-  T+A Music Player

Tangent
~~~~~~~

-  Quattro MkII

Telegent
~~~~~~~~

-  TG100

The TG100 client has a problem browsing containers, where item titles exceed 101 characters. We implemented a server-side workaround which allows you to limit the lengths of all titles and descriptions. Use the following settings in the <server> section of your configuration file:

::

    <upnp-string-limit>101</upnp-string-limit>

TerraTec
~~~~~~~~

-  NOXON iRadio

-  NOXON 2 Audio

Western Digital
~~~~~~~~~~~~~~~

-  WD TV Live

Vistron
~~~~~~~

-  MX-200I

Xtreamer
~~~~~~~~

-  Xtreamer

Yamaha
~~~~~~

-  RX-V2065

ZyXEL
~~~~~

-  DMA-1000

-  DMA-2500

Some users reported problems where the DMA will show an error ”Failed to retrieve list” and the DMA disconnecting from the server. Incresing the alive interval seems to solve the problem - add the following option to the <server> section of your configuration file:

::

    <alive>600</alive>

Additionally, the DMA expects that avi files are serverd with the mime type of video/avi, so add the following to the <extensoin-mimetype> section in your configuration file:

::

    <map from="avi" to="video/avi"/>

Also, add this to the <mimetype-contenttype> section:

::

    <treat mimetype="video/avi" as="avi"/>

Network Attached Storage Devices
--------------------------------

We provide a bitbake metadata file for the OpenEmbedded environment, it allows to easily cross compile MediaTomb for various platforms. We have successfully tested MediaTomb on ARM and MIPSel based devices, so it should be possible to install and run the server on various Linux based NAS products that are available on the market.

So far two devices are shipped with a preinstalled version of MediaTomb, community firmware versions are available for the rest.

Asus
~~~~

-  WL500g

Use the statically linked mips32el binary package that is provided on our download site.

Buffalo
~~~~~~~

-  KuroBox-HG

-  LinkStation

Excito
~~~~~~

-  Bubba Mini Server (preinstalled)

Iomega
~~~~~~

-  StorCenter (preinstalled)

Linksys
~~~~~~~

-  NSLU2

Available via Optware.

Maxtor
~~~~~~

-  MSS-I

Either use the Optware feeds or the statically linked mips2el binary package that is provided on our download site.

Raidsonic
~~~~~~~~~

-  IB-NAS4200-B

Use the statically linked binary armv4 package that is provided on our download site.

Xtreamer
~~~~~~~~

-  Xtreamer eTRAYz

Western Digital
~~~~~~~~~~~~~~~

-  MyBook

Running The Server
==================

When you run MediaTomb for the first time a default configuration will be created in the ~/.mediatomb directory. If you are using the sqlite database no further intervention is necessary, if you are using MySQL you will have to make some adjustments (see Configuration section for more details). To start the server simply run "mediatomb" from the console, to shutdown cleanly press Ctrl-C. At start up MediaTomb will print a link to the web UI.

Note:
    Internet Explorer support is limited and not yet finished. It is very difficult to support this browser because of a huge number of bugs in its javascript implementation. If you don't believe us - just visit http://selfhtml.org/ and see how often IE is mentioned in not following the specs or simply not working with certain functions and features. We recommend Firefox.

If you want to run a second server from the same PC, make sure to use a different configuration file with a different udn and a different database.

After server launch the bookmark file is created in the ~/.mediatomb directory. You now can manually add the bookmark ~/.mediatomb/mediatomb.html in your browser. This will redirect you to the UI if the server is running.

Assuming that you enabled the UI, you should now be able to get around quite easily.

We also support the daemon mode which allows to start the server in background, the --user and --group parameters should be used to run the server under an unprivileged account. A script for starting/stopping the server is provided.

Legal
=====

**THIS SOFTWARE COMES WITH ABSOLUTELY NO WARRANTY! USE AT YOUR OWN RISK!**

Copyright
---------

**Copyright (C) 2005**

Gena Batyan <bgeradz at mediatomb dot cc>

Sergey Bostandzhyan <jin at mediatomb dot cc>

**Copyright (C) 2006-2008**

Gena Batyan <bgeradz at mediatomb dot cc>

Sergey Bostandzhyan <jin at mediatomb dot cc>

Leonhard Wimmer <leo at mediatomb dot cc>

License
-------

MediaTomb is free software; you can redistribute it and/or modify it under the terms of the GNU General Public License version 2 as published by the Free Software Foundation. MediaTomb is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for more details. You should have received a copy of the GNU General Public License version 2 along with MediaTomb; if not, write to the Free Software Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301, USA.

Acknowledgments
===============

We are using the following code in our tree:

-  uuid from E2fsprogs 1.35 under GNU GPL, Copyright (C) 1996, 1997, 1998 Theodore Ts'o. <tytso at mit dot edu> Some functions from the UPnP SDK were conflicting with libuuid, so we had to take the sources in and do some renaming.

-  md5 implementation by L. Peter Deutsch <ghost at aladdin dot com>, Copyright (c) 1999 Aladdin Enterprises. All rights reserved. (See source headers for further details)

-  md5 javascript implementation distributed under BSD License, Copyright (c) Paul Johnston 1999 - 2002. http://pajhome.org.uk/crypt/md5

-  Prototype JavaScript Framework http://www.prototypejs.org/ (c) 2005-2007 Sam Stephenson, MIT-style license.

-  (heavily modified version of) NanoTree http://nanotree.sourceforge.net/ (c) 2003 (?) Martin Mouritzen <martin at nano dot dk>; LGPL

-  IE PNG fix from http://webfx.eae.net/dhtml/pngbehavior/pngbehavior.html

-  tombupnp is based on pupnp (http://pupnp.sf.net) which is based on libupnp (http://upnp.sf.net), originally distributed under the BSD license, Copyright (c) 2000-2003 Intel Corporation. Note that all changes to libupnp/pupnp code that were made by the MediaTomb team are covered by the LGPL license.

-  ACX\_PTHREAD autoconf script http://autoconf-archive.cryp.to/acx\_pthread.html (c) 2006 Steven G. Johnson <stevenj at alum dot mit dot edu>

-  The the Inotify::nextEvent() function is based on code from the inotify tools package, http://inotify-tools.sf.net/, distributed under GPL v2, (c) Rohan McGovern <rohan at mcgovern dot id dot au>

Contributions
=============

-  Initial version of the MediaTomb start up script was contributed by Iain Lea <iain at bricbrac dot de>

-  TagLib support patch was contributed by Benhur Stein <benhur.stein at gmail dot com>

-  ffmpeg metadata handler was contributed by Ingo Preiml <ipreiml at edu dot uni-klu dot ac dot at>

-  ID3 keyword extraction patch was contributed by Gabriel Burca <gburca-mediatomb at ebixio dot com>

-  tombupnp is kept in sync with the latest pupnp (http://pupnp.sf.net/) patches, see documentation in the tombupnp directory

-  Photo layout by year/month was contributed by Aleix Conchillo Flaqué <aleix at member dot fsf dot org>

-  lastfmlib patch was contributed by Dirk Vanden Boer <dirk dot vdb at gmail dot com>

-  NetBSD patches were contributed by Jared D. McNeill <jmcneill at NetBSD dot org>


