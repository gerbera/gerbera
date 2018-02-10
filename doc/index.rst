.. gerbera documentation master file, created by
   sphinx-quickstart on Sat Dec 23 17:22:17 2017.
   You can adapt this file completely to your liking, but it should at least
   contain the root `toctree` directive.

Welcome to gerbera's documentation!
===================================

.. toctree::
   :maxdepth: 1
   :caption: Contents:

   Install Gerbera            <install>
   Compile Gerbera            <compile>
   Run Gerbera                <run>
   Gerbera Daemon             <daemon>
   Configuration Overview     <config-overview>
   Configure Server           <config-server>
   Configure Extended Options <config-extended>
   Configure Content Import   <config-import>
   Configure Online Content   <config-online>
   Configure Transcoding      <config-transcode>
   Gerbera UI                 <ui>
   Scripting                  <scripting>
   Transcoding                <transcoding>
   Supported Devices          <supported-devices>

Indices and Tables
==================

* :ref:`genindex`

Introduction
============

Gerbera is an open source (GPL) UPnP MediaServer with a nice web user interface, it allows you to stream your digital
media through your home network and listen to/watch it on a variety of UPnP compatible devices.

Gerbera implements the UPnP MediaServer V 1.0 specification that can be found on http://www.upnp.org/.
The current implementation focuses on parts that are required by the specification, however we look into extending the
functionality to cover the optional parts of the spec as well.

Gerbera should work with any UPnP compliant MediaRenderer, please tell us if you experience difficulties with
particular models, also take a look at the :ref:`Supported Devices <supported-devices>` list for more information.

**WARNING!**

The server has an integrated :ref:`file system browser <filesystem-view>` in the UI, that means that anyone who has access to the UI can browse
your file system (with user permissions under which the server is running) and also download your data!
If you want maximum security - disable the UI completely! :ref:`Account authentication <ui>` offers simple protection that might
hold back your kids, but it is not secure enough for use in an untrusted environment!

Note:
    Since the server is meant to be used in a home LAN environment the UI is enabled by default and accounts are
    deactivated, thus allowing anyone on your network to connect to the user interface.

Currently Supported Features
----------------------------

-  browse and playback your media via UPnP
-  metadata extraction from mp3, ogg, flac, jpeg, etc. files.
-  Exif thumbnail support
-  user defined server layout based on extracted metadata (scriptable virtual containers)
-  automatic directory rescans
-  sophisticated web UI with a tree view of the database and the file system, allowing to add/remove/edit/browse your media
-  highly flexible media format transcoding via plugins / scripts
-  supports last fm scrobbing using lastfmlib
-  on the fly video thumbnail generation with libffmpegthumbnailer
-  support for external URLs (create links to internet content and serve them via UPnP to your renderer)
-  support for ContentDirectoryService container updates
-  Active Items (experimental feature), allows execution of server side scripts upon HTTP GET requests to certain items
-  highly flexible configuration, allowing you to control the behavior of various features of the server
-  runs on Linux, FreeBSD, NetBSD, Mac OS, eCS
-  runs on x86, Alpha, ARM, MIPS, Sparc, PowerPC

.. _gerbera-requirements:

Requirements
============

Note:
    remember to install associated development packages, because development headers are needed for compilation!

Note:
    you need at least one database in order to compile and run Gerbera - either sqlite or mysql.

In order to compile Gerbera you will have to install the following packages.:


+-------------------+-----------+---------------+----------------------------+------------------------+----------+
| Library           | Version   | Required?     | Note                       | Compile-time option    | Default  |
+===================+===========+===============+============================+========================+==========+
| libupnp           | 1.8.3     | Required      | `pupnp <https://github.com/mrjimenez/pupnp>`_       |          |
+-------------------+-----------+---------------+----------------------------+------------------------+----------+
| libuuid           |           | Depends on OS | On BSD native libuuid is   |                        |          |
|                   |           |               | used others require        |                        |          |
|                   |           |               | e2fsprogs-libuuid          |                        |          |
+-------------------+-----------+---------------+----------------------------+------------------------+----------+
| expat             |           | Required      |                            |                        |          |
+-------------------+-----------+---------------+----------------------------+------------------------+----------+
| libiconv          |           | Required      |                            |                        |          |
+-------------------+-----------+---------------+----------------------------+------------------------+----------+
| sqlite3           |           | Required      | Database storage           |                        |          |
+-------------------+-----------+---------------+----------------------------+------------------------+----------+
| duktape           | 2.1.0     | Optional      | Scripting Support          | WITH_JS                | Enabled  |
+-------------------+-----------+---------------+----------------------------+------------------------+----------+
| mysql             |           | Optional      | Alternate database storage | WITH_MYSQL             | Disabled |
+-------------------+-----------+---------------+----------------------------+------------------------+----------+
| curl              |           | Optional      | Enables web services       | WITH_CURL              | Enabled  |
+-------------------+-----------+---------------+----------------------------+------------------------+----------+
| taglib            | 1.11.1    | Optional      | Audio tag support          | WITH_TAGLIB            | Enabled  |
+-------------------+-----------+---------------+----------------------------+------------------------+----------+
| libmagic          |           | Optional      | File type detection        | WITH_MAGIC             | Enabled  |
+-------------------+-----------+---------------+----------------------------+------------------------+----------+
| ffmpeg/libav      |           | Optional      | File metadata              | WITH_AVCODEC           | Disabled |
+-------------------+-----------+---------------+----------------------------+------------------------+----------+
| libexif           |           | Optional      | JPEG Exif metadata         | WITH_EXIF              | Enabled  |
+-------------------+-----------+---------------+----------------------------+------------------------+----------+
| libexiv2          |           | Optional      | Exif, IPTC, XMP metadata   | WITH_EXIV2             | Disabled |
+-------------------+-----------+---------------+----------------------------+------------------------+----------+
| lastfmlib         | 0.4.0     | Optional      | Enables scrobbling         | WITH_LASTFM            | Disabled |
+-------------------+-----------+---------------+----------------------------+------------------------+----------+
| ffmpegthumbnailer |           | Optional      | Generate video thumbnails  | WITH_FFMPEGTHUMBNAILER | Disabled |
+-------------------+-----------+---------------+----------------------------+------------------------+----------+
| inotify           |           | Optional      | Efficient file monitoring  | WITH_INOTIFY           | Enabled  |
+-------------------+-----------+---------------+----------------------------+------------------------+----------+


