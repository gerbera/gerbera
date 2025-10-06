.. index:: Configure Server

################
Configure Server
################

These settings define the server configuration, this includes UPnP behavior, selection of database, accounts for the UI as well as installation locations of shared data.

.. contents::
   :backlinks: entry
.. sectnum::
   :start: 1

.. _server:

******
Server
******

.. confval:: server
   :type: :confval:`Section`
   :required: true

   .. code-block:: xml

       <server> ... </server>

This section defines the server configuration parameters.

Server Attributes
=================

      .. confval:: debug-mode
         :type: enum
         :required: false
         :default: unset
      .. versionadded:: 2.0.0
      .. versionchanged:: 2.6.1 new option ``inotify``
      .. versionchanged:: 2.6.2 new option ``thumbnailer``

      Activate debugging messages only for certain subsystems.
      The following subsystems are available:
      ``thread``, ``sqlite3``, ``cds``, ``server``, ``config``,
      ``content``, ``update``, ``mysql``,
      ``sql``, ``proc``, ``autoscan``, ``script``, ``web``, ``layout``,
      ``exif``, ``exiv2``, ``transcoding``, ``taglib``, ``ffmpeg``, ``wavpack``,
      ``requests``, ``device``, ``connmgr``, ``mrregistrar``, ``xml``,
      ``clients``, ``iohandler``, ``online``, ``metadata``, ``matroska``,
      ``curl``, ``util``, ``inotify``, ``thumbnailer`` and ``verbose``.
      Multiple subsystems can be combined with a ``|``. Names are not case
      sensitive. ``verbose`` turns on even more messages for the subsystem.
      This is for developers and testers mostly and has to be
      activted in cmake options at compile time (``-DWITH_DEBUG_OPTIONS=YES``).

      * Example: ``debug-mode="Cds|Content|Web"`` for messages when accessing the server via upnp or web.


      .. confval:: upnp-max-jobs
         :type: :confval:`Integer`
         :required: false
         :default: ``500``
      .. versionadded:: 2.4.0

      Set maximum number of jobs in libpupnp internal threadpool.
      Allows pending requests to be handled.

Server Items
============

Port
----

.. confval:: port
   :type: :confval:`Integer`
   :required: false
   :default: ``0`` `(automatic)`

   .. code-block:: xml

       <port>0</port>

Specifies the port where the server will be listening for HTTP requests. Note, that because of the implementation in the UPnP SDK
only ports above 49152 are supported. The value of zero means, that a port will be automatically selected by the SDK.

IP Address
----------

.. confval:: ip
   :type: :confval:`String`
   :required: false
   :default: ip of the first available network interface

   .. code-block:: xml

       <ip>192.168.0.23</ip>

Specifies the IP address to bind to, by default one of the available interfaces will be selected.

Network interface
-----------------

.. confval:: interface
   :type: :confval:`String`
   :required: false
   :default: first available network interface

   .. code-block:: xml

       <interface>eth0</interface>

Specifies the interface to bind to, by default one of the available interfaces will be selected.

Name
----

.. confval:: server name
   :type: :confval:`String`
   :required: true
   :default: ``Gerbera``

   .. code-block:: xml

       <name>Gerbera</name>

Server's friendly name, you will see this on your devices that you use to access the server.

Manufacturer
------------

.. confval:: manufacturer
   :type: :confval:`String`
   :required: false
   :default: empty

   .. code-block:: xml

       <manufacturer>Gerbera Developers</manufacturer>

This tag sets the manufacturer name of a UPnP device.

Manufacturer Url
----------------

.. confval:: manufacturerURL
   :type: :confval:`String`
   :required: false
   :default: ``https://gerbera.io/``

   .. code-block:: xml

       <manufacturerURL>https://gerbera.io/</manufacturerURL>

This tag sets the manufacturer URL of a UPnP device, a custom setting may be necessary to trick some renderers in order
to enable special features that otherwise are only active with the vendor implemented server.

Virtual Url
-----------

.. confval:: virtualURL
   :type: :confval:`String`
   :required: false
   :default: unset

   .. code-block:: xml

       <virtualURL>https://gerbera.io/</virtualURL>

This tag sets the virtual URL of Gerbera content which is part of the browse response.
The value defaults to `http://<ip>:<port>`.

External Url
------------

.. confval:: externalURL
   :type: :confval:`String`
   :required: false
   :default: unset

   .. versionadded:: 2.0.0
   .. code-block:: xml

       <externalURL>https://gerbera.io/</externalURL>

This tag sets the external URL of Gerbera web UI, a custom setting may be necessary if you want to access the web page via a reverse proxy.
The value defaults to virtualURL or `http://<ip>:<port>` if virtualURL is not set.

Model Name
----------

.. confval:: modelName
   :type: :confval:`String`
   :required: false
   :default: ``Gerbera``

   .. code-block:: xml

       <modelName>Gerbera</modelName>

This tag sets the model name of a UPnP device, a custom setting may be necessary to trick some renderers in order to
enable special features that otherwise are only active with the vendor implemented server.

Model Number
------------

.. confval:: modelNumber
   :type: :confval:`String`
   :required: false
   :default: Gerbera version

   .. code-block:: xml

       <modelNumber>42.7.0</modelNumber>

This tag sets the model number of a UPnP device, a custom setting may be necessary to trick some renderers in order
to enable special features that otherwise are only active with the vendor implemented server.

Model Url
---------

.. confval:: modelURL
   :type: :confval:`String`
   :required: false
   :default: empty

   .. code-block:: xml

       <modelURL>http://example.org/product-23</modelURL>

This tag sets the model URL (homepage) of a UPnP device.

Serial Number
-------------

.. confval:: serialNumber
   :type: :confval:`String`
   :required: false
   :default: ``1``

   .. code-block:: xml

       <serialNumber>42</serialNumber>

This tag sets the serial number of a UPnP device.

