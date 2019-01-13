<img src="https://github.com/gerbera/gerbera/blob/master/artwork/logo-horiz.png?raw=true" />

# Gerbera - UPnP Media Server

 [![Current Release](https://img.shields.io/github/release/gerbera/gerbera.svg)](https://github.com/gerbera/gerbera/releases/latest) [![Build Status](https://travis-ci.org/gerbera/gerbera.svg?branch=master)](https://travis-ci.org/gerbera/gerbera) [![Documentation Status](https://readthedocs.org/projects/gerbera/badge/?version=latest)](http://docs.gerbera.io/en/latest/?badge=latest) [![IRC](https://img.shields.io/badge/IRC-on%20freenode-orange.svg)](https://webchat.freenode.net/?channels=#gerbera)

Gerbera is a UPnP media server which allows you to stream your digital media through your home network and consume it on a variety of UPnP compatible devices.

_Gerbera is based on [MediaTomb](http://mediatomb.cc) which is dead upstream, so this is a community attempt to kick it back into life._

**Pull requests are very welcome and reporting issues is encouraged.**

## Documentation

View our documentation online at [http://docs.gerbera.io](http://docs.gerbera.io).

## Features

* Browse and playback your media via your network on all kinds of devices.
* Metadata extraction from MP3, OGG, AAC, M4A, FLAC, JPG (and many more!) files.
* Media thumbnail support
* Web UI with a tree view of the database and the file system, allowing to add/remove/edit/browse your media
* Highly flexible media format transcoding via plugins / scripts
* Automatic directory rescans (timed, inotify)
* User defined server layout based on extracted metadata
* Supports last fm scrobbing using lastfmlib
* On the fly video thumbnail generation with libffmpegthumbnailer
* Support for external URLs (create links to internet content and serve them via UPnP to your renderer)
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
- Dropped broken youtube support.

## Installing

### Ubuntu
Stephen Czetty maintains a [Ubuntu PPA].

### Gentoo
The latest version and git ebuild are in [the main portage tree](https://packages.gentoo.org/packages/net-misc/gerbera).

### Arch
Gerbera is available in AUR [stable](https://aur.archlinux.org/packages/gerbera/) or [git version](https://aur.archlinux.org/packages/gerbera-git/).

### openSUSE
Gerbera is avaiable on [software.opensuse.org](https://software.opensuse.org/package/gerbera).

### Debian
Gerbera is included in [Testing](https://packages.debian.org/buster/gerbera) and [Unstable](https://packages.debian.org/sid/gerbera).

### Entware (Optware)
Gerbera is available in [Entware](https://github.com/Entware/rtndev/tree/master/gerbera) for your embedded device/router!

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

#### On FreeBSD

_The following has bee tested on FreeBSD 11.0 using a clean jail environment._ 

1. Install the required prerequisites as root using either ports or packages. This can be done via Package manager or ports. (pkg manager is used here.)  Include mysql if you wish to use that instead of SQLite3.
```
pkg install wget git autoconf automake libtool taglib cmake gcc libav ffmpeg libexif pkgconf liblastfm gmake
````

2. Clone repository, build dependencies in current in ports and then build gerbera.
````
git clone https://github.com/gerbera/gerbera.git 
mkdir build
cd build
sh ../gerbera/scripts/install-pupnp18.sh
sh ../gerbera/scripts/install-duktape.sh
cmake ../gerbera -DWITH_MAGIC=1 -DWITH_MYSQL=0 -DWITH_CURL=1 -DWITH_JS=1 -DWITH_TAGLIB=1 -DWITH_AVCODEC=1 -DWITH_EXIF=1 -DWITH_LASTFM=0 -DWITH_SYSTEMD=0
make -j4
sudo make install
````

#### On macOS

The Gerbera Team maintains a Homebrew Tap to allow for easy installation of Gerbera Media Server on macOS.

[https://github.com/gerbera/homebrew-gerbera](https://github.com/gerbera/homebrew-gerbera)

### Quick start build instructions:

```
git clone https://github.com/gerbera/gerbera.git
mkdir build
cd build
cmake ../gerbera -DWITH_MAGIC=1 -DWITH_MYSQL=1 -DWITH_CURL=1 -DWITH_JS=1 \
-DWITH_TAGLIB=1 -DWITH_AVCODEC=1 -DWITH_FFMPEGTHUMBNAILER=1 -DWITH_EXIF=1 -DWITH_LASTFM=1
make -j4
sudo make install
```
Alternatively, the options can be set using a GUI (make sure to press "c" to configure after toggling settings in the GUI):
```
git clone https://github.com/gerbera/gerbera.git
mkdir build
cd build
cmake ../gerbera
make edit_cache
# Enable some of the WITH... options
make -j4
sudo make install
```

## Dependencies

| Lib          	| Version 	| Required? 	| Note                 	     | Compile-time option | Default  |
|--------------	|---------	|-----------	|--------------------------- | --------------------| -------- |
| libupnp      	| 1.8.3   	| Required  	| [pupnp]                    |                     |          |
| libuuid      	|         	| Depends on OS | On \*BSD native libuuid is used, others require e2fsprogs-libuuid | | |
| expat        	|         	| Required  	|                      	     |                     |          |
| libiconv     	|         	| Required  	|                      	     |                     |          |
| sqlite3      	|         	| Required  	| Database storage     	     |                     |          |
| zlib          |        	| Required  	|                            |                     |          |
| duktape      	| 2.1.0   	| Optional  	| Scripting Support    	     | WITH_JS             | Enabled  |
| mysql        	|         	| Optional  	| Alternate database storage | WITH_MYSQL          | Disabled |
| curl         	|         	| Optional  	| Enables web services 	     | WITH_CURL           | Enabled  |
| [taglib]      | 1.11.1  	| Optional  	| Audio tag support          | WITH_TAGLIB         | Enabled  |
| libmagic     	|         	| Optional  	| File type detection  	     | WITH_MAGIC          | Enabled  |
| ffmpeg/libav 	|         	| Optional  	| File metadata              | WITH_AVCODEC        | Disabled |
| libexif      	|         	| Optional  	| JPEG Exif metadata         | WITH_EXIF           | Enabled  |
| libexiv2    	|         	| Optional  	| Exif, IPTC, XMP metadata   | WITH_EXIV2          | Disabled |
| lastfmlib    	| 0.4.0   	| Optional  	| Enables scrobbling   	     | WITH_LASTFM         | Disabled |
| ffmpegthumbnailer |           | Optional      | Generate video thumbnails  | WITH_FFMPEGTHUMBNAILER | Disabled |
| inotify       |               | Optional      | Efficient file monitoring  | WITH_INOTIFY      | Enabled |

## Licence

    GPLv2

    Copyright (C) 2005
       Gena Batyan <bgeradz at mediatomb dot cc>
       Sergey Bostandzhyan <jin at mediatomb dot cc>

    Copyright (C) 2006-2008
       Gena Batyan <bgeradz at mediatomb dot cc>
       Sergey Bostandzhyan <jin at mediatomb dot cc>
       Leonhard Wimmer <leo at mediatomb dot cc>

    Copyright (C) 2016-2019
        Gerbera Contributors

[1]: https://sourceforge.net/p/mediatomb/discussion/440751/thread/258c3cf7/?limit=250
[pupnp]: https://github.com/mrjimenez/pupnp.git
[taglib]: http://taglib.org/
[CMake]: https://cmake.org/
[Ubuntu PPA]: https://launchpad.net/~stephenczetty/+archive/ubuntu/gerbera-updates
[v00d00 overlay]: https://github.com/v00d00/overlay
[duktape]: http://duktape.org
