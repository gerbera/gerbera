<img src="https://github.com/gerbera/gerbera/blob/master/artwork/logo-horiz.png?raw=true" />

# Gerbera - UPnP Media Server

 [![Current Release](https://img.shields.io/github/release/gerbera/gerbera.svg?style=for-the-badge)](https://github.com/gerbera/gerbera/releases/latest) [![Build Status](https://img.shields.io/github/workflow/status/gerbera/gerbera/CI%20validation?style=for-the-badge)](https://github.com/gerbera/gerbera/actions?query=workflow%3A%22CI+validation%22+branch%3Amaster) [![Docker Version](https://img.shields.io/docker/v/gerbera/gerbera?color=teal&label=docker&logoColor=white&sort=semver&style=for-the-badge)](https://hub.docker.com/r/gerbera/gerbera/tags?name=v) [![Documentation Status](https://img.shields.io/readthedocs/gerbera?style=for-the-badge)](http://docs.gerbera.io/en/latest/?badge=latest) [![IRC](https://img.shields.io/badge/IRC-on%20freenode-orange.svg?style=for-the-badge)](https://webchat.freenode.net/?channels=#gerbera)

Gerbera is a UPnP media server which allows you to stream your digital media through your home network and consume it on a variety of UPnP compatible devices.

**Pull requests are very welcome and reporting issues is encouraged.**

Gerbera was originally based on [MediaTomb](http://web.archive.org/web/20170911172945/http://mediatomb.cc/)

## Documentation

View our documentation online at [http://docs.gerbera.io](http://docs.gerbera.io).

## Features

* Browse and playback your media via your network on all kinds of devices.
* Web UI with a tree view of the database and the file system, allowing to add/remove/edit/browse your media
* Metadata extraction from MP3, OGG, AAC, M4A, FLAC, JPG (and many more!) files.
* Media thumbnail support
* Highly flexible media format transcoding via plugins / scripts
* Automatic directory rescans (timed, inotify)
* User defined server layout based on extracted metadata
* Supports last.fm scrobbing
* On the fly video thumbnail generation
* Support for external URLs (create links to internet content and serve them via UPnP to your renderer)
* Runs on Linux, BSD, Mac OS X, and more!
* Runs on x86, ARM, MIPS, and more!

## Installing

Head over to the docs page on [Installing Gerbera](http://docs.gerbera.io/en/latest/install.html).

## Building

Gerbera uses [CMake].

### Quick start build instructions:

```
# Assuming Ubuntu base
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

### Install prerequisites.

#### On Ubuntu 16.04
```
apt-get install uuid-dev libsqlite3-dev libmysqlclient-dev \
libmagic-dev libexif-dev libcurl4-openssl-dev libspdlog-dev libpugixml-dev
# If building with LibAV/FFmpeg (-DWITH_AVCODEC=1)
apt-get install libavutil-dev libavcodec-dev libavformat-dev libavdevice-dev \
libavfilter-dev libavresample-dev libswscale-dev libswresample-dev libpostproc-dev
```

The following packages are too old in 16.04 and must be installed from source:
`taglib` (1.11.x), and `libupnp` (1.12.x).

`libupnp` must be configured/built with `--enable-ipv6`. See
`scripts/install-pupnp.sh` for details.

#### On macOS

The Gerbera Team maintains a Homebrew Tap to allow for easy installation of Gerbera Media Server on macOS.

[https://github.com/gerbera/homebrew-gerbera](https://github.com/gerbera/homebrew-gerbera)

## Dependencies

| Lib          	| Version 	| Required? 	| Note                 	     | Compile-time option | Default  |
|--------------	|---------	|-----------	|--------------------------- | --------------------| -------- |
| libupnp      	| >=1.12.1 	| XOR libnpupnp | [pupnp]                    |                     |          |
| libnpupnp     | >=4.0.11 	| XOR libupnp   | [nppupnp]                  |                     |          |
| libuuid      	|         	| Depends on OS | On \*BSD native libuuid is used, others require e2fsprogs-libuuid | | |
| pugixml     	|         	| Required  	| [pugixml]         	     |                     |          |
| libiconv     	|         	| Required  	|                      	     |                     |          |
| sqlite3      	|         	| Required  	| Database storage     	     |                     |          |
| zlib          |        	| Required  	|                            |                     |          |
| fmtlib        |        	| Required  	|                            |                     |          |
| spdlog        |        	| Required  	|                            |                     |          |
| duktape      	| 2.1.0   	| Optional  	| Scripting Support    	     | WITH_JS             | Enabled  |
| mysql        	|         	| Optional  	| Alternate database storage | WITH_MYSQL          | Disabled |
| curl         	|         	| Optional  	| Enables web services 	     | WITH_CURL           | Enabled  |
| [taglib]      | 1.11.1  	| Optional  	| Audio tag support          | WITH_TAGLIB         | Enabled  |
| libmagic     	|         	| Optional  	| File type detection  	     | WITH_MAGIC          | Enabled  |
| libmatroska 	|         	| Optional  	| MKV metadata               | WITH_MATROSKA       | Enabled  |
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

    Copyright (C) 2016-2021
        Gerbera Contributors

[1]: https://sourceforge.net/p/mediatomb/discussion/440751/thread/258c3cf7/?limit=250
[pupnp]: https://github.com/mrjimenez/pupnp.git
[pugixml]: https://github.com/zeux/pugixml
[taglib]: http://taglib.org/
[CMake]: https://cmake.org/
[Ubuntu PPA]: https://launchpad.net/~stephenczetty/+archive/ubuntu/gerbera-updates
[v00d00 overlay]: https://github.com/v00d00/overlay
[duktape]: http://duktape.org
[Docker Hub]: https://hub.docker.com/r/gerbera/gerbera
