.. _layout:
.. index:: Import
.. index:: Layout

Import and Layout
=================

Autoscan
~~~~~~~~

The typical way to get you files loaded into the library is by defining autoscan folder either via web UI (:ref:`Edit Dialog<autoscan-edit>`)
or via (:ref:`Autoscan Configuration <autoscan>`). On startup each autoscan directory is fully scanned for new or changed files. Depending
on the type the scan is repeated after the set interval (timed autoscan) or new files are imported as soon as they show up on the disk
(inotify autoscan; only if compiled in and support by the operating system). In both cases the file is created in `PC Directory` and the
virtual layout is created or updated.

There also is the option to overwrite autoscan settings for subfolders (:ref:`Tweak Dialog<tweak-edit>`) to change the expected character
encoding or other import flags. Tweaking only is effective for the next scan, i.e. you have to force a rescan of the folder by deleting the
respective `PC Directory` entries.

PC Directory
~~~~~~~~~~~~

``PC Directory`` is simply a special non-removable container. Any media file added will have an entry inside the 
PC Directory tree. PC Directory's hierarchy reflects the file system hierarchy, all objects inside the PC Directory
including itself are NON-VIRTUAL objects.

All other entries are virtual objects and may have a different title, description, etc., but they are still referencing
to objects in the PC-Directory. That's why it is not possible to change the phyiscal location property of a virtual object 
- the only exceptions are URL items.

If an entry is deleted from the PC Directory all its referenes in the virtual structure are deleted as well.

If your virtual layout is the only thing needed, the PC Directory can be hidden completely from browsing.


Virtual Layout Basics
~~~~~~~~~~~~~~~~~~~~~

This section will give you some overview on how virtual objects work.

Understanding Virtual Objects
-----------------------------

When you add a file or directory to the database several things happen.

1. The object is inserted into the ``PC Directory``.

2. Once an item is added to the PC Directory it is forwarded to the virtual object engine. The virtual object engine's
   mission is to organize and present the media database in a logical hierarchy based on the available metadata of the
   items.

Each UPnP server implements this so called virtual object hierarchy in a different way. Files are usually sorted by
artist, album, some servers may just present a view similar to the file system and so on. In Gerbera we try to allow 
changing this by a scriptable virtual object engine.

Theory of Operation
-------------------

After an item is added to the PC Directory it is automatically fed as input to the layout creation. This creates one
or more virtual items for the given original item. Items created from layout are always marked virtual.

When a playlist item is encountered, it is automatically fed as input to the playlist script. The playlist script
attempts to parse the playlist and adds new item to the database, the item is then processed by the import script.

Updating items only works in import-mode ``grb``.


Virtual Layout Options
~~~~~~~~~~~~~~~~~~~~~~

The type of virtual layout can be selection by option :ref:`virtual-layout <virtual-layout>`.

Builtin Layout
--------------

In order to provide the basic virtual layout you can find elsewhere and for really low key systems.
Builtin layout is hardcoded and cannot be changed without a new gerbera version but it also offers some configuration
options via :ref:`Boxlayout Configuration <boxlayout>`.


js Layout
---------

To implement more sophisticated layouts and populate additional metadata fields we integrated a javascript runtime engine. 
For more details see :ref:`Scripting <scripting>`.

At the moment there are three different layout functions for audio files and one for videos and images.
The layout function can be set in configuration of :ref:`Import Function <import-function>`.

+-----------------------+-----------------------------------------------------------------------+------+
| Layout Function       | Description                                                           | Code |
+=======================+=======================================================================+======+
| importAudio           | Create default audio layout similar to builtin layout                 | da   |
+-----------------------+-----------------------------------------------------------------------+------+
| importAudioInitial    | Create a folder for each album artist. Suitable for large collections | ia   |
+-----------------------+-----------------------------------------------------------------------+------+
| importAudioStructured | Create boxes for a group of initial letters in one box                | sa   |
+-----------------------+-----------------------------------------------------------------------+------+
| importVideo           | Create video layout similar to builtin layout                         | dv   |
+-----------------------+-----------------------------------------------------------------------+------+
| importImage           | Create image layout similar to builtin layout                         | di   |
+-----------------------+-----------------------------------------------------------------------+------+


Playlists
---------

When a playlist item is encountered, it is automatically fed as input to the playlist function ``importPlaylist``. The playlist function
attempts to parse the playlist according to its file type and adds new items to the database, which item is then processed
by the import script. Changing ``size`` for boxlayout ``Playlist/allDirectories`` changes the depth of the hierarchy under the **Directories** node.


Layout Configuration
~~~~~~~~~~~~~~~~~~~~

Boxlayout
---------

In order to avoid changing scripts or code, layout configuration allows, e.g., setting the captions of entries in the virtual layout tree.
Each box allows to disable it by setting the attribute ``enabled="false"``. If a parent box is disabled all child boxes will disappear from the layout.
Boxes in ``AudioStructured`` support setting the size which means the number of initials in that box.

The following table lists the settings that are handled by the respective layout.
The codes in *js layout* column refer to the *Code* column above.

