#MediaTomb-v00d00 - UPnP MediaServer

MediaTomb is an open source (GPL) UPnP MediaServer which allows you to stream your digital media
through your home network and listen to/watch it on a variety of UPnP compatible devices.

_MediaTomb is pretty much dead upstream, so this is my attempt to get kick it back into life._

**Pull requests are very welcome and reporting issues is encouraged.**

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

## Differences to Vanilla (So far)
- Actively maintained/developed.
- Removed bundled `libupnp` - Now requires upstream 1.8 version.
- Removed bundled `libuuid`.
- Ported to CMake.
- Enhanced cover art support for MP4, FLAC, Vorbis & WMA files (via TagLib).
- TagLib handler opens files read only: stops inotify rescanning the dir on access causing "Object not found" [see here][1].
- Album folders have "creator" metadata (artist).
- Album folders have artwork: either from external files or the tracks embedded artwork.
- Per-track external art support: `filename-of-track.jp\*` (minus the audio file extension).
- WIP port to "Modern C++" / tidying.
- Removed `libflac` use/dep.
- Remove `libmp4v2` use/dep.
- Remove `id3lib` use/dep.
- Fixed `dvd_image_import_script` with spidermonkey 1.8.5.
- Lots of other stuff.

## Building

The project has been ported to [CMake].

### Install prerequisites.

#### On Ubuntu 16.04
```
apt-get install uuid-dev libexpat1-dev libsqlite3-dev libmysqlclient-dev
apt-get install libmozjs185-dev libmagic-dev libexif-dev libcurl4-openssl-dev
```

The following packages are too old in 16.04 and must be installed from source:
`taglib` (1.11.x), and `libupnp` (1.8.x).

### Quick start build instructions:

```
git clone https://github.com/v00d00/mediatomb.git
mkdir build
cd build
cmake ../mediatomb -DWITH_MAGIC=1 -DWITH_MYSQL=1 -DWITH_CURL=1 -DWITH_JS=1 \
-DWITH_TAGLIB=1 -DWITH_AVCODEC=1 -DWITH_DVDNAV=1 -DWITH_EXIF=1 -DWITH_LASTFM=1
make -j4 VERBOSE=1
make install
```
Alternatively, the options can be set using a GUI (make sure to press "c" to configure after toggling settings in the GUI):
```
git clone https://github.com/v00d00/mediatomb.git
mkdir build
cd build
cmake ../mediatomb
make edit_cache
# Enable some of the WITH... options
make -j4 VERBOSE=1
make install
```

## Dependencies

| Lib          	| Version 	| Required? 	| Note                 	|
|--------------	|---------	|-----------	|----------------------	|
| libupnp      	| 1.8     	| Required  	| Must use: [pupnp]	|
| libuuid      	|         	| Required  	|                      	|
| expat        	|         	| Required  	|                      	|
| sqlite3      	|         	| Required  	|                      	|
| mysql        	|         	| Optional  	| Client Libs          	|
| curl         	|         	| Optional  	| Enables web services 	|
| spidermonkey 	| 1.8.5   	| Optional  	| Enables scripting    	|
| taglib       	| 1.11    	| Optional  	| Audio tag support    	|
| libmagic     	|         	| Optional  	| File type detection  	|
| ffmpeg/libav 	|         	| Optional  	| File metadata        	|
| libdvdnav    	|         	| Optional  	| DVD import script    	|
| libexif      	|         	| Optional  	| JPEG Exif metadata   	|
| lastfmlib    	|         	| Optional  	| Enables scrobbling   	|
|              	|         	|           	|                      	|

## Branches
**master**: Where the action happens

**vanilla**: Sourceforge mediatomb with patches to build in 2016

**gentoo**: Pretty much as vanilla (shipped as net-misc/mediatomb).

##Licence

    GPLv2

    Copyright (C) 2005
       Gena Batyan <bgeradz at mediatomb dot cc>
       Sergey Bostandzhyan <jin at mediatomb dot cc>

    Copyright (C) 2006-2008
       Gena Batyan <bgeradz at mediatomb dot cc>
       Sergey Bostandzhyan <jin at mediatomb dot cc>
       Leonhard Wimmer <leo at mediatomb dot cc>

[1]: https://sourceforge.net/p/mediatomb/discussion/440751/thread/258c3cf7/?limit=250
[pupnp]: https://github.com/mrjimenez/pupnp.git
[CMake]: https://cmake.org/