.. index:: Import

Configure Import
================


The import settings define various options on how to aggregate the content.

``import``
~~~~~~~~~~

::

    <import hidden-files="no" follow-symlinks="no">

* Optional

This tag defines the import section.

**Attributes:**

    ::

        hidden-files="yes|no"

    * Optional

    * Default: **no**

    This attribute defines if files starting with a dot will be imported into the database (”yes”). Autoscan can
    override this attribute on a per directory basis.

    ::

        follow-symlinks="yes|no"

    * Optional

    * Default: **yes**

    This attribute defines if symbolic links should be treated as regular items and imported into the database (”yes”). This can cause duplicate entries if the link target is also scanned.

**Child tags:**

``filesystem-charset``
~~~~~~~~~~~~~~~~~~~~~~

::

    <filesystem-charset>UTF-8</filesystem-charset>

* Optional
* Default: **if ``nl\_langinfo()`` function is present, this setting will be auto detected based on your system locale, else set to UTF-8**

Defines the charset of the filesystem. For example, if you have file names in Cyrillic KOI8-R encoding, then you
should specify that here. The server uses UTF-8 internally, this import parameter will help you to correctly import your data.


``metadata-charset``
~~~~~~~~~~~~~~~~~~~~

::

    <metadata-charset>UTF-8</metadata-charset>

* Optional
* Default: **if ``nl\_langinfo()`` function is present, this setting will be auto detected based on your system locale, else set to UTF-8**

Same as above, but defines the charset of the metadata (i.e. id3 tags, Exif information, etc.)

``scripting``
~~~~~~~~~~~~~

::

    <scripting script-charset="UTF-8">

* Optional

Defines the scripting section.

    ::

        script-charset=...

    * Optional
    * Default: **UTF-8**

Below are the available scripting options:

    ``virtual-layout``

    ::

        <virtual-layout type="builtin">

    * Optional

    Defines options for the virtual container layout; the so called ”virtual container layout” is the way how the
    server organizes the media according to the extracted metadata. For example, it allows sorting audio files by Album, Artist, Year and so on.

        ::

            type="builtin|js|disabled"

        * Optional
        * Default: **builtin**

        Specifies what will be used to create the virtual layout, possible values are:

        -  **builtin**: a default layout will be created by the server
        -  **js**: a user customizable javascript will be used (Gerbera must be compiled with js support)
        -  **disabled**: only PC-Directory structure will be created, i.e. no virtual layout

        The virtual layout can be adjusted using an import script which is defined as follows:

        ::

            <import-script>/path/to/my/import-script.js</import-script>

        * Required:  **if virtual layout type is ”js\ ”**
        * Default: ``${prefix}/share/gerbera/js/import.js``, **where ${prefix} is your installation prefix directory.**

        Points to the script invoked upon media import. For more details read about :ref:`scripting <scripting>`

``common-script``
~~~~~~~~~~~~~~~~~

::

    <common-script>/path/to/my/common-script.js</common-script>

* Optional
* Default: ``${prefix}/share/gerbera/js/common.js``, **where ${prefix} is your installation prefix directory.**

Points to the so called common script - think of it as a custom library of js helper functions, functions added
there can be used in your import and in your playlist scripts. For more details read :ref:`scripting <scripting>`

``playlist-script``
~~~~~~~~~~~~~~~~~~~

::

    <playlist-script create-link="yes">/path/to/my/playlist-script.js</playlist-script>

* Optional
* Default: ``${prefix}/share/gerbera/js/playlists.js``, **where ${prefix} is your installation prefix directory.**

Points to the script that is parsing various playlists, by default parsing of pls and m3u playlists is implemented,
however the script can be adapted to parse almost any kind of text based playlist. For more details read :ref:`scripting <scripting>`

    ::

        create-link="yes|no"

    * Optional
    * Default: **yes**

    Links the playlist to the virtual container which contains the expanded playlist items. This means, that
    if the actual playlist file is removed from the database, the virtual container corresponding to the playlist will also be removed.