+---------------------------------+------------------+-----------+----------------+
| Box                             | Default Caption  | js layout | builtin layout |
+=================================+==================+===========+================+
| Audio/allAlbums                 | Albums           | da, ia    | yes            |
+---------------------------------+------------------+-----------+----------------+
| Audio/allArtists                | Artists          | da, ia    | yes            |
+---------------------------------+------------------+-----------+----------------+
| Audio/allAudio                  | All Audio        | da, ia    | yes            |
+---------------------------------+------------------+-----------+----------------+
| Audio/allComposers              | Composers        | da, ia    | yes            |
+---------------------------------+------------------+-----------+----------------+
| Audio/allDirectories            | Directories      | da, ia    | yes            |
+---------------------------------+------------------+-----------+----------------+
| Audio/allGenres                 | Genres           | da, ia    | yes            |
+---------------------------------+------------------+-----------+----------------+
| Audio/allSongs                  | All Songs        | da, ia    | yes            |
+---------------------------------+------------------+-----------+----------------+
| Audio/allTracks                 | All - full name  | da, ia    | yes            |
+---------------------------------+------------------+-----------+----------------+
| Audio/allYears                  | Year             | da, ia    | yes            |
+---------------------------------+------------------+-----------+----------------+
| Audio/audioRoot                 | Audio            | da, ia    | yes            |
+---------------------------------+------------------+-----------+----------------+
| Audio/artistChronology          | Album Chronology | da, ia    | yes            |
+---------------------------------+------------------+-----------+----------------+
| AudioInitial/abc                | ABC              | ia        | no             |
+---------------------------------+------------------+-----------+----------------+
| AudioInitial/allArtistTracks    | 000 All          | ia        | no             |
+---------------------------------+------------------+-----------+----------------+
| AudioInitial/allBooks           | Books            | ia        | no             |
+---------------------------------+------------------+-----------+----------------+
| AudioInitial/audioBookRoot      | AudioBooks       | ia        | no             |
+---------------------------------+------------------+-----------+----------------+
| AudioStructured/allAlbums       | -Album-          | sa        | no             |
+---------------------------------+------------------+-----------+----------------+
| AudioStructured/allArtistTracks | all              | sa        | no             |
+---------------------------------+------------------+-----------+----------------+
| AudioStructured/allArtists      | -Artist-         | sa        | no             |
+---------------------------------+------------------+-----------+----------------+
| AudioStructured/allGenres       | -Genre-          | sa        | no             |
+---------------------------------+------------------+-----------+----------------+
| AudioStructured/allTracks       | -Track-          | sa        | no             |
+---------------------------------+------------------+-----------+----------------+
| AudioStructured/allYears        | -Year-           | sa        | no             |
+---------------------------------+------------------+-----------+----------------+
| Video/allDates                  | Date             | dv        | yes            |
+---------------------------------+------------------+-----------+----------------+
| Video/allDirectories            | Directories      | dv        | yes            |
+---------------------------------+------------------+-----------+----------------+
| Video/allVideo                  | All Video        | dv        | yes            |
+---------------------------------+------------------+-----------+----------------+
| Video/allYears                  | Year             | dv        | yes            |
+---------------------------------+------------------+-----------+----------------+
| Video/unknown                   | Unknown          | dv        | yes            |
+---------------------------------+------------------+-----------+----------------+
| Video/videoRoot                 | Video            | dv        | yes            |
+---------------------------------+------------------+-----------+----------------+
| Image/allDates                  | Date             | di        | yes            |
+---------------------------------+------------------+-----------+----------------+
| Image/allDirectories            | Directories      | di        | yes            |
+---------------------------------+------------------+-----------+----------------+
| Image/allImages                 | All Photos       | di        | yes            |
+---------------------------------+------------------+-----------+----------------+
| Image/allYears                  | Year             | di        | yes            |
+---------------------------------+------------------+-----------+----------------+
| Image/imageRoot                 | Photos           | di        | yes            |
+---------------------------------+------------------+-----------+----------------+
| Image/unknown                   | Unknown          | di        | yes            |
+---------------------------------+------------------+-----------+----------------+
| Trailer/trailerRoot             | Online Services  | dt        | yes            |
+---------------------------------+------------------+-----------+----------------+
| Trailer/allTrailers             | All Trailers     | dt        | yes            |
+---------------------------------+------------------+-----------+----------------+
| Trailer/allGenres               | Genres           | dt        | yes            |
+---------------------------------+------------------+-----------+----------------+
| Trailer/relDate                 | Release Date     | dt        | yes            |
+---------------------------------+------------------+-----------+----------------+
| Trailer/postDate                | Post Date        | dt        | yes            |
+---------------------------------+------------------+-----------+----------------+
| Trailer/unknown                 | Unknown          | dt        | yes            |
+---------------------------------+------------------+-----------+----------------+
| Playlist/playlistRoot           | Playlists        | dt        | no             |
+---------------------------------+------------------+-----------+----------------+
| Playlist/allPlaylists           | All Playlists    | dt        | no             |
+---------------------------------+------------------+-----------+----------------+
| Playlist/allDirectories         | Directories      | dt        | no             |
+---------------------------------+------------------+-----------+----------------+

The complete list of configuration options can be found in :ref:`Boxlayout Configuration <boxlayout>`.

Script Options
--------------

Furthermore there are some script options that allow tweaking the layout. The script options and their interpretation
depend on the layout function. The options can be set in the config.xml section :ref:`script-options <script-options>`

+---------------------------------+------------------+--------------------------------------------------------------------------+
| Option                          | Layout Function  | Description                                                              |
+=================================+==================+==========================================================================+
| trackNumbers                    | da, sa, ia       | Use 'show' or 'hide' to add track number in front of the track title     |
|                                 |                  | the default behaviour depends on the function.                           |
+---------------------------------+------------------+--------------------------------------------------------------------------+
| specialGenre                    | ia               | Add disk number to tracks matching this genre (regular expression)       |
+---------------------------------+------------------+--------------------------------------------------------------------------+
| spokenGenre                     | ia               | Do not add tracks to 'All' section                                       |
|                                 |                  | if the genre matches (regular expression)                                |
+---------------------------------+------------------+--------------------------------------------------------------------------+
