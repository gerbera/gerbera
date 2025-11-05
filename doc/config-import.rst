.. index:: Configure Import

################
Configure Import
################

The import settings define various options on how to aggregate the content.

.. contents::
   :backlinks: entry
.. sectnum::
   :start: 2

.. admonition:: `Overall structure of import section (collapsible)`
   :collapsible: closed


   .. code-block:: xml

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
              <chain>
                <link/>
              </chain>
            </boxlayout>
          </virtual-layout>
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
        <virtual-directories>
          <key/>
        </virtual-directories>
        <library-options>
          <libexif>
            <auxdata>
              <add-data/>
            </auxdata>
            <metadata>
              <add-data/>
            </metadata>
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
          <mkv>
            <auxdata>
              <add-data/>
            </auxdata>
            <metadata>
              <add-data/>
            </metadata>
          </mkv>
          <wavpack/>
            <auxdata>
              <add-data/>
            </auxdata>
            <metadata>
              <add-data/>
            </metadata>
          </wavpack>
        </library-options>
        <online-content>
          ... see respective page
        </online-content>
      </import>


******
Import
******

.. Note::

    Whenever config entries in this section are changed, it is recommended to clear the database
    and restart a full import again. Otherwise the virtual layout can be broken or in some
    mixed state.

.. confval:: import
   :type: :confval:`Section`
   :required: true

   .. code:: xml

       <import hidden-files="no" follow-symlinks="no">

This tag defines the import section.

Import Attributes
-----------------

.. confval:: import hidden-files
   :type: :confval:`Boolean`
   :required: false
   :default: ``no``

   .. code:: xml

      hidden-files="yes"

This attribute defines if files starting with a dot will be imported into the database (``yes``). Autoscan can
override this attribute on a per directory basis. Hidden directories can also be marked with the ``nomedia-file``.

.. confval:: import follow-symlinks
   :type: :confval:`Boolean`
   :required: false
   :default: ``yes``

   .. code:: xml

       follow-symlinks="no"

This attribute defines if symbolic links should be treated as regular items and imported into the database (``yes``).
This can cause duplicate entries if the link target is also scanned.

.. confval:: default-date
   :type: :confval:`Boolean`
   :required: false
   :default: ``yes``

   .. code:: xml

       default-date="no"

This attribute defines that each imported item will get a default media date set based on the modification
time in order to ensure that sorting by "dc:date" works on upnp requests.

.. confval:: nomedia-file
   :type: :confval:`String`
   :required: false
   :default: ``.nomedia``

   .. code:: xml

       nomedia-file=".skip"

This attribute defines that a directory containing a file with this name is not imported into gerbera database.
Only supported in "grb" import mode.

.. confval:: readable-names
   :type: :confval:`Boolean`
   :required: false
   :default: ``yes``

   .. code:: xml

       readable-names="no"

This attribute defines that filenames are made readable on import, i.e. underscores are replaced by space and 
extensions are removed.
This changes the title of the entry if no metadata is available

.. confval:: case-sensitive-tags
   :type: :confval:`Boolean`
   :required: false
   :default: ``yes``

   .. code:: xml

       case-sensitive-tags="no"

This attribute defines that virtual paths are case sensitive, e.g. artist names like `Ace Of Grace` and `Ace of Grace` are treated as different (``yes``) or identical (``no``).
This changes the location property of created virtual entries.

.. confval:: import-mode
   :type: :confval:`Enum` ``grb|mt``
   :required: false
   :default: ``mt``

   .. code:: xml

       import-mode="grb"

This attribute sets the import process:

- ``mt``: traditional mode, that handles each file completely
- ``grb``: modern mode, that first imports all files and then processes the layout of the files


Import Items
------------

Filesystem Charset
^^^^^^^^^^^^^^^^^^

.. confval:: filesystem-charset
   :type: :confval:`String`
   :required: false
   :default: if ``nl_langinfo()`` function is present, this setting will be auto detected based on your system locale, else set to ``UTF-8``.

   .. code:: xml

      <filesystem-charset>UTF-8</filesystem-charset>

Defines the charset of the filesystem. For example, if you have file names in Cyrillic KOI8-R encoding, then you
should specify that here. The server uses UTF-8 internally, this import parameter will help you to correctly import your data.


Metadata Charset
^^^^^^^^^^^^^^^^

.. confval:: metadata-charset
   :type: :confval:`String`
   :required: false
   :default: if ``nl_langinfo()`` function is present, this setting will be auto detected based on your system locale, else set to ``UTF-8``.

   .. code:: xml

      <metadata-charset>UTF-8</metadata-charset>

Same as above, but defines the charset of the metadata (i.e. id3 tags, Exif information, etc.). This can be overwritten by
the character set selection of the metadata library.

Magic File
^^^^^^^^^^

.. confval:: magic-file
   :type: :confval:`String`
   :required: false
   :default: `System default`

   .. code:: xml

      <magic-file>/path/to/my/magic-file</magic-file>

Specifies an alternative file for filemagic, containing mime type information.


*********
Scripting
*********

.. confval:: scripting
   :type: :confval:`Section`
   :required: false

   .. code:: xml

      <scripting script-charset="UTF-8" scan-mode="inotify">

Defines the scripting section.

Scripting Attributes
--------------------

Character Set
^^^^^^^^^^^^^

.. confval:: script-charset
   :type: :confval:`String`
   :required: false
   :default: ``UTF-8``

   .. code:: xml

      script-charset="Latin1"

Change character set for scripts.

Scan Mode for Script Folders
^^^^^^^^^^^^^^^^^^^^^^^^^^^^

.. confval:: scripting scan-mode
   :type: :confval:`Enum` (``manual|inotify|timed``)
   :required: false
   :default: ``manual``

   .. versionadded:: 2.6.1
   .. code:: xml

      scan-mode="inotify"

Set mode to rescan script-folders.

+---------+---------------------------------------------------------------+
| Mode    | Meaning                                                       |
+---------+---------------------------------------------------------------+
| manual  | folders are only rescanned on startup                         |
+---------+---------------------------------------------------------------+
| inotify | | changes are detected automatically and scripts are reloaded |
|         | | if gerbera is compiled with inotify support                 |
+---------+---------------------------------------------------------------+
| timed   | folder is rescanned after :confval:`scripting scan-interval`  |
+---------+---------------------------------------------------------------+

Timed Scan Interval
^^^^^^^^^^^^^^^^^^^

.. confval:: scripting scan-interval
   :type: :confval:`Time`
   :required: false
   :default: ``48:00``

   .. code:: xml

      scan-interval="24:00"

Set interval in minutes to rescan script-folders if :confval:`scripting scan-mode` is set to ``timed``.

Scripting Items
---------------

Below are the available scripting options:

Script Folders
^^^^^^^^^^^^^^

.. confval:: script-folder
   :type: :confval:`Section`
   :required: true
.. versionadded:: 2.0
..

   .. code:: xml

      <script-folder>
          <common>/usr/local/share/gerbera/js</common>
          <custom>/home/dev/Source/gerbera/scripts/js</custom>
      </script-folder>


Defines the locations of the script folders. If set, first all Javascript files from ``common`` are loaded and
then all Javascript files from ``custom``. If a function is defined in a common and a custom file the custom defintion
overwrites the common defintion. No function should be duplicate in the same folder as loading order is not defined.

Setting ``script-folder`` is the replacement for setting the various script files with the former option :confval:`common-script` and :confval:`import-script`.

.. confval:: script-folder common
   :type: :confval:`Path`
   :required: true
   :default: ``${prefix}/share/gerbera/js``, `where ${prefix} is your installation prefix directory.`

   .. code:: xml

      <common>/usr/local/share/gerbera/js</common>

Define the path to preinstalled script files. The files are part of the gerbera installation and should not be changed,
because they may be overwritten by an update.