Presentation Url
----------------

.. confval:: presentationURL
   :type: :confval:`String`
   :required: false
   :default: ``/``

   .. code-block:: xml

       <presentationURL append-to="ip">80/index.html</presentationURL>

The presentation URL defines the location of the servers user interface, usually you do not need to change this
however, vendors who want to ship our server along with their NAS devices may want to point to the main configuration
page of the device.

Attributes

        .. confval:: append-to
           :type: enum
           :required: false
           :default: ``none``

           .. code-block:: xml

               append-to="ip"

      The append-to attribute defines how the text in the presentationURL tag should be treated.
      The allowed values are:

      +-------+--------------------------------------------------------------------------------------------+
      | Value | Meaning                                                                                    |
      +=======+============================================================================================+
      | none  | Use the string exactly as it appears in the presentationURL tag.                           |
      +-------+--------------------------------------------------------------------------------------------+
      | ip    | | Append the string specified in the presentationURL tag to the ip address of the server,  |
      |       | | this is useful in a dynamic ip environment where you do not know the ip                  |
      |       | | but want to point the URL to the port of your web server.                                |
      +-------+--------------------------------------------------------------------------------------------+
      | port  | | Append the string specified in the presentationURL tag to the serverip and port,         |
      |       | | this may be useful if you want to serve some static pages using the built in web server. |
      +-------+--------------------------------------------------------------------------------------------+

UDN
---

.. confval:: udn
   :type: :confval:`String`
   :required: true
   :default: none

   .. code-block:: xml

       <udn>uuid:[generated-uuid]</udn>

Unique Device Name, according to the UPnP spec it must be consistent throughout reboots. You can fill in something
yourself.  Review the :ref:`Generating Configuration <generateConfig>` section of the documentation to see how to use
``gerbera`` to create a default configuration file.

Home Directory
--------------

.. confval:: home
   :type: :confval:`Path`
   :required: true
   :default: ``~`` `- the HOME directory of the user running gerbera.`

   .. code-block:: xml

      <home override="yes">/home/your_user_name/gerbera</home>

Server home - the server will search for the data that it needs relative to this directory -
basically for the sqlite database file.
The gerbera.html bookmark file will also be generated in that directory.
The home directory is only relevant if the config file or the config dir was specified
in the command line. Otherwise it defaults to the ``HOME`` path of the user runnung
Gerbera. The environment variable ``GERBERA_HOME`` can be used to point to another directory,
in which case the config file is expected as ``${GERBERA_HOME}/.config/gerbera``.

    Attributes:

      .. confval:: override
         :type: :confval:`Boolean`
         :required: false
         :default: ``no``

         .. code-block:: xml

             override="yes"

      Force all relative paths to base on the home directory of the config file even
      if it was read relative to the environment variables or from command line. This
      means that Gerbara changes its home during startup.

Temporary Directory
-------------------

.. confval:: tmpdir
   :type: :confval:`Path`
   :required: true
   :default: ``/tmp/``

   .. code-block:: xml

       <tmpdir>/tmp/</tmpdir>

Selects the temporary directory that will be used by the server.

Web Directory
-------------

.. confval:: webroot
   :type: :confval:`Path`
   :required: true
   :default: `depends on the installation prefix that is passed to the configure script.`

   .. code-block:: xml

       <webroot>/usr/share/gerbera/web</webroot>

Root directory for the web server, this is the location where device description documents,
UI html and js files, icons, etc. are stored.

Alive Interval
--------------

.. confval:: alive
   :type: :confval:`Integer`
   :required: false
   :default: ``180``, (Results in alive messages every 60s, see below) `this is according to the UPnP specification.`

   .. code-block:: xml

       <alive>180</alive>

* Min: 62 (A message sent every 1s, see below)

Interval for broadcasting SSDP:alive messages

An advertisement will be sent by LibUPnP every (this value/2)-30 seconds, and will have a cache-control max-age of this value.

Example:
   A value of 62 will result in an SSDP advertisement being sent every second. ``(62 / 2 = 31) - 30 = 1``.
   The default value of 180 results results in alive messages every 60s. ``(180 / 2 = 90) - 30 = 60``.

Note:
   If you experience disconnection problems from your device, e.g. Playstation 4, when streaming videos after about 5 minutes,
   you can try changing the alive value to 86400 (which is 24 hours).

PC Directory
------------

.. confval:: pc-directory
   :type: :confval:`Section`
   :required: false

   .. code-block:: xml

       <pc-directory upnp-hide="yes" web-hide="yes"/>

Tweak visibility of PC directory, i.e. root entry for physical structure.

Attributes
^^^^^^^^^^

    .. confval:: upnp-hide
       :type: :confval:`Boolean`
       :required: false
       :default: ``no``

       .. code-block:: xml

           upnp-hide="yes"

    Enabling this option will make the PC-Directory container invisible for UPnP devices.

    .. confval:: web-hide
       :type: :confval:`Boolean`
       :required: false
       :default: ``no``
    ..

       .. versionadded:: 2.6.0
       .. code-block:: xml

            web-hide="yes"

    Enabling this option will make the PC-Directory container invisible in the web UI.

Bookmark File
-------------

.. confval:: bookmark
   :type: :confval:`String`
   :required: false
   :default: ``gerbera.html``

   .. code-block:: xml

       <bookmark>gerbera.html</bookmark>

The bookmark file offers an easy way to access the user interface, it is especially helpful when the server is
not configured to run on a fixed port. Each time the server is started, the bookmark file will be filled in with a
redirect to the servers current IP address and port. To use it, simply bookmark this file in your browser,
the default location is ``~/.config/gerbera/gerbera.html``

UPnP String Limit
-----------------

.. confval:: upnp-string-limit
   :type: :confval:`Integer`
   :required: false
   :default: ``-1`` (`disabled`)

   .. code-block:: xml

       <upnp-string-limit>100</upnp-string-limit>

