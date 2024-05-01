.. index:: Troubleshooting

Troubleshooting
===============

Installation
~~~~~~~~~~~~

If you want to update from a version that was provided by another package source (like ubuntu main -> jfrog) perform the following steps
- back up your configuration and database
- uninstall the old version completely (look for left-over files under /usr/local/share)
- install the new version
- generate config file with ``--create-config``
- merge your old settings into the new file and copy it into the right place
- copy the database file to the right place (depending on version differences it is even recommended to create a new database)
- start gerbera and check the log for migration errors in the database (if you migrate)

Network
~~~~~~~

If you get startup errors like ``INVALID_INTERFACE`` try to add the interface and the port to the command line or config file.
Make sure your network device is configured correctly (multicasting).

IP Address
----------

The UPnP library tries to identify the correct network connection automatically. This may fail if there are multiple interfaces
(including local loopback) or the configuration of the interface is not as required (see Network Setup). In that case either specify
the correct IP address or the network interface.

::

    --ip or -i

The server will bind to the given IP address, currently we can not bind to multiple interfaces so binding to ``0.0.0.0``
is not be possible.

Network Interface
-----------------

::

    --interface or -e

Interface to bind to, for example eth0, this can be specified instead of the IP address.

.. _troubleshoot_port:

Port
----

In addition to UDP port ``1900`` (required by UPnP/SSDP), Gerbera uses a port in the range of ``49152``
or above for serving media and for UPnP requests. 
If multiple instances of Gerbera or other UPnP media servers are running at the same time the port may be blocked.

::

    --port or -p

Specify the server port that will be used for the web user interface, for serving media and for UPnP requests.
The minimum allowed value is ``49152``. This can also be specified in the config file.
If this option is omitted a default port will be chosen, but in
this case it is possible that the port will change upon server restart. 

Gerbera's startup messages will tell you which server port it's actually using.

Firewall
--------

If gerbera appears to be running but other devices on the network can't see it, ensure that your 
firewall is not blocking UDP port ``1900`` or the server port that Gerbera is using
(e.g. ``49152``, see the :ref:`Port <troubleshoot_port>` section above). 

If you have opened only a single server port in your firewall for Gerbera to use, consider adding that port 
to the command line or config file to prevent it from changing upon server restart.
If multiple instances of Gerbera or other UPnP media servers are running at the same time you may need 
to open multiple ports.

Debugging
~~~~~~~~~

If you experience unexpected behaviour, you can run gerbera with ``--debug`` for
a full log output or ``--debug-mode`` for specific parts of gerbera.

Log To File
-----------

Normally gerbera writes all messages to the system log (via standard output). Writing to your own log file
helps to search the logs for particular messages. Make sure you backup the file before restarting gerbera.

::

    --logfile or -l

Do not output log messages to stdout, but redirect everything to a specified file.

Debug Output
------------

Full debug output produces a large amount of text in order to analyse problems. Enabling all messages is the
fastest way to start debugging but it may be hard to find the right message in the output.

::

    --debug or -D

Enable full debug log output.

Debug Mode
----------

::

    --debug-mode "subsystem[|subsystem2|...]"

Activate debugging messages only for certain subsystems. The following subsystems are available:
``thread``, ``sqlite3``, ``cds``, ``server``, ``content``, ``update``, ``mysql``, ``sqldatabase``, ``proc``, ``autoscan``, ``script``, ``web``, ``layout``,
``exif``, ``transcoding``, ``taglib``, ``ffmpeg``, ``wavpack``, ``requests``, ``connmgr``, ``mrregistrar``, ``xml``, ``clients``, ``iohandler``, ``online``,
``metadata``, ``matroska``.

Multiple subsystems can be combined with a ``|``. Names are not case sensitive. This is for developers and testers mostly and has to be activted in cmake 
options at compile time (``-DWITH_DEBUG_OPTIONS=YES``).

Check Config
------------

If you have startup problems or unexpected behaviour you can check the configuration to verify that all your settings have been accepted.

::

    --check-config

Check the current configuration and exit. Useful to check new settings before running gerbera as a service.
Best use with --debug in case of problems.

Compile Info
------------

Gerbera has some compile options regarding support for media file formats. If your media files are not scanned at all or metadata is not detected completely
the compile info may help to look at the right place.

::

    --compile-info

Print the configuration summary (used libraries and enabled features) and exit.

Version Information
-------------------

::

    --version

Print version information and exit.


Content
~~~~~~~

Duplicate Folders
-----------------

If you have albums with identical name it is likely that you get two or more folders under `Albums` with the same name or the one folder with the content mixed up.
The default configuration expects ``AlbumArtist`` and ``Date`` to be available and unique.
A solution to this is that you define the properties that identify one particular album by setting appropriate keys in :ref:`Virtual Directories <virtual-directories>`


Configuration
-------------

If you have broken Gerbera by modifying values on the config page, have to clear the database or at least remove all entries from the table ``grb_config_value``.

