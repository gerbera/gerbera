.. _supported-devices:
.. index:: Supported Devices

Supported Devices
=================

**Attention Hardware Manufacturers:**

If you want to improve compatibility between Gerbera and your renderer device or if you are interested in a port of
Gerbera for your NAS device please contact Gerbera Contributors `https://github.com/gerbera <https://github.com/gerbera>`_

Automatic Device Detection
--------------------------

With each UPnP request a device sends its signature which is used by gerbera to assign special functionality to the response.
The automatic detection supports the following devices and software:

+-------------------------------------+--------------------------------------------------+---------------------------------------------------------+
| Device Type                         | Description                                      | Device Flags                                            |
+=====================================+==================================================+=========================================================+
| StandardUPnP                        | all devices sending signature UPnP/1.0           |                                                         |
+-------------------------------------+--------------------------------------------------+---------------------------------------------------------+
| BubbleUPnP                          | running on smartphones                           |                                                         |
+-------------------------------------+--------------------------------------------------+---------------------------------------------------------+
| SamsungAllShare                     | AllShare running on Windows                      |                                                         |
+-------------------------------------+--------------------------------------------------+---------------------------------------------------------+
| SamsungSeriesQ                      | Samsung Series [Q] TVs                           | ``SAMSUNG|SAMSUNG_FEATURES|SAMSUNG_HIDE_DYNAMIC|DCM10`` |
+-------------------------------------+--------------------------------------------------+---------------------------------------------------------+
| SamsungSeriesCDE                    | Samsung other TVs                                | ``SAMSUNG|SAMSUNG_FEATURES|SAMSUNG_HIDE_DYNAMIC``       |
+-------------------------------------+--------------------------------------------------+---------------------------------------------------------+
| SamsungBDP                          | Samsung Blu-ray Player BD-D5100                  | ``SAMSUNG|SAMSUNG_FEATURES``                            |
+-------------------------------------+--------------------------------------------------+---------------------------------------------------------+
| SamsungBDJ5500                      | Samsung Blu-ray Player J5500                     | ``SAMSUNG|SAMSUNG_FEATURES``                            |
+-------------------------------------+--------------------------------------------------+---------------------------------------------------------+
| EC-IRadio                           | e.g. Dual CR 510                                 | ``IRADIO``                                              |
+-------------------------------------+--------------------------------------------------+---------------------------------------------------------+
| FSL/Frontier Silicon/Frontier Smart | e.g. Technisat DigitRadio 580                    | ``SIMPLE_DATE``                                         |
+-------------------------------------+--------------------------------------------------+---------------------------------------------------------+
| PanasonicTV                         | all Panasonic TV                                 | ``PANASONIC``                                           |
+-------------------------------------+--------------------------------------------------+---------------------------------------------------------+
| BoseSoundtouch                      | Running on Smartphone to support Bose Multiroom  | ``STRICTXML``                                           |
+-------------------------------------+--------------------------------------------------+---------------------------------------------------------+
| Yamaha RX                           | e.g RX-V675                                      | ``ASCIIXML``                                            |
+-------------------------------------+--------------------------------------------------+---------------------------------------------------------+
| Freebox Player                      |                                                  | ``NO_CONVERSION``                                       |
+-------------------------------------+--------------------------------------------------+---------------------------------------------------------+

.. _device-flags:

Device Flags
~~~~~~~~~~~~

The device flags have the following meaning

