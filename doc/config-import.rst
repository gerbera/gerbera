.. index:: Import

Configure Import
================

The import settings define various options on how to aggregate the content.

.. code-block:: xml
   :caption: Overall structure of import section

    <import>
      <filesystem-charset/>
      <metadata-charset/>
      <scripting>
        <script-folder>
          <common/>
          <custom/>
        </script-folder>
        <import-function/>
        <virtual-layout>
          <import-script/>
          <script-options>
            <script-option/>
          </script-options>
          <genre-map>
            </genre>
          </genre-map>
          <strctured-layout/>
          <boxlayout>
            <box/>
          </boxlayout>
        </virtual-layout>
        <common-script/>
        <custom-script/>
        <playlist-script/>
        <metafile-script/>
      </scripting>
      <magic-file/>
      <autoscan>
        <directory/>
      </autoscan>
      <system-directories>
        <add-path/>
      </system-directories>
      <visible-directories>
        <add-path/>
      </visible-directories>
      <layout>
        <path/>
      </layout>
      <resources>
        <order/>
        <fanart/>
        <subtitle/>
        <metafile/>
        <resource/>
        <container/>
      </resources>
      <mappings>
        <ignore-extensions/>
        <extension-mimetype/>
        <mimetype-contenttype/>
        <mimetype-upnpclass/>
        <mimetype-dlnatransfermode/>
        <contenttype-dlnaprofile/>
      </mappings>
      <library-options>
        <libexif>
          <auxdata>
            <add-data/>
          </auxdata>
        </libexif>
        <id3>
          <auxdata>
            <add-data/>
          </auxdata>
          <metadata>
            <add-data/>
          </metadata>
        </id3>
        <ffmpeg>
          <auxdata>
            <add-data/>
          </auxdata>
          <metadata>
            <add-data/>
          </metadata>
        </ffmpeg>
        <exiv2>
          <auxdata>
            <add-data/>
          </auxdata>
          <metadata>
            <add-data/>
          </metadata>
        </exiv2>
        <mkv/>
        <wavpack/>
      </library-options>
      <online-content>
        ... see respective page
      </online-content>
    </import>


``import``
~~~~~~~~~~

.. Note::

    Whenever config entries in this section are changed, it is recommended to clear the database and restart a full import again. Otherwise the virtual layout can be broken or in some
    mixed state.

.. code:: xml

    <import hidden-files="no" follow-symlinks="no">

* Optional

This tag defines the import section.

**Attributes:**

    .. code:: xml

        hidden-files="yes|no"

    * Optional

    * Default: **no**

    This attribute defines if files starting with a dot will be imported into the database (``yes``). Autoscan can
    override this attribute on a per directory basis. Hidden directories can also be marked with the ``nomedia-file``.

    .. code:: xml

        follow-symlinks="yes|no"

    * Optional

    * Default: **yes**

    This attribute defines if symbolic links should be treated as regular items and imported into the database (``yes``). This can cause duplicate entries if the link target is also scanned.

    .. code:: xml

        default-date="yes|no"

    * Optional

    * Default: **yes**

    This attribute defines that each imported item will get a default media date set based on the modification
    time in order to ensure that sorting by "dc:date" works on upnp requests.

    .. code:: xml

        nomedia-file=".skip"

    * Optional

    * Default: **.nomedia**

    This attribute defines that a directory containing a file with this name is not imported into gerbera database. Only supported in "grb" import mode.

    .. code:: xml

        readable-names="yes|no"

    * Optional

    * Default: **yes**

    This attribute defines that filenames are made readable on import, i.e. underscores are replaced by space and extensions are removed.
    This changes the title of the entry if no metadata is available

    .. code:: xml

        case-sensitive-tags="yes|no"

    * Optional

    * Default: **yes**

    This attribute defines that virtual paths are case sensitive, e.g. artist names like `Ace Of Grace` and `Ace of Grace` are treated as different (``yes``) or identical (``no``).
    This changes the location property of created virtual entries.

    .. code:: xml

        import-mode="mt|grb"

    * Optional

    * Default: **mt**

    This attribute sets the import process:

    - **mt**: traditional mode, that handles each file completely
    - **grb**: modern mode, that first imports all files and then processes the layout of the files


**Child tags:**

``filesystem-charset``
~~~~~~~~~~~~~~~~~~~~~~

.. code:: xml

    <filesystem-charset>UTF-8</filesystem-charset>

* Optional
* Default: **if ``nl\_langinfo()`` function is present, this setting will be auto detected based on your system locale, else set to UTF-8**

Defines the charset of the filesystem. For example, if you have file names in Cyrillic KOI8-R encoding, then you
should specify that here. The server uses UTF-8 internally, this import parameter will help you to correctly import your data.


``metadata-charset``
~~~~~~~~~~~~~~~~~~~~

.. code:: xml

    <metadata-charset>UTF-8</metadata-charset>

* Optional
* Default: **if ``nl\_langinfo()`` function is present, this setting will be auto detected based on your system locale, else set to UTF-8**

Same as above, but defines the charset of the metadata (i.e. id3 tags, Exif information, etc.)


``scripting``
~~~~~~~~~~~~~

.. code:: xml

    <scripting script-charset="UTF-8">

* Optional

Defines the scripting section.

    .. code:: xml

        script-charset=...

    * Optional
    * Default: **UTF-8**

