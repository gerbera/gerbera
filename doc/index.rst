.. image:: _static/gerbera-logo.png
   :alt: Gerbera Media Server Logo
   :target: .

~~~~

Gerbera is an UPnP Media Server.

It allows you to stream your digital media through your home network and listen to/watch it on a variety of UPnP compatible devices.

Gerbera should work with any UPnP compliant client, please tell us if you experience difficulties with
particular models, also take a look at the :ref:`Supported Devices <supported-devices>` list for more information.

Features
~~~~~~~~

 - Browse and playback your media via your network on all kinds of devices.
 - Metadata extraction from MP3, OGG, AAC, M4A, FLAC, JPG (and many more!) files.
 - Media thumbnail support
 - Web UI with a tree view of the database and the file system, allowing to add/remove/edit/browse your media
 - Highly flexible media format transcoding via plugins / scripts
 - Automatic directory rescans (timed, inotify)
 - User defined server layout based on extracted metadata
 - Supports last fm scrobbing using lastfmlib
 - On the fly video thumbnail generation with libffmpegthumbnailer
 - Support for external URLs (create links to internet content and serve them via UPnP to your renderer)
 - runs on Linux, FreeBSD, NetBSD, Mac OS X, eCS
 - runs on x86, Alpha, ARM, MIPS, Sparc, PowerPC

UPnP
~~~~
Gerbera implements the UPnP MediaServer V 1.0 specification that can be found on http://www.upnp.org/.
The current implementation focuses on parts that are required by the specification, however we look into extending the
functionality to cover the optional parts of the spec as well.

Universal Plug and Play (UPnP) is a set of networking protocols that permits networked devices, such as PCs, printers, mobile phones
TVs, stereos, amplifiers and more to seamlessly discover each other's presence on the network and network services
for data sharing,


Legal
~~~~~

**THIS SOFTWARE COMES WITH ABSOLUTELY NO WARRANTY! USE AT YOUR OWN RISK!**

Copyright
~~~~~~~~~
   Copyright (C) 2005
      | Gena Batyan <bgeradz at mediatomb dot cc>
      | Sergey Bostandzhyan <jin at mediatomb dot cc>

   Copyright (C) 2006-2008
      | Gena Batyan <bgeradz at mediatomb dot cc>
      | Sergey Bostandzhyan <jin at mediatomb dot cc>
      | Leonhard Wimmer <leo at mediatomb dot cc>

   Copyright (C) 2016-2019
      | Gerbera Contributors

License
~~~~~~~

Gerbera is free software; you can redistribute it and/or modify it under the terms of the
GNU General Public License version 2 as published by the Free Software Foundation.
Gerbera is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY;
without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
See the GNU General Public License for more details. You should have received a copy of the GNU General Public License
version 2 along with Gerbera; if not, write to the **Free Software Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301, USA.**

Acknowledgments
===============

We are using the following code in our tree:

- md5 implementation by L. Peter Deutsch <ghost at aladdin dot com>, Copyright (c) 1999 Aladdin Enterprises. All rights reserved. (See source headers for further details)
- md5 javascript implementation distributed under BSD License, Copyright (c) Paul Johnston 1999 - 2002. http://pajhome.org.uk/crypt/md5
- Prototype JavaScript Framework http://www.prototypejs.org/ (c) 2005-2007 Sam Stephenson, MIT-style license.
- (heavily modified version of) NanoTree http://nanotree.sourceforge.net/ (c) 2003 (?) Martin Mouritzen <martin at nano dot dk>; LGPL
- IE PNG fix from http://webfx.eae.net/dhtml/pngbehavior/pngbehavior.html
- The the Inotify::nextEvent() function is based on code from the inotify tools package, http://inotify-tools.sf.net/, distributed under GPL v2, (c) Rohan McGovern <rohan at mcgovern dot id dot au>

Contributions
=============

- Gerbera is built on MediaTomb 0.12.1 GIT
- Initial version of the MediaTomb start up script was contributed by Iain Lea <iain at bricbrac dot de>
- TagLib support patch was contributed by Benhur Stein <benhur.stein at gmail dot com>
- ffmpeg metadata handler was contributed by Ingo Preiml <ipreiml at edu dot uni-klu dot ac dot at>
- ID3 keyword extraction patch was contributed by Gabriel Burca <gburca-mediatomb at ebixio dot com
- Photo layout by year/month was contributed by Aleix Conchillo Flaqué <aleix at member dot fsf dot org>
- lastfmlib patch was contributed by Dirk Vanden Boer <dirk dot vdb at gmail dot com>
- NetBSD patches were contributed by Jared D. McNeill <jmcneill at NetBSD dot org>

.. toctree::
   :maxdepth: 1
   :hidden:

   Install Gerbera            <install>
   Run Gerbera                <run>
   Gerbera Daemon             <daemon>
   Gerbera UI                 <ui>
   Supported Devices          <supported-devices>
   Transcoding                <transcoding>
   Scripting                  <scripting>
   Configuration              <config-overview>
   Compile Gerbera            <compile>

Index
==================

* :ref:`genindex`
