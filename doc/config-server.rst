.. index:: Server Configuration

Configure Server
================

These settings define the server configuration, this includes UPnP behavior, selection of database, accounts for the UI as well as installation locations of shared data.

``server``
~~~~~~~~~~

.. code-block:: xml

    <server> ... </server>

* Required

This section defines the server configuration parameters.

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

``manufacturerURL``
~~~~~~~~~~~~~~~~~~~

.. code-block:: xml

    <manufacturerURL>http://gerbera.io/</manufacturerURL>

* Optional
* Default: **http://gerbera.io/**

This tag sets the manufacturer URL of a UPnP device, a custom setting may be necessary to trick some renderers in order
to enable special features that otherwise are only active with the vendor implemented server.

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

    <home>/home/your_user_name/.config/gerbera</home>

* Required
* Default: **`~/.config/gerbera`**

Server home - the server will search for the data that it needs relative to this directory - basically for the sqlite database file.
The gerbera.html bookmark file will also be generated in that directory.

``webroot``
~~~~~~~~~~~

.. code-block:: xml

    <webroot>/usr/share/gerbera/web</webroot>

* Required
* Default: **depends on the installation prefix that is passed to the configure script.**

Root directory for the web server, this is the location where device description documents, UI html and js files, icons, etc. are stored.

``serverdir``
~~~~~~~~~~~~~

.. code-block:: xml

    <servedir>/home/myuser/mystuff</servedir>

* Optional
* Default: **empty (disabled)**

Files from this directory will be served as from a regular web server. They do not need to be added to the database,
but they are also not served via UPnP browse requests. Directory listing is not supported, you have to specify full paths.

**Example:**
    The file something.jar is located in ``/home/myuser/mystuff/javasubdir/something.jar`` on your filesystem.
    Your ip address is 192.168.0.23, the server is running on port 50500. Assuming the above configuration you
    could download it by entering this link in your web browser: ``http://192.168.0.23:50500/content/serve/javasubdir/something.jar``

``alive``
~~~~~~~~~

.. code-block:: xml

    <alive>180</alive>

* Optional
* Default: **180**, `this is according to the UPnP specification.`

Interval for broadcasting SSDP:alive messages

``protocolInfo``
~~~~~~~~~~~~~~~~

.. code-block:: xml

    <protocolInfo extend="no"/>

* Optional
* Default: **no**

Adds specific tags to the protocolInfo attribute, this is required to enable MP3 and MPEG4 playback on Playstation 3.

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

``custom-http-headers``
~~~~~~~~~~~~~~~~~~~~~~~

`Not yet supported with` **pupnp 1.8.x**

.. code-block:: xml

    <custom-http-headers>

* Optional

This section holds the user defined HTTP headers that will be added to all HTTP responses that come from the server.

    **Child tags:**

       .. code-block:: xml

            <add header="..."/>
            <add header="..."/>

    * Optional

    | Specify a header to be added to the response.
    | If you have a DSM-320 use ``<add header="X-User-Agent: redsonic"/>``
    | to fix the .AVI playback problem.

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

        poll-interval=...

    * Optional
    * Default: **2**

    The poll-interval is an integer value which specifies how often the UI will poll for tasks. The interval is
    specified in seconds, only values greater than zero are allowed.

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
        after session-timeout + 5 minutes.

    Accounts can be defined as shown below:

    .. code-block:: xml

        <account user="name" password="password"/>
        <account user="name" password="password"/>

    * Optional

    There can be multiple users, however this is mainly a feature for the future. Right now there are
    no per-user permissions.

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

    **Default: 10, 25, 50, 100**

    Note:
        this list must contain the default value, i.e. if you define a default value of 25, then one of the
        ``<option>`` tags must also list this value.


.. _storage:

``storage``
~~~~~~~~~~~

.. code-block:: xml

    <storage caching="yes">

* Required

Defines the storage section - database selection is done here. Currently sqlite3 and mysql are supported.
Each storage driver has it's own configuration parameters.

    **Child Tags**
    ::

        caching="yes"

    * Optional

    * Default: **yes**

    Enables caching, this feature should improve the overall import speed.

    .. code-block:: xml

        <sqlite enabled="yes>

    * Required **if MySQL is not defined**

    Allowed values are ``sqlite3`` or ``mysql``, the available options depend on the selected driver.

    ::

        enabled="yes"

    * Optional
    * Default: **yes**

    Below are the sqlite driver options:

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

    Possible values are ``off``, ``normal`` and ``full``.

    This option sets the SQLite pragma **synchronous**. This setting will affect the performance of the database
    write operations. For more information about this option see the SQLite documentation: http://www.sqlite.org/pragma.html#pragma_synchronous

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

        <backup enabled="no" interval="6000"/>

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

        Defines the backup interval in seconds.

    .. code-block:: xml

        <mysql enabled="no"/>

    Defines the MySQL storage driver section.

        ::

            enabled=...

        * Optional
        * Default: **yes**

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
