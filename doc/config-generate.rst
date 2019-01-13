.. _generateConfig:

Generating Configuration
~~~~~~~~~~~~~~~~~~~~~~~~

A Gerbera instance requires a configuration file to launch.

Gerbera provides a command line flag ``--create-config`` to generate a default ``config.xml`` file. You will need to generate
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
