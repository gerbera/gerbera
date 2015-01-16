#MediaTomb - UPnP MediaServer

MediaTomb is an open source (GPL) UPnP MediaServer which allows you to stream your digital media
through your home network and listen to/watch it on a variety of UPnP compatible devices.

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

##Building

The usual:

    $ ./configure
    $ make
    $ make install

If you have a recently cloned source tree remember to

    $ autoreconf -i

##Licence

    Copyright (C) 2005
       Gena Batyan <bgeradz at mediatomb dot cc>
       Sergey Bostandzhyan <jin at mediatomb dot cc>
    
    Copyright (C) 2006-2008
       Gena Batyan <bgeradz at mediatomb dot cc>
       Sergey Bostandzhyan <jin at mediatomb dot cc>
       Leonhard Wimmer <leo at mediatomb dot cc>