This will limit title and description length of containers and items in UPnP browse replies, this feature was added
as a workaround for the TG100 bug which can only handle titles no longer than 100 characters.
A negative value will disable this feature, the minimum allowed value is "4" because three dots will be appended
to the string if it has been cut off to indicate that limiting took place.

.. _logging:

Logging
-------

.. confval:: logging
   :type: :confval:`Section`
   :required: false

   .. versionadded:: 2.2.0

   .. code-block:: xml

       <logging rotate-file-size="1000000" rotate-file-count="3"/>

This section defines various logging settings.


Attributes
^^^^^^^^^^

    .. confval:: rotate-file-size
       :type: :confval:`Integer`
       :required: false
       :default: ``5242880`` (5 MB)

       .. code-block:: xml

           rotate-file-size="1024000"

    When using command line option ``--rotatelog`` this value defines the maximum size of the log file before rotating.

    .. confval:: rotate-file-count
       :type: :confval:`Integer`
       :required: false
       :default: ``10``

       .. code-block:: xml

           rotate-file-count="5"

    When using command line option ``--rotatelog`` this value defines the number of files in the log rotation.


.. _ui:

*************
Web Interface
*************

.. confval:: ui
   :type: :confval:`Section`
   :required: false

   .. code-block:: xml

       <ui enabled="yes" poll-interval="2" poll-when-idle="no"/>

This section defines various user interface settings.

**WARNING!**
    The server has an integrated filesystem browser, that means that anyone who has access to the UI can browse
    your filesystem (with user permissions under which the server is running) and also download your data!
    If you want maximum security - disable the UI completely! Account authentication offers simple protection that
    might hold back your kids, but it is not secure enough for use in an untrusted environment!

Note:
   since the server is meant to be used in a home LAN environment the UI is enabled by default and accounts are
   deactivated, thus allowing anyone on your network to connect to the user interface.

Web Interface Attributes
========================

    .. confval:: ui enabled
       :type: :confval:`Boolean`
       :required: false
       :default: ``yes``

       .. code-block:: xml

           enabled="no"

    Enables (``yes``) or disables (``no``) the web user interface.

    .. confval:: show-tooltips
       :type: :confval:`Boolean`
       :required: false
       :default: ``yes``

       .. code-block:: xml

           show-tooltips="no"

    This setting specifies if icon tooltips should be shown in the web UI.

    .. confval:: show-numbering
       :type: :confval:`Boolean`
       :required: false
       :default: ``yes``

       .. code-block:: xml

           show-numbering="no"

    Set track number to be shown in the web UI.

    .. confval:: show-thumbnail
       :type: :confval:`Boolean`
       :required: false
       :default: ``yes``

       .. code-block:: xml

           show-thumbnail="no"

    This setting specifies if thumbnails or cover art should be shown in the web UI.

    .. confval:: poll-interval
       :type: :confval:`Integer`
       :required: false
       :default: ``2``

       .. code-block:: xml

           poll-interval="10"

    The poll-interval is an integer value which specifies how often the UI will poll for tasks. The interval is
    specified in seconds, only values greater than zero are allowed. The value can be given in a valid time format.

    .. confval:: fs-add-item
       :type: :confval:`Boolean`
       :required: false
       :default: ``no``

       .. versionadded:: 2.5.0
       .. code-block:: xml

           fs-add-item="yes"

    Show the (deprecated) option to add items without autoscan functionality.

    .. confval:: edit-sortkey
       :type: :confval:`Boolean`
       :required: false
       :default: ``no``

       .. versionadded:: 2.6.0
       .. code-block:: xml

           edit-sortkey="yes"

    Show the edit field ``sortKey`` for objects.

    .. confval:: poll-when-idle
       :type: :confval:`Boolean`
       :required: false
       :default: ``no``

       .. code-block:: xml

           poll-when-idle="yes"

    The poll-when-idle attribute influences the behavior of displaying current tasks: - when the user does something
    in the UI (i.e. clicks around) we always poll for the current task and will display it - if a task is active,
    we will continue polling in the background and update the current task view accordingly - when there is no
    active task (i.e. the server is currently idle) we will stop the background polling and only request updates
    upon user actions, but not when the user is idle (i.e. does not click around in the UI)

    Setting poll-when-idle to "yes" will do background polling even when there are no current tasks; this may be
    useful if you defined multiple users and want to see the tasks the other user is queuing on the server while
    you are actually idle.

    The tasks that are monitored are:

    -  adding files or directories
    -  removing items or containers
    -  automatic rescans

Source Documentation Url
========================

.. confval:: source-docs-link
   :type: :confval:`String`
   :required: false
   :default: empty

   .. versionadded:: 2.4.0
   .. code-block:: xml

      <source-docs-link>./dev/index.html</source-docs-link>

Add link to some source documentation which can be generated by ``make doc``. If it is empty the link in the web UI will be hidden.

User Documentation Url
======================

.. confval:: user-docs-link
   :type: :confval:`String`
   :required: false
   :default: for release builts: "https://docs.gerbera.io/en/stable/", for test builts: "https://docs.gerbera.io/en/latest/"

   .. versionadded:: 2.4.0
   .. code-block:: xml

      <user-docs-link>./doc/index.html</user-docs-link>

Add link to the user documentation if you want it locally hosted or make sure the version is matching you installation.

Content Security Policy
=======================

.. confval:: content-security-policy
   :type: :confval:`String`
   :required: false
   :default: ``default-src %HOSTS% 'unsafe-eval' 'unsafe-inline'; img-src *; media-src *; child-src 'none';``

   .. versionadded:: 2.4.0
   .. code-block:: xml

      <content-security-policy>default-src %HOSTS% 'unsafe-eval' 'unsafe-inline'; img-src *; media-src *; child-src 'none';</content-security-policy>

