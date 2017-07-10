<img src="https://github.com/v00d00/gerbera/blob/master/artwork/logo-horiz.png?raw=true" />

# Gerbera - UPnP Media Server

 [![Current Release](https://img.shields.io/github/release/v00d00/gerbera.svg)](https://github.com/v00d00/gerbera/releases/latest) [![Build Status](https://travis-ci.org/v00d00/gerbera.svg?branch=master)](https://travis-ci.org/v00d00/gerbera) [![IRC](https://img.shields.io/badge/chat-on%20freenode-blue.svg)](https://webchat.freenode.net/?channels=#gerbera)

Gerbera is a UPnP media server which allows you to stream your digital media through your home network and consume it on a variety of UPnP compatible devices.

_Gerbera is based on [MediaTomb 12.1](http://mediatomb.cc) which is dead upstream, so this is my attempt to kick it back into life._

**Pull requests are very welcome and reporting issues is encouraged.**

## Features

* Browse and playback your media via UPnP
* Metadata extraction from mp3, ogg, flac, jpeg, etc. files.
* Exif thumbnail support
* User defined server layout based on extracted metadata (scriptable virtual containers)
* Automatic directory rescans (timed, inotify)
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

## Differences to Mediatomb (So far)
- Rebranded as Gerbera, new Logo!
- Actively maintained/developed.
- Removed bundled `libupnp` - Now requires upstream 1.8 version.
- Removed bundled `libuuid`.
- Ported to CMake.
- Enhanced cover art support for MP4, FLAC, Vorbis & WMA files (via TagLib).
- TagLib handler opens files read only: stops inotify rescanning the dir on access causing "Object not found" [see here][1].
- Album folders have "creator" metadata (artist).
- Album folders have artwork: either from external files or the tracks embedded artwork.
- Per-track external art support: `filename-of-track.jp\*` (minus the audio file extension).
- Removed `libflac` use/dep.
- Remove `libmp4v2` use/dep.
- Remove `id3lib` use/dep.
- Removed broken DVD image support (dvdnav).
- IPv6 Support
- Replaced SpiderMonkey (mozjs) dependency with embedded [duktape] engine.
- WIP port to "Modern C++" / tidying.
- Lots of other stuff.

## Installing

### Ubuntu
Stephen Czetty maintains a [Ubuntu PPA].

### Gentoo
The latest version and git ebuild are in [the main portage tree](https://packages.gentoo.org/packages/net-misc/gerbera).

### Arch
Gerbera is available in [AUR](https://aur.archlinux.org/packages/gerbera/).

### openSUSE
Gerbera is avaiable on [software.opensuse.org](https://software.opensuse.org/package/gerbera).

### Entware (Optware)
Gerbera is available in [Entware-ng](https://github.com/Entware-ng/rtndev/tree/master/gerbera) for your embedded device/router!

## Building

The project has been ported to [CMake].

### Install prerequisites.

#### On Ubuntu 16.04
```
apt-get install uuid-dev libexpat1-dev libsqlite3-dev libmysqlclient-dev \
libmagic-dev libexif-dev libcurl4-openssl-dev
# If building with LibAV/FFmpeg (-DWITH_AVCODEC=1)
apt-get install libavutil-dev libavcodec-dev libavformat-dev libavdevice-dev \
libavfilter-dev libavresample-dev libswscale-dev libswresample-dev libpostproc-dev
```

The following packages are too old in 16.04 and must be installed from source:
`taglib` (1.11.x), and `libupnp` (1.8.x).

`libupnp` must be configured/built with `--enable-ipv6`. See
`scripts/install-pupnp18.sh` for details.

### Quick start build instructions:

```
git clone https://github.com/v00d00/gerbera.git
mkdir build
cd build
cmake ../gerbera -DWITH_MAGIC=1 -DWITH_MYSQL=1 -DWITH_CURL=1 -DWITH_JS=1 \
-DWITH_TAGLIB=1 -DWITH_AVCODEC=1 -DWITH_FFMPEGTHUMBNAILER=1 -DWITH_EXIF=1 -DWITH_LASTFM=1
make -j4
sudo make install
```
Alternatively, the options can be set using a GUI (make sure to press "c" to configure after toggling settings in the GUI):
```
git clone https://github.com/v00d00/gerbera.git
mkdir build
cd build
cmake ../gerbera
make edit_cache
# Enable some of the WITH... options
make -j4
sudo make install
```

## Dependencies

| Lib          	| Version 	| Required? 	| Note                 	    |
|--------------	|---------	|-----------	|-------------------------- |
| libupnp      	| 1.8     	| Required  	| [pupnp] with IPv6 support |
| libuuid      	|         	| Required  	|                      	    |
| expat        	|         	| Required  	|                      	    |
| sqlite3      	|         	| Required  	|                      	    |
| duktape      	|         	| Optional  	| Scripting Support    	    |
| mysql        	|         	| Optional  	| Client Libs          	    |
| curl         	|         	| Optional  	| Enables web services 	    |
| [taglib]      | 1.11    	| Optional  	| Audio tag support         |
| libmagic     	|         	| Optional  	| File type detection  	    |
| ffmpeg/libav 	|         	| Optional  	| File metadata             |
| libexif      	|         	| Optional  	| JPEG Exif metadata        |
| lastfmlib    	|         	| Optional  	| Enables scrobbling   	    |
| ffmpegthumbnailer |      	| Optional   	| Generate video thumbnails |

## Branches
**master**: Where the action happens

**vanilla**: Sourceforge mediatomb with patches to build on more modern systems

**gentoo**: Pretty much as vanilla (shipped as net-misc/mediatomb).

## Licence

    GPLv2

    Copyright (C) 2005
       Gena Batyan <bgeradz at mediatomb dot cc>
       Sergey Bostandzhyan <jin at mediatomb dot cc>

    Copyright (C) 2006-2008
       Gena Batyan <bgeradz at mediatomb dot cc>
       Sergey Bostandzhyan <jin at mediatomb dot cc>
       Leonhard Wimmer <leo at mediatomb dot cc>

    Copyright (C) 2016-2017
        Gerbera Contributors

[1]: https://sourceforge.net/p/mediatomb/discussion/440751/thread/258c3cf7/?limit=250
[pupnp]: https://github.com/mrjimenez/pupnp.git
[taglib]: http://taglib.org/
[CMake]: https://cmake.org/
[Ubuntu PPA]: https://launchpad.net/~stephenczetty/+archive/ubuntu/gerbera
[v00d00 overlay]: https://github.com/v00d00/overlay
[duktape]: http://duktape.org
