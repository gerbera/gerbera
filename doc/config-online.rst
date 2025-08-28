.. index:: Online Content

########################
Configure Online Content
########################

.. contents::
   :backlinks: entry
.. sectnum::
   :prefix: 2.
   :start: 11

This section resides under import and defines options for various supported online services.
Currently there is none supported.

Example of online content XML configuration

.. code-block:: xml

    <import>
        <online-content>
        </online-content>
    </import>


**************
Online Content
**************

.. confval:: online-content
   :type: :confval:`Section`
   :required: false
..

   .. code-block:: xml

      <online-content fetch-buffer-size="262144" fetch-buffer-fill-size="0">

This tag defines the online content section.

Attributes
==========

.. confval:: online fetch-buffer-size
   :type: :confval:`Integer`
   :required: false
   :default: ``1048576``
..

   .. code-block:: xml

      fetch-buffer-size="524288"

Often, online content can be directly accessed by the player - we will just give it the URL. However, sometimes it
may be necessary to proxy the content through Gerbera. This setting defines the buffer size in bytes, that will be
used when fetching content from the web. The value must not be less than allowed by the curl library (usually 16384 bytes).

.. confval:: online fetch-buffer-fill-size
   :type: :confval:`Integer`
   :required: false
   :default: ``0`` `(disabled)`
..

   .. code-block:: xml

      fetch-buffer-fill-size="262144"

This setting allows to prebuffer a certain amount of data, given in bytes, before sending it to the player, this
should ensure a constant data flow in case of slow connections. Usually this setting is not needed, because most
players will anyway have some kind of buffering, however if the connection is particularly slow you may want to try enable this setting.

