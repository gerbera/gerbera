.. _scripting:
.. index:: Scripting

Scripting
=========

Gerbera allows you to customize the structure of how your media is being presented to your renderer. One of the most
important features introduced since the version 0.8 (`mediatomb`) are the virtual containers and virtual items.

Let's think of possible scenarios:

* You may want to separate your content by music, photo, video, maybe create a special container with all non
  playable stuff

* You may want your music to be sorted by genre, year, artist, album, or maybe by starting letters, so you can
  more easily find your favorite song when browsing the server

* You want to have your photos that you took with your favorite digital camera to appear in a special folder, or
  maybe you even want to separate the photos that you took with flash-on from the ones that you made without flash

* Your media player does not support video, so you do not even want to see the Video container

* It's up to your imagination :)

The scenarios described above and much more can be achieved with the help of an import script.

Gerbera supports a playlist parsing feature, which is also handled by scripting.


How It Works
~~~~~~~~~~~~

This section will give you some overview on how virtual objects work and on how they are related to scripting.

.. Note::

    In order to use the import scripting feature you have to change the layout type from builtin to js in ``config.xml``

.. Note::

    The sorting of Video and Photo items using the ``rootpath`` object is still somewhat experimental and not
    described here.

Understanding Virtual Objects
-----------------------------

When you add a file or directory to the database via the web
interface several things happen.

1. The object is inserted into the PC Directory. PC Directory is simply a special non-removable container.
   Any media file added will have an entry inside the PC Directory tree. PC Directory's hierarchy reflects the file
   system hierarchy, all objects inside the PC Directory including itself are NON-VIRTUAL objects. All virtual objects
   may have a different title, description, etc., but they are still references to objects in the PC-Directory.
   That's why it is not possible to change a location of a virtual object - the only exceptions are URL items and
   Active items.

2. Once an item is added to the PC Directory it is forwarded to the virtual object engine. The virtual object engine's
   mission is to organize and present the media database in a logical hierarchy based on the available metadata of the
   items.

Each UPnP server implements this so called virtual object hierarchy in a different way. Audio files are usually sorted by
artist, album, some servers may just present a view similar to the file system and so on. Most servers have strong limitations
on the structure of the virtual containers, they usually offer a predefined layout of data and the user has to live with it.
In Gerbera we try to address this shortcoming by introducing the scriptable virtual object engine. It is designed to be:

* maximally flexible
* easily customizable and extendable
* robust and efficient

We try to achieve these goals by embedding a scripting runtime environment that allows the execution of ECMAScript E5/5.1 conform
scripts better known as JavaScript. Gerbera uses `duktape <http://duktape.org/>`_ scripting engine to run JavaScript.

Theory of Operation
-------------------

After an item is added to the PC Directory it is automatically fed as input to the import script. The script then creates one
or more virtual items for the given original item. Items created from scripts are always marked virtual.

When the virtual object engine gets notified of an added item, following happens: a javascript object is created mirroring the
properties of the item. The object is introduced to the script environment and bound to the predefined variable 'orig'. This
way a variable orig is always defined for every script invocation and represents the original data of the added item.
Then the script is invoked.

.. Note::

    In the current implementation, if you modify the script then you will have to restart the server for the new logic to take
    ffect.

The script is only triggered when new objects are added to the database, also note that the script
does not modify any objects that already exist in the database - it only processes new objects that are being added.

When a playlist item is encountered, it is automatically fed as input to the playlist script. The playlist script
attempts to parse the playlist and adds new item to the database, the item is then processed by the import script.


Global Variables And Constants
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

In this section we will introduce the properties of the object that will be processed by the script,
as well as functions that are offered by the server.


The Media Object
----------------

Each time an item is added to the database the import script is invoked. So, one script
invocation processes exactly one non virtual item, and creates a number of virtual items and containers. The original item is
made available in the form of the global variable 'orig'. Additionally, when the object being imported is a playlist, it
is made available to the playlist parser script in the form of the global variable 'playlist'. It is usually a good idea to
only read from these variables and to create and only modify local copies.

.. Note::

    modifying the properties of the orig object will not
    propagate the changes to the database, only a call to
    the ``addCdsObject()`` will permanently add the object.

General Properties
------------------

Here is a list of properties of an object, you can set them you create a new object or when you modify a copy of the 'orig'
object.

**RW** means read/write, i.e. - changes made to that property will be transferred into the database.

**RO** means, that this is a read only property, any changes made to it will get lost.

