.. index:: Server Configuration

Configure Server
================

These settings define the server configuration, this includes UPnP behavior, selection of database, accounts for the UI as well as installation locations of shared data.

.. _server:

``server``
~~~~~~~~~~

.. code-block:: xml

    <server> ... </server>

* Required

This section defines the server configuration parameters.

    **attributes:**

        ::

            debug-mode=...

      * Optional
      * Default: **””**

      Activate debugging messages only for certain subsystems.
      The following subsystems are available:
      ``thread``, ``sqlite3``, ``cds``, ``server``, ``config``,
      ``content``, ``update``, ``mysql``,
      ``sql``, ``proc``, ``autoscan``, ``script``, ``web``, ``layout``,
      ``exif``, ``exiv2``, ``transcoding``, ``taglib``, ``ffmpeg``, ``wavpack``,
      ``requests``, ``device``, ``connmgr``, ``mrregistrar``, ``xml``,
      ``clients``, ``iohandler``, ``online``, ``metadata``, ``matroska``,
      ``curl``, ``util`` and ``verbose``.
      Multiple subsystems can be combined with a ``|``. Names are not case
      sensitive. ``verbose`` turns on even more messages for the subsystem.
      This is for developers and testers mostly and has to be
      activted in cmake options at compile time (``-DWITH_DEBUG_OPTIONS=YES``).

      * Example: ”Cds|Content|Web” for messages when accessing the server via upnp or web.

        ::

            upnp-max-jobs="300"

      * Optional
      * Default: **500**

      Set maximum number of jobs in libpupnp internal threadpool.
      Allows pending requests to be handled.

``port``
~~~~~~~~

.. code-block:: xml

    <port>0</port>

* Optional
* Default: **0** `(automatic)`

Specifies the port where the server will be listening for HTTP requests. Note, that because of the implementation in the UPnP SDK
only ports above 49152 are supported. The value of zero means, that a port will be automatically selected by the SDK.

``ip``
~~~~~~

.. code-block:: xml

    <ip>192.168.0.23</ip>

* Optional
* Default: **ip of the first available interface.**

Specifies the IP address to bind to, by default one of the available interfaces will be selected.

``interface``
~~~~~~~~~~~~~

.. code-block:: xml

    <interface>eth0</interface>

* Optional
* Default: **first available interface.**

Specifies the interface to bind to, by default one of the available interfaces will be selected.

``name``
~~~~~~~~

.. code-block:: xml

    <name>Gerbera</name>

* Optional
* Default: **Gerbera**

Server friendly name, you will see this on your devices that you use to access the server.

``manufacturer``
~~~~~~~~~~~~~~~~~~~

.. code-block:: xml

    <manufacturer>Gerbera Developers</manufacturer>

* Optional
* Default: empty

This tag sets the manufacturer name of a UPnP device.

``manufacturerURL``
~~~~~~~~~~~~~~~~~~~

.. code-block:: xml

    <manufacturerURL>https://gerbera.io/</manufacturerURL>

* Optional
* Default: **https://gerbera.io/**

This tag sets the manufacturer URL of a UPnP device, a custom setting may be necessary to trick some renderers in order
to enable special features that otherwise are only active with the vendor implemented server.

``virtualURL``
~~~~~~~~~~~~~~

.. code-block:: xml

    <virtualURL>https://gerbera.io/</virtualURL>

* Optional
* Default: unset

This tag sets the virtual URL of Gerbera content which is part of the browse response.
The value defaults to `http://<ip>:<port>`.

``externalURL``
~~~~~~~~~~~~~~~

.. code-block:: xml

    <externalURL>https://gerbera.io/</externalURL>

* Optional
* Default: unset

This tag sets the external URL of Gerbera web UI, a custom setting may be necessary if you want to access the web page via a reverse proxy.
The value defaults to virtualURL or `http://<ip>:<port>` if virtualURL is not set.

``modelName``
~~~~~~~~~~~~~

.. code-block:: xml

    <modelName>Gerbera</modelName>

* Optional
* Default: **Gerbera**

