.. index:: Configure Client

#######################
Configure Client Quirks
#######################

.. contents::
   :backlinks: entry
.. sectnum::
   :start: 3

These settings define the additions to the automatic client type detection.

.. _clients:

*******
Clients
*******

.. confval:: clients
   :type: :confval:`Section`
   :required: false
..

   .. code-block:: xml

      <clients enabled="yes"> ... </clients>

This section defines the client behaviour additions. It has to be explicitly enabled.

Clients Attributes
==================

.. confval:: clients enabled
   :type: :confval:`Boolean`
   :required: false
   :default: ``no``
..

   .. code:: xml

        enabled="yes"

This attribute defines if client overriding is enabled as a whole.

.. confval:: cache-threshold
   :type: :confval:`Time`
   :required: false
   :default: ``6``
..

    .. code:: xml

        cache-threshold="24"

This attribute sets the amount of hours a client entry is kept in the cache after its last contact.

.. confval:: bookmark-offset
   :type: :confval:`Time`
   :required: false
   :default: ``10``
..

    .. code:: xml

        bookmark-offset="8"

This attribute sets the amount of seconds a playposition (Samsung bookmark) is reduced on resume to continue a bit before the last scene.

Client Details
==============

.. confval:: client
   :type: :confval:`Section`
   :required: false
..

   .. code-block:: xml

      <client>...</client>

This section defines the client behaviour for one client.

Client Attributes
-----------------

.. confval:: client ip
   :type: :confval:`String`
   :required: false
   :default: `empty`
..

   .. code:: xml
  
       ip="10.10.10.10"

This allows to select clients by IP address. Allowed values are ip addresses (v4 or v6) which can be followed by ``/pref`` where pref is any allowed prefix length for the protocol.

.. confval:: client userAgent
   :type: :confval:`String`
   :required: false
   :default: `empty`
..

   .. code:: xml
  
       userAgent="DLNADOC/1.50"

This allows to filter clients by ``userAgent`` signature. It contains a part of the UserAgent http-signature of your client.
It can be found on the clients page of the web ui.
In some cases it can help to run a network sniffer like wireshark or some UPnP utility to discover the signature.
Most clients do not report exactly the same User-Agent for UPnP services and file request.
If :confval:`client ip` is set :confval:`client userAgent` is ignored.

.. confval:: client friendlyName
   :type: :confval:`String`
   :required: false
   :default: `empty`
..
.. versionadded:: 2.3.0

.. confval:: client modelName
   :type: :confval:`String`
   :required: false
   :default: `empty`
..
.. versionadded:: 2.3.0

.. confval:: client manufacturer
   :type: :confval:`String`
   :required: false
   :default: `empty`
..
.. versionadded:: 2.3.0
..

   .. code:: xml

       friendlyName="..." modelName="..." manufacturer="..."

This allows to filter clients by their UPnP description. It contains a property which can be found in the device section of the description xml.
UPnP network tools will provide you with the link to the xml document.
It is only used if :confval:`client ip` and :confval:`client userAgent` are not set. ``friendlyName`` overwrites ``modelName`` which overwrites ``manufacturer``.

.. confval:: client group
   :type: :confval:`String`
   :required: false
   :default: ``default``
..

   .. code:: xml
  
       group="wombat"

This assigns the client to a group which is key to store details on played items (playbackCount, lastPlaybackTime, lastPlaybackPosition, bookmarkPosition).
If you set another group here all actions are recorded for this group.

.. confval:: flags
   :type: :confval:`String`
   :required: false
   :default: ``0``
..

   .. code:: xml

       flags="SAMSUNG|0x100"

Containing the flags you want to set. Must be given in the following format ``SAMSUNG|0x100``, where the text either contains 
one of the known flags or an integer number if the flags has no name.
For valid flags see :doc:`Supported Devices </supported-devices>`.

.. confval:: client caption-info-count
   :type: :confval:`Integer`
   :required: false
   :default: :confval:`caption-info-count`
..

   .. code:: xml

       caption-info-count="0"

Number of ``sec::CaptionInfoEx`` entries to write to UPnP result.

.. confval:: client upnp-string-limit
   :type: :confval:`Integer`
   :required: false
   :default: :confval:`upnp-string-limit`
