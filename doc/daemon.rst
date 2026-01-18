.. _daemon:
.. index:: Daemon

Gerbera Daemon
==============

You can setup Gerbera to run as a **daemon** background system process

.. index:: Systemd

Using systemd
~~~~~~~~~~~~~

This outlines steps of how to add the Gerbera runtime
as a system daemon using the **Systemd**.  The gerbera installation uses cmake to configure and install the
systemd daemon for gerbera.  Default install path is depending on the target distribution. Typical locations are
``/etc/systemd/system/gerbera.service``, ``/lib/systemd/system/gerbera.service`` or ``/usr/lib/systemd/system/gerbera.service``


Create a Gerbera System User
----------------------------

You should run Gerbera as a separate user to avoid vulnerabilities in
exposing root access.

Here is a way to create a system user in linux command line

.. code-block:: console

  $ sudo useradd --system gerbera

Verify that the ``gerbera`` user was created

.. code-block:: console

  $ id -u gerbera
  | Returns the user id of the user


Set Gerbera Permissions
-----------------------

The **gerbera** user must have access to ``config.xml`` file and
to the directory referenced as the gerbera home.  For example ``/etc/gerbera``

.. code-block:: console

  $ sudo mkdir /etc/gerbera
  $ sudo chown -Rv gerbera:gerbera /etc/gerbera


Enable Systemd Daemon
---------------------

The cmake installation adds ``gerbera.service`` service file by default.

Note:
  The installation does **not** enable the service daemon.

1. Notify ``systemd`` that a new gerbera.service file exists by executing the following command:

    .. code-block:: console

        $ sudo systemctl daemon-reload

2. Start up the daemon

    .. code-block:: console

      $ sudo systemctl start gerbera


Success
-------

Check the status of gerbera.  You should see success similar to below

.. code-block:: console

  $ sudo systemctl status gerbera

  ● gerbera.service - Gerbera Media Server
     Loaded: loaded (/etc/systemd/system/gerbera.service; disabled; vendor preset: disabled)
     Active: active (running) since Wed 2026-01-01 19:48:44 EDT; 47s ago
   Main PID: 4818 (gerbera)
      Tasks: 12 (limit: 4915)
     CGroup: /system.slice/gerbera.service
             └─4818 /usr/local/bin/gerbera -c /etc/gerbera/config.xml


Troubleshooting
---------------

If for some reason the service fails to start.  You can troubleshoot the behaviour
by starting gerbera from the shell.

.. code-block:: console

  $ su gerbera
  Password:
  $  /usr/local/bin/gerbera -c /etc/gerbera/config.xml

  2026-01-01 19:54:47    INFO: Gerbera UPnP Server version 3.1.1 - https://gerbera.io/
  2026-01-01 19:54:47    INFO: ===============================================================================
  2026-01-01 19:54:47    INFO: Gerbera is free software, covered by the GNU General Public License version 2
  2026-01-01 19:54:47    INFO: Copyright 2016-2026 Gerbera Contributors.
  2026-01-01 19:54:47    INFO: Gerbera is based on MediaTomb: Copyright 2005-2010 Gena Batsyan, Sergey Bostandzhyan, Leonhard Wimmer.
  2026-01-01 19:54:47    INFO: ===============================================================================
  2026-01-01 19:54:47    INFO: Loading configuration from: /etc/gerbera/config.xml
  2026-01-01 19:54:47    INFO: Checking configuration...

.. index:: Commandline options

Using Commandline options
~~~~~~~~~~~~~~~~~~~~~~~~~

If your system uses an old style system V init, commandline options are available to start gerbera as a Daemon:

``--daemon`` or ``-d``:  daemonize after startup.

``--user`` or ``-u``:    when started by root, try to change to user USER after startup. All UIDs, GIDs and supplementary Groups will be set.

``--pidfile`` or ``-P``: write a pidfile to the specified location. Full path is needed, e.g. /run/gerbera.pid.


.. index:: Solaris

Using solaris / OmniOS
~~~~~~~~~~~~~~~~~~~~~~


You can use the solaris script provided in ``scripts/sunos`` to add Gerbera as a service in Solaris / OmniOS.


.. index:: LaunchD

Using launchd
~~~~~~~~~~~~~


**launchd** is the daemon engine in macOS.


Create new Launch Agent
-----------------------

Use the ``scripts/gerbera.io.plist`` as a starting point.

Save to user's launch agent path --> ``~/Library/LaunchAgents/gerbera.io.plist``


Load the Launch Agent
---------------------

.. code-block:: console

  $ launchctl load ~/Library/LaunchAgents/gerbera.io.plist


Start the Launch Agent
----------------------

.. code-block:: console

  $ launchctl start gerbera.io


Stop the Launch Agent
---------------------

.. code-block:: console

  $ launchctl stop gerbera.io