Below are the available scripting options:

``script-folder``
-----------------

    .. code:: xml

        <script-folder>
            <common>/usr/local/share/gerbera/js</common>
            <custom>/home/dev/Source/gerbera/scripts/js</custom>
        </script-folder>

    Defines the locations of the script folders. If set, first all Javascript files from ``common`` are loaded and then all Javascript files from ``custom``.
    If a function is defined in a common and a custom file the custom defintion overwrites the common defintion. No function should be duplicate in
    the same folder. Setting ``script-folder`` is the replacement for setting the various script files ``common-script``.

.. _import-function:

``import-function``
-------------------

    .. code:: xml

        <import-function>
            <audio-file>importAudioInitial</audio-file>
            <video-file>importVideo</video-file>
            <image-file>importImage</image-file>
            <playlist create-link="yes">importPlaylist</playlist>
            <meta-file>importMetadata</meta-file>
        </import-function>

    Set the entry points for the virtual layout functions and file parsing functions. Selecting the entry point is the replacement for setting
    the layout type in ``virtual-layout``. The entry points are directly called for Gerbera code and must have a defined synopsis.
    For further details see :ref:`Scripting <scripting>`.

.. _virtual-layout:

``virtual-layout``
~~~~~~~~~~~~~~~~~~

    .. code:: xml

        <virtual-layout type="builtin" audio-layout="Default">

    * Optional

    Defines options for the virtual container layout; the so called ”virtual container layout” is the way how the
    server organizes the media according to the extracted metadata. For example, it allows sorting audio files by Album, Artist, Year and so on.

        .. code:: xml

            type="builtin|js|disabled"

        * Optional
        * Default: **builtin**

        Specifies what will be used to create the virtual layout, possible values are:

        -  **builtin**: a default layout will be created by the server
        -  **js**: a user customizable javascript will be used (Gerbera must be compiled with js support)
        -  **disabled**: only PC-Directory structure will be created, i.e. no virtual layout

        .. code:: xml

            audio-layout="Default|Structured"
            video-layout="Default"
            image-layout="Default"

        * Optional
        * Default: **Default**

        Specifies the virtual layout to be created:

        -  **Default**: ``addAudio`` is used to create the virtual layout
        -  **Structured**: ``addAudioStructured`` is used to create the virtual layout (only in combination with javascript)

        The virtual layout can be adjusted using an import script which is defined as follows:


``import-script``
-----------------

        .. code:: xml

            <import-script>/path/to/my/import-script.js</import-script>

        * Required:  **if virtual layout type is ”js\ ”**
        * Default: ``${prefix}/share/gerbera/js/import.js``, **where ${prefix} is your installation prefix directory.**
        * Deprecated: Think about migrating your configuration to use :ref:`import-function`.

        Points to the script invoked upon media import. For more details read about :ref:`scripting <scripting>`.

.. _script-options:

``script-options``
------------------

        .. code:: xml

            <script-options></script-options>

        * Optional

        Contains options to pass into scripts. All values are available in scripts as e.g.
        ``config['/import/scripting/virtual-layout/script-options/script-option'].test``.
        For more details see :ref:`scripting <scripting>`


        **Child tags:**

``script-option``
^^^^^^^^^^^^^^^^^

            .. code:: xml

                <script-option name="test" value="42"/>

            * Optional

            Set option ``value`` for option ``name``

                .. code:: xml

                    name="..."

                * Required

                Name of the option.

                .. code:: xml

                    value="..."

                * Required

                Value of the option.

``genre-map``
-------------

        .. code:: xml

            <genre-map></genre-map>

        * Optional

        Define mapping of genres to other text.


        **Child tags:**

``genre``
^^^^^^^^^

            .. code:: xml

                <genre from="Disco" to="Pop"/>

            * Optional

            Replace genre ``from`` by genre ``to``.

                .. code:: xml

                    from="..."

                * Required

                Original genre value. Can be a regular expression.

                .. code:: xml

                    to="..."

                * Required

                Target genre value.


``structured-layout``
---------------------

        .. code:: xml

            <structured-layout skip-chars="" div-char="-" />

        * Optional

        Adjust layout of boxes for large collections in structured layout. Set audio-layout to **Structured** and choose values best for your media library.

            .. code:: xml

                div-char="-"

            * Optional
            * Default: **-**

            Symbols to use around the box text.

            .. code:: xml

                skip-chars="-"

            * Optional

            Special characters in the beginning of a title that are not used for building a box.


.. _boxlayout:

``boxlayout``
-------------

        .. code:: xml

            <boxlayout extend="true"></boxlayout>

        * Optional
        * Extensible Default: Without ``extend="true"`` all missing entries are reported

        **Child tags:**

