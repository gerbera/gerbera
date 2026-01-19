.. index:: Run Gerbera

Run Gerbera
===========

.. Note:: Whilst you can run Gerbera as a "regular" application. It is strongly recommended to run it as a :ref:`system service <daemon>` instead.

.. Warning::
    The server has an integrated :ref:`file system browser <filesystem-view>` in the UI, that means that anyone who has access to the UI can browse
    your file system (with user permissions under which the server is running) and also download your files! If you want maximum security - disable
    the UI. :ref:`Account authentication <ui>` offers simple protection that might hold back your kids, but it is not secure enough for use
    in an untrusted environment!

.. Note::
    Since the server is meant to be used in a home LAN environment the UI is enabled by default and accounts are
    deactivated, thus allowing anyone on your network to connect to the user interface.

First Time Launch
~~~~~~~~~~~~~~~~~

If you decide against running as a system service for whatever reason, then when run by a user the first time startup of Gerbera creates a folder
called ``~/.config/gerbera`` in your home directory.

You must generate a ``config.xml`` file for Gerbera to use.

Review the :ref:`Generating Configuration <generateConfig>` section of the documentation to see how to use ``gerbera`` to create a
default configuration file.

Multiple Instances
~~~~~~~~~~~~~~~~~~

If you want to run a second server from the same PC, make sure to use a different configuration file with a different udn and a different database.

After server launch the bookmark file is created in the ``~/.config/gerbera`` directory. You now can manually add the bookmark
``~/.config/gerbera/gerbera.html`` in your browser. This will redirect you to the UI if the server is running.

Assuming that you enabled the UI, you should now be able to get around quite easily.

Network Setup
~~~~~~~~~~~~~

Some systems require a special setup on the network interface. If Gerbera exits with UPnP Error -117, or if it does not
respond to M-SEARCH requests from the renderer (i.e. Gerbera is running, but your renderer device does not show it)
you should try the following settings
(the lines below assume that Gerbera is running on a Linux machine, on network interface eth1):

.. code-block:: console

    $ route add -net 239.0.0.0 netmask 255.0.0.0 eth1
    $ ifconfig eth1 allmulti

For distros that only support ``ip`` command suite it has to be

.. code-block:: console

    $ ip route add 239.0.0.0/8 dev eth1 scope link
    $ ip link set dev eth1 allmulti on

Those settings can be applied automatically by a ``init.d`` startup script (if you are still running init system).
Otherwise add them to your network settings in ``/etc/sysconfig/network`` or ``/etc/network`` depending on your distro.

You should also make sure that your firewall is not blocking port UDP port ``1900`` (required for SSDP) and UDP/TCP
port of Gerbera. By default Gerbera will select a free port starting with ``49152``, however you can specify a port
of your choice in the configuration file.

Some systems turn on return path filtering by default. In this case gerbera may not receive the multicast packats, also.
Filtering can be turned off by (select ``all`` if you only have one network interface):

.. code-block:: console

    $ echo 0 > /proc/sys/net/ipv4/conf/eth1/rp_filter
    $ echo 0 > /proc/sys/net/ipv4/conf/all/rp_filter

or

.. code-block:: console

    $ sysctl net.ipv4.conf.eth1.rp_filter=0
    $ sysctl net.ipv4.conf.all.rp_filter=0

or add the matching version of the following lines to ``/etc/sysctl.d/75-gerbera.conf``

.. code-block:: console

    $ net.ipv4.conf.eth1.rp_filter = 0
    $ net.ipv4.conf.all.rp_filter = 0


Reverse Proxy Setup
~~~~~~~~~~~~~~~~~~~

If you want to access the web interface from other sources or use a ssl certificate it is recommended to hide gerbera UI behind a reverse proxy.

* Set virtualURL in config.xml to point to ``https://gerbera.DOMAINNAME``
* Add ``gerbera`` to your DNS and have it point to the server

Apache
------

* Enable Apache modules

.. code-block:: sh

    $ sudo a2enmod proxy proxy_http ssl

* Add virtual host to your apache config (``/etc/apache2/vhosts.d/``) and modify according to your settings

    .. literalinclude:: ../scripts/apache/gerbera.conf

* Restart apache service

Nginx
-----

* Add server config to your nginx config (``/etc/nginx/vhosts.d/``) and modify according to your settings

    .. literalinclude:: ../scripts/nginx/gerbera.conf

* Restart Nginx service