Define the "Content-Security-Policy" string for the web ui. The string ``%HOHSTS%`` will be replaced by the IP 
address and known server names.
Newlines will automatically be replaced by ``;``.

Example:
    Content security policy to host source documentation

    .. code-block:: xml

       <content-security-policy>
           font-src %HOSTS% https://fonts.gstatic.com/
           style-src %HOSTS% https://fonts.googleapis.com 'unsafe-inline'
           img-src *
           media-src *
           child-src 'none'
           default-src %HOSTS% 'unsafe-eval' 'unsafe-inline'
       </content-security-policy>

Extension Mimetype Mapping
==========================

.. confval:: ui extension-mimetype
   :type: :confval:`Section`
   :required: false
   :default: Extensible default see above, see :confval:`extend`

   .. code-block:: xml

      <extension-mimetype default="application/octet-stream">
          <map from="html" to="text/html"/>
          <map from="js" to="application/javascript"/>
          <map from="json" to="application/json"/>
          <map from="css" to="text/css"/>
      </extension-mimetype>

For description see :ref:`Import Extension Mimetype Mapping <extension-mimetype>`.

Attributes

    .. confval:: extension-mimetype default
       :type: :confval:`String`
       :required: false
       :default: ``application/octet-stream``

       .. code-block:: xml

           default="application/octet-stream"

Accounts
========

.. confval:: accounts
   :type: :confval:`Section`
   :required: false

   .. code-block:: xml

      <accounts enabled="yes" session-timeout="30"/>

This section holds various account settings.

Attributes

    .. confval:: accounts enabled
       :type: :confval:`Boolean`
       :required: false
       :default: ``yes``

       .. code-block:: xml

           enabled="no"

    Specifies if accounts are enabled ``yes`` or disabled ``no``.

    .. confval:: session-timeout
       :type: :confval:`Integer`
       :required: false
       :default: ``30``

       .. code-block:: xml

           session-timeout="120"

    The session-timeout attribute specifies the timeout interval in minutes. The server checks every
    five minutes for sessions that have timed out, therefore in the worst case the session times out
    after session-timeout + 5 minutes. The value can be given in a valid time format.

Example:
    Accounts can be defined as shown below:

    .. code-block:: xml

        <account user="name" password="password"/>
        <account user="name" password="password"/>

    There can be multiple users, however this is mainly a feature for the future. Right now there are
    no per-user permissions.

Items Per Page
==============

.. confval:: items-per-page
   :type: :confval:`Section`
   :required: false

   .. code-block:: xml

       <items-per-page default="25">

Attributes

    .. confval:: items-per-page default
       :type: :confval:`Integer`
       :required: false
       :default: ``25``

       .. code-block:: xml

          default="50"

    This sets the default number of items per page that will be shown when browsing the database in the web UI.
    The values for the items per page drop down menu can be defined in the following manner:

Items

    .. confval:: items-per-page option
       :type: :confval:`Integer`
       :required: false
       :default: Extensible Default: ``10, 25, 50, 100``, see :confval:`extend`

       .. code-block:: xml

           <option>10</option>
           <option>25</option>
           <option>50</option>
           <option>100</option>

    Note:
        this list must contain the default value, i.e. if you define a default value of 25, then one of the
        ``<option>`` tags must also list this value.


.. _storage:

*******
Storage
*******

.. confval:: storage
   :type: :confval:`Section`
   :required: true

   .. code-block:: xml

       <storage use-transactions="yes">

Defines the storage section - database selection is done here. Currently SQLite3 and MySQL are supported.
Each storage driver has it's own configuration parameters.

Exactly one driver must be enabled: ``sqlite3`` or ``mysql``. The available options depend on the selected driver.

Storage Attributes
==================

.. confval:: use-transactions
   :type: :confval:`Boolean`
   :required: false
   :default: ``no``

   .. code-block:: xml

       use-transactions="yes"

Enables transactions. This feature should improve the overall import speed and avoid race-conditions on import.
The feature caused some issues and set to ``no``. If you want to support testing, turn it to ``yes`` and report
if you can reproduce the issue.

.. confval:: enable-sort-key
   :type: :confval:`Boolean`
   :required: false
   :default: ``yes``

   .. versionadded:: 2.6.0
   .. code-block:: xml

       enable-sort-key="no"

Switches default sorting by property of ``dc_title`` to ``sort_key``. The sort key is derived from the filename by
expanding all numbers to fixed digits.

.. confval:: string-limit
   :type: :confval:`Boolean`
   :required: false
   :default: ``255``

   .. versionadded:: 2.6.0
   .. code-block:: xml

       string-limit="250"

Set the maximum length of indexed string columns like ``dc_title``. Changing this value after
initializing the database will produce a warning in gerbera log and may cause
database errors because the string is not correctly truncated.


SQLite
======

.. confval:: sqlite3
   :type: :confval:`Section`
   :required: false

   .. code-block:: xml

       <sqlite3 enabled="yes">

Defines the SQLite storage driver section.

SQLite Attributes
-----------------

.. confval:: sqlite3 enabled
   :type: :confval:`Boolean`
   :required: false
   :default: ``yes``

   .. code-block:: xml

       enabled="no"

Enables SQLite database storage. If SQLite is enabled MySQL must be disabled.

.. confval:: shutdown-attempts
   :type: :confval:`Integer`
   :required: false
   :default: ``5``

   .. versionadded:: 2.5.0
   .. code-block:: xml

       shutdown-attempts="10"

Number of attempts to shutdown the sqlite driver before forcing the application down.

Init SQL File
-------------

Below are the sqlite driver options:

.. confval:: sqlite3 init-sql-file
   :type: :confval:`Path`
   :required: false
   :default: ``${datadir}/sqlite3.sql``

   .. code-block:: xml

       <init-sql-file>/etc/gerbera/sqlite3.sql</init-sql-file>

The full path to the init script for the database.

Upgrade Statement File
----------------------