.. js:attribute:: orig.objectType

    **RW**

    This defines the object type, following types are available:

    +----------------------------------+-----------------------------------------------+
    | Key                              | Description                                   |
    +==================================+===============================================+
    | OBJECT_TYPE_CONTAINER            | Object is a container                         |
    +----------------------------------+-----------------------------------------------+
    | OBJECT_TYPE_ITEM                 | Object is an item                             |
    +----------------------------------+-----------------------------------------------+
    | OBJECT_TYPE_ACTIVE_ITEM          | Object is an active item                      |
    +----------------------------------+-----------------------------------------------+
    | OBJECT_TYPE_ITEM_EXTERNAL_URL    | Object is a link to a resource on the Internet|
    +----------------------------------+-----------------------------------------------+
    | OBJECT_TYPE_ITEM_INTERNAL_URL    | Object is an internal link                    |
    +----------------------------------+-----------------------------------------------+

.. js:attribute:: orig.title

    **RW**

    This is the title of the original object, since the object represents an entry in the PC-Directory, the title will be
    set to it's file name. This field corresponds to dc:title in the DIDL-Lite XML.

.. js:attribute:: orig.id

    **RO**

    The object ID, make sure to set all refID's (reference IDs) of your virtual objects to that ID.

.. js:attribute:: orig.parentID

    **RO**

    The object ID of the parent container.

.. js:attribute:: orig.upnpclass

    **RW**

    The UPnP class of the item, this corresponds to ``upnp:class`` in the DIDL-Lite XML.

.. js:attribute:: orig.location

    **RO**

    Location on disk, given by the absolute path and file name.

.. js:attribute:: orig.theora

    **RO**

    This property is a boolean value, it is non zero if the particular item is of type OGG Theora. This is useful to
    allow proper sorting of media and thus placing OGG Vorbis into the Audio container and OGG Theora into the Video
    container.

.. js:attribute:: orig.onlineservice

    **RO**

    Identifies if the item belongs to an online service and thus has extended properties. Following types are
    available:

    +----------------------------------+--------------------------------------------------------+
    | Key                              | Description                                            |
    +==================================+========================================================+
    | ONLINE_SERVICE_NONE              | The item does not belong to an online service and does |
    |                                  | not have extended properties.                          |
    +----------------------------------+--------------------------------------------------------+
    | ONLINE_SERVICE_APPLE_TRAILERS    | The item belongs to the Apple Trailers service and has |
    |                                  | extended properties.                                   |
    +----------------------------------+--------------------------------------------------------+


.. js:attribute:: orig.mimetype

    **RW**

    Mimetype of the object.

.. js:attribute:: orig.meta

    **RW**

    Array holding the metadata that was extracted from the object (i.e. id3/exif/etc. information)


    .. js:attribute:: orig.meta[M_TITLE]

        **RW**

        Extracted title (for example the id3 title if the object is an mp3 file), if you want that your new
        virtual object is displayed under this title you will have to set `obj.title = orig.meta[M_TITLE]`

    .. js:attribute:: orig.meta[M_ARTIST]

        **RW**

        Artist information, this corresponds to ``upnp:artist`` in the DIDL-Lite XML.

    .. js:attribute:: orig.meta[M_ALBUM]

        **RW**

        Album information, this corresponds to ``upnp:album`` in the DIDL-Lite XML.

    .. js:attribute:: orig.meta[M_DATE]

        **RW**

        Date, must be in the format of **YYYY-MM-DD** (required by the UPnP spec), this corresponds to dc:date in the
        DIDL-Lite XML.


    .. js:attribute:: orig.meta[M_GENRE]

        **RW**

        Genre of the item, this corresponds to ``upnp:genre`` in the DIDL-Lite XML.

    .. js:attribute:: orig.meta[M_DESCRIPTION]

        **RW**

        Description of the item, this corresponds to ``dc:description`` in the DIDL-Lite XML.

    .. js:attribute:: orig.meta[M_REGION]

        **RW**

        Region description of the item, this corresponds to ``upnp:region`` in the DIDL-Lite XML.

    .. js:attribute:: orig.meta[M_TRACKNUMBER]

        **RW**

        Track number of the item, this corresponds to ``upnp:originalTrackNumber`` in the DIDL-Lite XML.

    .. js:attribute:: orig.meta[M_AUTHOR]

        **RW**

        Author of the media, this corresponds to ``upnp:author`` in the DIDL-Lite XML.

    .. js:attribute:: orig.meta[M_DIRECTOR]

        **RW**

        Director of the media, this corresponds to ``upnp:director`` in the DIDL-Lite XML.

    .. js:attribute:: orig.meta[M_PUBLISHER]

        **RW**

        Director of the media, this corresponds to ``dc:publisher`` in the DIDL-Lite XML.

    .. js:attribute:: orig.meta[M_RATING]

        **RW**
    
        Director of the media, this corresponds to ``upnp:rating`` in the DIDL-Lite XML.

    .. js:attribute:: orig.meta[M_ACTOR]

        **RW**
    
        Director of the media, this corresponds to ``upnp:actor`` in the DIDL-Lite XML.

    .. js:attribute:: orig.meta[M_PRODUCER]

        **RW**

        Director of the media, this corresponds to ``upnp:producer`` in the DIDL-Lite XML.