Reverse proxies can also be used to handle really old devices and convert their request, e.g. to enable transcoding on http 1.0 protocol.
The example only works in conjunction with the respective gerbera configuration and `iptables` settings.

    .. literalinclude:: ../scripts/nginx/gerbera-transcode.conf


Database Setup
~~~~~~~~~~~~~~

Gerbera can be compiled with additional support for MySQL/MariaDB and PostgreSQL databases. By default Gerbera will use an SQLite database,
it requires no configuration - you are ready to go! In order to switch to another database you need to modify the ``config.xml`` section :confval:`storage`.

.. index:: Sqlite

Using Sqlite Database
---------------------

The database file will be created automatically and will be located ``~/.config/gerbera/gerbera.db``.
If needed you can adjust the database file name and location in the configuration :confval:`database-file`.

.. index:: MySQL

Using MySQL Database
--------------------

If Gerbera was compiled with support for MySQL/MariaDB databases, sqlite will still be chosen as default because the initial database
can be created and used without any user interaction. In order to use MySQL it has to be enabled with :confval:`mysql enabled`.
All other databases have to be disabled.

Gerbera has to be able to connect to the MySQL server and at least the empty database has to exist.
To create the database and provide Gerbera with the ability to connect to the MySQL server you need to have
the appropriate permissions. Note that user names and passwords in MySQL have nothing to do with UNIX accounts,
MySQL has it's own user names/passwords. Connect to the MySQL database as ”root” or any other user with the
appropriate permissions:

.. code-block:: sh

    $ mysql [-u <username>] [-p]

