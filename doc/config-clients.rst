.. index:: Client Configuration

Configure Client Quirks
=======================

These settings define the additions to the automatic client type detection.

``clients``
~~~~~~~~~~~

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


    ::

        cache-threshold=...

    * Optional
    * Default: **6**

    This attribute sets the amount of hours a client entry is kept in the cache.

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
    If ``ip`` is set ``userAgent`` is ignored.

    ::

        group=...

    * Optional
    * Default: "default"

    This assigns the client to a group which is key to store details on played items (playbackCount, lastPlaybackTime, lastPlaybackPosition, bookmarkPosition).
    If you set another group here all actions are recorded for this group.

    ::
    
        flags=...

    * Optional
    * Default: 0
    
    Containing the flags you want to set. Must be given in the following format ``SAMSUNG|0x100``, where the text either contains 
    one of the known flags or an integer number if the flags has no name.
    For valid flags see :doc:`Supported Devices </supported-devices>`.

    ::

        caption-info-count="0"

    * Optional

    * Default: set by option server/upnp/caption-info-count

    Number of ``sec::CaptionInfoEx`` entries to write to UPnP result.

    ::

        upnp-string-limit="80"

    * Optional

    * Default: -1

    Override the default ``upnp-string-limit`` of server.

    ::

        multi-value="no"

    * Optional

    * Default: the same as the current value of ``server/upnp/multi-value`` (defaults to **yes**)

    Override the default ``server/upnp/multi-value`` of server.
