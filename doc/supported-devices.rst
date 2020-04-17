.. _supported-devices:
.. index:: Supported Devices

Supported Devices
=================

**Attention Hardware Manufacturers:**

If you want to improve compatibility between Gerbera and your renderer device or if you are interested in a port of
Gerbera for your NAS device please contact Gerbera Contributors `https://github.com/gerbera <https://github.com/gerbera>`_

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

Firmware 1.80 introduces UPnP/DLNA support, add the following to the <server> section of your configuration file to enable Gerbera PS3 compatibility:

Syabas
~~~~~~

-  Popcorn Hour A110

T+A
~~~

-  T+A Music Player

Tangent
~~~~~~~

-  Quattro MkII

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

Some users reported problems where the DMA will show an error ”Failed to retrieve list” and the DMA disconnecting from the server. Incresing the alive interval seems to solve the problem - add the following option to the <server> section of your configuration file:

.. code-block:: xml

    <alive>600</alive>

Additionally, the DMA expects that avi files are serverd with the mime type of video/avi, so add the following to the <extensoin-mimetype> section in your configuration file:

.. code-block:: xml

    <map from="avi" to="video/avi"/>

Also, add this to the <mimetype-contenttype> section:

.. code-block:: xml

    <treat mimetype="video/avi" as="avi"/>

Network Attached Storage Devices
--------------------------------

We provide a bitbake metadata file for the OpenEmbedded environment, it allows to easily cross compile Gerbera for various platforms. We have successfully tested Gerbera on ARM and MIPSel based devices, so it should be possible to install and run the server on various Linux based NAS products that are available on the market.

So far two devices are shipped with a preinstalled version of Gerbera, community firmware versions are available for the rest.

Asus
~~~~

-  WL500g

Use the statically linked mips32el binary package that is provided on our download site.

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

Either use the Optware feeds or the statically linked mips2el binary package that is provided on our download site.

Raidsonic
~~~~~~~~~

-  IB-NAS4200-B

Use the statically linked binary armv4 package that is provided on our download site.

Xtreamer
~~~~~~~~

-  Xtreamer eTRAYz

Western Digital
~~~~~~~~~~~~~~~

-  MyBook