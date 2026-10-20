.. _supported-types:
.. index:: Supported File Types

Supported File Types
====================

Generally file type support depends on the linked libraries. Please check documentation for taglib, ffmpeg, libexif, libexiv2, libmatroska for the whole list and the restrictions.
Gerbera depends on these libraries to extract the metadata.

In case you have special files you want to treat in a particular way you can map the extension in config section ``<extension-mimetype>``:

.. code-block:: xml

    <map from="avi" to="video/avi"/>

and then tweak its content type in ``<mimetype-contenttype>`` section:

.. code-block:: xml

    <treat mimetype="video/avi" as="avi"/>

It may also be necessary to set a DLNA profile in ``<contenttype-dlnaprofile>``

.. code-block:: xml

    <map from="avi" to="AVI"/>

See :ref:`Import section <import-mappings>` for the detailled description of these options.


Audio/Video/Image Files
-----------------------

The following properties are automatically extracted to metadata:

.. _supported-properties:

    +---------------+-----------------+
    | Property      | Key             |
    +===============+=================+
    | title         | M_TITLE         |
    +---------------+-----------------+
    | artist        | M_ARTIST        |
    +---------------+-----------------+
    | album_artist  | M_ALBUMARTIST   |
    +---------------+-----------------+
    | album         | M_ALBUM         |
    +---------------+-----------------+
    | date          | M_DATE          |
    | date          | M_UPNP_DATE     |
    +---------------+-----------------+
    | creation_time | M_CREATION_DATE |
    +---------------+-----------------+
    | genre         | M_GENRE         |
    +---------------+-----------------+
    | description   | M_DESCRIPTION   |
    +---------------+-----------------+
    | track         | M_TRACKNUMBER   |
    +---------------+-----------------+
    | discnumber    | M_PARTNUMBER    |
    +---------------+-----------------+
    | composer      | M_COMPOSER      |
    +---------------+-----------------+
    | CONDUCTOR     | M_CONDUCTOR     |
    +---------------+-----------------+
    | ORCHESTRA     | M_ORCHESTRA     |
    +---------------+-----------------+

The following metadata properties can be set in import script extensions: M_CREATOR, M_AUTHOR, M_ACTOR, M_PUBLISHER, M_REGION

Resource data is extracted as well and largely depends on the media type: R_DURATION, R_SIZE, R_BITRATE, R_BITS_PER_SAMPLE, R_VIDEOCODEC, R_AUDIOCODEC, R_RESOLUTION, R_SAMPLEFREQUENCY, R_NRAUDIOCHANNELS.


Online Content
--------------

The following properties are automatically extracted to metadata:

    +---------------+-----------------+---------------------+
    | Property      | Key             | Comment             |
    +===============+=================+=====================+
    | director      | M_DIRECTOR      |                     |
    +---------------+-----------------+---------------------+
    | rating        | M_RATING        |                     |
    +---------------+-----------------+---------------------+
    | studio        | M_PRODUCER      |                     |
    +---------------+-----------------+---------------------+


Playlist Files
--------------

Playlist can be used to get tracks in special sequence or to link external content like web radio stations or videos.

m3u
~~~

There are two flavours:

- Simple layout just lists the files (as absolute or relative path).

- Extended layout (first line in file is ``#EXTM3U``) adds meta-info in lines starting with ``#EXTINF``. The standard requires ``#EXTINF:<duration>,<title>``.

- There is a Gerbera specific extension that allows  ``#EXTINF:<duration>,<title>,<mimetype>``


pls
~~~

PLS contains lines like ``Title1=``, ``File1=`` etc.

There is a Gerbera specific extension that allows ``MimeType1=``

asx
~~~

ASX files are XML files. Only version 3 is supported. Gerbera accepts param entries ``mimetype`` and ``protocol`` and copies them to the respective properties in the playlist item.

Local references first retain an existing native filename, including literal
``?``, ``#``, ``%`` and ``+`` characters. If that native file does not exist,
Gerbera resolves the reference as a URI relative to the ASX file. Percent escapes
are decoded exactly once, and ``+`` is not converted to a space. An existing
literal ``Why%3F.wav`` therefore retains priority over ``Why?.wav`` for the
ambiguous reference ``Why%3F.wav``; use a local file URI to request URI semantics
explicitly.

``file:`` references with an empty or ``localhost`` authority use local media
metadata and Gerbera HTTP resources. Remote file authorities, malformed escapes,
embedded NUL bytes and invalid decoded UTF-8 are rejected. Local file URIs must
not contain a query or fragment. Existing external streaming URLs, including
their encoded query strings, are retained without fetching them during import.
XML-parser output and local media metadata remain UTF-8, independently of the
``playlist-charset`` configured for line-based playlists. The existing root
``BASE`` prefix and Gerbera ``PARAM``/``writethrough`` extensions are retained.

.. code-block:: xml

    <asx version="3.0">
      <title>Test-List</title>
      <entry>
        <title>Track</title>
        <author>Artist</author>
        <ref href="http://85.14.216.232:9000"/>
        <param name="mimetype" value="video/mp4"/>
        <param name="protocol" value="http-get" />
      </entry>
    </asx>