+------------------------------+---------+--------------------------------------------------------------------------------+
| Key                          | Value   | Description                                                                    |
+==============================+=========+================================================================================+
| ``SAMSUNG``                  | 0x01    | Add "CaptionInfo.sec" to video header                                          |
+------------------------------+---------+--------------------------------------------------------------------------------+
| ``SAMSUNG_BOOKMARK_SEC``     | 0x02    | Restore last played position when the media is played again                    |
|                              |         | (only for Samsung). Not applicable for transcoded media                        |
+------------------------------+---------+--------------------------------------------------------------------------------+
| ``SAMSUNG_BOOKMARK_MSEC``    | 0x04    | Same as ``SAMSUNG_BOOKMARK_SEC``, but the bookmark value is received           |
|                              |         | from the Samsung TV as msec                                                    |
+------------------------------+---------+--------------------------------------------------------------------------------+
| ``SAMSUNG_FEATURES``         | 0x10    | Activate Samsung features protocol                                             |
|                              |         |                                                                                |
|                              |         | (support for additional request like                                           |
|                              |         | `X_GetFeatureList`, `X_GetObjectIDfromIndex`, `X_GetIndexfromRID`)             |
+------------------------------+---------+--------------------------------------------------------------------------------+
| ``SAMSUNG_HIDE_DYNAMIC``     | 0x20    | Some Samsung devices do not work with dynamic folders on top level,            |
|                              |         | so this flag hides them for those devices                                      |
+------------------------------+---------+--------------------------------------------------------------------------------+
| ``IRADIO``                   | 0x08    | Don't send ``<?xml ...?>`` declaration in response                             |
+------------------------------+---------+--------------------------------------------------------------------------------+
| ``PV_SUBTITLES``             | 0x40    | Add attributes ``pv:subtitleFileType`` and ``pv:subtitleFileUri``              |
|                              |         | to video files if subtitles exist                                              |
+------------------------------+---------+--------------------------------------------------------------------------------+
| ``PANASONIC``                | 0x80    | Avoid adding part of the filename in item uri. Filename in item uri is         |
|                              |         | used by other players,                                                         |
|                              |         |                                                                                |
|                              |         | e.g. VLC to detect the language of a subtitle file.                            |
+------------------------------+---------+--------------------------------------------------------------------------------+
| ``STRICTXML``                | 0x100   | Encode most of special chars also, like ``'`` as ``&amp;apos;``                |
+------------------------------+---------+--------------------------------------------------------------------------------+
| ``HIDE_THUMBNAIL_RESOURCE``  | 0x200   | Hide thumbnail resource in UPnP data (only adds albumArtUri)                   |
+------------------------------+---------+--------------------------------------------------------------------------------+
| ``HIDE_SUBTITLE_RESOURCE``   | 0x400   | Hide subtitle resource in UPnP data (only add captionInfo)                     |
+------------------------------+---------+--------------------------------------------------------------------------------+
| ``HIDE_TRANSCODE_RESOURCE``  | 0x800   | Hide transcoded resources in UPnP data (only adds content stream resources)    |
+------------------------------+---------+--------------------------------------------------------------------------------+
| ``SIMPLE_DATE``              | 0x1000  | Return ``dc:date`` without timezone as Zulu time.                              |
+------------------------------+---------+--------------------------------------------------------------------------------+
| ``DCM10``                    | 0x2000  | Add DCM10 as device capability.                                                |
+------------------------------+---------+--------------------------------------------------------------------------------+
| ``HIDE_CONTAINER_SHORTCUTS`` | 0x4000  | Disable UPnP feature for shortcuts.                                            |
+------------------------------+---------+--------------------------------------------------------------------------------+
| ``ASCIIXML``                 | 0x8000  | UPnP client supports only ascii characters                                     |
+------------------------------+---------+--------------------------------------------------------------------------------+
| ``NO_CONVERSION``            | 0x10000 | UPnP client does not support conversion                                        |
+------------------------------+---------+--------------------------------------------------------------------------------+
| ``SHOW_INTERNAL_SUBTITLES``  | 0x20000 | UPnP client does not automatically read internal subtitles from stream         |
+------------------------------+---------+--------------------------------------------------------------------------------+


Manual Overrides
~~~~~~~~~~~~~~~~

If your device needs one of the implemented special mechanisms you can define this in config.xml section 
<clients>. This may look like:

.. code-block:: xml

    <clients enabled="yes">
        <client userAgent="Microsoft-DLNA" flags="0"></client>
        <client ip="192.168.1.69" flags="SAMSUNG"></client>
    </clients>


MediaRenderers
--------------

Gerbera supports all UPnP compliant MediaRenderers, however there can always be various problems that
depend on the particular device implementation. We always try to implement workarounds to compensate for
failures and limitations of various renderers.

