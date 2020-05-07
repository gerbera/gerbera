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
    
    This allows to select clients by IP address.

    ::
    
        userAgent=...

    * Optional
    * Default: empty
    
    This allows to filter clients by userAgent signature. It contains a part of the UserAgent http-signature of your client. 
    Run a network sniffer like wireshark or some UPnP utility to discover the signature. 
    If ``ip`` is set ``userAgent``is ignored.

    ::
    
        flags=...

    * Optional
    * Default: 0
    
    Integer containing the flags you want to set.
    For valid flags types see :doc:`Supported Devices </supported-devices>`.