``magic-file``
~~~~~~~~~~~~~~

::

    <magic-file>/path/to/my/magic-file</magic-file>

* Optional
* Default: **System default**

Specifies an alternative file for filemagic, containing mime type information.

``autoscan``
~~~~~~~~~~~~

::

    <autoscan use-inotify="auto">

* Optional

Specifies a list of default autoscan directories.

This section defines persistent autoscan directories. It is also possible to define autoscan directories in the UI,
the difference is that autoscan directories that are defined via the config file can not be removed in the UI.
Even if the directory gets removed on disk, the server will try to monitor the specified location and will re add
the removed directory if it becomes available/gets created again.

    ::

        use-inotify="yes|no|auto"

    * Optional
    * Default: **auto**

    Specifies if the inotify autoscan feature should be enabled. The default value is ``auto``, which means that
    availability of inotify support on the system will be detected automatically, it will then be used if available.
    Setting the option to 'no' will disable inotify even if it is available. Allowed values: "yes", "no", "auto"

    **Child tags:**

    ::

        <directory location="/media" mode="timed" interval="3600"
          recursive="no" hidden-files="no"/>
        <directory location="/audio" mode="inotify"
          recursive="yes" hidden-files="no"/>

    * Optional

    Defines an autoscan directory and it's parameters.

    The attributes specify various autoscan options:

        ::

            location=...

        * Required

        Absolute path to the directory that shall be monitored.

        ::

            mode="inotify|timed"

        * Required

        Scan mode, currently ``inotify`` and ``timed`` are supported. Timed mode rescans the given directory in specified
        intervals, inotify mode uses the kernel inotify mechanism to watch for filesystem events.

        ::

            interval=...

        * Required: **for ”timed” mode**

        Scan interval in seconds.

        ::

            recursive="yes|no"

        * Required

        Values of ``yes`` or ``no`` are allowed, specifies if autoscan shall monitor the given directory including all sub directories.

        ::

            hidden-files="yes|no"

        * Optional
        * Default: **value specified in <import hidden-files=””/>**

        Allowed values: ``yes`` or ``no``, process hidden files, overrides the hidden-files value in the ``<import/>`` tag.

``layout``
~~~~~~~~~~

::

    <layout>

* Optional

Defines various layout options for generated virtual layout.

        ::

            parent-path="yes|no"

        * Optional
        * Default: **no**

        Values of ``yes`` or ``no`` are allowed, specifies if parent path is added to virtual layout. If set to``no`` "/home/.../Videos/Action/a.mkv" with rootpath "/home/.../Videos" becomes "Action" otherwise "Videos/Action". Setting to ``yes`` produces the layout of gerbera before version 1.5.

    **Child tags:**

        ::

            <path from="Videos/Action" to="Action-Videos"/>

        * Optional

        Map a virtual path element. This allows reducing path elements or merging different sources into a common tree. Thema replacement is executed after calculation of virtual layout, i.e. after buildin or layout script.

            ::

                from="..."

            * Required

            Source path. Can be a regular expression.

            ::

                to="..."

            * Required

            Target path. / can be used to create sub structure.


``resources``
~~~~~~~~~~~~~

::

    <resources case-sensitive="yes">

* Optional

Defines various resource options for file based resources. Older versions of Gerbera added sereral files automatically. For performance reasons no pattern is added anymore. You can set up your correct fanart file by yourself, if no image is embedded in your media. If you have subtitle files, add the correct pattern, also.

    ::

        case-sensitive="yes|no"

    * Optional

    * Default: **yes**

    This attribute defines whether search patterns are treated case sensitive or not, i.e. if set to **no** ``cover.png`` matches anything like ``Cover.PNG`` or ``cover.PNG``.