.. js:attribute:: orig.aux

    **RO**

    Array holding the so called auxiliary data. Aux data is metadata that is not part of UPnP, for example -
    this can be a camera model that was used to make a photo, or the information if the photo was taken with or without flash.


    Currently aux data can be gathered from **libexif** (see the Import section in the main
    documentation for more details). So, this array will hold the tags that you specified in your config.xml, allowing
    you to create your virtual structure according to your liking.

.. js:attribute:: orig.playlistOrder

    **RW**

    This property is only available if the object is being created by the playlist script. It's similar to ID3 track
    number, but is used to set the position of the newly created object inside a parsed playlist container. Usually
    you will increment the number for each new object that you create while parsing the playlist, thus ensuring that the
    resulting order is the same as in the original playlist.

Constants
---------

Actually there are no such things as constants in JS, so those are actually predefined global variables that are set during JS
engine initialization. Do not assign any values to them, otherwise following script invocation will be using wrong
values.

+---------------------------------------+---------+--------------------------------------+-----------------------------------------------+
| Constant                              | Type    | Value                                | Notes                                         |
+=======================================+=========+======================================+===============================================+
| ``UPNP_CLASS_CONTAINER``              | string  | object.container                     |                                               |
+---------------------------------------+---------+--------------------------------------+-----------------------------------------------+
| ``UPNP_CLASS_CONTAINER_MUSIC_ARTIST`` | string  | object.container.person.musicArtist  |                                               |
+---------------------------------------+---------+--------------------------------------+-----------------------------------------------+
| ``UPNP_CLASS_CONTAINER_MUSIC_GENRE``  | string  | object.container.genre.musicGenre    |                                               |
+---------------------------------------+---------+--------------------------------------+-----------------------------------------------+
| ``UPNP_CLASS_CONTAINER_MUSIC_ALBUM``  | string  | object.container.album.musicAlbum    | | This container class will be treated by the |
|                                       |         |                                      | | server in a special way, all music items in |
|                                       |         |                                      | | this container will be sorted by ID3 track  |
|                                       |         |                                      | | number.                                     |
+---------------------------------------+---------+--------------------------------------+-----------------------------------------------+
| ``UPNP_CLASS_PLAYLIST_CONTAINER``     | string  | object.container.playlistContainer   | | This container class will be treated by the |
|                                       |         |                                      | | server in a special way, all items in this  |
|                                       |         |                                      | | container will be sorted by the number      |
|                                       |         |                                      | | specified in the playlistOrder property     |
|                                       |         |                                      | | (this is set when an object is created by   |
|                                       |         |                                      | | the playlist script).                       |
+---------------------------------------+---------+--------------------------------------+-----------------------------------------------+
| ``UPNP_CLASS_ITEM``                   | string  | object.item                          |                                               |
+---------------------------------------+---------+--------------------------------------+-----------------------------------------------+
| ``UPNP_CLASS_ITEM_MUSIC_TRACK``       | string  | object.item.audioItem.musicTrack     |                                               |
+---------------------------------------+---------+--------------------------------------+-----------------------------------------------+
| ``UPNP_CLASS_ITEM_VIDEO``             | string  | object.item.videoItem                |                                               |
+---------------------------------------+---------+--------------------------------------+-----------------------------------------------+
| ``UPNP_CLASS_ITEM_IMAGE``             | string  | object.item.imageItem                |                                               |
+---------------------------------------+---------+--------------------------------------+-----------------------------------------------+
| ``OBJECT_TYPE_CONTAINER``             | integer | 1                                    |                                               |
+---------------------------------------+---------+--------------------------------------+-----------------------------------------------+
| ``OBJECT_TYPE_ITEM``                  | integer | 2                                    |                                               |
+---------------------------------------+---------+--------------------------------------+-----------------------------------------------+
| ``OBJECT_TYPE_ACTIVE_ITEM``           | integer | 4                                    |                                               |
+---------------------------------------+---------+--------------------------------------+-----------------------------------------------+
| ``OBJECT_TYPE_ITEM_EXTERNAL_URL``     | integer | 8                                    |                                               |
+---------------------------------------+---------+--------------------------------------+-----------------------------------------------+
| ``OBJECT_TYPE_ITEM_INTERNAL_URL``     | integer | 16                                   |                                               |
+---------------------------------------+---------+--------------------------------------+-----------------------------------------------+


