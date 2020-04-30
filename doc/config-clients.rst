.. index:: Client Configuration

Configure Client Quirks
=======================

These settings define the additions to the automatic client type detection.

``clients``
~~~~~~~~~~

.. code-block:: xml

    <clients> ... </clients>

* Optional

This section defines the client behaviour additions.

**Attributes:**

    ::

        enabled=...

    * Optional
    * Default: **no**

    This attribute defines if client overriding is enabled as a whole, possible values are ”yes” or ”no”.

**Child tags:**

``client``
~~~~~~~~~~

.. code-block:: xml

    <client> ... </client>

* Optional

This section defines the client behaviour for one client.

**Attributes:**

    ::

        ip=...
    
    * Optional
    * Default: empty
    
    This allows to filter clients by IP address.

    ::
    
        userAgent=...

    * Optional
    * Default: empty
    
    This allows to filter clients by userAgent signature. It contains a part of the userAgent signature of your client. 
    Run a network sniffer like wireshark or some UPnP utility to discover the signature. 
    ``userAgent`` and ``ip`` can be used to assign different rule to differnt software on the same host.

    ::
    
        clientType=...

    * Optional
    * Default: empty
    
    Assign a predefined client type to your client. For valid client types see :doc:`Supported Devices </supported-devices>`.

    ::
    
        flags=...

    * Optional
    * Default: 0
    
    Integer containing the flags you want to set. If ``clientType`` is set, the flags are enabled in addition to settings for the client type.
    For valid client types see :doc:`Supported Devices </supported-devices>`.
