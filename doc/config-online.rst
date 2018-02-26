.. index:: Online Content

Configure Online Content
========================


This section resides under import and defines options for various supported online services.

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
    * Default: **262144**

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

    Below are the settings for supported services.