Functions
~~~~~~~~~

The server offers various native functions that can be called from the scripts, additionally there
are some js helper functions that can be used.

Native Server Functions
-----------------------

The so called native functions are implemented in C++ in the server and can be called from the scripts.

Native Functions Available To All Scripts
-----------------------------------------

The server offers three functions which can be called from
within the import and/or the playlist script:


.. js:function:: addCdsObject(object, containerChain, lastContainerClass)

    Adds the object as a virtual object to the container chain

    :param object object:
        A virtual object that is either a copy of or a reference to 'orig'
    :param string containerChain:
        A string, defining where the object will be added in the database hierarchy. The containers in the chain
        are separated by a slash '/', for example, a value of '/Audio/All Music' will add the object to the Audio,
        All Music container in the server hierarchy. Make sure to properly escape the slash characters in container
        names. You will find more information on container chain escaping later in this chapter.
    :param string lastContainerClass:
        A string, defining the upnp:class of the container that appears last in the chain. This parameter can be
        omitted, in this case the default value ``object.container`` will be taken. Setting specific upnp container classes
        is useful to define the special meaning of a particular container; for example, the server will always sort
        songs by track number if upnp class of a container is set to ``object.container.album.musicAlbum``.


.. js:function:: copyObject(originalObject)

    This function returns a copy of the virtual object.

    :param object originalObject:
    :returns: A copy of the virtual object

.. js:function:: print(...)

    This function is useful for debugging scripts, it simply
    prints to the standard output.

.. js:function:: f2i(string)

    Converts filesystem charset to internal UTF-8.

    :param string string:

    `The 'from' charsets can be defined in the server configuration`

.. js:function:: m2i(string)

    Converts metadata charset to internal UTF-8.

    :param string string:

    `The 'from' charsets can be defined in the server configuration`

.. js:function:: p2i(string)

    Converts playlist charset to internal UTF-8.

    :param string string:

    `The 'from' charsets can be defined in the server configuration`

.. js:function:: j2i(string)

    Converts js charset to internal UTF-8.

    :param string string:

    `The 'from' charsets can be defined in the server configuration`



Native Functions Available To The Playlist Script
-------------------------------------------------

The following function is only available to the playlist script.

.. js:function:: readln()

    :returns: string

    This function reads and returns exactly one line of text from the playlist that is currently being processed, end of
    line is identified by carriage return/line feed characters. Each subsequent call will return the next line, there is no
    way to go back. The idea is, that you can process your playlist line by
    line and gather the required information to create new objects which can be added to the database.


Helper Functions
----------------

There is a set of helper JavaScript functions which reside in the common.js script.
They can be used by the import and by the playlist script.

.. js:function:: escapeSlash(name)

    Escapes slash '/' characters in a string. This is necessary, because the container chain is
    defined by a slash separated string, where slash has a special meaning - it defines the container hierarchy. That
    means, that slashes that appear in the object's title need to be properly escaped.

    :param string name: A string to be escaped
    :returns: string


.. js:function:: createContainerChain(arr)

    Verifies that the names are properly escaped and adds the slash separators as necessary

    :param array arr: An array of container names
    :returns: string formatted for use in ``addCdsObject``

    .. code-block:: js

        function createContainerChain(arr)
        {
            var path = '';
            for (var i = 0; i < arr.length; i++)
            {
                path = path + '/' + escapeSlash(arr[i]);
            }
            return path;
        }


.. js:function:: getYear(date)

    :param string: A date formatted in ``yyyy-mm-dd``
    :returns: string - Year value

.. code-block:: js

  function getYear(date)
  {
      var matches = date.match(/^([0-9]{4})-/);
      if (matches)
          return matches[1];
      else
          return date;
  }

.. js:function:: getPlaylistType(mimetype)

    This function identifies the type of the playlist by the mimetype, it is used in the playlist script to select an
    appropriate parser.

    :param string: A valid mime-type
    :returns: string - playlist type

.. code-block:: js

  function getPlaylistType(mimetype)
  {
      if (mimetype == 'audio/x-mpegurl')
          return 'm3u';
      if (mimetype == 'audio/x-scpls')
          return 'pls';
      return '';
  }



Walkthrough
~~~~~~~~~~~

