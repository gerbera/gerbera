.. index:: Online Content

Configure Online Content
========================


This section resides under import and defines options for various supported online services.

Example of online content XML configuration

::

    <import>
        <online-content>
            <AppleTrailers enabled="[yes,no]" refresh="[seconds]"
                purge-after="[seconds]"
                update-at-start="[yes,no]"
                resolution="[640,720]"/>
        </online-content>
    </import>


``online-content``
~~~~~~~~~~~~~~~~~~

.. code-block:: xml

    <online-content fetch-buffer-size="262144" fetch-buffer-fill-size="0">

* Optional

This tag defines the online content section.

    **Attributes:**

    .. code-block:: xml

        fetch-buffer-size=...

    * Optional
    * Default: **1048576**

    Often, online content can be directly accessed by the player - we will just give it the URL. However, sometimes it
    may be necessary to proxy the content through Gerbera. This setting defines the buffer size in bytes, that will be
    used when fetching content from the web. The value must not be less than allowed by the curl library (usually 16384 bytes).

    .. code-block:: xml

        fetch-buffer-fill-size=...

    * Optional
    * Default: **0 (disabled)**

    This setting allows to prebuffer a certain amount of data, given in bytes, before sending it to the player, this
    should ensure a constant data flow in case of slow connections. Usually this setting is not needed, because most
    players will anyway have some kind of buffering, however if the connection is particularly slow you may want to try enable this setting.


``AppleTrailers``
~~~~~~~~~~~~~~~~~

.. code-block:: xml

    <AppleTrailers enabled="[yes,no]"
        refresh="[seconds]" update-at-start="[yes,no]" resolution="[640,720]"/>

* Optional

This tag defines the online content for `Apple Trailers <https://trailers.apple.com/>`_

    **Attributes**

    .. code-block:: xml

        enabled=...

    * Default: **no**

    .. code-block:: xml

        refresh=...

    * Default: **43200**

    The amount of time to wait before refreshing the online content

    .. code-block:: xml

        update-at-start=...

    * Default: **no**

    Upon starting Gerbera, the Apple Trailers content will be refreshed.

    .. code-block:: xml

        resolution=...

    * Default: **720**

    Sets the Apple Trailers URL to retrieve the content, affecting the resolution size that is downloaded.

``mimetype-mappings``
----------------------
::

    <mimetype-mappings>

* Optional

This section holds the file name extension to mime type mappings.

**Child tags:**

``map``
-------

::

    <map from="wmv" to="video/x-ms-wmv"/>
    <map from="mp3" to="audio/mpeg"/>
    <map from="wma" to="audio/x-ms-wma"/>
    <map from="*" to="application/octet-stream"/>

* Optional

Specifies a mapping from a certain file name extension (everything after the last dot ".") to mime type.
``*`` is used to set the default value.