This tag sets the model name of a UPnP device, a custom setting may be necessary to trick some renderers in order to
enable special features that otherwise are only active with the vendor implemented server.

``modelNumber``
~~~~~~~~~~~~~~~

.. code-block:: xml

    <modelNumber>0.9.0</modelNumber>

* Optional
* Default: **Gerbera version**

This tag sets the model number of a UPnP device, a custom setting may be necessary to trick some renderers in order
to enable special features that otherwise are only active with the vendor implemented server.

``modelURL``
~~~~~~~~~~~~~~~

.. code-block:: xml

    <modelURL>http://example.org/product-23</modelURL>

* Optional
* Default: empty

This tag sets the model URL (homepage) of a UPnP device.

``serialNumber``
~~~~~~~~~~~~~~~~

.. code-block:: xml

    <serialNumber>1</serialNumber>

* Optional
* Default: **1**

This tag sets the serial number of a UPnP device.

``presentationURL``
~~~~~~~~~~~~~~~~~~~

.. code-block:: xml

    <presentationURL append-to="ip">80/index.html</presentationURL>

* Optional
* Default: **”/”**

The presentation URL defines the location of the servers user interface, usually you do not need to change this
however, vendors who want to ship our server along with their NAS devices may want to point to the main configuration
page of the device.

    **attributes:**

        ::

            append-to=...

      * Optional
      * Default: **”none”**

      The append-to attribute defines how the text in the presentationURL tag should be treated.
      The allowed values are:

          ::

              append-to="none"

          Use the string exactly as it appears in the presentationURL tag.

          ::

              append-to="ip"

          Append the string specified in the presentationURL tag to the ip address of the server, this is useful in a
          dynamic ip environment where you do not know the ip but want to point the URL to the port of your web server.

          ::

              append-to="port"

          Append the string specified in the presentationURL tag to the server ip and port, this may be useful if you want
          to serve some static pages using the built in web server.


``udn``
~~~~~~~

.. code-block:: xml

    <udn>uuid:[generated-uuid]</udn>

* Required
* Default: **none**

Unique Device Name, according to the UPnP spec it must be consistent throughout reboots. You can fill in something
yourself.  Review the :ref:`Generating Configuration <generateConfig>` section of the documentation to see how to use
``gerbera`` to create a default configuration file.

``home``
~~~~~~~~

.. code-block:: xml

    <home override="yes">/home/your_user_name/gerbera</home>

* Required
* Default: **`~`** - the HOME directory of the user running gerbera.

Server home - the server will search for the data that it needs relative to this directory - basically for the sqlite database file.
The gerbera.html bookmark file will also be generated in that directory.
The home directory is only relevant if the config file or the config dir was specified
in the command line. Otherwise it defaults to the ``HOME`` path of the user runnung
Gerbera. The environment variable ``GERBERA_HOME`` can be used to point to another directory,
in which case the config file is expected as ``${GERBERA_HOME}/.config/gerbera``.

      ::

          override="yes"

      * Optional
      * Default: **”no”**

      Force all relative paths to base on the home directory of the config file even
      if it was read relative to the environment variables or from command line. This
      means that Gerbara changes its home during startup.

``webroot``
~~~~~~~~~~~

.. code-block:: xml

    <webroot>/usr/share/gerbera/web</webroot>

* Required
* Default: **depends on the installation prefix that is passed to the configure script.**

Root directory for the web server, this is the location where device description documents, UI html and js files, icons, etc. are stored.

``alive``
~~~~~~~~~

.. code-block:: xml

    <alive>180</alive>

* Optional
* Default: **180**, (Results in alive messages every 60s, see below) `this is according to the UPnP specification.`
* Min: 62 (A message sent every 1s, see below)

Interval for broadcasting SSDP:alive messages

An advertisement will be sent by LibUPnP every (this value/2)-30 seconds, and will have a cache-control max-age of this value.

For example: A value of 62 will result in an SSDP advertisement being sent every second. (62 / 2 = 31) - 30 = 1.
The default value of 180 results results in alive messages every 60s. (180 / 2 = 90) - 30 = 60.