``box``
^^^^^^^

            .. code:: xml

                <box key=".." title=".." class=".." size=".." enabled=".." />

            * Optional

            Configure Box ``key``.

            .. code:: xml

                key="Audio/audioRoot"

            * Required

            The key that is used in javascript and builtin layout to refer the the config.

            .. code:: xml

                title="Music"

            * Required

            The title to use for the respective box.

            .. code:: xml

                class="object.container.album.musicAlbum"

            * Optional

            Set the upnp class for the respective box.

            .. code:: xml

                upnp-shortcut="MUSIC_ARTISTS"

            * Optional

            Set the upnp shortcut label for this box. The last container created based on this
            box will be added to the shortcuts feature field.
            For more details see UPnP-av-ContentDirectory-v4-Service, page 357.

            .. code:: xml

                enabled="no"

            * Optional
            * Default: "yes"

            Disable the respective box. Not all boxes can be disabled.

            .. code:: xml

                size="6"

            * Optional
            * Default: 1

            Type of the box in structured layout. The following values are supported

            -  **1**: One large box
            -  **2**: Two boxes with 13 items each
            -  **3**: Boxes with 8, 9, 9 letters
            -  **4**: Boxes with 7, 6, 7, 6 letters
            -  **5**: Boxes with 5, 5, 5, 6, 5 letters
            -  **6**: Boxes with 4, 5, 4, 4, 5, 4 letters
            -  **7**: Boxes with 4, 3, 4, 4, 4, 3, 4 letters
            -  **9**: Boxes with 5, 5, 5, 4, 1, 6 letters; a large box for T
            -  **13**: Boxes with 2 letters each
            -  **26**: A speparate box for each letter


``common-script``
~~~~~~~~~~~~~~~~~

.. code:: xml

    <common-script>/path/to/my/common-script.js</common-script>

* Optional
* Default: ``${prefix}/share/gerbera/js/common.js``, **where ${prefix} is your installation prefix directory.**
* Deprecated: Think about migrating your configuration to use :ref:`import-function`.

Points to the so called common script - it is a shared library of js helper functions.
For more details read :ref:`scripting <scripting>`

``custom-script``
~~~~~~~~~~~~~~~~~

.. code:: xml

    <custom-script>/path/to/my/custom-script.js</custom-script>

* Optional
* Default: **empty**
* Deprecated: Think about migrating your configuration to use :ref:`import-function`.

Points to the custom script - think of it as a custom library of js helper functions, functions added
there can be used in your import and in your playlist scripts. Theses functions also overwrite functions from the common script.
For more details read :ref:`scripting <scripting>`

``playlist-script``
~~~~~~~~~~~~~~~~~~~

.. code:: xml

    <playlist-script create-link="yes">/path/to/my/playlist-script.js</playlist-script>

* Optional
* Default: ``${prefix}/share/gerbera/js/playlists.js``, **where ${prefix} is your installation prefix directory.**
* Deprecated: Think about migrating your configuration to use :ref:`import-function`.

Points to the script that is parsing various playlists, by default parsing of pls, m3u and asx playlists is implemented,
however the script can be adapted to parse almost any kind of text based playlist. For more details read :ref:`scripting <scripting>`

    .. code:: xml

        create-link="yes|no"

    * Optional
    * Default: **yes**

    Links the playlist to the virtual container which contains the expanded playlist items. This means, that
    if the actual playlist file is removed from the database, the virtual container corresponding to the playlist will also be removed.


``metafile-script``
~~~~~~~~~~~~~~~~~~~

.. code:: xml

    <metafile-script>/path/to/my/metadata-script.js</metafile-script>

* Optional
* Default: ``${prefix}/share/gerbera/js/metadata.js``, **where ${prefix} is your installation prefix directory.**
* Deprecated: Think about migrating your configuration to use :ref:`import-function`.

