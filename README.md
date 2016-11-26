#MediaTomb-v00d00 - UPnP MediaServer

MediaTomb is an open source (GPL) UPnP MediaServer which allows you to stream your digital media
through your home network and listen to/watch it on a variety of UPnP compatible devices.

It is pretty much dead upstream, so this is my attempt to get kick it back into life.

Patches are very welcome as are issue reports.

##Features

* Browse and playback your media via UPnP
* Metadata extraction from mp3, ogg, flac, jpeg, etc. files.
* Exif thumbnail support
* User defined server layout based on extracted metadata (scriptable virtual containers)
* Automatic directory rescans
* Web UI with a tree view of the database and the file system, allowing to add/remove/edit/browse your media
* Highly flexible media format transcoding via plugins / scripts
* Allows to watch YouTube(tm) videos on your UPnP player device [Broken]
* Supports last fm scrobbing using lastfmlib 
* On the fly video thumbnail generation with libffmpegthumbnailer
* Support for external URLs (create links to internet content and serve them via UPnP to your renderer)
* Support for ContentDirectoryService container updates
* Active Items (experimental feature), allows execution of server side scripts upon HTTP GET requests to certain items
* Highly flexible configuration, allowing you to control the behavior of various features of the server
* runs on Linux, FreeBSD, NetBSD, Mac OS X, eCS
* runs on x86, Alpha, ARM, MIPS, Sparc, PowerPC

## Branches
master: Where the action happens

vanilla: Sourceforge mediatomb with patches to build in 2016

gentoo: Pretty much as vanilla (shipped as net-misc/mediatomb).


## Differences to Vanilla
- Removed bundled libupnp - Now requires upstream 1.8 version.
- Removed bundled libuuid
- Ported to CMake removal of autotools
- Fixed dvd_image_import_script with spidermonkey 1.8.5

## Building

The project has been ported to [CMake](https://cmake.org/).

```
cmake CMakeLists.txt -DWITH_MAGIC=1 -DWITH_MYSQL=1 -DWITH_CURL=1 -DWITH_JS=1 \
-DWITH_TAGLIB=1 -DWITH_AVCODEC=1 -DWITH_DVDNAV=1 -DWITH_EXIF=1 -DWITH_LASTFM=1
make -j4
make install

```

### Dependencies

| Lib          	| Version 	| Required? 	| Note                 	|
|--------------	|---------	|-----------	|----------------------	|
| libupnp      	| 1.8     	| Required  	|                      	|
| libuuid      	|         	| Required  	|                      	|
| expat        	|         	| Required  	|                      	|
| sqlite3      	|         	| Required  	|                      	|
| mysql        	|         	| Optional  	| Client Libs          	|
| curl         	|         	| Optional  	| Enables web services 	|
| spidermonkey 	| 1.8.5   	| Optional  	| Enables scripting    	|
| taglib       	|         	| Optional  	| Audio tag support    	|
| libmagic     	|         	| Optional  	| File type detection  	|
| ffmpeg/libav 	|         	| Optional  	| File metadata        	|
| libdvdnav    	|         	| Optional  	| DVD import script    	|
| libexif      	|         	| Optional  	| JPEG Exif metadata   	|
| lastfmlib    	|         	| Optional  	| Enables scrobbling   	|
|              	|         	|           	|                      	|

##Licence

    GPLv2

    Copyright (C) 2005
       Gena Batyan <bgeradz at mediatomb dot cc>
       Sergey Bostandzhyan <jin at mediatomb dot cc>

    Copyright (C) 2006-2008
       Gena Batyan <bgeradz at mediatomb dot cc>
       Sergey Bostandzhyan <jin at mediatomb dot cc>
       Leonhard Wimmer <leo at mediatomb dot cc>