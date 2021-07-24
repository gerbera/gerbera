<img src="https://github.com/gerbera/gerbera/blob/master/artwork/logo-horiz.png?raw=true" />

# Gerbera - UPnP Media Server

 [![Current Release](https://img.shields.io/github/release/gerbera/gerbera.svg?style=for-the-badge)](https://github.com/gerbera/gerbera/releases/latest) [![Build Status](https://img.shields.io/github/workflow/status/gerbera/gerbera/CI%20validation?style=for-the-badge)](https://github.com/gerbera/gerbera/actions?query=workflow%3A%22CI+validation%22+branch%3Amaster) [![Docker Version](https://img.shields.io/docker/v/gerbera/gerbera?color=teal&label=docker&logoColor=white&sort=semver&style=for-the-badge)](https://hub.docker.com/r/gerbera/gerbera/tags?name=v) [![Documentation Status](https://img.shields.io/readthedocs/gerbera?style=for-the-badge)](http://docs.gerbera.io/en/stable/?badge=stable) [![IRC](https://img.shields.io/badge/IRC-on%20freenode-orange.svg?style=for-the-badge)](https://webchat.freenode.net/?channels=#gerbera)

Gerbera is a UPnP media server which allows you to stream your digital media through your home network and consume it on a variety of UPnP compatible devices.

**Pull requests are very welcome and reporting issues is encouraged.**

## Documentation
View our documentation online at [https://docs.gerbera.io](https://docs.gerbera.io).

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
Head over to the docs page on [Installing Gerbera](https://docs.gerbera.io/en/stable/install.html) for instructions on
how to install Gerbera.

## Building
Visit our docs for instructions to [Compile Gerbera](https://docs.gerbera.io/en/stable/compile.html).

### Quick start build instructions:
```
git clone https://github.com/gerbera/gerbera.git
mkdir build
cd build
cmake ../gerbera -DWITH_DEBUG=YES
make -j4
sudo make install
```

## Dependencies

| Library       | Min Version   | Required?     | Note                       | Compile-time option    | Default  | Script             |
|---------------|---------------|---------------|----------------------------|------------------------|----------|--------------------|
| libupnp       | 1.14.0        | XOR libnpupnp | [pupnp]                    |                        |          | install-pupnp.sh   |
| libnpupnp     | 4.1.2         | XOR libupnp   | [npupnp]                   | WITH_NPUPNP            | Disabled |                    |
| libuuid       |               | Depends on OS | Not required on \*BSD      |                        |          |                    |
| [pugixml]     |               | Required      | XML file and data support  |                        |          | install-pugixml.sh |
| libiconv      |               | Required      | Charset conversion         |                        |          |                    |
| sqlite3       | 3.7.0         | Required      | Database storage           |                        |          |                    |
| zlib          |               | Required      | Data compression           |                        |          |                    |
| [fmtlib]      | 5.3           | Required      | Fast string formatting     |                        |          | install-fmt.sh     |
| [spdlog]      |               | Required      | Runtime logging            |                        |          | install-spdlog.sh  |
| [duktape]     | 2.1.0         | Optional      | Scripting Support          | WITH_JS                | Enabled  | install-duktape.sh |
| mysql         |               | Optional      | Alternate database storage | WITH_MYSQL             | Disabled |                    |
| curl          |               | Optional      | Enables web services       | WITH_CURL              | Enabled  |                    |
| [taglib]      | 1.11.1        | Optional      | Audio tag support          | WITH_TAGLIB            | Enabled  | install-taglib.sh  |
| libmagic      |               | Optional      | File type detection        | WITH_MAGIC             | Enabled  |                    |
| libmatroska   |               | Optional      | MKV metadata               | WITH_MATROSKA          | Enabled  |                    |
| ffmpeg/libav  |               | Optional      | File metadata              | WITH_AVCODEC           | Disabled |                    |
| libexif       |               | Optional      | JPEG Exif metadata         | WITH_EXIF              | Enabled  |                    |
| libexiv2      |               | Optional      | Exif, IPTC, XMP metadata   | WITH_EXIV2             | Disabled |                    |
| [lastfmlib]   | 0.4.0         | Optional      | Enables scrobbling         | WITH_LASTFM            | Disabled | install-lastfm.sh  |
| [ffmpegthumbnailer] |         | Optional      | Generate video thumbnails  | WITH_FFMPEGTHUMBNAILER | Disabled |                    |
| inotify       |               | Optional      | Efficient file monitoring  | WITH_INOTIFY           | Enabled  |                    |

Scripts for installation of (build) dependencies from source can be found under `scripts`.

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

[Docker Hub]: https://hub.docker.com/r/gerbera/gerbera
[duktape]: http://duktape.org
[ffmpegthumbnailer]: https://github.com/dirkvdb/ffmpegthumbnailer
[fmtlib]: https://github.com/fmtlib/fmt
[lastfmlib]: https://github.com/dirkvdb/lastfmlib
[npupnp]: https://www.lesbonscomptes.com/upmpdcli/npupnp-doc/libnpupnp.html
[pugixml]: https://github.com/zeux/pugixml
[pupnp]: https://github.com/pupnp/pupnp
[spdlog]: https://github.com/gabime/spdlog
[taglib]: http://taglib.org/