.. confval:: sqlite3 upgrade-file
   :type: :confval:`Path`
   :required: false
   :default: ``${datadir}/sqlite3-upgrade.xml``

   .. code-block:: xml

       <upgrade-file>/etc/gerbera/sqlite3-upgrade.xml</upgrade-file>

Database File
-------------

The full path to the upgrade settings for the database

.. confval:: database-file
   :type: :confval:`String`
   :required: false
   :default: ``gerbera.db``

   .. code-block:: xml

       <database-file>gerbera.db</database-file>

The database location is relative to the server's home, if the sqlite database does not exist it will be
created automatically.

Sync Setting
------------

.. confval:: synchronous
   :type: :confval:`Enum`
   :required: false
   :default: ``off``

   .. code-block:: xml

       <synchronous>off</synchronous>

Possible values are ``off``, ``normal``, ``full`` and ``extra``.

This option sets the SQLite pragma ``synchronous``. This setting will affect the performance of the database
write operations. For more information about this option see the SQLite documentation: https://www.sqlite.org/pragma.html#pragma_synchronous

Journal Mode
------------

.. confval:: journal-mode
   :type: :confval:`Enum`
   :required: false
   :default: ``WAL``

   .. code-block:: xml

       <journal-mode>off</journal-mode>

Possible values are ``OFF``, ``DELETE``, ``TRUNCATE``, ``PERSIST``, ``MEMORY`` and ``WAL``

This option sets the SQLite pragma ``journal_mode``. This setting will affect the performance of the database
write operations. For more information about this option see the SQLite documentation: https://www.sqlite.org/pragma.html#pragma_journal_mode

Error Behaviour
---------------

.. confval:: on-error
   :type: :confval:`Enum` (``restore|fail``)
   :required: false
   :default: ``restore``

   .. code-block:: xml

       <on-error>restore</on-error>

This option tells Gerbera what to do if an SQLite error occurs (no database or a corrupt database).
If it is set to ``restore`` it will try to restore the database from a backup file (if one exists) or try to
recreate a new database from scratch.

If the option is set to ``fail``, Gerbera will abort on an SQLite error.

SQLite Auto-Backup
------------------

.. confval:: backup
   :type: :confval:`Section`
   :required: false

   .. code-block:: xml

       <backup enabled="no" interval="15:00"/>

Create a database backup file for easy recovery if the main file cannot be read. The backup file can also be used to analyse the database
contents while the main database is in use. This does not avoid loss of data like a regular backup.

Attributes:

     .. confval:: backup enabled
        :type: :confval:`Boolean`
        :required: false
        :default: ``yes``

        .. code-block:: xml

            enabled="no"

     Enables or disables database backup.

     .. confval:: backup interval
        :type: :confval:`Integer`
        :required: false
        :default: ``600``
     ..

        .. code-block:: xml

            interval="300"

     Defines the backup interval in seconds. The value can be given in a valid time format.


MySQL
=====

.. confval:: mysql
   :type: :confval:`Section`
   :required: false

   .. code-block:: xml

       <mysql enabled="no"/>

Defines the MySQL storage driver section.

MySQL Attributes
----------------

.. confval:: mysql enabled
   :type: :confval:`Boolean`
   :required: false
   :default: ``no``

   .. code-block:: xml

       enabled="yes"

Enables MySQL database storage. If MySQL is enabled SQLite must be disabled.

Server Host
-----------

.. confval:: mysql host
   :type: :confval:`String`
   :required: false
   :default: ``localhost``

   .. code-block:: xml

      <host>localhost</host>

This specifies the host where your MySQL database is running.

Server Port
-----------

.. confval:: mysql port
   :type: :confval:`Integer`
   :required: false
   :default: ``0``

   .. code-block:: xml

      <port>0</port>

This specifies the port where your MySQL database is running.

Server User
-----------

.. confval:: mysql username
   :type: :confval:`String`
   :required: false
   :default: ``gerbera``

   .. code-block:: xml

      <username>root</username>

This option sets the user name that will be used to connect to the database.

Server Password
---------------

.. confval:: mysql password
   :type: :confval:`String`
   :required: false
   :default: `no password`

   .. code-block:: xml

      <password>5eryS€cre!</password>

Defines the password for the MySQL user. If the tag doesn't exist Gerbera will use no password, if
the tag exists, but is empty Gerbera will use an empty password. MySQL has a distinction between
no password and an empty password.

Database Name
-------------

.. confval:: database
   :type: :confval:`String`
   :required: false
   :default: ``gerbera``

   .. code-block:: xml

      <database>gerbera</database>

Name of the database that will be used by Gerbera.

Init SQL File
-------------

.. confval:: mysql init-sql-file
   :type: :confval:`String`
   :required: false
   :default: ``${datadir}/mysql.sql``

   .. code-block:: xml

      <init-sql-file>/etc/gerbera/mysql.sql</init-sql-file>

The full path to the init script for the database.

Upgrade Statement File
----------------------

