.. index:: Configuration Overview

Configuration Overview
======================

Gerbera is highly configurable and allows the user to set various options and preferences that define the server's
behavior. Rather than enforcing certain features upon the user, we prefer to offer a number of choices where possible.
The heart of Gerbera configuration is the config.xml file, which is located in the ``~/.config/gerbera`` directory.
If the configuration file is not found in the default location and no configuration was specified on the command line,
Gerbera will generate a default config.xml file in the ``~/.config/gerbera`` directory. The file is in the XML format and can
be edited by a simple text editor, here is the list of all available options:

-  **Required** means that the server will not start if the tag is missing in the configuration.

-  **Optional**  means that the tag can be left out of the configuration file.

The root tag of Gerbera configuration is:

::

    <?xml version="1.0" encoding="UTF-8"?>
    <config version="2"
        xmlns="http://mediatomb.cc/config/2"
        xmlns:xsi="http://www.w3.org/2001/XMLSchema-instance"
        xsi:schemaLocation="http://mediatomb.cc/config/2 http://mediatomb.cc/config/2.xsd">
        ...
    </config>

The server configuration file has several options.  Below are links to the configuration sections


* :doc:`Server </config-server>`
* :doc:`Extended Runtime Options </config-extended>`
* :doc:`Import Content </config-import>`
* :doc:`Online Content </config-online>`
* :doc:`Transcode Content </config-transcode>`

.. _generateConfig:

Generating Configuration
~~~~~~~~~~~~~~~~~~~~~~~~

The gerbera runtime requires a configuration file to launch the application. Gerbera provides a command line utility
``--create-config`` to generate a standard ``config.xml`` file with defaults.  You will need to generate
the configuration file when running Gerbera for the first time.  Gerbera reports the missing configuration upon startup
similar to the message below

::

  The server configuration file could not be found in ~/.config/gerbera
  Gerbera could not find a default configuration file.
  Try specifying an alternative configuration file on the command line.
  For a list of options run: gerbera -h

* Run command to create configuration

::

  $ gerbera --create-config

This command outputs the ``config.xml`` to the standard output.

* Run command to create configuration, storing in the ``/etc/gerbera`` directory.

::

  $ gerbera --create-config | sudo tee /etc/gerbera/config.xml

You can start Gerbera with similar command as below:

::

  $ gerbera -c /etc/gerbera/config.xml


**NOTE**

* You might need to create the directory gerbera inside the `~/.config/` folder and change the owner to gerbera
    - `mkdir ~/.config/gerbera`
    - `sudo chown gerbera:gerbera gerbera`
* Gerbera sets the ``<home>`` to the runtime user's home by default.  Be sure to update accordingly.
* Ensure the **gerbera** user has proper permissions to the ``config.xml`` file.