.. confval:: script-folder custom
   :type: :confval:`Path`
   :required: false

   .. code:: xml

      <custom>/etc/gerbera/js</custom>

Path for custom layout import scripts. Functions defined in js files in this folder are added to the script library.

.. _import-function:

Layout Functions
^^^^^^^^^^^^^^^^

.. confval:: import-function
   :type: :confval:`Section`
   :required: true
.. versionadded:: 2.0
..

   .. code:: xml

       <import-function>
           <audio-file>importAudioInitial</audio-file>
           <video-file>importVideo</video-file>
           <image-file>importImage</image-file>
           <playlist create-link="yes">importPlaylist</playlist>
           <meta-file>importMetadata</meta-file>
       </import-function>

Set the entry points for the virtual layout functions and file parsing functions. Selecting the entry point is the replacement for setting
the layout type in :confval:`audio-layout`. The entry points are directly called for Gerbera code and must have a defined synopsis.
For further details see :ref:`Scripting <scripting>`.

.. confval:: audio-file
   :type: :confval:`String`
   :required: false
   :default: ``importAudio``

Name of the javascript function called to create the virtual layout for an audio file.
In addition to ``importAudio`` also ``importAudioStructured`` and ``importAudioInitial`` are part of gerbera installation.

.. confval:: video-file
   :type: :confval:`String`
   :required: false
   :default: ``importVideo``

Name of the javascript function called to create the virtual layout for a video file.

.. confval:: image-file
   :type: :confval:`String`
   :required: false
   :default: ``importImage``

Name of the javascript function called to create the virtual layout for an image file.

.. confval:: playlist
   :type: :confval:`String`
   :required: false
   :default: ``importPlaylist``

Name of the javascript function called to parse a playlist file.

   .. confval:: playlist create-link
          :type: :confval:`Boolean`
          :required: false
          :default: ``yes``
   ..

      .. code:: xml

         create-link="no"

    Links the playlist to the virtual container which contains the expanded playlist items. This means, that
    if the actual playlist file is removed from the database, the virtual container corresponding to the playlist will also be removed.

.. confval:: meta-file
   :type: :confval:`String`
   :required: false
   :default: ``importMetadata``