This is the list of client devices that Gerbera has been tested with and that are known to work.
Please drop us a mail if you are using Gerbera with a device that is not in the list, report any success and failure.
We will try to fix the issues and will add the device to the list.

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
-  RCD-N9
-  HEOS Multiroom, all devices with buildin HEOS by Denon or Marantz (set ``searchable-container-flag="yes"`` in ``upnp``-section to avoid duplicate entries in HEOS app while browsing your library)

D-Link
~~~~~~

-  DSM-320
-  DSM-320RD
-  DSM-510
-  DSM-520

Some additional settings in Gerbera configuration are required to enable special features for the DSM renderers. If you have a DSM-320 and are experiencing problems during AVI playback.
Further, the DSM-320 behaves differently if it thinks that it is dealing with the D-Link server. Add the following to the server section of your configuration to enable srt subtitle support:

.. code-block:: xml

    <manufacturerURL>redsonic.com</manufacturerURL>
    <modelNumber>105</modelNumber>

It is still being investigated, but we were able to get subtitles working with a U.S. DSM-320 unit running firmware version 1.09

Also, the DSM-510 (probably also valid for other models) will only play avi files if the mimetype is set to video/avi, you may want to add a mapping for that to the extension-mimetype section in your config.xml:

.. code-block:: xml

    <map from="avi" to="video/avi"/>

Dual
~~~~

-  CR 510: set flag IRADIO by IP because userAgent is not sent reliably

Freecom
~~~~~~~

-  MusicPal

Häger
~~~~~

-  OnAir (also known as BT Internet Radio)

HP
~~

-  MediaSmart TV

Users reported that after a firmwre upgrade the device stopped working properly. It seems that it does not sue the UPnP Browse action anymore, but now uses the optional Search action which is not implemented in Gerbera.

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

LG
~~

-  All TV based on WebOS 6

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

Microsoft
~~~~~~~~~

-  Windows Explorer (Browse in directory and play with selectable player)
-  Windows MediaPlayer 12 (downloads all item info and builds up cache)

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

.. code-block:: xml

    <map from="ogg" to="audio/ogg"/>

Also, add this to the <mimetype-contenttype> section:

.. code-block:: xml

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

Firmware 1.80 introduces UPnP/DLNA support.

-  Playstation 4

MediaPlayer seems to be flawed, so SSDP advertisements stop playback of videos. Set alive value in config.xml to e.g. 86400.

Syabas
~~~~~~

-  Popcorn Hour A110

T+A
~~~

-  T+A Music Player

Tangent
~~~~~~~

-  Quattro MkII

Technisat
~~~~~~~~~

- DigitRadio 580

Is based on a FSL chipset and needs some quirks because of a parsing issue in ``dc:date``.

Telegent
~~~~~~~~

-  TG100

The TG100 client has a problem browsing containers, where item titles exceed 101 characters. We implemented a server-side workaround which allows you to limit the lengths of all titles and descriptions. Use the following settings in the <server> section of your configuration file:

.. code-block:: xml

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

VLC
~~~

For subtitles to work the client configuration needs to contain

.. code-block:: xml

    <client userAgent="VLC">
       <map from="application/x-srt" to="text/srt"/>
    </client>

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

Some users reported problems where the DMA will show an error ”Failed to retrieve list” and the DMA disconnecting from the server. Increasing the alive interval seems to solve the problem - add the following option to the <server> section of your configuration file:

.. code-block:: xml

    <alive>600</alive>

Additionally, the DMA expects that avi files are serverd with the mime type of video/avi, so add the following to the <extension-mimetype> section in your configuration file:

.. code-block:: xml

    <map from="avi" to="video/avi"/>

Also, add this to the <mimetype-contenttype> section:

.. code-block:: xml

    <treat mimetype="video/avi" as="avi"/>

Network Attached Storage Devices
--------------------------------

We have successfully tested Gerbera on ARM and MIPSel based devices, so it should be possible to install and run the server on various Linux based NAS products
that are available on the market.

So far two devices are shipped with a preinstalled version of Gerbera, community firmware versions are available for the rest.

Asus
~~~~

-  WL500g

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

Use the Optware feeds.

Raidsonic
~~~~~~~~~

-  IB-NAS4200-B

Xtreamer
~~~~~~~~

-  Xtreamer eTRAYz

Western Digital
~~~~~~~~~~~~~~~

-  MyBook
