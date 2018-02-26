.. index:: Run Gerbera

Run Gerbera
===========

When you run Gerbera for the first time a default configuration is created in
the ``~/.config/gerbera`` directory. If you are using the sqlite database no further intervention is necessary,
if you are using MySQL you will have to make some adjustments (see :ref:`Configuration <storage>` section for more details).
To start the server simply run "gerbera" from the console, to shutdown cleanly press Ctrl-C. At start up Gerbera
will print a link to the web UI.

If you want to run a second server from the same PC, make sure to use a different
configuration file with a different udn and a different database.

After server launch the bookmark file is created in the ``~/.config/gerbera`` directory.
You now can manually add the bookmark ``~/.config/gerbera/gerbera.html`` in your browser.
This will redirect you to the UI if the server is running.

Assuming that you enabled the UI, you should now be able to get around quite easily.

We also support the :ref:`daemon mode <daemon>` which allows to start the server in background, the --user and --group
parameters should be used to run the server under an unprivileged account.
A script for starting/stopping the server is provided.


Network Setup
~~~~~~~~~~~~~

Some systems require a special setup on the network interface. If Gerbera exits with UPnP Error -117, or if it does not
respond to M-SEARCH requests from the renderer (i.e. Gerbera is running, but your renderer device does not show it)
you should try the following settings
(the lines below assume that Gerbera is running on a Linux machine, on network interface eth1):

::

    # route add -net 239.0.0.0 netmask 255.0.0.0 eth1
    # ifconfig eth1 allmulti

Those settings will be applied automatically by the init.d startup script.

You should also make sure that your firewall is not blocking port UDP port ``1900`` (required for SSDP) and UDP/TCP
port of Gerbera. By default Gerbera will select a free port starting with ``49152``, however you can specify a port
of your choice in the configuration file.

First Time Launch
~~~~~~~~~~~~~~~~~

First time startup of Gerbera creates a folder called ``~/.config/gerbera`` in your home directory.
Gerbera also generates a default server configuration file, called ``config.xml`` in the directory.

.. index:: Sqlite

Using Sqlite Database
~~~~~~~~~~~~~~~~~~~~~

If you are using sqlite - you are ready to go, the database file will be created automatically and will be
located ``~/.config/gerbera/gerbera.db`` If needed you can adjust the database file name and location in the
server configuration file.

.. index:: MySQL

Using MySQL Database
~~~~~~~~~~~~~~~~~~~~

If Gerbera was compiled with support for both databases, sqlite will be chosen as default because the initial database
can be created and used without any user interaction. If Gerbera was compiled only with MySQL support,
the appropriate config.xml file will be created in the ``~/.config/gerbera`` directory, but the server will
then terminate, because user interaction is required.

Gerbera has to be able to connect to the MySQL server and at least the (empty) database has to exist.
To create the database and provide Gerbera with the ability to connect to the MySQL server you need to have
the appropriate permissions. Note that user names and passwords in MySQL have nothing to do with UNIX accounts,
MySQL has it's own user names/passwords. Connect to the MySQL database as ”root” or any other user with the
appropriate permissions:

::

    $ mysql [-u <username>] [-p]

(You'll probably need to use ”-u” to specify a different MySQL user and ”-p” to specify a password.)

Create a new database for Gerbera: (substitute ”<database name>” with the name of the database)

::

    mysql> CREATE DATABASE <database name>;

(You can also use ”mysqladmin” instead.)

Give Gerbera the permissions to access the database:

::

    mysql> GRANT ALL ON <database name>.*
           TO '<user name>'@'<hostname>'
           IDENTIFIED BY '<password>';

If you don't want to set a password, omit ``IDENTIFIED BY`` completely. You could also use the MySQL ”root” user
with Gerbera directly, but this is not recommended.

To create a database and a user named **gerbera** (who is only able to connect via ``localhost``) without a
password (the defaults) use:

::

    mysql> CREATE DATABASE gerbera;
    mysql> GRANT ALL ON gerbera.* TO 'gerbera'@'localhost';

If Gerbera was compiled with database auto creation the tables will be created automatically during the first startup.
All table names have a ``mt_`` prefix, so you can theoretically share the database with a different application.
However, this is not recommended.

If database auto creation was not compiled in you have to create the tables manually:

::

    $ mysql [-u <username>] [-p] \
      <database name> < \
      <install prefix>/share/gerbera/mysql.sql

After creating the database and making the appropriate changes in your Gerbera config file you are ready to go -
launch the server, and everything should work.

Command Line Options
~~~~~~~~~~~~~~~~~~~~

There is a number of options that can be passed via command line upon server start up, for a short summary you can
invoke Gerbera with the following parameter:

::

    $ gerbera --help

Note:
    the command line options override settings in the configuration file!

IP Address
----------

::

    --ip or -i

The server will bind to the given IP address, currently we can not bind to multiple interfaces so binding to ``0.0.0.0``
will not be possible.

Interface
---------

::

    --interface or -e

Interface to bind to, for example eth0, this can be specified instead of the ip address.

Port
----

::

    --port or -p

Specify the server port that will be used for the web user interface, for serving media and for UPnP requests,
minimum allowed value is ``49152``. If this option is omitted a default port will be chosen, however, in
this case it is possible that the port will change upon server restart.

Configuration File
------------------

::

     --config or -c

By default Gerbera will search for a file named **config.xml** in the ``~/.config/gerbera`` directory.
This option allows you to specify a config file by the name and location of your choice.
The file name must be absolute.

Daemon Mode
-----------

::

    --daemon or -d

Run the server in background, Gerbera will shutdown on SIGTERM, SIGINT and restart on SIGHUP.

Home Directory
--------------

::

    --home or -m

Specify an alternative home directory. By default Gerbera will try to retrieve the users home directory from the
environment, then it will look for a ``.config/gerbera`` directory in users home. If ``.config/gerbera`` was found the system tries to
find the default configuration file (config.xml), if not found the system creates both, the ``.config/gerbera`` directory and the default config file.

This option is useful in two cases: when the home directory can not be retrieved from the environment (in this case
you could also use -c to point Gerbera to your configuration file or when you want to create a new configuration
in a non standard location (for example, when setting up daemon mode). In the latter case you can combine this parameter
with the parameter described in ?

Config Directory
----------------

::

    --cfgdir or -f

The default configuration directory is combined out of the users home and the default that equals to ``.config/gerbera``,
this option allows you to override the default directory naming. This is useful when you want to setup the server in a
nonstandard location, but want that the default configuration to be written by the server.

Write PID File
--------------

::

    --pidfile or -P

Specify a file that will hold the server process ID, the filename must be absolute.

Run Under Different User Name
-----------------------------

::

    --user or -u

Run Gerbera under the specified user name, this is especially useful in combination with the daemon mode.

Run Under Different Group
-------------------------

::

    --group or -g

Run Gerbera under the specified group, this is especially useful in combination with the daemon mode.

Add Content
-----------

::

    --add or -a

Add the specified directory or file name to the database without UI interaction. The path must be absolute, if
path is a directory then it will be added recursively. If path is a file, then only the given file will be imported.

Log To File
-----------

::

    --logfile or -l

Do not output log messages to stdout, but redirect everything to a specified file.

Debug Output
------------

::

    --debug or -D

Enable debug log output.

Compile Info
------------

::

    --compile-info

Print the configuration summary (used libraries and enabled features) and exit.

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