.. confval:: mysql upgrade-file
   :type: :confval:`String`
   :required: false
   :default: ``${datadir/mysql-upgrade.xml``

   .. code-block:: xml

       <upgrade-file>/etc/gerbera/mysql-upgrade.xml</upgrade-file>

The full path to the upgrade settings for the database

Database Engine
---------------

.. confval:: engine
   :type: :confval:`String`
   :required: false
   :default: ``MyISAM``

   .. versionadded:: 2.6.0
   .. code-block:: xml

       <engine>Aria</engine>

Select the storage engine for the tables. Only effective if database has to be created on first start.
The storage engines for MariaDB can be found here https://mariadb.com/kb/en/choosing-the-right-storage-engine/ but may depend on your actual version.

Charset
-------

.. confval:: mysql charset
   :type: :confval:`String`
   :required: false
   :default: ``utf8``

   .. versionadded:: 2.6.0
   .. code-block:: xml

       <charset>utf8mb4</charset>

Select the character set for the tables. Only effective if database has to be created on first start.
The character sets for MariaDB can be found here https://mariadb.com/kb/en/supported-character-sets-and-collations/ but may depend on your actual version.

Collation
---------

.. confval:: mysql collation
   :type: :confval:`String`
   :required: false
   :default: ``utf8_general_ci``

   .. versionadded:: 2.6.0
   .. code-block:: xml

       <collation>utf8mb4_unicode_ci</collation>

Select the collation for the string columns. Only effective if database has to be created on first start.
The collations for MariaDB can be found here https://mariadb.com/kb/en/supported-character-sets-and-collations/#collations but may depend on your actual version.

.. _upnp:

*************
UPnP Protocol
*************

.. confval:: upnp
   :type: :confval:`Section`
   :required: false

   .. code-block:: xml

      <upnp multi-value="yes" search-result-separator=" : ">

Modify the settings for UPnP items.

This section defines the properties which are sent to UPnP clients as part of the response.

UPnP Attributes
===============

.. confval:: searchable-container-flag
   :type: :confval:`Boolean`
   :required: false
   :default: ``no``

   .. code-block:: xml

       searchable-container-flag="yes"

Only return containers that have the flag ``searchable`` set.

.. confval:: dynamic-descriptions
   :type: :confval:`Boolean`
   :required: false
   :default: ``yes``

   .. versionadded:: 2.2.0
   .. code-block:: xml

       dynamic-descriptions="no"

Return UPnP description requests based on the client type. This hides,
e.g., Samsung specific extensions in ``description.xml`` and ``cds.xml``
from clients that don't handle the respective requests.

.. confval:: literal-host-redirection
   :type: :confval:`Boolean`
   :required: false
   :default: ``no``

   .. versionadded:: 2.0.0
   .. code-block:: xml

       literal-host-redirection="yes"

Enable literal IP redirection.

.. confval:: search-result-separator
   :type: :confval:`String`
   :required: false
   :default: ``" - "``

   .. code-block:: xml

       search-result-separator=" : "

String used to concatenate result segments as defined in ``search-item-result``

.. confval:: multi-value
   :type: :confval:`Boolean`
   :required: false
   :default: ``yes``

   .. code-block:: xml

       multi-value="no"

Enables sending multi-valued metadata in separate items. If set to ``no`` all values are concatenated by :confval:`multi-value-separator`. Otherwise each item is added separately.

    Example:
        The follow data is sent if set to ``no``

        .. code-block:: xml

            <upnp:artist>First Artist / Second Artist</upnp:artist>

        The follow data is sent if set to ``yes``

        .. code-block:: xml

            <upnp:artist>First Artist</upnp:artist>
            <upnp:artist>Second Artist</upnp:artist>

.. confval:: search-filename
   :type: :confval:`Boolean`
   :required: false
   :default: ``no``

   .. code-block:: xml

       search-filename="yes"

Older versions of gerbera have been searching in the file name instead of the title metadata. If set to yes this behaviour is back, even if the result of the search shows another title.

.. confval:: caption-info-count
   :type: :confval:`Integer`
   :required: false
   :default: ``-1``

   .. code-block:: xml

       caption-info-count="0"

Number of ``sec::CaptionInfoEx`` entries to write to UPnP result. Default can be overwritten by clients setting. ``-1`` means unlimited.

Search Item Result
==================

   .. code-block:: xml

       <search-item-result>
           <add-data tag="M_ARTIST"/>
           <add-data tag="M_TITLE"/>
       </search-item-result>

.. confval:: search-item-result
   :type: :confval:`Section`
   :required: false
   :default: Extensible Default, see :confval:`extend`

Set the meta-data search tags to use in search result for title. The default settings as shown above produces ``artist - title`` in the result list.

.. confval:: search-item-result add-data
   :type: :confval:`Section`
   :required: false

Add tag to result string.

.. confval:: search-item-result add-data tag
   :type: :confval:`String`
   :required: true

The list of valid tags can be found under :ref:`tags <upnp-tags>`

Response Properties
===================

.. code-block:: xml

    <album-properties>...</album-properties>
    <artist-properties>...</artist-properties>
    <genre-properties>...</genre-properties>
    <playlist-properties>...</playlist-properties>
    <title-properties>...</title-properties>

Defines the properties to send in the response.

.. confval:: album-properties
   :type: :confval:`Section`
   :required: false
   :default: Extensible Default, see :confval:`extend`
.. versionadded:: 2.4.0

+----------------------+-------------------+
| upnp-tag             | meta-data         |
+======================+===================+
| ``dc:creator``       | ``M_ALBUMARTIST`` |
+----------------------+-------------------+
| ``dc:date``          | ``M_UPNP_DATE``   |
+----------------------+-------------------+
| ``dc:publisher``     | ``M_PUBLISHER``   |
+----------------------+-------------------+
| ``upnp:artist``      | ``M_ALBUMARTIST`` |
+----------------------+-------------------+
| ``upnp:albumArtist`` | ``M_ALBUMARTIST`` |
+----------------------+-------------------+
| ``upnp:composer``    | ``M_COMPOSER``    |
+----------------------+-------------------+
| ``upnp:conductor``   | ``M_CONDUCTOR``   |
+----------------------+-------------------+
| ``upnp:date``        | ``M_UPNP_DATE``   |
+----------------------+-------------------+
| ``upnp:genre``       | ``M_GENRE``       |
+----------------------+-------------------+
| ``upnp:orchestra``   | ``M_ORCHESTRA``   |
+----------------------+-------------------+
| ``upnp:producer``    | ``M_PRODUCER``    |
+----------------------+-------------------+

.. confval:: artist-properties
   :type: :confval:`Section`
   :required: false
   :default: Extensible Default, see :confval:`extend`
.. versionadded:: 2.4.0

+----------------------+-------------------+
| upnp-tag             | meta-data         |
+======================+===================+
| ``upnp:artist``      | ``M_ALBUMARTIST`` |
+----------------------+-------------------+
| ``upnp:albumArtist`` | ``M_ALBUMARTIST`` |
+----------------------+-------------------+
| ``upnp:genre``       | ``M_GENRE``       |
+----------------------+-------------------+

.. confval:: genre-properties
   :type: :confval:`Section`
   :required: false
   :default: Extensible Default, see :confval:`extend`
.. versionadded:: 2.4.0

+----------------------+-------------------+
| upnp-tag             | meta-data         |
+======================+===================+
| ``upnp:genre``       | ``M_GENRE``       |
+----------------------+-------------------+

.. confval:: playlist-properties
   :type: :confval:`Section`
   :required: false
   :default: Extensible Default, see :confval:`extend`
.. versionadded:: 2.4.0

+----------------------+-------------------+
| upnp-tag             | meta-data         |
+======================+===================+
| ``dc:date``          | ``M_UPNP_DATE``   |
+----------------------+-------------------+

.. confval:: title-properties
   :type: :confval:`Section`
   :required: false
   :default: Fixed Defaults
.. versionadded:: 2.4.0

The title properties are automatically added and cannot be changed, but you may add them under another tag.

+-----------------------------------+-------------------------------+
| upnp-tag                          | meta-data                     |
+===================================+===============================+
| ``dc:date``                       | ``M_DATE``                    |
+-----------------------------------+-------------------------------+
| ``dc:description``                | ``M_DESCRIPTION``             |
+-----------------------------------+-------------------------------+
| ``dc:publisher``                  | ``M_PUBLISHER``               |
+-----------------------------------+-------------------------------+
| ``dc:title``                      | ``M_TITLE``                   |
+-----------------------------------+-------------------------------+
| ``upnp:actor``                    | ``M_ACTOR``                   |
+-----------------------------------+-------------------------------+
| ``upnp:album``                    | ``M_ALBUM``                   |
+-----------------------------------+-------------------------------+
| ``upnp:albumArtURI``              | ``M_ALBUMARTURI``             |
+-----------------------------------+-------------------------------+
| ``upnp:artist``                   | ``M_ARTIST``                  |
+-----------------------------------+-------------------------------+
| ``upnp:artist@role[AlbumArtist]`` | ``M_ALBUMARTIST``             |
+-----------------------------------+-------------------------------+
| ``upnp:author``                   | ``M_AUTHOR``                  |
+-----------------------------------+-------------------------------+
| ``upnp:composer``                 | ``M_COMPOSER``                |
+-----------------------------------+-------------------------------+
| ``upnp:conductor``                | ``M_CONDUCTOR``               |
+-----------------------------------+-------------------------------+
| ``upnp:date``                     | ``M_UPNP_DATE``               |
+-----------------------------------+-------------------------------+
| ``upnp:director``                 | ``M_DIRECTOR``                |
+-----------------------------------+-------------------------------+
| ``upnp:episodeSeason``            | ``M_PARTNUMBER``              |
+-----------------------------------+-------------------------------+
| ``upnp:genre``                    | ``M_GENRE``                   |
+-----------------------------------+-------------------------------+
| ``upnp:longDescription``          | ``M_LONGDESCRIPTION``         |
+-----------------------------------+-------------------------------+
| ``upnp:orchestra``                | ``M_ORCHESTRA``               |
+-----------------------------------+-------------------------------+
| ``upnp:originalTrackNumber``      | ``M_TRACKNUMBER``             |
+-----------------------------------+-------------------------------+
| ``upnp:producer``                 | ``M_PRODUCER``                |
+-----------------------------------+-------------------------------+
| ``upnp:rating``                   | ``M_RATING``                  |
+-----------------------------------+-------------------------------+
| ``upnp:region``                   | ``M_REGION``                  |
+-----------------------------------+-------------------------------+
| ``upnp:playbackCount``            | ``upnp:playbackCount``        |
+-----------------------------------+-------------------------------+
| ``upnp:lastPlaybackTime``         | ``upnp:lastPlaybackTime``     |
+-----------------------------------+-------------------------------+
| ``upnp:lastPlaybackPosition``     | ``upnp:lastPlaybackPosition`` |
+-----------------------------------+-------------------------------+

Response properties contain the following entries.

    .. code-block:: xml

        <upnp-namespace xmlns="gerbera" uri="https://gerbera.io"/>
        <upnp-property upnp-tag="gerbera:artist" meta-data="M_ARTIST"/>

    Defines an UPnP property and references the namespace for the property.

    The attributes specify the property:

Property Namespace
------------------

.. confval:: upnp-namespace
   :type: :confval:`Section`
   :required: false

Add namespace required for properties.

    .. confval:: xmlns
       :type: :confval:`String`
       :required: true

       .. code-block:: xml

           xmlns="..."

    Key for the namespace

    .. confval:: uri
       :type: :confval:`String`
       :required: true

       .. code-block:: xml

           uri="..."

    Uri for the namespace

Property Content
----------------

.. confval:: upnp-property
   :type: :confval:`Section`
   :required: false

Define value of an additional property

    .. confval:: upnp-tag
       :type: :confval:`String`
       :required: true

       .. code-block:: xml

           upnp-tag="..."

    UPnP tag to be send. See the UPnP specification for valid entries.

    .. confval:: meta-data
       :type: :confval:`String`
       :required: true

       .. code-block:: xml

           meta-data="..."

.. _upnp-tags:

    Name of the metadata tag to export in upnp response. The following values are supported:
    M_TITLE, M_ARTIST, M_ALBUM, M_DATE, M_UPNP_DATE, M_GENRE, M_DESCRIPTION, M_LONGDESCRIPTION,
    M_PARTNUMBER, M_TRACKNUMBER, M_ALBUMARTURI, M_REGION, M_CREATOR, M_AUTHOR, M_DIRECTOR, M_PUBLISHER,
    M_RATING, M_ACTOR, M_PRODUCER, M_ALBUMARTIST, M_COMPOSER, M_CONDUCTOR, M_ORCHESTRA.

    Instead of metadata, you may also use auxdata entries as defined in :confval:`library-options`.

Property Defaults
=================

.. confval:: resource-defaults
   :type: :confval:`Section`
   :required: false
   :default:  Extensible Default, see :confval:`extend`
.. versionadded:: 2.4.0

.. confval:: object-defaults
   :type: :confval:`Section`
   :required: false
   :default:  Extensible Default, see :confval:`extend`
.. versionadded:: 2.4.0

.. confval:: container-defaults
   :type: :confval:`Section`
   :required: false
   :default:  Extensible Default, see :confval:`extend`

   .. versionadded:: 2.4.0
   .. code-block:: xml

       <resource-defaults>...</resource-defaults>
       <object-defaults>...</object-properties>
       <container-defaults>...</container-defaults>

Defines the default values of upnp properties if these properties are required by the UPnP request filter.
If there is no defined default value, the required filter is not exported.

It contains the following entries.

    .. confval:: property-default
       :type: :confval:`Section`
       :required: false

       .. code-block:: xml

           <property-default tag="duration" value="0"/>

    Defines an UPnP property and the default value of the property.

    The attributes specify the property:

    .. confval:: property-default tag
       :type: :confval:`String`
       :required: true

       .. code-block:: xml

           tag="..."

    UPnP property to define the default. Tags starting with a ``@`` will be generated as an attribute.

    .. confval:: property-default value
       :type: :confval:`String`
       :required: true

       .. code-block:: xml

           value="..."

    Default value for the property.


******************
Dynamic Containers
******************

.. confval:: containers
   :type: :confval:`Section`
   :required: false
   :default: Extensible Default, see :confval:`extend`

   .. code-block:: xml

       <containers enabled="yes">

Add dynamic containers to virtual layout.

This section sets the rules for additional containers which have calculated content.

Attributes:

    .. confval:: containers enabled
       :type: :confval:`Boolean`
       :required: true
       :default: ``yes``

       .. code-block:: xml

           enabled="no"

    Enables or disables the dynamic containers driver.

Dynamic Containers Content
==========================

.. confval:: containers container
   :type: :confval:`Section`
   :required: false

   .. code-block:: xml

       <container location="/New" title="Recently added" sort="-last_updated" max-count="500">
           <filter>upnp:class derivedfrom "object.item" and last_updated &gt; "@last7"</filter>
       </container>
       <container location="/NeverPlayed" title="Music Never Played" sort="upnp:album" upnp-shortcut="MUSIC_NEVER_PLAYED" max-count="100">
           <filter>upnp:class derivedfrom "object.item.audioItem" and (upnp:playbackCount exists false or upnp:playbackCount = "0")</filter>
       </container>

Defines the properties of the dynamic container.

Containers Attributes
---------------------

   The following attributes can be set for containers

    .. confval:: containers container location
       :type: :confval:`String`
       :required: true

       .. code-block:: xml

           location="..."

    Position in the virtual layout where the node is added. If it is in a sub-container, e.g. ``/Audio/New``, it only
    becomes visible if the import generates the parent container.

    .. confval:: containers container title
       :type: :confval:`String`
       :required: false
       :default: `empty`

       .. code-block:: xml

           title="..."

    Text to display as title of the container. If it is empty the last section of the location is used.

    .. confval:: containers ontainer sort
       :type: :confval:`String`
       :required: false
       :default: `empty`

       .. code-block:: xml

           sort="..."

    UPnP sort statement to use as sorting criteria for the container.

    .. confval:: containers ontainer upnp-shortcut
       :type: :confval:`String`
       :required: false
       :default: `empty`

       .. versionadded:: 2.4.0
       .. code-block:: xml

           upnp-shortcut="..."

    Set the upnp shortcut label for this container.
    For more details see UPnP-av-ContentDirectory-v4-Service, page 357.

    .. confval:: containers container max-count
       :type: :confval:`Integer`
       :required: false
       :default: ``500``

       .. code-block:: xml

           max-count="200"

    Limit the number of item in dynamic container.

    .. confval:: containers container image
       :type: :confval:`Path`
       :required: false
       :default: `empty`

       .. code-block:: xml

           image="..."

    Path to an image to display for the container. It still depends on the client whether the image becomes visible.

Dynamic Content Filter
----------------------

.. confval:: container filter
   :type: :confval:`String`
   :required: true

   .. code-block:: xml

       <filter>upnp:class derivedfrom "object.item" and last_updated &gt; "@last7"</filter>

Define a filter to run in order to get the contents of the container.
The ``<filter>`` uses the syntax of UPnP search with additional properties ``last_modified`` (date), ``last_updated`` (date),
``upnp:lastPlaybackTime`` (date), ``play_group`` (string, ``group`` from client config) and ``upnp:playbackCount`` (number).
Date properties support comparing against a special value ``"@last*"`` where ``*`` can be any integer which evaluates to
the current time minus the number of days as specified.

UPnP search syntax is defined in

- `UPnP ContentDirectory:1 <https://upnp.org/specs/av/UPnP-av-ContentDirectory-v1-Service.pdf>`_ section 2.5.5,
- `UPnP ContentDirectory:2 <https://upnp.org/specs/av/UPnP-av-ContentDirectory-v1-Service.pdf>`_ section 2.3.11.1,
- `UPnP ContentDirectory:3 <https://upnp.org/specs/av/UPnP-av-ContentDirectory-v3-Service.pdf>`_ section 2.3.13.1
- and `UPnP ContentDirectory:4 <https://upnp.org/specs/av//UPnP-av-ContentDirectory-v4-Service.pdf>`_ section 5.3.16.1.