Points to the main metadata parsing script which is invoked during the first import phase to gather metadata from additional files.
Currently support for nfo files is implemented. (https://kodi.wiki/view/NFO_files/Templates)

The search pattern is set in resources section.


``magic-file``
~~~~~~~~~~~~~~

.. code:: xml

    <magic-file>/path/to/my/magic-file</magic-file>

* Optional
* Default: **System default**

Specifies an alternative file for filemagic, containing mime type information.

.. _autoscan:

``autoscan``
~~~~~~~~~~~~

.. code:: xml

    <autoscan use-inotify="auto" inotify-attrib="yes">

* Optional

Specifies a list of default autoscan directories.

This section defines persistent autoscan directories. It is also possible to define autoscan directories in the UI,
the difference is that autoscan directories that are defined via the config file can not be removed in the UI.
Even if the directory gets removed on disk, the server will try to monitor the specified location and will re add
the removed directory if it becomes available/gets created again.

    .. code:: xml

        use-inotify="yes|no|auto"

    * Optional
    * Default: **auto**

    Specifies if the inotify autoscan feature should be enabled. The default value is ``auto``, which means that
    availability of inotify support on the system will be detected automatically, it will then be used if available.
    Setting the option to 'no' will disable inotify even if it is available. Allowed values: "yes", "no", "auto"

    .. code:: xml

        inotify-attrib="yes|no"

    * Optional
    * Default: **no**

    Specifies if the inotify will also monitor for attribute changes like owner change or access given.

    **Child tags:**

``directory``
-------------

    .. code:: xml

        <directory location="/media" mode="timed" interval="3600"
          recursive="no" hidden-files="no"/>
        <directory location="/audio" mode="inotify"
          recursive="yes" hidden-files="no"/>

    * Optional

    Defines an autoscan directory and it's parameters.

    The attributes specify various autoscan options:

        .. code:: xml

            location=...

        * Required

        Absolute path to the directory that shall be monitored.

        .. code:: xml

            mode="inotify|timed"

        * Required

        Scan mode, currently ``inotify`` and ``timed`` are supported. Timed mode rescans the given directory in specified
        intervals, inotify mode uses the kernel inotify mechanism to watch for filesystem events.

        .. code:: xml

            interval=...

        * Required: **for ”timed” mode**

        Scan interval in seconds. The value can be given in a valid time format.

        .. code:: xml

            recursive="yes|no"

        * Required

        Values of ``yes`` or ``no`` are allowed, specifies if autoscan shall monitor the given directory including all sub directories.

        .. code:: xml

            dirtypes="yes|no"

        * Optional
        * Default: ``yes``

        Values of ``yes`` or ``no`` are allowed, specifies if containers in
        PC Directory will get container types like albums based on the majority
        of child items.

        .. code:: xml

            hidden-files="yes|no"

        * Optional
        * Default: **value specified in <import hidden-files=””/>**

        Allowed values: ``yes`` or ``no``, process hidden files, overrides the ``hidden-files`` value in the ``<import/>`` tag.

        .. code:: xml

            follow-symlinks="yes|no"

        * Optional
        * Default: **value specified in <import follow-symlinks=””/>**

        Allowed values: ``yes`` or ``no``, symbolic links should be treated as regular items and imported into the database (``yes``).
        This can cause duplicate entries if the link target is also scanned
        overrides the ``follow-symlinks`` value in the ``<import/>`` tag.

        .. code:: xml

            retry-count="3"

        * Optional
        * Default: **0**

        Allowed values: non-negative numbers.
        This attribute can be used to allow multiple attempts to access files
        in case of mounted volumes. In some cases inotify events are raised
        before the directory or file is fully available, causing an access
        permission error and the import fails.
        This attribute is only available in config.xml at the moment.

        .. code:: xml

            force-reread-unknown="yes|no"

        * Optional

        * Default: **no**

        This attribute forces that files without changes are reread (on startup) if their upnp class is unset or "object.item".
        This can happen if the first scan (e.g. via inotify) did not get all details of the file correctly.
        This is mostly the case if the media folder is exported on the network and files are written via network.
        Be aware that the startup will take longer if there is a large number of non-media files in the folder

        .. code:: xml

            media-type="Music|AudioBook"

        * Optional
        * Default: **Any**

        Only import audio/image/video from directory to virtual layout if upnp class is subclass.
        Values can be concatenated by ``|``. Allowed values are :

        +------------------+--------------------------------------+
        | Value            | Upnp Class                           |
        +==================+======================================+
        | Any              | object.item                          |
        +------------------+--------------------------------------+
        | Audio            | object.item.audioItem                |
        +------------------+--------------------------------------+
        | Music            | object.item.audioItem.musicTrack     |
        +------------------+--------------------------------------+
        | AudioBook        | object.item.audioItem.audioBook      |
        +------------------+--------------------------------------+
        | AudioBroadcast   | object.item.audioItem.audioBroadcast |
        +------------------+--------------------------------------+
        | Image            | object.item.imageItem                |
        +------------------+--------------------------------------+
        | Photo            | object.item.imageItem.photo          |
        +------------------+--------------------------------------+
        | Video            | object.item.videoItem                |
        +------------------+--------------------------------------+
        | Movie            | object.item.videoItem.movie          |
        +------------------+--------------------------------------+
        | MusicVideo       | object.item.videoItem.musicVideoClip |
        +------------------+--------------------------------------+
        | TV               | object.item.videoItem.videoBroadcast |
        +------------------+--------------------------------------+

        .. code:: xml

            container-type-audio="object.container"
            container-type-image="object.container"
            container-type-video="object.container"

        * Optional
        * Default: **object.container.album.musicAlbum**/ **object.container.album.photoAlbum**/ **object.container**

        Set the default container type for virtual containers and for physical containers in grb-mode during import.
        This is especially useful if the virtual layout simulates the filesystem structure and is not derived from metadata.
        The first object that is added to the container determines the property (audio/image/video) used.


``system-directories``
~~~~~~~~~~~~~~~~~~~~~~

.. code:: xml

    <system-directories>

* Optional
* Extensible Default

Specifies a list of system directories hidden in filesystem web ui.

If the element does not exists, the default list of system directories is set to ``/bin, /boot, /dev, /etc, /lib, /lib32, /lib64, /libx32, /proc, /run, /sbin, /sys, /tmp, /usr, /var``

    **Child tags:**

``add-path``
------------

    .. code:: xml

        <add-path name="/sys"/>

    * Optional

    Defines a system directory.

    The attributes specify various options:

        .. code:: xml

            name=...

        * Required

        Absolute path to the directory that shall be hidden.


``visible-directories``
~~~~~~~~~~~~~~~~~~~~~~~

.. code:: xml

    <visible-directories>

* Optional

Specifies a list of system directories visible in filesystem web ui. It can contain any path which is accessible by the gerbera server.

If the element exists it supercedes ``system-directories``, i.e. only visible directories can be selected in web ui.
This is the more forward way of defining content but cannot be defaulted.

    **Child tags:**

``add-path``
------------

    .. code:: xml

        <add-path name="/home/media"/>

    * Optional

    Defines a visible directory.

    The attributes specify various options:

        .. code:: xml

            name=...

        * Required

        Absolute path to the directory that shall be visible.


``layout``
~~~~~~~~~~

.. code:: xml

    <layout>

* Optional

Defines various layout options for generated virtual layout.

        .. code:: xml

            parent-path="yes|no"

        * Optional
        * Default: **no**

        Values of ``yes`` or ``no`` are allowed, specifies if parent path is added to virtual layout. If set to``no`` "/home/.../Videos/Action/a.mkv" with rootpath "/home/.../Videos" becomes "Action" otherwise "Videos/Action". Setting to ``yes`` produces the layout of gerbera before version 1.5.

    **Child tags:**

``path``
--------

        .. code:: xml

            <path from="Videos/Action" to="Action-Videos"/>

        * Optional

        Map a virtual path element. This allows reducing path elements or merging different sources into a common tree. Thema replacement is executed after calculation of virtual layout, i.e. after buildin or layout script.

            .. code:: xml

                from="..."

            * Required

            Source path. Can be a regular expression.

            .. code:: xml

                to="..."

            * Required

            Target path. / can be used to create sub structure.


``resources``
~~~~~~~~~~~~~

.. code:: xml

    <resources case-sensitive="yes">

* Optional

Defines various resource options for file based resources. Older versions of Gerbera added sereral files automatically. For performance reasons no pattern is added by default anymore.
You can set up your correct fanart file by yourself, if no image is embedded in your media. If you have subtitle files, add the correct pattern, also.

    .. code:: xml

        case-sensitive="yes|no"

    * Optional

    * Default: **yes**

    This attribute defines whether search patterns are treated case sensitive or not, i.e. if set to **no** ``cover.png`` matches anything like ``Cover.PNG`` or ``cover.PNG``.

**Child tags:**

``order``
---------

    .. code:: xml

        <order>...</order>

    * Optional

    Define the order in which the metadata is rendered in the output

    **Child tags:**

``handler``
^^^^^^^^^^^

        .. code:: xml

            <handler name="Fanart"/>
                name="..."

        * Required

        Valid handler names are ``Default``, ``LibExif``, ``TagLib``, ``Transcode``, ``Fanart``, ``Exturl``, ``MP4``, ``FFmpegThumbnailer``, ``Flac``, ``Matroska``, ``Subtitle``, ``Resource``, ``ContainerArt``

``container`` / ``fanart`` / ``subtitle`` / ``metafile`` / ``resource``
-----------------------------------------------------------------------

    .. code:: xml

        <container>...</container>
        <fanart>...</fanart>
        <subtitle>...</subtitle>
        <metafile>...</metafile>
        <resource>...</resource>

    * Optional
    * Extensible Default

    Define file patterns to search for fanart, subtitle and resources respectivly.

    ``container``, ``fanart``, ``metafile`` and ``subtitle`` patterns are used to identify external resources which are added to each item if the pattern matches.

    ``resource`` patterns are used to trigger rescan of the whole directory if such a file was changed, added or removed.

    Each of these tags can contain multiple ``add-file`` or ``add-dir`` entries. ``container`` has additional attributes.

``container``
^^^^^^^^^^^^^

    .. code:: xml

        <container location="images" parentCount="2" minDepth="2"/>

    * Optional

    Set up container images. The fanart of a media file is added automatically as a thumbnail to the container (e.g. the album container).
    The setting depends on the chosen layout and is only fully respected if the layout script does not set own properties (which was the case in older javascript layouts).

        .. code:: xml

            location="..."

        * Optional

        Path to the directory containing the images to load. Relative paths are assumed to be under the server's home.
        Drop your artists' images or logos for default containers here and they are displayed as thumbnail when browsing with a compatible client.
        If the image is not found in that location, it is also searched in the physical folder itself.

        .. code:: xml

            parentCount="..."

        * Optional

        This setting allows to increase the number of levels which the fanart of a media file can be propagated upwards (examples refer to basic layout /Root/Audio/Artist/Album/song).
        A value of 1 adds the fanart only to the direct parent container when a media file is added (e.g. the Album container).
        A value of 2 means you propagate that image to the parent container as well (e.g. the Artist container).
        A value of 0 blocks propagation completely.

        .. code:: xml

            minDepth="..."

        * Optional

        Depending on the virtual layout propagating thumbnails can reach containers like Video or Audio. This settings forces a minimal depth for propagation to apply.
        It is setting the minimum number of path elements for container using fanart from media files (e.g. /Root/Audio/Artist has level 3 so the image can be set).


``add-file``
^^^^^^^^^^^^

    .. code:: xml

        <add-file name="cover.png"/>
        <add-file name="%filename%.srt"/>

    * Optional

        .. code:: xml

            name="..."

        * Required

        Add file search pattern to resource handler. The search pattern can contain variables:

        - ``%album%``: Value of the album tag
        - ``%albumArtist%``: Value of the albumArtist tag
        - ``%artist%``: Value of the artist tag
        - ``%filename%``: Name of the file without extension or name of the container
        - ``%genre%``: Value of the genre tag
        - ``%title%``: Value of the title tag
        - ``%composer%``: Value of the composer tag

``add-dir``
^^^^^^^^^^^

    .. code:: xml

        <add-dir name="/data/subtitles/%title%" ext="srt"/>
        <add-dir name="/data/subtitles" ext="%title%*.srt"/>
        <add-dir name="%filename%" ext="srt"/>
        <add-dir pattern="%filename%*" ext="srt"/>

    * Optional

        .. code:: xml

            name="..."

        * Optional

        Add directory search pattern to resource handler. The search pattern can contain the same variables as ``add-file``.
        If the directory is relative the file is searched in a subdirectory of the directory containing the media file.
        If the directory is empty or just "." it is replaced by the directory of the media file.

        .. code:: xml

            ext="..."

        * Optional

        Define the extension or file name pattern. The search pattern can contain the same variables as ``add-file``.
        If it does not contain a ``.`` it is considered as extension.
        If it contains a ``.`` the part before can contain ``*`` and ``?`` as wildcards and must exactly match the resource file name.

        .. code:: xml

            pattern="..."

        * Optional

        Define the file name pattern in the directory. The search pattern can contain the same variables as ``add-file``.
        It can contain ``*`` and ``?`` as wildcards and must exactly match the resource file name.


A sample configuration would be:

.. code-block:: xml

    <resources case-sensitive="no">
        <fanart>
            <add-file name="cover.png"/>
        </fanart>
        <subtitle>
            <add-file name="%filename%.srt"/>
            <add-dir name="/data/subtitles/%title%" ext="srt"/>
            <add-dir pattern="%filename%*" ext="srt"/>
        </subtitle>
        <resource>
            <add-file name="cover.png"/>
            <add-file name="%filename%.srt"/>
        </resource>
    </resources>

.. _import-mappings:

``mappings``
~~~~~~~~~~~~

.. code:: xml

    <mappings>

* Optional

Defines various mapping options for importing media, currently two subsections are supported.

This section defines mime type and upnp:class mappings, it is vital if filemagic is not available - in this case
media type auto detection will fail and you will have to set the mime types manually by matching the file extension.
It is also helpful if you want to override auto detected mime types or simply skip filemagic processing for known file types.


``ignore-extensions``
---------------------
.. code:: xml

    <ignore-extensions>

* Optional

This section holds the file name extension to mime type mappings.

**Child tags:**

``add-file``
^^^^^^^^^^^^

.. code:: xml

    <add-file name="part"/>

* Optional

Specifies a file name extension (everything after the last dot ".") to ignore.

Note:
    This improves the import speed, because files are ignored completely.

Note:
    The extension is case sensitive, if ``case-sensitive`` in the element ``extension-mimetype`` is set to ``yes``


.. _extension-mimetype:

``extension-mimetype``
----------------------

.. code:: xml

    <extension-mimetype ignore-unknown="no" case-sensitive="no">

* Optional
* Extensible Default

This section holds the file name extension to mime type mappings.

    **Attributes:**

        .. code:: xml

            ignore-unknown=...

        * Optional
        * Default: **no**

        If ignore-unknown is set to "yes", then only the extensions that are listed in this section are imported.

        .. code:: xml

            case-sensitive=...

        * Optional
        * Default: **no**

        Specifies if extensions listed in this section are case sensitive, allowed values are "yes" or "no".

**Child tags:**

``map``
^^^^^^^

.. code:: xml

    <map from="mp3" to="audio/mpeg"/>

* Optional

Specifies a mapping from a certain file name extension (everything after the last dot ".") to mime type.

Note:
    This improves the import speed, because invoking libmagic to discover the right mime type of a file is
    omitted for files with extensions listed here.

Note:
    The extension is case sensitive, if ``case-sensitive`` is set to ``yes``.


``mimetype-upnpclass``
----------------------

.. code:: xml

    <mimetype-upnpclass>

* Optional
* Extensible Default

This section holds the mime type to upnp:class mappings.


**Child tags:**

``map``
^^^^^^^

.. code:: xml

    <map from="audio/*" to="object.item.audioItem.musicTrack"/>

* Optional

Specifies a mapping from a certain mime type to upnp:class in the Content Directory. The mime type can either be
entered explicitly "audio/mpeg" or using a wildcard after the slash ``audio/\*``. The values of **from** and **to**
attributes are case sensitive.

For detailled mapping the **from** attribute can specify further filtering criteria like ``upnp:genre=Book`` which is
expanded to `if genre contains Book`.

* Example

.. code:: xml

    <mimetype-upnpclass>
      <map from="application/ogg" to="object.item.audioItem.musicTrack"/>
      <map from="audio/*" to="object.item.audioItem"/>
      <map from="audio/*;tracknumber>0" to="object.item.audioItem.musicTrack"/>
      <map from="audio/*;upnp:genre=Book" to="object.item.audioItem.audioBook"/>
      <map from="image/*" to="object.item.imageItem"/>
      <map from="image/*;location=Camera" to="object.item.imageItem.photo"/>
      <map from="video/*" to="object.item.videoItem"/>
    </mimetype-upnpclass>


``mimetype-dlnatransfermode``
-----------------------------

.. code:: xml

    <mimetype-dlnatransfermode>

* Optional
* Extensible Default

This section holds the mime type to dlna transfer mode mappings. It is added to the http-header ``transferMode.dlna.org`` of the file request.


**Child tags:**

``map``
^^^^^^^

.. code:: xml

     <map from="audio/*" to="Streaming"/>
     <map from="video/*" to="Streaming"/>
     <map from="image/*" to="Interactive"/>
     <map from="text/*" to="Background"/>

* Optional

Specifies a mapping from a certain mime type to transfer mode. The mime type can either be
entered explicitly "audio/mpeg" or using a wildcard after the slash ``audio/\*``. The values of **from** and **to**
attributes are case sensitive.

``mimetype-contenttype``
------------------------

.. code:: xml

  <mimetype-contenttype>

* Optional
* Extensible Default

This section makes sure that the server knows about remapped mimetypes and still extracts the metadata correctly.
If the user remaps mp3 files to a different mimetype, we must know about it so we can still pass this item to taglib
for metadata extraction.

Note:
  If this section is not present in your config file, the defaults will be filled in automatically.
  However, if you add an empty tag, without defining the following ``<treat>`` tags, the server assumes that
  you want to have an empty list and no files will be process by the metadata handler.


``treat``
^^^^^^^^^

.. code:: xml


    <treat mimetype="audio/mpeg" as="mp3"/>

* Optional

Tells the server what content the specified mimetype actually is.

Note:
    It makes no sense to define 'as' values that are not below, the server only needs to know the content
    type of the ones specified, otherwise it does not matter.

The ``as`` attribute can have following values:

**Mapping Table**

+-----------------------------------+---------------+----------------------------------------+
| mimetype                          | as            | Note                                   |
+===================================+===============+========================================+
| | audio/mpeg                      | mp3           | | The content is an mp3 file and should|
|                                   |               | | be processed by taglib               |
|                                   |               | | (if available).                      |
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

.. _contenttype-dlnaprofile:

``contenttype-dlnaprofile``
---------------------------

.. code:: xml

    <contenttype-dlnaprofile>

* Optional
* Extensible Default

This section holds the content type to dlnaprofile mappings.


**Child tags:**

``map``
^^^^^^^

.. code:: xml

    <map from="mp4" to="AVC_MP4_BL_CIF30_AAC_MULT5"/>

* Optional

Specifies a mapping from a certain content type to a dlna profile in the Content Directory. The values of **from** and **to**
attributes are case sensitive.
In order to access special profiles you can specify a resource attribute with its required value. If multiple entries for the same
mimetype exist, mappings with more details are preferred to simple from-to mappings.
Resource attributes can be seen in the details page for an item on the web UI. The value must either match exactly the transformed value (incl. unit) or the raw value.

* Example:

.. code:: xml

    <map from="mp4" videoCodec="h264" audioCodec="aac" to="AVC_MP4_MP_HD_720p_AAC"/>

* Profile Catalog:

If you want to make your DLNA device play specific media the following list of profiles may be helpful:

.. literalinclude:: ./profiles.xml
    :language: xml

.. Note::

    Feel free to provide us improvements to this list!

.. _virtual-directories:

``virtual-directories``
~~~~~~~~~~~~~~~~~~~~~~~

.. code:: xml

    <virtual-directories>

* Optional

This section holds the additional identifiers for virtual directories to make sure, e.g. albums with the same title
are distiguished by their artist even if the displayed text is identical.

* Extensible Default

.. code:: xml

    <virtual-directories>
        <key metadata="M_ALBUMARTIST" class="object.container.album.musicAlbum"/>
        <key metadata="M_UPNP_DATE" class="object.container.album.musicAlbum"/>
    </virtual-directories>

**Child tags:**

``key``
-------

.. code:: xml

    <key metadata="M_ARTIST" class="object.container.album"/>

* Optional

Specifies the metadata fields to add for identification internally. For a list of valid metadata see :ref:`Metadata <upnp-tags>`.
If ``_1`` is added to the name of the key only the first item in the list is picked (if there are multiple like for ``M_ARTIST``.
In addition ``LOCATION`` references to the location property retrieved from the layout script.
The attribute ``class`` can be used to restrict the key to the upnp class.

* Example

.. code:: xml

    <virtual-directories>
        <key metadata="LOCATION" class="object.container.album.musicAlbum"/>
        <key metadata="M_ARTIST_1" class="object.container.album.musicAlbum"/>
    </virtual-directories>


.. _library-options:

``library-options``
~~~~~~~~~~~~~~~~~~~

.. code:: xml

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
library, currently we support **taglib**, **ffmpeg** and **libexif**, **mkv**, **wavpack** and **exiv2** (if compiled with respective library enabled) which provide a default set of keys
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

    .. code:: xml

        multi-value-separator="..."

    * Optional
    * Default: **"; "**

    This string is used to join multi-valued items (e.g. Composer, Performer) into one string.

    .. code:: xml

        legacy-value-separator="..."

    * Optional
    * Default: **empty**

    This string is used to split items into lists before joining them with multi-value-separator.
    This option can be used to import files from legacy tools which did not support multi-valued items.
    The empty string is used to disable legacy handling.


**Additional data:**

Gerbera imports a set of common tags by default in order to populate UPnP content. If you need further properties there are two options

* auxdata : Read the value in order to use it in an import script
* metadata : Read value into in order to send it as UPnP property

The following library sections can contain both of these entries as well as
a ``charset`` and a ``enabled`` attribute:

**Attributes:**

``charset``
-----------

* Optional

Overwrite the ``metadata-charset`` for the respective type of file.

``enabled``
-----------

* Optional

Exclude the metadata parser from the import process.

* Default: **yes**


**Tags:**

``auxdata``
-----------

.. code-block:: xml

    <auxdata>

* Optional

Auxdata can be read by the import javascript to gain more control over the media structure. The available tags depend on the respective library.

**Child tags:**

``add-data``
^^^^^^^^^^^^

.. code-block:: xml

    <add-data tag="tag1"/>
    <add-data tag="tag2"/>
    ...

* Optional

If the library was able to extract the data according to the given keyword, it will be added to auxdata.
You can then use that data in your import scripts.

``metadata``
------------

.. code-block:: xml

    <metadata>

* Optional

Metadata can be read by the import javascript as ``meta`` to gain more control over the media structure and is automatically added to the UPnP output.

**Child tags:**

``add-data``
^^^^^^^^^^^^

.. code-block:: xml

    <add-data tag="tag3" key="upnp:Key"/>

* Optional

If the library was able to extract the data according to the given keyword, it will be added to metadata.
The attribute ``key`` sets the UPnP meta property and is only accepted inside a ``metadata`` element.

``comment``
-----------

.. code-block:: xml

    <comment enabled="yes"> ... </comment>

* Optional

Fabricate a comment (description) from metadata. The comment will only be created if there is no description set and in the mechanism is ``enabled``.

**Child tags:**

``add-comment``
^^^^^^^^^^^^^^^

.. code-block:: xml

    <add-comment label="My favourite Detail" tag="MEDIA_TAG"/>

* Optional

If the library was able to extract the data according to the given tag, it will be added to the comment.
The attribute ``tag`` sets the media property and the ``label`` contains the text to add the the comment.


**Library sections:**

``libexif``
-----------

.. code-block:: xml

  <libexif> </libexif>

* Optional

Options for the exif library.

Currently only adding keywords to auxdata is supported. For a list of keywords/tags see the libexif documentation.

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

    <id3> </id3>

* Optional

These options apply to taglib library.

The keywords are those defined in the specifications, e.g.
`ID3v2.4 <https://id3.org/id3v2.4.0-frames>`__ or `Vorbis comments. <https://www.xiph.org/vorbis/doc/v-comment.htm>`__
We do not perform any extra checking, so you could try to use any string as a keyword - if it does not exist in the tag
nothing bad will happen.

Here is a list of some extra keywords not beeing part of UPnP:

* ID3v2.4 / MP3

TBPM, TCOP, TDLY, TENC, TEXT, TFLT, TIT1, TIT3, TKEY, TLAN, TLEN, TMCL, TMED, TOAL,
TOFN, TOLY, TOPE, TOWN, TPE4, TPOS, TPUB, TRSN, TRSO, TSOA, TSRC, TSSE, TXXX:Artist, TXXX:Work, ...

* Vorbis / FLAC

ALBUMSORT, ARTISTS, CATALOGNUMBER, COMPOSERSORT, ENCODEDBY, LYRICIST, ORIGINALDATE, PRODUCER, RELEASETYPE, REMIXER, TITLESORT, WORK, ...

* any other user defined keyword, for APEv2 or iTunes MP4, see e.g. `table of mapping <https://picard.musicbrainz.org/docs/mappings>`__ between various tagging formats at MusicBrainz.

A sample configuration for the example described above would be:

.. code-block:: xml

    <id3>
        <auxdata>
            <add-data tag="TXXX:Work"/>
            <add-data tag="WORK"/>
            <add-data tag="TMCL"/>
        </auxdata>
        <metadata>
            <add-data tag="PERFORMER" key="upnp:artist@role[Performer]"/>
        </metadata>
    </id3>


``ffmpeg``
----------

.. code-block:: xml

    <ffmpeg> </ffmpeg>

* Optional

These options apply to ffmpeg library.

`This page <https://wiki.multimedia.cx/index.php?title=FFmpeg_Metadata>`__
documents all of the metadata keys that FFmpeg honors, depending on the format being encoded.

A sample configuration for the example described above would be:

.. code-block:: xml

    <ffmpeg>
        <auxdata>
            <add-data tag="COLLECTION"/>
            <add-data tag="SHOW"/>
            <add-data tag="NETWORK"/>
            <add-data tag="EPISODE-ID"/>
        </auxdata>
        <metadata>
            <add-data tag="performer" key="upnp:artist@role[Performer]"/>
        </metadata>
    </ffmpeg>


``exiv2``
----------

.. code-block:: xml

    <exiv2> </exiv2>

* Optional

These options apply to exiv2 library.

`This page <https://www.exiv2.org/metadata.html>`__
documents all of the metadata keys that exiv2 honors, depending on the format being encoded.

A sample configuration for the example described above would be:

.. code-block:: xml

    <exiv2>
        <auxdata>
            <add-data tag="Exif.Image.Model"/>
            <add-data tag="Exif.Photo.DateTimeOriginal"/>
            <add-data tag="Exif.Image.Orientation"/>
            <add-data tag="Exif.Image.Rating"/>
            <add-data tag="Xmp.xmp.Rating" />
            <add-data tag="Xmp.dc.subject"/>
        </auxdata>
    </exiv2>

``mkv``
-------

.. code-block:: xml

    <mkv/>

* Optional

These options apply to mkv library.


``wavpack``
-----------

.. code-block:: xml

    <wavpack/>

* Optional

These options apply to wavpack library.