**Child tags:**

    ::

        <fanart>...</fanart>
        <subtitle>...</subtitle>
        <resource>...</resource>

    * Optional

    Define file patterns to search for fanart, subtitle and resources respectivly.

    ``fanart`` and ``subtitle`` patterns are used to identify external resources which are added to each item if the pattern matches.

    ``resource`` patterns are used to trigger rescan of the whole directory if such a file was changed, added or removed.

    Each of these tags can contain the following

**Child tags:**

``add-file``
------------

    ::

        <add-file name="cover.png"/>
        <add-file name="%filename%.srt"/>

    * Optional

    Add search pattern to resource handler. The search pattern can contain variables:

    - ``%filename%``: Name of the file without extension
    - ``%album%``: Value of the album tag
    - ``%title%``: Value of the title tag

A sample configuration would be:

.. code-block:: xml

  <resources case-sensitive="no">
      <fanart>
          <add-file name="cover.png"/>
      </fanart>
      <subtitle>
          <add-file name="%filename%.srt"/>
      </subtitle>
      <resource>
          <add-file name="cover.png"/>
          <add-file name="%filename%.srt""/>
      </resource>
  </resources>


``mappings``
~~~~~~~~~~~~

::

    <mappings>

* Optional

Defines various mapping options for importing media, currently two subsections are supported.

This section defines mime type and upnp:class mappings, it is vital if filemagic is not available - in this case
media type auto detection will fail and you will have to set the mime types manually by matching the file extension.
It is also helpful if you want to override auto detected mime types or simply skip filemagic processing for known file types.


``extension-mimetype``
~~~~~~~~~~~~~~~~~~~~~~
::

    <extension-mimetype ignore-unknown="no" case-sensitive="no">

* Optional

This section holds the file name extension to mime type mappings.

    **Attributes:**

        ::

            ignore-unknown=...

        * Optional
        * Default: **no**

        If ignore-unknown is set to "yes", then only the extensions that are listed in this section are imported.

        ::

            case-sensitive=...

        * Optional
        * Default: **no**

        Specifies if extensions listed in this section are case sensitive, allowed values are "yes" or "no".

**Child tags:**

``map``
-------

::

    <map from="mp3" to="audio/mpeg"/>

* Optional

Specifies a mapping from a certain file name extension (everything after the last dot ".") to mime type.

Note:
    this improves the import speed, because invoking libmagic to discover the right mime type of a file is
    omitted for files with extensions listed here.

Note:
    extension is case sensitive, this will probably need to be fixed.


``mime-type-upnpclass``
~~~~~~~~~~~~~~~~~~~~~~~

::

    <mimetype-upnpclass>

* Optional

This section holds the mime type to upnp:class mappings.


**Child tags:**

``map``
-------

::

     <map from="audio/*" to="object.item.audioItem.musicTrack"/>

* Optional

Specifies a mapping from a certain mime type to upnp:class in the Content Directory. The mime type can either be
entered explicitly "audio/mpeg" or using a wildcard after the slash ``audio/\*``. The values of **from** and **to**
attributes are case sensitive.

``mimetype-contenttype``
------------------------

::

  <mimetype-contenttype>

* Optional

This section makes sure that the server knows about remapped mimetypes and still extracts the metadata correctly.
For example, we know that id3lib can only handle mp3 files, the default mimetype of mp3 content is audio/mpeg.
If the user remaps mp3 files to a different mimetype, we must know about it so we can still pass this item to id3lib
for metadata extraction.

Note:
  if this section is not present in your config file, the defaults will be filled in automatically.
  However, if you add an empty tag, without defining the following ``<treat>`` tags, the server assumes that
  you want to have an empty list and no files will be process by the metadata handler.


``treat``
---------

::

 <treat mimetype="audio/mpeg" as="mp3"/>

* Optional

Tells the server what content the specified mimetype actually is.

Note:
    it makes no sense to define 'as' values that are not below, the server only needs to know the content
    type of the ones specified, otherwise it does not matter.