Name of the javascript function invoked during the first import phase to gather metadata from additional files.
Currently support for ``nfo`` files is implemented (https://kodi.wiki/view/NFO_files/Templates).

The search pattern to identify metadata files is set in :confval:`resources` section.

.. _virtual-layout:

Virtual Layout
^^^^^^^^^^^^^^

.. confval:: virtual-layout
   :type: :confval:`Section`
   :required: true

   .. code:: xml

      <virtual-layout type="builtin" audio-layout="Default">

Defines options for the virtual container layout; the so called ”virtual container layout” is the way how the
server organizes the media according to the extracted metadata.
For example, it allows sorting audio files by Album, Artist, Year and so on.

.. confval:: virtual-layout type
   :type: :confval:`Enum` (``builtin|js|disabled``)
   :required: false
   :default: ``js`` if gerbera was compiled with javascript support, ``builtin`` otherwise.

   .. code:: xml
   
       type="js"

Specifies what will be used to create the virtual layout, possible values are:

-  ``builtin``: a default layout will be created by the server
-  ``js``: a user customizable javascript will be used (Gerbera must be compiled with js support)
-  ``disabled``: only PC-Directory structure will be created, i.e. no virtual layout

Specifies the virtual layout to be created:

.. confval:: audio-layout
   :type: :confval:`String`
   :required: false
   :default: ``Default``
.. versionremoved:: 3.0.0 Migrate your configuration to use :ref:`import-function`.

-  **Default**: ``addAudio`` is used to create the virtual layout
-  **Structured**: ``addAudioStructured`` is used to create the virtual layout (only in combination with javascript)

.. confval:: video-layout
   :type: :confval:`String`
   :required: false
   :default: ``Default``
.. versionremoved:: 3.0.0 Migrate your configuration to use :ref:`import-function`.

.. confval:: image-layout
   :type: :confval:`String`
   :required: false
   :default: ``Default``
.. versionremoved:: 3.0.0 Migrate your configuration to use :ref:`import-function`.

The virtual layout can be adjusted using an import script which is defined as follows:

   .. code:: xml

      audio-layout="Default|Structured"
      video-layout="Default"
      image-layout="Default"

Layout Scripts (deprecated)
^^^^^^^^^^^^^^^^^^^^^^^^^^^

.. confval:: common-script
   :type: :confval:`Path`
   :required: false
   :default: ``${prefix}/share/gerbera/js/common.js``, `where ${prefix} is your installation prefix directory.`
.. versionremoved:: 3.0.0 Migrate your configuration to use :ref:`import-function`.
.. versionchanged:: 2.0 Was required until 2.0, if virtual layout type is "js".
..

   .. code:: xml

       <common-script>/path/to/my/common-script.js</common-script>

Points to the so called common script - it is a shared library of js helper functions.
For more details read :ref:`scripting <scripting>`

.. confval:: custom-script
   :type: :confval:`Path`
   :required: false
   :default: empty
.. versionremoved:: 3.0.0 Migrate your configuration to use :ref:`import-function`.
..

   .. code:: xml

      <custom-script>/path/to/my/custom-script.js</custom-script>

Points to the custom script - think of it as a custom library of js helper functions, functions added
there can be used in your import and in your playlist scripts. Theses functions also overwrite functions from the common script.
For more details read :ref:`scripting <scripting>`

.. confval:: import-script
   :type: :confval:`Path`
   :required: false
   :default: ``${prefix}/share/gerbera/js/import.js``, `where ${prefix} is your installation prefix directory.`
.. versionremoved:: 3.0.0 Migrate your configuration to use :ref:`import-function`.
.. versionchanged:: 2.0 Was required until 2.0, if virtual layout type is "js".
..

   .. code:: xml

      <import-script>/path/to/my/import-script.js</import-script>

Points to the script invoked upon media import. For more details read about :ref:`scripting <scripting>`.

.. confval:: playlist-script
   :type: :confval:`Path`
   :required: false
   :default:  ``${prefix}/share/gerbera/js/playlists.js``, `where ${prefix} is your installation prefix directory.`
.. versionremoved:: 3.0.0 Migrate your configuration to use :ref:`import-function`.
.. versionchanged:: 2.0 Was required until 2.0, if virtual layout type is "js".
..

   .. code:: xml

      <playlist-script create-link="yes">/path/to/my/playlist-script.js</playlist-script>

Points to the script that is parsing various playlists, by default parsing of pls, m3u and asx playlists is implemented,
however the script can be adapted to parse almost any kind of text based playlist. For more details read :ref:`scripting <scripting>`


   .. confval:: playlist-script create-link
          :type: :confval:`Boolean`
          :required: false
          :default: ``yes``
   ..

      .. code:: xml

         create-link="no"

    Links the playlist to the virtual container which contains the expanded playlist items. This means, that
    if the actual playlist file is removed from the database, the virtual container corresponding to the playlist will also be removed.


.. confval:: metafile-script
   :type: :confval:`Path`
   :required: false
   :default:  ``${prefix}/share/gerbera/js/metadata.js``, `where ${prefix} is your installation prefix directory.`
.. versionremoved:: 3.0.0 Migrate your configuration to use :ref:`import-function`.
.. versionchanged:: 2.0 Was required until 2.0, if virtual layout type is "js".
..

   .. code:: xml

      <metafile-script>/path/to/my/metadata-script.js</metafile-script>

Points to the main metadata parsing script which is invoked during the first import phase to gather metadata from additional files.
Currently support for nfo files is implemented (https://kodi.wiki/view/NFO_files/Templates).

The search pattern is set in resources section.

Structured Layout
^^^^^^^^^^^^^^^^^

.. confval:: structured-layout
   :type: :confval:`Section`
   :required: false

   .. code:: xml

      <structured-layout skip-chars="" div-char="-" />

Adjust layout of boxes for large collections in structured layout. Set :confval:`audio-file` to ``importAudioStructured`` and choose values best for your media library.

   .. confval:: div-char
      :type: :confval:`String`
      :required: false
      :default: ``-``
   ..

      .. code:: xml

         div-char="+"

   Symbols to use around the box text.

   .. confval:: skip-chars
      :type: :confval:`String`
      :required: false
      :default: ``-``
   ..

      .. code:: xml

        skip-chars="+-"

   Special characters in the beginning of a title that are not used for building a box.

.. _script-options:

Script Options
^^^^^^^^^^^^^^

.. confval:: script-options
   :type: :confval:`Section`
   :required: false

   .. code:: xml

       <script-options></script-options>

Contains options to pass into scripts. All values are available in scripts as e.g.
``config['/import/scripting/virtual-layout/script-options/script-option'].test``.
For more details see :ref:`scripting <scripting>`

**Child tags:**

.. confval:: script-option
   :type: :confval:`Section`
   :required: false

   .. code:: xml

      <script-option name="test" value="42"/>

Set option ``value`` for option ``name``.

   .. confval:: script-option name
      :type: :confval:`String`
      :required: true
   ..

      .. code:: xml

          name="test"

   Name of the option.

   .. confval:: script-option value
      :type: :confval:`String`
      :required: true
   ..


      .. code:: xml

         value="42"

    Value of the option.

Mapping for Genres
------------------

Media files are tagged with a variety of genres. The goal of genre-mapping is to move serveral of these original genres into the
same box instead of creating a box for each genre.

.. confval:: genre-map
   :type: :confval:`Section`
   :required: false

   .. code:: xml

       <genre-map>
          <genre from="Special Disco Mix" to="Pop"/>
       </genre-map>

Define mapping of genres to other text.
Genres is relevant for :confval:`virtual-layout type` ``builtin`` and ``js`` regardless of its position
in the configuration file.

Genre
^^^^^

.. confval:: genre
   :type: :confval:`Section`
   :required: false

   .. code:: xml

      <genre from="Special Disco Mix" to="Pop"/>

Replace genre ``from`` by genre ``to``.

   .. confval:: genre from
      :type: :confval:`String`
      :required: true
   ..

      .. code:: xml

          from="Special Disco Mix"

   Original genre value. Can be a regular expression.

   .. confval:: genre to
      :type: :confval:`String`
      :required: true
   ..

      .. code:: xml

         to="Pop"

   Target genre value.

.. _boxlayout:

Properties of Layout Boxes
--------------------------

.. confval:: boxlayout
   :type: :confval:`Section`
   :required: false
   :default: Extensible Default: Without ``extend="true"`` all missing entries are reported

   .. versionadded:: 2.0.0
   .. code:: xml

      <boxlayout extend="true">
         <box key="Audio/myBox" title="New" class="object.container.album.musicAlbum" size="8" enabled="yes" />
      </boxlayout>

Boxlayout is relevant for :confval:`virtual-layout type` ``builtin`` and ``js`` regardless of its position
in the configuration file.

Box Properties
^^^^^^^^^^^^^^

.. confval:: box
   :type: :confval:`Section`
   :required: false

   .. code:: xml

       <box key="Audio/myBox" title="New" class="object.container.album.musicAlbum" size="8" enabled="yes" />

Set properties for box.

   .. confval:: box key
      :type: :confval:`String`
      :required: true
   ..

      .. code:: xml

          key="Audio/audioRoot"

   Configure Box ``key``.
   The key that is used in javascript and builtin layout to refer the the config.

   .. confval:: box title
      :type: :confval:`String`
      :required: true
   ..

      .. code:: xml

         title="Music"

   The title to use for the respective box.

   .. confval:: box class
      :type: :confval:`String`
      :required: false
      :default: ``object.container``
   ..

      .. code:: xml

         class="object.container.album.musicAlbum"

   Set the upnp class for the respective box.

   .. confval:: box upnp-shortcut
      :type: :confval:`String`
      :required: false
      :default: empty
   ..

      .. versionadded:: 2.4.0
      .. code:: xml

         upnp-shortcut="MUSIC_ARTISTS"

   Set the upnp shortcut label for this box. The last container created based on this
   box will be added to the shortcuts feature field.
   For more details see UPnP-av-ContentDirectory-v4-Service, page 357.

   .. confval:: box sort-key
      :type: :confval:`String`
      :required: false
      :default: empty
   ..

      .. versionadded:: 2.6.0
      .. code:: xml

         sort-key="0123"

   Define special sorting key for the box instead of its title.

   .. confval:: box enabled
      :type: :confval:`Boolean`
      :required: false
      :default: ``yes``
   ..

      .. code:: xml

         enabled="no"

   Disable the respective box. Disabling boxes depends on support by the respective layout.

   .. confval:: box searchable
      :type: :confval:`Boolean`
      :required: false
      :default: ``yes``
   ..

      .. versionadded:: 3.0.0
      .. code:: xml

         searchable="no"

   Activate/deactivate box content for search. Especially relevant for artists, albums and genres.
   To avoid duplicate results in search only one box of each type should be searchable.
   The respective layout may overwrite the setting to enforce this rule.

   .. confval:: box size
      :type: :confval:`Interval`
      :required: false
      :default: ``1``
   ..

      .. code:: xml

         size="6"

   Type of the box in structured layout. The following values are supported

   -  ``1``: One large box
   -  ``2``: Two boxes with 13 items each
   -  ``3``: Boxes with 8, 9, 9 letters
   -  ``4``: Boxes with 7, 6, 7, 6 letters
   -  ``5``: Boxes with 5, 5, 5, 6, 5 letters
   -  ``6``: Boxes with 4, 5, 4, 4, 5, 4 letters
   -  ``7``: Boxes with 4, 3, 4, 4, 4, 3, 4 letters
   -  ``9``: Boxes with 5, 5, 5, 4, 1, 6 letters; a large box for T
   -  ``13``: Boxes with 2 letters each
   -  ``26``: A speparate box for each letter

Layout Sub Trees
^^^^^^^^^^^^^^^^

.. confval:: chain
   :type: :confval:`Section`
   :required: false

   .. versionadded:: 2.6.0
   .. code:: xml

      <chain type="audio|video|image">
        <link key=".." />
      </chain>

Define a user specific sub tree in virtual layout. Only available for ``js`` layout.

   .. confval:: chain type
      :type: :confval:`Enum` (``audio|video|image``)
      :required: true
   ..

      .. code:: xml

         type="audio|video|image"

      Set the import script type where the chain is added. Valid values are:
      ``audio``, ``video`` and ``image``.

**Child tags:**

.. confval:: link
   :type: :confval:`Section`
   :required: false

   .. code:: xml

       <link key="Audio/audioTest" title="obj.title" metaData="obj.metaData" />
       <link key="Audio/audioRoot"/>

Define or reference link in chain.

**Attributes:**

   .. confval:: link key
      :type: :confval:`String`
      :required: true
   ..

      .. code:: xml

         key="Audio/audioRoot"

   Either the key of an existing ``<box>`` or the identifier of a new box.

   .. confval:: link properties
      :type: :confval:`String`
      :required: false
   ..

      .. code:: xml

         title="obj.title"
         class="'object.container.genre.musicGenre'"
         upnpShortcut="''"
         sortKey="'0000' + obj.title"
         res="obj.res"
         aux="obj.aux"
         refID="obj.id"

   Set the properties of the container to be created. Property stateents
   are evaluated in javascript with ``obj`` and ``media`` objects as
   sources. Constant values must be enclosed in ticks ``''``.

   .. confval:: link metaData
      :type: :confval:`String`
      :required: false
   ..

      .. code:: xml

          metaData="obj.metaData"
          metaData="M_ALBUM: obj.metaData[M_ALBUM], M_ARTIST: obj.metaData[M_ALBUMARTIST]"

   Define the metadata of the new container. For metadata there is a second format for the
   properties: ``M_ALBUM: obj.metaData[M_ALBUM], M_ARTIST: obj.metaData[M_ALBUMARTIST]``.
   Individual metadata properties can be set and must be separated by commas. The list of
   available properties can be found in :ref:`scripting <scripting>`.


.. _autoscan:

********
Autoscan
********

.. confval:: autoscan
   :type: :confval:`Section`
   :required: false

   .. code:: xml

      <autoscan use-inotify="auto" inotify-attrib="yes">

Specifies a list of default autoscan directories.

This section defines persistent autoscan directories. It is also possible to define autoscan directories in the UI,
the difference is that autoscan directories that are defined via the config file can not be removed in the UI.
Even if the directory gets removed on disk, the server will try to monitor the specified location and will re add
the removed directory if it becomes available/gets created again.

Autoscan Attributes
-------------------

   .. confval:: use-inotify
      :type: :confval:`Enum` (``yes|no|auto``)
      :required: false
      :default: ``auto``
   ..

      .. code:: xml

         use-inotify="yes"

   Specifies if the inotify autoscan feature should be enabled. The default value is ``auto``, which means that
   availability of inotify support on the system will be detected automatically, it will then be used if available.
   Setting the option to 'no' will disable inotify even if it is available. Allowed values: "yes", "no", "auto"

   .. confval:: inotify-attrib
      :type: :confval:`Boolean`
      :required: false
      :default: ``no``
   ..

      .. versionadded:: 2.2.0
      .. code:: xml

         inotify-attrib="yes|no"

    Specifies if the inotify will also monitor for attribute changes like owner change or access given.

Autoscan Directory
------------------

.. confval:: directory
   :type: :confval:`Section`
   :required: false

   .. code:: xml

      <directory location="/media" mode="timed" interval="3600"
                 recursive="no" hidden-files="no"/>
      <directory location="/audio" mode="inotify"
                 recursive="yes" hidden-files="no"/>
      <directory location="/video" mode="manual"
                 recursive="yes" hidden-files="no"/>

Defines an autoscan directory and it's parameters.

Directory Attributes
^^^^^^^^^^^^^^^^^^^^

.. confval:: directory location
   :type: :confval:`Path`
   :required: true
..

   .. code:: xml

      location=...

Absolute path to the directory that shall be monitored.

.. confval:: directory mode
   :type: :confval:`Enum` (``inotify|timed|manual``)
   :required: true
..

   .. code:: xml

      mode="inotify|timed|manual"

The values have the following meaning:

- ``timed`` mode rescans the given directory in specified intervals
- ``inotify`` mode uses the kernel inotify mechanism to watch for filesystem events.
- ``manual`` requires user interaction on web ui.

.. confval:: directory interval
   :type: :confval:`Time`
   :required: true
..

   .. code:: xml

      interval="1500"

Scan interval in seconds. The value can be given in a valid time format.

.. confval:: directory recursive
   :type: :confval:`Boolean`
   :required: true
   :default: ``no``
..

   .. code:: xml

      recursive="yes"

Specifies if autoscan shall monitor the given directory including all sub directories.

.. confval:: directory dirtypes
   :type: :confval:`Boolean`
   :required: false
   :default: ``yes``
..

   .. versionadded:: 2.3.0
   .. code:: xml

      dirtypes="no"

Specifies if containers in PC Directory will get container types like albums based
on the majority of child items.

.. confval:: directory hidden-files
   :type: :confval:`Boolean`
   :required: false
   :default: `value specified in` :confval:`import hidden-files`
..

   .. code:: xml

      hidden-files="no"

Process hidden files, overrides the ``hidden-files`` value in the ``<import/>`` tag.

.. confval:: directory follow-symlinks
   :type: :confval:`Boolean`
   :required: false
   :default: `value specified in` :confval:`import follow-symlinks`
..

   .. versionadded:: 2.0.0
   .. code:: xml

      follow-symlinks="no"

Symbolic links should be treated as regular items and imported into the database (``yes``).
This can cause duplicate entries if the link target is also scanned
overrides the ``follow-symlinks`` value in the ``<import/>`` tag.

.. confval:: directory retry-count
   :type: :confval:`Integer` (non-negative)
   :required: false
   :default: ``0``
..

   .. versionadded:: 2.2.0
   .. code:: xml

      retry-count="3"

This attribute can be used to allow multiple attempts to access files
in case of mounted volumes. In some cases inotify events are raised
before the directory or file is fully available, causing an access
permission error and the import fails.
This attribute is only available in config.xml at the moment.

.. confval:: force-reread-unknown
   :type: :confval:`Boolean`
   :required: false
   :default: ``no``
..

   .. versionadded:: 2.5.0
   .. code:: xml

      force-reread-unknown="yes"

This attribute forces that files without changes are reread (on startup) if their upnp class is unset or "object.item".
This can happen if the first scan (e.g. via inotify) did not get all details of the file correctly.
This is mostly the case if the media folder is exported on the network and files are written via network.
Be aware that the startup will take longer if there is a large number of non-media files in the folder

.. confval:: media-type
   :type: :confval:`String`
   :required: false
   :default: ``Any``
..

   .. code:: xml
   
       media-type="Music|AudioBook"

Only import audio/image/video from directory to virtual layout if upnp class is subclass.
Values can be concatenated by ``|``. Allowed values are:

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

.. confval:: container-type-audio
   :type: :confval:`String`
   :required: false
   :default: ``object.container.album.musicAlbum``
..

.. confval:: container-type-image
   :type: :confval:`String`
   :required: false
   :default: ``object.container.album.photoAlbum``
..

.. confval:: container-type-video
   :type: :confval:`String`
   :required: false
   :default: ``object.container``
..

   .. code:: xml

      container-type-audio="object.container"
      container-type-image="object.container"
      container-type-video="object.container"

Set the default container type for virtual containers and for physical containers in grb-mode during import.
This is especially useful if the virtual layout simulates the filesystem structure and is not derived from metadata.
The first object that is added to the container determines the property (audio/image/video) used.

******************
System Directories
******************

.. confval:: system-directories
   :type: :confval:`Section`
   :required: false
   :default: Extensible Default, see :confval:`extend`

   .. code:: xml

      <system-directories>
         <add-path name="/sys"/>
      </system-directories>

Specifies a list of system directories hidden in filesystem web ui.

If the element does not exists, the default list of system directories is set to ``/bin, /boot, /dev, /etc, /lib, /lib32, /lib64, /libx32, /proc, /run, /sbin, /sys, /tmp, /usr, /var``.

Path
----

.. confval:: system-directories add-path
   :type: :confval:`Section`
   :required: false
..

   .. code:: xml

      <add-path name="/sys"/>

Defines a system directory.

   .. confval:: system-directories add-path name
      :type: :confval:`Path`
      :required: true
   ..

      .. code:: xml

         name="/sys"

   Absolute path to the directory that shall be hidden.


*******************
Visible Directories
*******************

.. confval:: visible-directories
   :type: :confval:`Section`
   :required: false
   :default: Extensible Default, see :confval:`extend`

   .. code:: xml

      <visible-directories>
          <add-path name="/home/media"/>
      </visible-directories>

Specifies a list of system directories visible in filesystem web ui. It can contain any path which is accessible by the gerbera server.

If the element exists it supercedes :confval:`system-directories`, i.e., only visible directories can be selected in web ui.
This is the more forward way of defining content but cannot be defaulted.

Path
----

.. confval:: visible-directories add-path
   :type: :confval:`Section`
   :required: false
..

   .. code:: xml
 
       <add-path name="/home/media"/>

Defines a visible directory.

The attributes specify various options:

   .. confval:: visible-directories add-path name
      :type: :confval:`Path`
      :required: true
   ..

      .. code:: xml

         name="/home/media"

   Absolute path to the directory that shall be visible.


**********************
Virtual Layout Details
**********************

.. confval:: layout
   :type: :confval:`Section`
   :required: false
..

   .. code:: xml

      <layout parent-path="yes">
        <path from="Videos/Action" to="Action-Videos"/>
      </layout>

Defines various layout options for generated virtual layout.

   .. confval:: parent-path
      :type: :confval:`Boolean`
      :required: true
      :default: ``no``
   ..

      .. code:: xml

         parent-path="yes"

   Values of ``yes`` or ``no`` are allowed, specifies if parent path is added to virtual layout.
   If set to ``no`` "/home/.../Videos/Action/a.mkv" with rootpath "/home/.../Videos" becomes "Action"
   otherwise "Videos/Action". Setting to ``yes`` produces the layout of gerbera before version 1.5.

Path
----

.. confval:: layout path
   :type: :confval:`Section`
   :required: false
..

   .. code:: xml

      <path from="Videos/Action" to="Action-Videos"/>

Map a virtual path element. This allows reducing path elements or merging different sources into a common tree.
Thema replacement is executed after calculation of virtual layout, i.e. after builtin or js layout script.

   .. confval:: path from
      :type: :confval:`String`
      :required: true
   ..

      .. code:: xml

         from="..."

   Source path. Can be a regular expression.

   .. confval:: path to
      :type: :confval:`String`
      :required: true
   ..

      .. code:: xml

         to="..."

   Target path. ``/`` can be used to create sub structure.


**************
Resource Files
**************

.. confval:: resources
   :type: :confval:`Section`
   :required: false
..

   .. code:: xml

      <resources case-sensitive="yes">

Defines various resource options for file based resources. Older versions of Gerbera added sereral files automatically.
For performance reasons no pattern is added by default anymore. You can set up your correct fanart file by yourself,
if no image is embedded in your media. If you have subtitle files, add the correct pattern, also.

   .. confval:: case-sensitive
      :type: :confval:`Boolean`
      :required: false
      :default: ``yes``
   ..

      .. code:: xml

         case-sensitive="no"

   This attribute defines whether search patterns are treated case sensitive or not, i.e. if set to ``no`` the file name
   ``cover.png`` matches anything like ``Cover.PNG`` or ``cover.PNG``.

Resource Order
--------------

.. confval:: order
   :type: :confval:`Section`
   :required: false
..

    .. code:: xml

        <order>...</order>

Define the order in which the metadata is rendered in the output

Handler
^^^^^^^

.. confval:: handler
   :type: :confval:`Section`
   :required: false
..

   .. code:: xml

      <handler name="Fanart"/>

   .. confval:: handler name
      :type: :confval:`String`
      :required: true
   ..

      .. code:: xml

         name="..."


    Valid handler names are ``Default``, ``LibExif``, ``TagLib``, ``Transcode``, ``Fanart``, ``Exturl``, ``MP4``, ``FFmpegThumbnailer``, ``Flac``, ``Matroska``, ``Subtitle``, ``Resource``, ``ContainerArt``

Resource Files
--------------

.. confval:: container
   :type: :confval:`Section`
   :required: false
   :default: Extensible Default, see :confval:`extend`
.. confval:: fanart
   :type: :confval:`Section`
   :required: false
   :default: Extensible Default, see :confval:`extend`
.. confval:: subtitle
   :type: :confval:`Section`
   :required: false
   :default: Extensible Default, see :confval:`extend`
.. confval:: metafile
   :type: :confval:`Section`
   :required: false
   :default: Extensible Default, see :confval:`extend`
.. confval:: resource
   :type: :confval:`Section`
   :required: false
   :default: Extensible Default, see :confval:`extend`

   .. code:: xml

      <container location="images" parentCount="2" minDepth="2">
         <add-file name="folder.jpg" />
         <add-file name="poster.jpg" />
         <add-file name="cover.jpg" />
         <add-file name="albumartsmall.jpg" />
      </container>
      <fanart>
         <add-dir name="." pattern="%filename%" mime="image/*" />
         <add-file name="%title%.jpg" />
         <add-file name="%filename%.jpg" />
         <add-file name="%album%.jpg" />
      </fanart>
      <subtitle>
         <add-dir name="." pattern="%filename%" mime="application/x-srt" />
      </subtitle>
      <metafile>...</metafile>
      <resource>...</resource>

Define file patterns to search for fanart, subtitle, metafiles and resources respectivly.

``container``, ``fanart``, ``metafile`` and ``subtitle`` patterns are used to identify external resources which are added to each item if the pattern matches.

``resource`` patterns are used to trigger rescan of the whole directory if such a file was changed, added or removed.

Each of these tags can contain multiple ``add-file`` or ``add-dir`` entries. ``container`` has additional attributes.

Container
^^^^^^^^^

Set up container images. The fanart of a media file is added automatically as a thumbnail to the container (e.g. the album container).
The setting depends on the chosen layout and is only fully respected if the layout script does not set own properties (which was the case in older javascript layouts).

.. confval:: resource container location
   :type: :confval:`Path`
   :required: false
   :default: `empty`
..

   .. code:: xml

      location="/mnt/images"

Path to the directory containing the images to load. Relative paths are assumed to be under the server's home.
Drop your artists' images or logos for default containers here and they are displayed as thumbnail when browsing with a compatible client.
If the image is not found in that location, it is also searched in the physical folder itself.

.. confval:: parentCount
   :type: :confval:`Integer`
   :required: false
   :default: ``2``
..

   .. code:: xml

      parentCount="4"

This setting allows to increase the number of levels which the fanart of a media file can be propagated upwards (examples refer to basic layout /Root/Audio/Artist/Album/song).
A value of 1 adds the fanart only to the direct parent container when a media file is added (e.g. the Album container).
A value of 2 means you propagate that image to the parent container as well (e.g. the Artist container).
A value of 0 blocks propagation completely.

.. confval:: minDepth
   :type: :confval:`Integer`
   :required: false
   :default: ``2``
..

   .. code:: xml

       minDepth="1"

Depending on the virtual layout propagating thumbnails can reach containers like Video or Audio. This settings forces a minimal depth for propagation to apply.
It is setting the minimum number of path elements for container using fanart from media files (e.g. /Root/Audio/Artist has level 3 so the image can be set).


Resource File Pattern
^^^^^^^^^^^^^^^^^^^^^

File patterns can be configured for each resource type.

.. confval:: resource add-file
   :type: :confval:`Section`
   :required: false
..

   .. code:: xml

      <add-file name="cover.png"/>
      <add-file name="%filename%.srt"/>

   .. confval:: resource add-file name
      :type: :confval:`String`
      :required: true
   ..

      .. code:: xml

         name="..."

   Add file search pattern to resource handler. The search pattern can contain variables:

   - ``%album%``: Value of the album tag
   - ``%albumArtist%``: Value of the albumArtist tag
   - ``%artist%``: Value of the artist tag
   - ``%filename%``: Name of the file without extension or name of the container
   - ``%genre%``: Value of the genre tag
   - ``%title%``: Value of the title tag
   - ``%composer%``: Value of the composer tag

Resource Directory Pattern
^^^^^^^^^^^^^^^^^^^^^^^^^^

.. confval:: resource add-dir
   :type: :confval:`Section`
   :required: false
..

   .. code:: xml

      <add-dir name="/data/subtitles/%title%" ext="srt"/>
      <add-dir name="/data/subtitles" ext="%title%*.srt"/>
      <add-dir name="%filename%" ext="srt"/>
      <add-dir pattern="%filename%*" ext="srt"/>
      <add-dir name="." pattern="%filename%*" mime="image/*"/>

   .. confval:: resource add-dir name
      :type: :confval:`String`
      :required: true
   ..

      .. code:: xml

         name="..."

   Add directory search pattern to resource handler. The search pattern can contain the same variables as :confval:`resource add-file name`.
   If the directory is relative the file is searched in a subdirectory of the directory containing the media file.
   If the directory is empty or just ``.`` it is replaced by the directory of the media file.

   .. confval:: resource add-dir ext
      :type: :confval:`String`
      :required: false
   ..

      .. code:: xml

         ext="..."

   Define the extension or file name pattern. The search pattern can contain the same variables as :confval:`resource add-file name`.
   If it does not contain a ``.`` it is considered as extension.
   If it contains a ``.`` the part before can contain ``*`` and ``?`` as wildcards and must exactly match the resource file name.

   .. confval:: resource add-dir pattern
      :type: :confval:`String`
      :required: false
   ..

      .. code:: xml

         pattern="..."

   Define the file name pattern in the directory. The search pattern can contain the same variables as :confval:`resource add-file name`.
   It can contain ``*`` and ``?`` as wildcards and must exactly match the resource file name.

   .. confval:: resource add-dir mime
      :type: :confval:`String`
      :required: false
   ..

      .. code:: xml

         mime="image/*"

   Define the mime type to search for with pattern as name. Specifying a mime type allows omitting the extension in conjunciton with
   :ref:`extension-to-mimetype-mapping <extension-mimetype>`.


A sample configuration would be:

.. code-block:: xml

    <resources case-sensitive="no">
        <fanart>
            <add-file name="cover.png"/>
            <add-dir name="." pattern="%filename%*" mime="image/*"/>
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

********
Mappings
********

.. confval:: mappings
   :type: :confval:`Section`
   :required: false
..

   .. code:: xml

      <mappings>...</mappings>

Defines various mapping options for importing media, currently two subsections are supported.

This section defines mime type and upnp:class mappings, it is vital if filemagic is not available - in this case
media type auto detection will fail and you will have to set the mime types manually by matching the file extension.
It is also helpful if you want to override auto detected mime types or simply skip filemagic processing for known file types.


Ignored Extensions
------------------
.. confval:: ignore-extensions
   :type: :confval:`Section`
   :required: false
..

.. code:: xml

    <ignore-extensions>
       <add-file name="part"/>
    </ignore-extensions>

This section holds the extensions to exclude from metdata handling.

Extension
^^^^^^^^^
.. confval:: ignore-extensions add-file
   :type: :confval:`Section`
   :required: false
..

   .. code:: xml

      <add-file name="part"/>

   .. confval:: ignore-extensions add-file name
      :type: :confval:`String`
      :required: true
   ..

      .. code:: xml

         name="..."

Specifies a file name extension (everything after the last dot ".") to ignore.

Note:
    This improves the import speed, because files are ignored completely.

Note:
    The extension is case sensitive, if :confval:`extension-mimetype case-sensitive` is set to ``yes``

.. _extension-mimetype:

Map Extension to Mimetype
-------------------------

.. confval:: extension-mimetype
   :type: :confval:`Section`
   :required: false
   :default: Extensible default, see :confval:`extend`
..

   .. code:: xml

      <extension-mimetype ignore-unknown="no" case-sensitive="no">
          <map from="mp3" to="audio/mpeg"/>
      </extension-mimetype>

This section holds the file name extension to mime type mappings.

Attributes
^^^^^^^^^^

.. confval:: extension-mimetype ignore-unknown
   :type: :confval:`String`
   :required: true
   :default: ``no``
..

   .. code:: xml

      ignore-unknown="yes"

If ignore-unknown is set to "yes", then only the extensions that are listed in this section are imported.

.. confval:: extension-mimetype case-sensitive
   :type: :confval:`String`
   :required: true
   :default: ``no``
..

   .. code:: xml

      case-sensitive="yes"

Specifies if extensions listed in this section are case sensitive.

Extension Map
^^^^^^^^^^^^^

.. confval:: extension-mimetype map
   :type: :confval:`Section`
   :required: false
..

   .. code:: xml

      <map from="mp3" to="audio/mpeg"/>

   .. confval:: extension-mimetype map from
      :type: :confval:`String`
      :required: true
   ..

      .. code:: xml

         from="mp3"

   .. confval:: extension-mimetype map to
      :type: :confval:`String`
      :required: true
   ..

      .. code:: xml

         to="audio/mpeg"

Specifies a mapping from a certain file name extension (everything after the last dot ".") to mime type.

Note:
    This improves the import speed, because invoking libmagic to discover the right mime type of a file is
    omitted for files with extensions listed here.

Note:
    The extension is case sensitive, if :confval:`extension-mimetype case-sensitive` is set to ``yes``


Map Mimetype to Upnpclass
-------------------------

.. confval:: mimetype-upnpclass
   :type: :confval:`Section`
   :required: false
   :default: Extensible default, see :confval:`extend`
..

   .. code:: xml

      <mimetype-upnpclass>
         <map from="audio/*" to="object.item.audioItem.musicTrack"/>
      </mimetype-upnpclass>

This section holds the mime type to upnp:class mappings.

Mimetype Map
^^^^^^^^^^^^

.. confval:: mimetype-upnpclass map
   :type: :confval:`Section`
   :required: false
..

   .. code:: xml

      <map from="audio/*" to="object.item.audioItem.musicTrack"/>

   .. confval:: mimetype-upnpclass map from
      :type: :confval:`String`
      :required: true
   ..

      .. code:: xml

         from="audio/*"

    Set source mimetype.

   .. confval:: mimetype-upnpclass map to
      :type: :confval:`String`
      :required: true
   ..

      .. code:: xml

         to="object.item.audioItem.musicTrack"

    Set target UPnPClass.

Specifies a mapping from a certain mime type to ``upnp:class`` in the Content Directory. The mime type can either be
entered explicitly ``audio/mpeg`` or using a wildcard after the slash ``audio/*``.
The values of :confval:`mimetype-upnpclass map from` and :confval:`mimetype-upnpclass map to`
attributes are case sensitive.

For detailled mapping the :confval:`mimetype-upnpclass map from` attribute can specify further filtering criteria like ``upnp:genre=Book`` which is
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


Map Mimetype to DLNA TransferMode
---------------------------------

.. confval:: mimetype-dlnatransfermode
   :type: :confval:`Section`
   :required: false
   :default: Extensible default, see :confval:`extend`
..

   .. code:: xml

      <mimetype-dlnatransfermode>
       <map from="audio/*" to="Streaming"/>
      <mimetype-dlnatransfermode>

This section holds the mime type to dlna transfer mode mappings. It is added to the http-header ``transferMode.dlna.org`` of the file request.

Mimetype Map
^^^^^^^^^^^^

.. confval:: mimetype-dlnatransfermode map
   :type: :confval:`Section`
   :required: false
..

   .. code:: xml

      <map from="audio/*" to="Streaming"/>
      <map from="video/*" to="Streaming"/>
      <map from="image/*" to="Interactive"/>
      <map from="text/*" to="Background"/>

   .. confval:: mimetype-dlnatransfermode map from
      :type: :confval:`String`
      :required: true
   ..

      .. code:: xml

         from="audio/*"

    Set source mimetype.

   .. confval:: mimetype-dlnatransfermode map to
      :type: :confval:`Enum` (``Streaming|Interactive|Background``)
      :required: true
   ..

      .. code:: xml

         to="Streaming"

    Set target DLNA transfermode.

Specifies a mapping from a certain mime type to transfer mode. The mime type can either be
entered explicitly "audio/mpeg" or using a wildcard after the slash ``audio/*``.
The values of :confval:`mimetype-dlnatransfermode map from` and :confval:`mimetype-dlnatransfermode map to`
attributes are case sensitive.

Map Mimetype to Contenttype
---------------------------

.. confval:: mimetype-contenttype
   :type: :confval:`Section`
   :required: false
   :default: Extensible default, see :confval:`extend`
..

   .. code:: xml

      <mimetype-contenttype>
        <treat mimetype="audio/mpeg" as="mp3"/>
      </mimetype-contenttype>

This section makes sure that the server knows about remapped mimetypes and still extracts the metadata correctly.
If the user remaps mp3 files to a different mimetype, we must know about it so we can still pass this item to taglib
for metadata extraction.

Note:
  If this section is not present in your config file, the defaults will be filled in automatically.
  However, if you add an empty tag, without defining the following ``<treat>`` tags, the server assumes that
  you want to have an empty list and no files will be process by the metadata handler.


Map Mimetype
^^^^^^^^^^^^

.. confval:: mimetype-contenttype treat
   :type: :confval:`Section`
   :required: false
..

   .. code:: xml

      <treat mimetype="audio/mpeg" as="mp3"/>

Tells the server what content the specified mimetype actually is in order to pick the correct DLNA profile.

   .. confval:: mimetype-contenttype treat mimetype
      :type: :confval:`String`
      :required: true
   ..

      .. code:: xml

         mimetype="audio/mpeg"

    Set source mimetype.

   .. confval:: mimetype-contenttype treat as
      :type: :confval:`String`
      :required: true
   ..

      .. code:: xml

         as="mp3"

    Set target content type.

    It makes no sense to use values that are not defined in :confval:`contenttype-dlnaprofile`.
    The attribute can have addional values:

    - ``playlist``: The content is a playlist and should be processed by the playlist parser script.

.. _contenttype-dlnaprofile:

Map Contenttype to DLNA Profile
-------------------------------

.. confval:: contenttype-dlnaprofile
   :type: :confval:`Section`
   :required: false
   :default: Extensible default, see :confval:`extend`
..

   .. code:: xml

      <contenttype-dlnaprofile>
        <map from="mp4" videoCodec="h264" audioCodec="aac" to="AVC_MP4_MP_HD_720p_AAC"/>
      </contenttype-dlnaprofile>

This section holds the content type to dlnaprofile mappings.

Map Content Type
^^^^^^^^^^^^^^^^

.. confval:: contenttype-dlnaprofile map
   :type: :confval:`Section`
   :required: false
..

   .. code:: xml

      <map from="mp4" to="AVC_MP4_BL_CIF30_AAC_MULT5"/>
      <map from="mp4" videoCodec="h264" audioCodec="aac" to="AVC_MP4_MP_HD_720p_AAC"/>

Specifies a mapping from a certain content type to a dlna profile in the Content Directory.
In order to access special profiles you can specify a resource attribute with its required value. If multiple entries for the same
mimetype exist, mappings with more details are preferred to simple from-to mappings.
Resource attributes can be seen in the details page for an item on the web UI. The value must either match exactly the transformed value (incl. unit) or the raw value.

   .. confval:: contenttype-dlnaprofile map from
      :type: :confval:`String`
      :required: true
   ..

      .. code:: xml

         from="mp4"

    Set source content type.

   .. confval:: contenttype-dlnaprofile map to
      :type: :confval:`String`
      :required: true
   ..

      .. code:: xml

         to="AVC_MP4_MP_HD_720p_AAC"

    Set target DLNA profile.

Profile Catalog:
   If you want to make your DLNA device play specific media the following list of profiles may be helpful:

   .. literalinclude:: ./profiles.xml
      :language: xml

.. Note::

    Feel free to provide us improvements to this list!


.. _virtual-directories:

*******************
Virtual Directories
*******************

.. confval:: virtual-directories
   :type: :confval:`Section`
   :required: false
   :default: Extensible default, see :confval:`extend`
..

.. code:: xml

   <virtual-directories>
      <key metadata="M_ALBUMARTIST" class="object.container.album.musicAlbum"/>
      <key metadata="M_UPNP_DATE" class="object.container.album.musicAlbum"/>
      <key metadata="LOCATION" class="object.container.album.musicAlbum"/>
      <key metadata="M_ARTIST_1" class="object.container.album.musicAlbum"/>
   </virtual-directories>

This section holds the additional identifiers for virtual directories to make sure, e.g. albums with the same title
are distiguished by their artist even if the displayed text is identical.

Directory Identifier
--------------------

.. confval:: virtual-directories key
   :type: :confval:`Section`
   :required: false
..

   .. code:: xml

      <key metadata="M_ARTIST" class="object.container.album"/>

Add additional identifier for directory of upnp class.

   .. confval:: virtual-directories key metadata
      :type: :confval:`String`
      :required: true
   ..

      .. code:: xml

         metadata="M_ARTIST_1"

   Specifies the metadata field to add for identification internally. For a list of valid metadata see :ref:`Metadata <upnp-tags>`.
   If ``_1`` is added to the name of the key only the first item in the list is picked (if there are multiple like for ``M_ARTIST``).
   In addition ``LOCATION`` references to the location property retrieved from the layout script.

   .. confval:: virtual-directories key class
      :type: :confval:`String`
      :required: false
      :default: `empty`
   ..

      .. code:: xml

         class="object.container.album"

   Restrict the key to containers of the given upnp class.


.. _library-options:

**********************
Import Library Options
**********************

.. confval:: library-options
   :type: :confval:`Section`
   :required: false

   .. code:: xml

      <library-options>...</library-options>

This section holds options for the various supported import libraries, it is useful in conjunction with virtual
container scripting, but also allows to tune some other features as well.

Currently the :confval:`library-options` allow additional extraction of the so called auxilary data (explained below) and
provide control over the video thumbnail generation.

Here is some information on the auxdata: UPnP defines certain tags to pass along metadata of the media
(like title, artist, year, etc.), however some media provides more metadata and exceeds the scope of UPnP.
This additional metadata can be used to fine tune the server layout, it allows the user to create a more
complex container structure using a customized import script. The metadata that can be extracted depends on the
library, currently we support **taglib**, **ffmpeg** and **libexif**, **matroska**, **wavpack** and **exiv2**
(if compiled with respective library enabled) which provide a default set of keys
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

General Attributes
------------------

.. confval:: multi-value-separator
   :type: :confval:`String`
   :required: false
   :default: ``"; "``

   .. code:: xml

      multi-value-separator="/ "

This string is used to join multi-valued items (e.g. Composer, Performer) into one string.

.. confval:: legacy-value-separator
   :type: :confval:`String`
   :required: false
   :default: `empty`

    .. code:: xml

        legacy-value-separator="/"

This string is used to split items into lists before joining them with multi-value-separator.
This option can be used to import files from legacy tools which did not support multi-valued items.
The empty string is used to disable legacy handling.


Library Settings
----------------

Gerbera imports a set of common tags by default in order to populate UPnP content.
If you need further properties there are two options

* :confval:`auxdata` : Read the value in order to use it in an import script
* :confval:`metadata` : Read value into in order to send it as UPnP property

The following library sections can contain both of these entries as well as
a :confval:`library charset` and a :confval:`library enabled` attribute:

- :confval:`id3`: Configure ``taglib`` settings used for audio file analysis.
- :confval:`libexiv`: Configure ``libexiv`` settings used for image file analysis.
- :confval:`evix2`: Configure ``libexiv2`` settings used for image file analysis.
- :confval:`wavpack`: Configure ``libwavpack`` settings used for enhanced wavpack file analysis.
- :confval:`ffmpeg`: Configure ``ffmpeg`` settings used for audio and video file analysis.
- :confval:`mkv`: Configure ``libmatroska`` settings used for enhanced mkv file analysis.

Character Set
^^^^^^^^^^^^^

.. confval:: library charset
   :type: :confval:`String`
   :required: false
   :default: `value of` :confval:`metadata-charset`

    .. code:: xml

        charset="Latin1"

Overwrite the :confval:`metadata-charset` for the respective type of file.

Enable Library
^^^^^^^^^^^^^^

.. confval:: library enabled
   :type: :confval:`Boolean`
   :required: false
   :default: ``yes``

    .. code:: xml

        enabled="no"

Exclude the metadata parser from the import process.


Additional Library Data
-----------------------

.. confval:: auxdata
   :type: :confval:`Section`
   :required: false
..

   .. code-block:: xml

      <auxdata>
         <add-data tag="tag1"/>
      </auxdata>

Auxdata can be read by the import javascript as ``aux`` to gain more control over the media structure.
The available tags depend on the respective library.

Add Tags
^^^^^^^^

.. confval:: auxdata add-data
   :type: :confval:`Section`
   :required: false
..

   .. code-block:: xml

      <add-data tag="tag1"/>
      <add-data tag="tag2"/>

   .. confval:: auxdata add-data tag
      :type: :confval:`String`
      :required: true
   ..

      .. code-block:: xml

         tag="tag1"

If the library was able to extract the data according to the given keyword, it will be added to auxdata.
You can then use that data in your import scripts.

Metadata Assignment
-------------------

.. confval:: metadata
   :type: :confval:`Section`
   :required: false
..

   .. code-block:: xml

      <metadata>
         <add-data tag="tag3" key="upnp:Key"/>
      </metadata>

Metadata can be read by the import javascript as ``meta`` to gain more control over the media structure and is automatically added to the UPnP output.

Map Tags
^^^^^^^^

.. confval:: metadata add-data
   :type: :confval:`Section`
   :required: false
..

   .. code-block:: xml

      <add-data tag="tag3" key="upnp:Key"/>

If the library was able to extract the data according to the given keyword, it will be added to metadata.

   .. confval:: metadata add-data tag
      :type: :confval:`String`
      :required: true
   ..

      .. code-block:: xml

         tag="tag3"

   Extract tag from media file.

   .. confval:: metadata add-data key
      :type: :confval:`String`
      :required: true
   ..

      .. code-block:: xml

         key="upnp:Key"

   Sets the UPnP meta property.

Fabricate Comment
-----------------

.. confval:: comment
   :type: :confval:`Section`
   :required: false
..

   .. versionadded:: 2.5.0
   .. code-block:: xml

      <comment enabled="yes"> ... </comment>

Fabricate a comment (description) from metadata. The comment will only be created if there is no description set and in the mechanism is ``enabled``.

.. confval:: comment enabled
   :type: :confval:`Boolean`
   :required: false
   :default: ``no``

   .. code:: xml

      enabled="yes"

Enable fabricating comment by the library code.

Comment Detail
^^^^^^^^^^^^^^

.. confval:: library comment
   :type: :confval:`Section`
   :required: false
..

   .. code-block:: xml

      <add-comment label="My favourite Detail" tag="MEDIA_TAG"/>

If the library was able to extract the data according to the given tag, it will be added to the comment.

   .. confval:: library comment label
      :type: :confval:`String`
      :required: true
   ..

      .. code-block:: xml

         label="My favourite Detail"

   Set the label shown before the tag value.

   .. confval:: library comment tag
      :type: :confval:`String`
      :required: true
   ..

      .. code-block:: xml

         tag="MEDIA_TAG"

   Set the media property.


Library Sections
----------------

LibExif
^^^^^^^

.. confval:: libexiv
   :type: :confval:`Section`
   :required: false
..

   .. code-block:: xml

      <libexif>...</libexif>

Configure ``libexiv`` settings used for image file analysis.

For a list of keywords/tags see the libexif documentation.

A sample configuration for the example described above would be:

.. code-block:: xml

   <libexif>
       <auxdata>
           <add-data tag="EXIF_TAG_MODEL"/>
       </auxdata>
   </libexif>


Taglib
^^^^^^

.. confval:: id3
   :type: :confval:`Section`
   :required: false
..

   .. code-block:: xml

      <id3>...</id3>

Configure ``taglib`` settings used for audio file analysis.

The keywords are those defined in the specifications, e.g.
`ID3v2.4 <https://id3.org/id3v2.4.0-frames>`__ or `Vorbis comments. <https://www.xiph.org/vorbis/doc/v-comment.htm>`__
We do not perform any extra checking, so you could try to use any string as a keyword - if it does not exist in the tag
nothing bad will happen.

Here is a list of some extra keywords not beeing part of UPnP:

- **ID3v2.4 / MP3**: TBPM, TCOP, TDLY, TENC, TEXT, TFLT, TIT1, TIT3, TKEY, TLAN, TLEN, TMCL, TMED, TOAL, TOFN, TOLY, TOPE, TOWN, TPE4, TPOS, TPUB, TRSN, TRSO, TSOA, TSRC, TSSE, TXXX:Artist, TXXX:Work, ...

- **Vorbis / FLAC**: ALBUMSORT, ARTISTS, CATALOGNUMBER, COMPOSERSORT, ENCODEDBY, LYRICIST, ORIGINALDATE, PRODUCER, RELEASETYPE, REMIXER, TITLESORT, WORK, ...

- any other user defined keyword, for APEv2 or iTunes MP4, see e.g. `table of mapping <https://picard.musicbrainz.org/docs/mappings>`__ between various tagging formats at MusicBrainz.

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


FFMpeg
^^^^^^

.. confval:: ffmpeg
   :type: :confval:`Section`
   :required: false
..

   .. code-block:: xml

      <ffmpeg artwork-enabled="yes">...</ffmpeg>

Configure ``ffmpeg`` settings used for audio and video file analysis.

`This page <https://wiki.multimedia.cx/index.php?title=FFmpeg_Metadata>`__
documents all of the metadata keys that FFmpeg honors, depending on the format being encoded.

   .. confval:: artwork-enabled
      :type: :confval:`Boolean`
      :required: false
      :default: ``no``

      .. versionadded:: 2.6.0
      .. code-block:: xml

         artwork-enabled="yes"

Create a separate resource for the artwork if detected by ffmpeg instead of merging
the attributes into the existing thumbnail resource (e.g. created by taglib)

   .. confval:: subtitle-seek-size
      :type: :confval:`Integer`
      :required: false
      :default: ``2048``

      .. versionadded:: 3.0.0
      .. code-block:: xml

         subtitle-seek-size="4096"

Number of bytes to read from a subtitle stream to identify the mime-type of an internal subtitle.

A sample configuration for the example described above would be:

.. code-block:: xml

   <ffmpeg artwork-enabled="yes">
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


Exiv2
^^^^^

.. confval:: evix2
   :type: :confval:`Section`
   :required: false
..

   .. code-block:: xml

      <exiv2>...</exiv2>

Configure ``libexiv2`` settings used for image file analysis.

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


Matroska
^^^^^^^^

.. confval:: mkv
   :type: :confval:`Section`
   :required: false
..

   .. code-block:: xml

      <mkv>...</mkv>

Configure ``matroska`` aka ``mkv`` settings used for enhanced mkv file analysis.


WavPack
^^^^^^^

.. confval:: wavpack
   :type: :confval:`Section`
   :required: false
..

   .. code-block:: xml

      <wavpack>...</wavpack>

Configure ``libwavpack`` settings used for enhanced wavpack file analysis.
