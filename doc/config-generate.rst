.. _generateConfig:

Generating Configuration
~~~~~~~~~~~~~~~~~~~~~~~~

A Gerbera instance requires a configuration file to launch.

Gerbera provides a command line flag ``--create-config`` to generate a default ``config.xml`` file. You will need to generate
the configuration file when running Gerbera for the first time.  Gerbera reports the missing configuration upon startup
similar to the message below

.. code-block:: console

  The server configuration file could not be found in ~/.config/gerbera
  Gerbera could not find a default configuration file.
  Try specifying an alternative configuration file on the command line.
  For a list of options run: gerbera -h

* Run command to create configuration

.. code-block:: console

  $ gerbera --create-config

This command outputs the ``config.xml`` to the standard output with all mandatory settings.
After upgrading you should run this as well to check for any new or changed options.

You can also ``gerbera --create-example-config`` to get almost all options written to the output. You have to validate all generated settings and
adjust them to your preferences. There are several empty options you have to fill in or remove before your configuration will work.

You can also run ``gerbera --create-config=sections`` to export only a relevant section. The output can be used to compare with the existing settings
but not as a full configuration. The following sections are supported: ``Server``, ``Ui``, ``ExtendedRuntime``, ``DynamicContainer``, ``Database``, ``Import``,
``Mappings``, ``Boxlayout``, ``Transcoding``. The pseudo section ``All`` is equivalent to stating no section at all.

You can use ``gerbera --check-config -c <file>`` to verify your settings before starting the service.

* Run command to create configuration, storing in the ``/etc/gerbera`` directory.

.. code-block:: console

  $ gerbera --create-config | sudo tee /etc/gerbera/config.xml

You can start Gerbera with similar command as below:

.. code-block:: console

  $ gerbera -c /etc/gerbera/config.xml


**NOTES**

* You might need to create the directory gerbera inside the ``~/.config/`` folder and change the owner to gerbera
    - ``mkdir ~/.config/gerbera``
    - ``sudo chown gerbera:gerbera gerbera``
* Gerbera sets the ``<home>`` to the runtime user's home by default.  Be sure to update accordingly.
* Ensure the **gerbera** user has proper permissions to the ``config.xml`` file.