In order to use the web UI you will need to have javascript enabled in your web browser.

The :ref:`Gerbera UI <gerbera-ui>` has been tested and works on recent evergreen browsers:

-  Chrome
-  Firefox


Legal
=====

**THIS SOFTWARE COMES WITH ABSOLUTELY NO WARRANTY! USE AT YOUR OWN RISK!**

Copyright
---------

GPLv2

   Copyright (C) 2005
      | Gena Batyan <bgeradz at mediatomb dot cc>
      | Sergey Bostandzhyan <jin at mediatomb dot cc>

   Copyright (C) 2006-2008
      | Gena Batyan <bgeradz at mediatomb dot cc>
      | Sergey Bostandzhyan <jin at mediatomb dot cc>
      | Leonhard Wimmer <leo at mediatomb dot cc>

   Copyright (C) 2016-2018
      | Gerbera Contributors

License
-------

Gerbera is free software; you can redistribute it and/or modify it under the terms of the
GNU General Public License version 2 as published by the Free Software Foundation.
Gerbera is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY;
without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
See the GNU General Public License for more details. You should have received a copy of the GNU General Public License
version 2 along with Gerbera; if not, write to the **Free Software Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301, USA.**

Acknowledgments
===============

We are using the following code in our tree:

-  uuid from E2fsprogs 1.35 under GNU GPL, Copyright (C) 1996, 1997, 1998 Theodore Ts'o. <tytso at mit dot edu> Some functions from the UPnP SDK were conflicting with libuuid, so we had to take the sources in and do some renaming.

-  md5 implementation by L. Peter Deutsch <ghost at aladdin dot com>, Copyright (c) 1999 Aladdin Enterprises. All rights reserved. (See source headers for further details)

-  md5 javascript implementation distributed under BSD License, Copyright (c) Paul Johnston 1999 - 2002. http://pajhome.org.uk/crypt/md5

-  Prototype JavaScript Framework http://www.prototypejs.org/ (c) 2005-2007 Sam Stephenson, MIT-style license.

-  (heavily modified version of) NanoTree http://nanotree.sourceforge.net/ (c) 2003 (?) Martin Mouritzen <martin at nano dot dk>; LGPL

-  IE PNG fix from http://webfx.eae.net/dhtml/pngbehavior/pngbehavior.html

-  ACX\_PTHREAD autoconf script http://autoconf-archive.cryp.to/acx\_pthread.html (c) 2006 Steven G. Johnson <stevenj at alum dot mit dot edu>

-  The the Inotify::nextEvent() function is based on code from the inotify tools package, http://inotify-tools.sf.net/, distributed under GPL v2, (c) Rohan McGovern <rohan at mcgovern dot id dot au>

Contributions
=============

-  Initial version of the MediaTomb start up script was contributed by Iain Lea <iain at bricbrac dot de>

-  TagLib support patch was contributed by Benhur Stein <benhur.stein at gmail dot com>

-  ffmpeg metadata handler was contributed by Ingo Preiml <ipreiml at edu dot uni-klu dot ac dot at>

-  ID3 keyword extraction patch was contributed by Gabriel Burca <gburca-mediatomb at ebixio dot com

-  Photo layout by year/month was contributed by Aleix Conchillo Flaqué <aleix at member dot fsf dot org>

-  lastfmlib patch was contributed by Dirk Vanden Boer <dirk dot vdb at gmail dot com>

-  NetBSD patches were contributed by Jared D. McNeill <jmcneill at NetBSD dot org>