Now it is time to take a closer look at the default scripts that are supplied with Gerbera. Usually it is installed in
the ``/usr/share/gerbera/js/`` directory, but you will also find it in ``scripts/js/`` in the Gerbera source tree.

.. Note::
  this is not a JavaScript tutorial, if you are new to JS you should probably make yourself familiar with the
  language.

Import Script
-------------

We start with a walkthrough of the default import script, it is called import.js in the Gerbera distribution.

Below are the import script functions that organize our content in the database by creating the virtual structure.
Each media type - audio, image and video is handled by a separate function.

Audio Content Handler
:::::::::::::::::::::

The biggest one is the function that handles audio - the reason
is simple: mp3 files offer a lot of metadata like album,
artist, genre, etc. information, this allows us to create a
nice container layout.

.. literalinclude:: ../scripts/js/import.js
    :start-after: // doc-add-audio-begin
    :end-before: // doc-add-audio-end
    :language: js


Image Content Handler
:::::::::::::::::::::

This function takes care of images. Currently it does very little sorting, but could easily be extended - photos made by
digital cameras provide lots of information in the Exif tag, so you could easily add code to sort your pictures by camera model
or anything Exif field you might be interested in.

.. Note::

    if you want to use those additional Exif fields you need to compile MediaTomb with libexif support and also
    specify the fields of interest in the import section of your configuration file
    (See documentation about library-options).

.. literalinclude:: ../scripts/js/import.js
    :start-after: // doc-add-image-begin
    :end-before: // doc-add-image-end
    :language: js

Just like in the addAudio() function - we construct our container chain and add the object.


Video Content Handler
:::::::::::::::::::::

Not much to say here... I think libextractor is capable of retrieving some information from video files, however I seldom
encountered any video files populated with metadata. You could also try ffmpeg to get more information, however by default we
keep it very simple - we just put everything into the 'All Video' container.

.. literalinclude:: ../scripts/js/import.js
    :start-after: // doc-add-video-begin
    :end-before: // doc-add-video-end
    :language: js


Apple Trailers Content Handler
::::::::::::::::::::::::::::::

This function processes items that are importent via the Apple Trailers feature. We will organize the trailers by genre, post
date and release date, additionally we will also add a container holding all trailers.

.. literalinclude:: ../scripts/js/import.js
    :start-after: // doc-add-trailer-begin
    :end-before: // doc-add-trailer-end
    :language: js

Putting it all together
:::::::::::::::::::::::

This is the main part of the script, it looks at the mimetype of the original object and feeds the object to the appropriate
content handler.

.. literalinclude:: ../scripts/js/import.js
    :start-after: // main script part
    :language: js


Playlist Script
---------------

The default playlist parsing script is called playlists.js, similar to the import script it works with a global object
which is called 'playlist', the fields are similar to the 'orig' that is used in the import script with the exception of
the playlistOrder field which is special to playlists.

Another big difference between playlist and import scripts is, that playlist scripts can add new media to the database, while
import scripts only process already existing objects (the ones found in PC Directory) and just add additional virtual items.

The default playlist script implementation supports parsing of m3u and pls formats, but you can add support for parsing of any
ASCII based playlist format.

Adding Items
::::::::::::

We will first look at a helper function:

``addPlaylistItem(location, title, playlistChain);``

It is defined in playlists.js, it receives the location (path on disk or HTTP URL), the title and the desired position of the
item in the database layout (remember the container chains used in the import script).

The function first decides if we are dealing with an item that represents a resource on the web, or if we are dealing with a
local file. After that it populates all item fields accordingly and calls the addCdsObject() that was introduced earlier. Note,
that if the object that is being added by the playlist script is not yet in the database, the import script will be invoked.

Below is the complete function with some comments:

.. literalinclude:: ../scripts/js/playlists.js
    :start-after: // doc-add-playlist-item-begin
    :end-before: // doc-add-playlist-item-end
    :language: js

Main Parsing
::::::::::::

The actual parsing is done in the main part of the script. First, the type of the playlist is determined (based on the
playlist mimetype), then the correct parser is chosen. The parsing itself is a loop, where each call to readln() returns
exactly one line of text from the playlist. There is no possibility to go back, each readln() invocation will retrieve
the next line until end of file is reached.

To keep things easy we will only list the m3u parsing here. Again, if you are not familiar with regular expressions, now is
probably the time to take a closer look.

.. literalinclude:: ../scripts/js/playlists.js
    :start-after: // doc-playlist-m3u-parse-begin
    :end-before: // doc-playlist-m3u-parse-end
    :language: js

**Happy scripting!**