..

   .. code:: xml

       upnp-string-limit="80"

Override the default :confval:`upnp-string-limit` of server.

.. confval:: client multi-value
   :type: :confval:`Boolean`
   :required: false
   :default: :confval:`multi-value`
..

   .. code:: xml

       multi-value="no"

Override the default :confval:`multi-value` of server.

.. confval:: full-filter
   :type: :confval:`Boolean`
   :required: false
   :default: ``no``
..

   .. versionadded:: 2.4.0
   .. code:: xml

       full-filter="yes"

Enable the full UPnP filter support for this client. This means that all requested
elements from the filter request property will be created in the response.
Make sure that the namespaces are added with the the upnp section :ref:`upnp`

.. confval:: client allowed
   :type: :confval:`Boolean`
   :required: false
   :default: ``yes``
..

   .. versionadded:: 2.3.0
   .. code:: xml

       allowed="no"

If set to ``no`` all requests from a client are blocked.


Child Entries
-------------

Mimetype Mapping
^^^^^^^^^^^^^^^^

.. confval:: client map
   :type: :confval:`Section`
   :required: false
..

   .. code:: xml

       <map from="application/x-srt" to="text/srt"/>

Map mimetype for client. Some clients require slightly different mimetype, e.g. for subtitles.

   .. confval:: client map from
      :type: :confval:`String`
      :required: true
   ..

      .. code:: xml

         from="application/x-srt"

   Set source mimetype.

   .. confval:: client map to
      :type: :confval:`String`
      :required: true
   ..

      .. code:: xml

         to="text/srt"

   Set target mimetype.

Header
^^^^^^

.. confval:: client header
   :type: :confval:`Section`
   :required: false
..

   .. versionadded:: 2.1.0
   .. code:: xml

       <header key="X-User-Agent" value="redsonic"/>

   Add or overwrite header value sent by responses for UPnP single files and Web Page content

   .. confval:: client header key
      :type: :confval:`String`
      :required: true
   ..

      .. code:: xml

         key="X-User-Agent"

   Set header key.

   .. confval:: client header value
      :type: :confval:`String`
      :required: true
   ..

      .. code:: xml

         value="redsonic"

   Set header value.

DLNA Profiles
^^^^^^^^^^^^^

.. confval:: client dlna
   :type: :confval:`Section`
   :required: false
..

   .. code:: xml

       <dlna from="mp4" videoCodec="h264" audioCodec="aac" to="AVC_MP4_MP_HD_720p_AAC"/>

Map DLNA profile for client. Some clients do not support basic dlna profiles.
It overwrites general settings from :confval:`contenttype-dlnaprofile` with the same format, see :ref:`contenttype-dlnaprofile`.

   .. confval:: client dlna from
      :type: :confval:`String`
      :required: true
   ..

      .. code:: xml

         from="mp4"

   Set source content type.

   .. confval:: client dlna to
      :type: :confval:`String`
      :required: true
   ..

      .. code:: xml

         to="AVC_MP4_MP_HD_720p_AAC"

   Set target DLNA profile name.

Client Group
============

.. confval:: group
   :type: :confval:`Section`
   :required: false
..

   .. versionadded:: 2.4.0
   .. code-block:: xml

      <group> ... </group>

This section defines the behaviour for a group of clients.

Group Attributes
----------------

.. confval:: group name
   :type: :confval:`String`
   :required: true
..

   .. code:: xml

       name="wombat"

Name of the group. Should correspond to one of the group names in client settings or ``default``

.. confval:: group allowed
   :type: :confval:`Boolean`
   :required: false
   :default: ``yes``
..

   .. versionadded:: HEAD
   .. code:: xml

       allowed="no"

If set to ``no`` all requests from a client assigned to this group are blocked.
If group ``default`` is not allowed, each allowed client must be configured or assigned an allowed group.

Group Details
-------------

.. confval:: hide
   :type: :confval:`Section`
   :required: false
..

   .. code:: xml

       <hide location="/path/not/visible"/>

   .. confval:: hide location
      :type: :confval:`Path`
      :required: true
   ..

    Define a location of files that have to be hidden from the output for the group.