(You'll probably need to use ”-u” to specify a different MySQL user and ”-p” to specify a password.)

Create a new database for Gerbera: (substitute ”<database name>” with the name of the database)

.. code:: sql

    mysql> CREATE DATABASE <database name>;

(You can also use ”mysqladmin” instead.)

Give Gerbera the permissions to access the database:

.. code:: sql

    mysql> GRANT ALL ON <database name>.*
           TO '<user name>'@'<hostname>'
           IDENTIFIED BY '<password>';

If you don't want to set a password, omit ``IDENTIFIED BY`` completely. You could also use the MySQL ”root” user
with Gerbera directly, but this is not recommended.

To create a database and a user named **gerbera** (who is only able to connect via ``localhost``) without a
password (the defaults) use:

.. code:: sql

    mysql> CREATE DATABASE gerbera;
    mysql> GRANT ALL ON gerbera.* TO 'gerbera'@'localhost';

The tables will be created automatically during the first startup.
All table names have a ``mt_`` or ``grb_`` prefix, so you can theoretically share the database with a different application.
However, this is not recommended.

.. index:: PostgreSQL

Using PostgreSQL Database
-------------------------

If Gerbera was compiled with support for PostgreSQL databases, sqlite will still be chosen as default because the initial database
can be created and used without any user interaction. In order to use PostgreSQL it has to be enabled with :confval:`postgres enabled`.
All other databases have to be disabled.

Gerbera has to be able to connect to the PostgreSQL server and at least the empty database has to exist.
To create the database and provide Gerbera with the ability to connect to the PostgreSQL server you need to have
the appropriate permissions. Note that user names and passwords in PostgreSQL have nothing to do with UNIX accounts,
PostgreSQL has it's own user names/passwords. Connect to the PostgreSQL database as ”postgres” or any other user with the
appropriate permissions.

.. code-block:: sh

    $ psql

Create the empty database and give the use the appropriate permissions.

.. code:: sql

    postgres=# CREATE USER "gerbera";
    postgres=# CREATE DATABASE "gerbera" WITH OWNER "gerbera" ENCODING = 'UTF8' LC_COLLATE = 'de_DE.UTF-8' LC_CTYPE = 'de_DE.UTF-8';
    postgres=# GRANT connect ON DATABASE "gerbera" TO "gerbera";
    postgres=# GRANT pg_read_all_data TO "gerbera";
    postgres=# GRANT pg_write_all_data TO "gerbera";

The names you choose for database and user must match with the values used in the ``config.xml`` section :confval:`postgres`.

The tables will be created automatically during the first startup.
All table names have a ``mt_`` or ``grb_`` prefix, so you can theoretically share the database with a different application.
However, this is not recommended.


Last.FM Setup
~~~~~~~~~~~~~~

In order to use the Last.fm integration of Gerbera you need an account and enable LastFM support with :confval:`lastfm enabled`.
If your Gerbera build is still on the old API using LastFMLib username and password have to be set in config.xml.
For API 2.0 you need to create an apiKey with apiSecret which you set in config.xml. After that you need to run
``gerbera --init-lastfm`` which prints out the session key. Copy the session key to :confval:`lastfm sessionkey` and start gerbera normally.


Command Line Options
~~~~~~~~~~~~~~~~~~~~

.. Note:: Command line options override settings in the configuration file

There is a number of options that can be passed via command line upon server start up, for a short summary you can
invoke Gerbera with the following parameter:

.. code-block:: sh

    $ gerbera --help

IP Address
----------

::

    --ip or -i

The server will bind to the given IP address, currently we can not bind to multiple interfaces so binding to ``0.0.0.0``
is not be possible.

Interface
---------

::

    --interface or -e

Interface to bind to, for example eth0, this can be specified instead of the IP address.

Port
----

::

    --port or -p

Specify the server port that will be used for the web user interface, for serving media and for UPnP requests,
minimum allowed value is ``49152``. If this option is omitted a default port will be chosen, however, in
this case it is possible that the port will change upon server restart.

Daemon
------

::

    --daemon or -d

Daemonize after startup. This option is useful if your system does not use Systemd or similar
mechanisms to start services. See also --user and --pidfile options, below.

User
----

::

    --user or -u

After startup when started by user root try to change all UIDs and GIDs to those belonging to user USER.
Also supplementary GIDs will be set.

Pidfile
-------

::

    --pidfile or -P

Write a pidfile to the specified location. Full path is needed, e.g. /run/gerbera.pid.

Configuration File
------------------

::

     --config or -c

By default Gerbera will search for a file named **config.xml** in the ``~/.config/gerbera`` directory.
This option allows you to specify a config file by the name and location of your choice.
The file name must be absolute.

Home Directory
--------------

::

    --home or -m

Specify an alternative home directory. By default Gerbera will try to retrieve the users home directory from the
environment, then it will look for a ``.config/gerbera`` directory in users home. If ``.config/gerbera`` was found the system tries to
find the default configuration file (config.xml), if not found the system creates the ``.config/gerbera`` directory.

This option is useful in two cases: when the home directory cannot be retrieved from the environment (in this case
you could also use -c to point Gerbera to your configuration file or when you want test
a non standard location (for example, when setting up daemon mode). In both cases you can also
set the environment variable ``GERBERA_HOME`` to override ``HOME``.

Config Directory
----------------

::

    --cfgdir or -f

The default configuration directory is combined out of the users home and the default that equals to ``.config/gerbera``,
this option allows you to override the default directory naming. This is useful when you want to setup the server in a
nonstandard location, but want that the default configuration to be written by the server.

Custom Scripts Directory
------------------------

::

    --scripts

The javascript files shipped with gerbera should not be modified. Instead, they can be extended or overwritten in custom scripts.
The folder for these can be either set in ``config.xml`` with :confval:`script-folder custom` or provided by the command line argument.

Magic File
----------

::

    --magic

The magic file is set/overwritten. By default it is read from environment variables ``GERBERA_MAGIC_FILE`` or ``MEDIATOMB_MAGIC_FILE``.

Add Content
-----------

::

    --add-file /path/to/file [--add-file /path/to/other/file]

Add the specified directory or file name to the database without UI interaction. The path must be absolute, if
path is a directory then it will be added recursively. If path is a file, then only the given file will be imported.
Can be supplied multiple times to add multiple paths

Set Option/Print Options
-------------------------

::

    --set-option

Set the specified option number to its proper value like in config.xml. This overwrites values from config.xml.
See --print-options for valid options. Multiple options can either be specified sperated by commas or
with another use of --set-option. The syntax is OPT=VAL.

::

    --print-options

Print all option numbers available for use with --set-option.

Offline
------------

::

    --offline

Do not answer UPnP requests like browse. This is helpful when running a large scan to initialize
the database so no client can slow down the import by accessing the database.

Drop tables
------------

::

    --drop-tables

Remove tables from database to trigger a new import.

Init Last.FM
------------

::

    --init-lastfm

Get the Last.FM session key. You need to set the apiKey in the :confval:`lastfm username` and the apiSecret in the :confval:`lastfm password`.
Follow the instrutions on the screen and copy the printed sessionKey to :confval:`lastfm sessionkey`.


Version Information
-------------------

::

    --version

Print version information and exit.

Display Command Line Summary
----------------------------

::

    --help or -h

Print a summary about the available command line options.
