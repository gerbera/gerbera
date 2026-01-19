.. index:: Configuration Basics

Basics
======

.. _extend:

Option Details
--------------

-  Required: ``True`` means that the server will not start if the tag is missing in the configuration.

-  Required: ``False``  means that the tag can be left out of the configuration file.

-  Default: contains the value or values if the section, entry or attribute is omitted. Sections with complex default values are completely overwriitwn by config file content.

-  Default: **Extensible Default**  means that the additional attribute ``extend="true"`` can be used to keep the list of default values and the config entries are added. The default values can be found in the output of ``gerbera --create-example-config``.

.. confval:: extend
   :type: :confval:`Boolean`
   :required: false
   :default: ``no``
.. versionadded:: 2.4.0

Data Types
----------

String
~~~~~~

.. confval:: String

Any characters and numbers

Path
~~~~

.. confval:: Path

Directory on the file system.

- Absolute paths that start with ``/`` have to contain the full directory hierarchy.
- Relative paths (without leading ``/``) must exist under the parent folder (in most cases the gerbera home directory).

Integer
~~~~~~~

.. confval:: Integer

Numbers without ``.``

Time
~~~~

.. confval:: Time

Interval of time which can be given in seconds/minutes (depending on property) or a time string like ``7:42``.

Enum
~~~~

.. confval:: Enum

List of given values.

Boolean
~~~~~~~

.. confval:: Boolean

- ``true``, ``yes``, ``1`` mean that the option is set
- ``false``, ``no``, ``0`` mean that the option is not set

Section
~~~~~~~

.. confval:: Section

Full xml sub structure.

.. confval:: from-file
   :type: :confval:`Path`
   :required: false
   :default: unset

   .. versionadded:: HEAD
   .. code-block:: xml

       <autoscan from-file="autoscan.xml"/>

Load section from file instead from content. If the file exists it overwrites
the full section.


Content
-------

The root tag of Gerbera configuration is:

.. code-block:: xml

    <?xml version="1.0" encoding="UTF-8"?>
    <config version="2"
        xmlns="http://gerbera.io/config/2"
        xmlns:xsi="http://www.w3.org/2001/XMLSchema-instance"
        xsi:schemaLocation="http://gerbera.io/config/2 http://gerbera.io/config/2.xsd">
        <server>...</server>
        <import>...</import>
        <clients>...</clients>
        <transcoding>...</transcoding>
    </config>