The ``as`` attribute can have following values:

**Mapping Table**

+-----------------------------------+---------------+----------------------------------------+
| mimetype                          | as            | Note                                   |
+===================================+===============+========================================+
| | audio/mpeg                      | mp3           | | The content is an mp3 file and should|
|                                   |               | | be processed by either id3lib or     |
|                                   |               | | taglib (if available).               |
+-----------------------------------+---------------+----------------------------------------+
| | application/ogg                 | ogg           | | The content is an ogg file and should|
|                                   |               | | be processed by taglib               |
|                                   |               | | (if available).                      |
+-----------------------------------+---------------+----------------------------------------+
| | audio/x-flac                    | flac          | | The content is a flac file and should|
|                                   |               | | be processed by taglib               |
|                                   |               | | (if available).                      |
+-----------------------------------+---------------+----------------------------------------+
| | image/jpeg                      | jpg           | | The content is a jpeg image and      |
|                                   |               | | should be processed by libexif       |
|                                   |               | | (if available).                      |
+-----------------------------------+---------------+----------------------------------------+
| | audio/x-mpegurl                 | playlist      | | The content is a playlist and should |
| | or                              |               | | be processed by the playlist parser  |
| | audio/x-scpls                   |               | | script.                              |
+-----------------------------------+---------------+----------------------------------------+
| | audio/L16                       | pcm           | | The content is a PCM file.           |
| | or                              |               |                                        |
| | audio/x-wav                     |               |                                        |
+-----------------------------------+---------------+----------------------------------------+
| | video/x-msvideo                 | avi           | | The content is an AVI container,     |
|                                   |               | | FourCC extraction will be attempted. |
|                                   |               |                                        |
+-----------------------------------+---------------+----------------------------------------+


``library-options``
~~~~~~~~~~~~~~~~~~~
::

    <library-options>

* Optional

This section holds options for the various supported import libraries, it is useful in conjunction with virtual
container scripting, but also allows to tune some other features as well.

Currently the **library-options** allow additional extraction of the so called auxilary data (explained below) and
provide control over the video thumbnail generation.

Here is some information on the auxdata: UPnP defines certain tags to pass along metadata of the media
(like title, artist, year, etc.), however some media provides more metadata and exceeds the scope of UPnP.
This additional metadata can be used to fine tune the server layout, it allows the user to create a more
complex container structure using a customized import script. The metadata that can be extracted depends on the
library, currently we support **taglib** (or id3lib if absent), **ffmpeg and libexif** which provide a default set of keys 
that can be passed in the options below. The data according to those keys will the be extracted from the media and imported 
into the database along with the item. When processing the item, the import script will have full access to the gathered 
metadata, thus allowing the user to organize the data with the use of the extracted information. A practical example would be: 
having more than one digital camera in your family you could extract the camera model from the Exif tags and sort your photos
in a structure of your choice, like:

- Photos/MyCamera1/All Photos
- Photos/MyCamera1/Date
- Photos/MyCamera2/All Photos
- Photos/MyCamera2/Date

etc.

**Attributes:**

    ::

        multi-value-separator="..."

    * Optional
    * Default: **"; "**

    This string is used to join multi-valued items (e.g. Composer, Performer) into one string.

    ::

        legacy-value-separator="..."

    * Optional
    * Default: **empty**

    This string is used to split items into lists before joining them with multi-value-separator.
    This option can be used to import files from legacy tools which did not support multi-valued items.
    The empty string is used to disable legacy handling.

**Child tags:**

``libexif``
-----------

.. code-block:: xml

  <libexif>

* Optional

Options for the exif library.

**Child tags:**

``auxdata``
-----------

.. code-block:: xml

    <auxdata>

* Optional

Currently only adding keywords to auxdata is supported. For a list of keywords/tags see the libexif documentation.
Auxdata can be read by the import java script to gain more control over the media structure.

 **Child tags:**