Note:
   If you experience disconnection problems from your device, e.g. Playstation 4, when streaming videos after about 5 minutes,
   you can try changing the alive value to 86400 (which is 24 hours)

``pc-directory``
~~~~~~~~~~~~~~~~

.. code-block:: xml

    <pc-directory upnp-hide="no"/>

* Optional
* Default: **no**

Enabling this option will make the PC-Directory container invisible for UPnP devices.

Note:
   independent of the above setting the container will be always visible in the web UI!

``tmpdir``
~~~~~~~~~~

.. code-block:: xml

    <tmpdir>/tmp/</tmpdir>

* Optional
* Default: **/tmp/**

Selects the temporary directory that will be used by the server.

``bookmark``
~~~~~~~~~~~~

.. code-block:: xml

    <bookmark>gerbera.html</bookmark>

* Optional
* Default: **gerbera.html**

The bookmark file offers an easy way to access the user interface, it is especially helpful when the server is
not configured to run on a fixed port. Each time the server is started, the bookmark file will be filled in with a
redirect to the servers current IP address and port. To use it, simply bookmark this file in your browser,
the default location is ``~/.config/gerbera/gerbera.html``

``upnp-string-limit``
~~~~~~~~~~~~~~~~~~~~~

.. code-block:: xml

    <upnp-string-limit>

* Optional
* Default: **disabled**

This will limit title and description length of containers and items in UPnP browse replies, this feature was added
as a workaround for the TG100 bug which can only handle titles no longer than 100 characters.
A negative value will disable this feature, the minimum allowed value is "4" because three dots will be appended
to the string if it has been cut off to indicate that limiting took place.

.. _logging:

``logging``
~~~~~~~~~~~

.. code-block:: xml

    <logging rotate-file-size="1000000" rotate-file-count="3"/>

* Optional

This section defines various logging settings.


    **Attributes:**

    ::

        rotate-file-size=...

    * Optional
    * Default: **5242880** (5 MB)

    When using command line option ``--rotatelog`` this value defines the maximum size of the log file before rotating.

    ::

        rotate-file-count=...

    * Optional
    * Default: **10**

    When using command line option ``--rotatelog`` this value defines the number of files in the log rotation.

.. _ui:

``ui``
~~~~~~

.. code-block:: xml

    <ui enabled="yes" poll-interval="2" poll-when-idle="no"/>

* Optional

This section defines various user interface settings.

**WARNING!**

The server has an integrated filesystem browser, that means that anyone who has access to the UI can browse
your filesystem (with user permissions under which the server is running) and also download your data!
If you want maximum security - disable the UI completely! Account authentication offers simple protection that
might hold back your kids, but it is not secure enough for use in an untrusted environment!

Note:
   since the server is meant to be used in a home LAN environment the UI is enabled by default and accounts are
   deactivated, thus allowing anyone on your network to connect to the user interface.

    **Attributes:**

    ::

        enabled=...

    * Optional
    * Default: **yes**

    Enables (”yes”) or disables (”no”) the web user interface.

    ::

        show-tooltips=...

    * Optional
    * Default: **yes**

    This setting specifies if icon tooltips should be shown in the web UI.

    ::

        show-numbering=...

    * Optional
    * Default: **yes**

    Set track number to be shown in the web UI.

    ::

        show-thumbnail=...

    * Optional
    * Default: **yes**

    This setting specifies if thumbnails or cover art should be shown in the web UI.

    ::

        poll-interval=...

    * Optional
    * Default: **2**

    The poll-interval is an integer value which specifies how often the UI will poll for tasks. The interval is
    specified in seconds, only values greater than zero are allowed. The value can be given in a valid time format.

    ::

        poll-when-idle=...

    * Optional
    * Default: **no**

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

   **Child tags:**

``source-docs-link``
--------------------

    .. code-block:: xml

       <source-docs-link>./dev/index.html</source-docs-link>

    * Optional
    * Default: empty

    Add link to some source documentation which can be generated by ``make doc``. If it is empty the link in the web UI will be hidden.

``user-docs-link``
------------------

    .. code-block:: xml

       <user-docs-link>./doc/index.html</user-docs-link>

    * Optional
    * Default: for release builts: "https://docs.gerbera.io/en/stable/", for test builts: "https://docs.gerbera.io/en/latest/"

    Add link to the user documentation if you want it locally hosted or make sure the version is matching you installation.

``content-security-policy``
---------------------------

    .. code-block:: xml

       <content-security-policy>default-src %HOSTS% 'unsafe-eval' 'unsafe-inline'; img-src *; media-src *; child-src 'none';</content-security-policy>

    * Optional
    * Default: **default-src %HOSTS% 'unsafe-eval' 'unsafe-inline'; img-src *; media-src *; child-src 'none';**
    * Example:

        .. code-block:: xml

           <content-security-policy>
               font-src %HOSTS% https://fonts.gstatic.com/
               style-src %HOSTS% https://fonts.googleapis.com 'unsafe-inline'
               img-src *
               media-src *
               child-src 'none'
               default-src %HOSTS% 'unsafe-eval' 'unsafe-inline'
           </content-security-policy>

    Define the "Content-Security-Policy" string for the web ui. The string ``%HOHSTS%`` will be replaced by the IP adress and known server names.
    Newlines will automatically be replaced by ``;``.

``extension-mimetype``
----------------------

    .. code-block:: xml

       <extension-mimetype default="application/octet-stream">
           <map from="html" to="text/html"/>
           <map from="js" to="application/javascript"/>
           <map from="json" to="application/json"/>
           <map from="css" to="text/css"/>
       </extension-mimetype>

    * Optional
    * Extensible Default: see above

    For description see :ref:`Import Extension Mimetype Mapping <extension-mimetype>`.

    Attributes:

        ::

            default="application/octet-stream"

    * Optional
    * Default: **application/octet-stream**

``accounts``
------------

    .. code-block:: xml

       <accounts enabled="yes" session-timeout="30"/>

    * Optional

    This section holds various account settings.

    Attributes:

        ::

            enabled=...

        * Optional
        * Default: **yes**

        Specifies if accounts are enabled ``yes`` or disabled ``no``.

        ::

            session-timeout=...

        * Optional
        * Default: **30**

        The session-timeout attribute specifies the timeout interval in minutes. The server checks every
        five minutes for sessions that have timed out, therefore in the worst case the session times out
        after session-timeout + 5 minutes. The value can be given in a valid time format.

    Accounts can be defined as shown below:

    .. code-block:: xml

        <account user="name" password="password"/>
        <account user="name" password="password"/>

    * Optional

    There can be multiple users, however this is mainly a feature for the future. Right now there are
    no per-user permissions.

``items-per-page``
------------------

    .. code-block:: xml

        <items-per-page default="25">

    * Optional
    * Default: **25**

    This sets the default number of items per page that will be shown when browsing the database in the web UI.
    The values for the items per page drop down menu can be defined in the following manner:

    .. code-block:: xml

        <option>10</option>
        <option>25</option>
        <option>50</option>
        <option>100</option>

    * Extensible Default: **10, 25, 50, 100**

    Note:
        this list must contain the default value, i.e. if you define a default value of 25, then one of the
        ``<option>`` tags must also list this value.


.. _storage:

``storage``
~~~~~~~~~~~

.. code-block:: xml

    <storage use-transactions="yes">

* Required

Defines the storage section - database selection is done here. Currently SQLite3 and MySQL are supported.
Each storage driver has it's own configuration parameters.

Exactly one driver must be enabled: ``sqlite3`` or ``mysql``. The available options depend on the selected driver.

    **Attributes**

    ::

        use-transactions="yes"

    * Optional

    * Default: **no**

    Enables transactions. This feature should improve the overall import speed and avoid race-conditions on import.
    The feature caused some issues and set to **no**. If you want to support testing, turn it to **yes** and report
    if you can reproduce the issue.

    **SQLite**

    .. code-block:: xml

        <sqlite3 enabled="yes">

    Defines the SQLite storage driver section.

        ::

            enabled="yes"

        * Optional
        * Default: **yes**

        ::

            shutdown-attempts="10"

        * Optional
        * Default: **5**

          Number of attempts to shutdown the sqlite adapter before forcing the application down.

        Below are the sqlite driver options:

        .. code-block:: xml

            <init-sql-file>/etc/gerbera/sqlite3.sql</init-sql-file>

        * Optional
        * Default: **Datadir / sqlite3.sql**

        The full path to the init script for the database

        .. code-block:: xml

            <upgrade-file>/etc/gerbera/sqlite3-upgrade.xml</upgrade-file>

        * Optional
        * Default: **Datadir / sqlite3-upgrade.xml**

        The full path to the upgrade settings for the database

        .. code-block:: xml

            <database-file>gerbera.db</database-file>

        * Optional
        * Default: **gerbera.db**

        The database location is relative to the server's home, if the sqlite database does not exist it will be
        created automatically.

        .. code-block:: xml

            <synchronous>off</synchronous>

        * Optional
        * Default: **off**

        Possible values are ``off``, ``normal``, ``full`` and ``extra``.

        This option sets the SQLite pragma **synchronous**. This setting will affect the performance of the database
        write operations. For more information about this option see the SQLite documentation: https://www.sqlite.org/pragma.html#pragma_synchronous

        .. code-block:: xml

            <journal-mode>off</journal-mode>

        * Optional
        * Default: **WAL**

        Possible values are  ``OFF``, ``DELETE``, ``TRUNCATE``, ``PERSIST``, ``MEMORY`` and ``WAL``

        This option sets the SQLite pragma **journal_mode**. This setting will affect the performance of the database
        write operations. For more information about this option see the SQLite documentation: https://www.sqlite.org/pragma.html#pragma_journal_mode

        .. code-block:: xml

            <on-error>restore</on-error>

        * Optional
        * Default: **restore**

        Possible values are ``restore`` and ``fail``.

        This option tells Gerbera what to do if an SQLite error occurs (no database or a corrupt database).
        If it is set to **restore** it will try to restore the database from a backup file (if one exists) or try to
        recreate a new database from scratch.

        If the option is set to **fail**, Gerbera will abort on an SQLite error.

        .. code-block:: xml

            <backup enabled="no" interval="15:00"/>

        * Optional

        Backup parameters:

                ::

                    enabled=...

                * Optional
                * Default: **no**

                Enables or disables database backup.

                ::

                    interval=...

                * Optional
                * Default: **600**

                Defines the backup interval in seconds. The value can be given in a valid time format.

    **MySQL**

    .. code-block:: xml

        <mysql enabled="no"/>

    Defines the MySQL storage driver section.

        ::

            enabled=...

        * Optional
        * Default: **no**

        Enables or disables the MySQL driver.

        Below are the child tags for MySQL:

        .. code-block:: xml

            <host>localhost</host>

        * Optional
        * Default: **"localhost"**

        This specifies the host where your MySQL database is running.

        .. code-block:: xml

            <port>0</port>

        * Optional
        * Default: **0**

        This specifies the port where your MySQL database is running.

        .. code-block:: xml

            <username>root</username>

        * Optional
        * Default: **"gerbera"**

        This option sets the user name that will be used to connect to the database.

        .. code-block:: xml

            <password></password>

        * Optional
        * Default: **no password**

        Defines the password for the MySQL user. If the tag doesn't exist Gerbera will use no password, if
        the tag exists, but is empty Gerbera will use an empty password. MySQL has a distinction between
        no password and an empty password.

        .. code-block:: xml

            <database>gerbera</database>

        * Optional

        * Default: **"gerbera"**

        Name of the database that will be used by Gerbera.

        .. code-block:: xml

            <init-sql-file>/etc/gerbera/mysql.sql</init-sql-file>

        * Optional
        * Default: **Datadir / mysql.sql**

        The full path to the init script for the database

        .. code-block:: xml

            <upgrade-file>/etc/gerbera/mysql-upgrade.xml</upgrade-file>

        * Optional
        * Default: **Datadir / mysql-upgrade.xml**

        The full path to the upgrade settings for the database

.. _upnp:

``upnp``
~~~~~~~~

.. code-block:: xml

    <upnp multi-value="yes" search-result-separator=" : ">

* Optional

Modify the settings for UPnP items.

This section defines the properties which are send to UPnP clients as part of the response.

    **Attributes**
        ::

            searchable-container-flag="yes"

        * Optional

        * Default: **"no"**

        Only return containers that have the flag **searchable** set.

        ::

            dynamic-descriptions="no"

        * Optional

        * Default: **"yes"**

        Return UPnP description requests based on the client type. This hides,
        e.g., Samsung specific extensions in ``description.xml`` and ``cds.xml``
        from clients that don't handle the respective requests.

        ::

            literal-host-redirection="yes"

        * Optional

        * Default: **"no"**

        Enable literal IP redirection.

        ::

            search-result-separator=" : "

        * Optional

        * Default: **" - "**

        String used to concatenate result segments as defined in ``search-item-result``

        ::

            multi-value="no"

        * Optional

        * Default: **yes**

        Enables sending multi-valued metadata in separate items. If set to **no** all values are concatenated by CFG_IMPORT_LIBOPTS_ENTRY_SEP. Otherwise each item is added separately.

        * Example:
            The follow data is sent if set to **no**

            .. code-block:: xml

                <upnp:artist>First Artist / Second Artist</upnp:artist>

            The follow data is sent if set to **yes**

            .. code-block:: xml

                <upnp:artist>First Artist</upnp:artist>
                <upnp:artist>Second Artist</upnp:artist>

        ::

            search-filename="yes"

        * Optional

        * Default: **no**

        Older versions of gerbera have been searching in the file name instead of the title metadata. If set to yes this behaviour is back, even if the result of the search shows another title.

        ::

            caption-info-count="0"

        * Optional

        * Default: **1**

        Number of ``sec::CaptionInfoEx`` entries to write to UPnP result. Default can be overwritten by clients setting.

    **Child tags:**

    .. code-block:: xml

        <search-item-result>
            <add-data tag="M_ARTIST"/>
            <add-data tag="M_TITLE"/>
        </search-item-result>

    * Optional
    * Extensible Default

    Set the meta-data search tags to use in search result for title. The default settings as shown above produces ``artist - title`` in the result list.

    .. code-block:: xml

        <album-properties>...</album-properties>
        <artist-properties>...</artist-properties>
        <genre-properties>...</genre-properties>
        <playlist-properties>...</playlist-properties>
        <title-properties>...</title-properties>

    * Optional

    Defines the properties to send in the response.

    It contains the following entries.

    .. code-block:: xml

        <upnp-namespace xmlns="gerbera" uri="https://gerbera.io"/>
        <upnp-property upnp-tag="gerbera:artist" meta-data="M_ARTIST"/>

    * Optional

    Defines an UPnP property and references the namespace for the property.

    The attributes specify the property:

        ::

            xmlns="..."

        * Required

        Key for the namespace

        ::

            uri="..."

        * Required

        Uri for the namespace

        ::

            upnp-tag="..."

        * Required

        UPnP tag to be send. See the UPnP specification for valid entries.

        ::

            meta-data="..."

        * Required

        Name of the metadata tag to export in upnp response. The following values are supported:
        M_TITLE, M_ARTIST, M_ALBUM, M_DATE, M_UPNP_DATE, M_GENRE, M_DESCRIPTION, M_LONGDESCRIPTION,
        M_PARTNUMBER, M_TRACKNUMBER, M_ALBUMARTURI, M_REGION, M_CREATOR, M_AUTHOR, M_DIRECTOR, M_PUBLISHER,
        M_RATING, M_ACTOR, M_PRODUCER, M_ALBUMARTIST, M_COMPOSER, M_CONDUCTOR, M_ORCHESTRA.

        Instead of metadata, you may also use auxdata entries as defined in ``library-options``.

    * Extensible Default:

.. _upnp-tags:

    * Album-Properties

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

    * Artist-Properties

    +----------------------+-------------------+
    | upnp-tag             | meta-data         |
    +======================+===================+
    | ``upnp:artist``      | ``M_ALBUMARTIST`` |
    +----------------------+-------------------+
    | ``upnp:albumArtist`` | ``M_ALBUMARTIST`` |
    +----------------------+-------------------+
    | ``upnp:genre``       | ``M_GENRE``       |
    +----------------------+-------------------+

    * Genre-Properties

    +----------------------+-------------------+
    | upnp-tag             | meta-data         |
    +======================+===================+
    | ``upnp:genre``       | ``M_GENRE``       |
    +----------------------+-------------------+

    * Playlist-Properties

    +----------------------+-------------------+
    | upnp-tag             | meta-data         |
    +======================+===================+
    | ``dc:date``          | ``M_UPNP_DATE``   |
    +----------------------+-------------------+

    * Title-Properties

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

    .. code-block:: xml

        <resource-defaults>...</resource-defaults>
        <object-defaults>...</object-properties>
        <container-defaults>...</container-defaults>

    * Optional

    Defines the default values of upnp properties if these properties are required by the UPnP request filter .
    If there is no defined default value, the required filter is not exported.

    It contains the following entries.

    .. code-block:: xml

        <property-default tag="duration" value="0"/>

    * Optional

    Defines an UPnP property and the default value of the property.

    The attributes specify the property:

        ::

            tag="..."

        * Required

        UPnP property to define the default. Tags starting with a ``@`` will be generated as an attribute.

        ::

            value="..."

        * Required

        Default value for the property.


``containers``
~~~~~~~~~~~~~~

.. code-block:: xml

    <containers enabled="yes">

* Optional
* Extensible Default

Add dynamic containers to virtual layout.

This section sets the rules for additional containers which have calculated content.

    **Attributes:**

    ::

        enabled=...

    * Optional
    * Default: **yes**

    Enables or disables the dynamic containers driver.

    **Child tags:**

    ::


        <container location="/New" title="Recently added" sort="-last_updated" max-count="500">
            <filter>upnp:class derivedfrom "object.item" and last_updated &gt; "@last7"</filter>
        </container>

    * Optional

    Defines the properties of the dynamic container.

    It contains the following entries.

        .. code-block:: xml

            <filter>upnp:class derivedfrom "object.item" and last_updated &gt; "@last7"</filter>

        * Required

        Define a filter to run in order to get the contents of the container.
        The ``<filter>`` uses the syntax of UPnP search with additional properties ``last_modified`` (date), ``last_updated`` (date),
        ``upnp:lastPlaybackTime`` (date), ``play_group`` (string, ``group`` from client config) and ``upnp:playbackCount`` (number).
        Date properties support comparing against a special value ``"@last*"`` where ``*`` can be any integer which evaluates to
        the current time minus the number of days as specified.

        UPnP search syntax is defined in
        `UPnP ContentDirectory:1 <https://upnp.org/specs/av/UPnP-av-ContentDirectory-v1-Service.pdf>`_ section 2.5.5,
        `UPnP ContentDirectory:2 <https://upnp.org/specs/av/UPnP-av-ContentDirectory-v1-Service.pdf>`_ section 2.3.11.1,
        `UPnP ContentDirectory:3 <https://upnp.org/specs/av/UPnP-av-ContentDirectory-v3-Service.pdf>`_ section 2.3.13.1
        and
        `UPnP ContentDirectory:4 <https://upnp.org/specs/av//UPnP-av-ContentDirectory-v4-Service.pdf>`_ section 5.3.16.1.

        ::

            location="..."

        * Required

        Position in the virtual layout where the node is added. If it is in a sub-container, e.g. ``/Audio/New``, it only
        becomes visible if the import generates the parent container.

        ::

            title="..."

        * Optional

        Text to display as title of the container. If it is empty the last section of the location is used.

        ::

            sort="..."

        * Optional

        UPnP sort statement to use as sorting criteria for the container.

        ::

            max-count="200"

        * Optional

        Limit the number of item in dynamic container.

        * Default: 500

        ::

            image="..."

        * Optional

        Path to an image to display for the container. It still depends on the client whether the image becomes visible.