``add-data``
------------

  .. code-block:: xml

    <add-data tag="keyword1"/>
    <add-data tag="keyword2"/>
    ...

* Optional

If the library was able to extract the data according to the given keyword, it will be added to auxdata.
You can then use that data in your import scripts.

A sample configuration for the example described above would be:

.. code-block:: xml

  <libexif>
      <auxdata>
          <add-data tag="EXIF_TAG_MODEL"/>
      </auxdata>
  </libexif>


``id3``
-------

.. code-block:: xml

  <id3>

* Optional

These options apply to id3lib or taglib libraries.

**Child tags:**

``auxdata``
-----------

.. code-block:: xml

     <auxdata>

* Optional

Currently only adding keywords to auxdata is supported. The keywords are those defined in the specifications, e.g. 
`ID3v2.4 <https://id3.org/id3v2.4.0-frames>`_ or `Vorbis comments. <https://www.xiph.org/vorbis/doc/v-comment.htm>`_
We do not perform any extra checking, so you could try to use any string as a keyword - if it does not exist in the tag 
nothing bad will happen.

Here is a list of some extra keywords not beeing part of UPnP:

* ID3v2.4 / MP3

TBPM, TCOP, TDLY, TENC, TEXT, TFLT, TIT1, TIT3, TKEY, TLAN, TLEN, TMCL, TMED, TOAL,
TOFN, TOLY, TOPE, TOWN, TPE4, TPOS, TPUB, TRSN, TRSO, TSOA, TSRC, TSSE, TXXX:Artist, TXXX:Work, ...

* Vorbis / FLAC

ALBUMSORT, ARTISTS, CATALOGNUMBER, COMPOSERSORT, ENCODEDBY, LYRICIST, ORIGINALDATE, PRODUCER, RELEASETYPE, REMIXER, TITLESORT, WORK, ...

* any other user defined keyword, for APEv2 or iTunes MP4, see e.g. `table of mapping <https://picard.musicbrainz.org/docs/mappings>`_ between various tagging formats at MusicBrainz.

 **Child tags:**

``add-data``
------------

.. code-block:: xml

    <add-data tag="TXXX:Work"/>
    <add-data tag="WORK"/>
    <add-data tag="TMCL"/>
    <add-data tag="PERFORMER"/>
    ...

* Optional

If the library was able to extract the data according to the given keyword, it will be added to auxdata.
You can then use that data in your import scripts.

A sample configuration for the example described above would be:

.. code-block:: xml

  <id3>
      <auxdata>
          <add-data tag="TXXX:Work"/>
          <add-data tag="WORK"/>
          <add-data tag="TMCL"/>
          <add-data tag="PERFORMER"/>
      </auxdata>
  </id3>


``ffmpeg``
----------

.. code-block:: xml

  <ffmpeg>

* Optional

These options apply to ffmpeg libraries.

**Child tags:**

``auxdata``
-----------

.. code-block:: xml

     <auxdata>

* Optional

Currently only adding keywords to auxdata is supported. `This page <https://wiki.multimedia.cx/index.php?title=FFmpeg_Metadata>`_ 
documents all of the metadata keys that FFmpeg honors, depending on the format being encoded.

 **Child tags:**

``add-data``
------------

.. code-block:: xml

    <add-data tag="COLLECTION"/>
    <add-data tag="SHOW"/>
    <add-data tag="NETWORK"/>
    <add-data tag="EPISODE-ID"/>
    ...

* Optional

If the library was able to extract the data according to the given keyword, it will be added to auxdata.
You can then use that data in your import scripts.

A sample configuration for the example described above would be:

.. code-block:: xml

  <ffmpeg>
      <auxdata>
          <add-data tag="COLLECTION"/>
          <add-data tag="SHOW"/>
          <add-data tag="NETWORK"/>
          <add-data tag="EPISODE-ID"/>
      </auxdata>
  </ffmpeg>

