Container icon theme
====================

Two transparent 3D PNG themes are supplied for DLNA/UPnP container artwork:
``Gerbera`` (20 originals with Gerbera flowers) and ``Techno`` (20 originals).
``Gerbera`` is selected by ``--create-advanced-config``. Both use the existing
``ContainerArt`` handler and dynamic-container ``image`` attribute.

Installation also supplies category aliases for the unmodified English layout,
such as ``Audio.png``, ``Albums.png``, and ``PC Directory.png``.
The original PNGs remain available under the names below. ``manifest.json``
records their SHA-256 checksums; see ``LICENSE.txt`` for license and provenance.

The ``Gerbera`` theme uses Gerbera flower details in a three-dimensional
style, with its own license and checksum manifest.
Its Audio, All Audio, All - full name and Recently Played artwork has distinct
symbols and colors. A clock with a play symbol identifies Recently Played;
a hammer and media library identify Recently Modified. Folder, single album
and album collection artwork are deliberately different.

Both sets, including the favorites star, were created using OpenAI image
generation; the Video play triangle includes an approved geometric correction.
They are freely available under GPLv2 (GPL-2.0-only), with no
additional restrictions imposed by the contributor beyond the license itself.

.. list-table:: Complete artwork set
   :header-rows: 1

   * - File
     - Intended category
   * - ``artists.png``
     - Artists / album artists
   * - ``albums.png``
     - Album collection
   * - ``album-single.png``
     - Single album
   * - ``tracks.png``
     - Tracks
   * - ``audio.png``
     - Audio category
   * - ``all-audio.png``
     - All Audio
   * - ``all-full-name.png``
     - All - full name
   * - ``recently-played.png``
     - Music Recently Played
   * - ``composers.png``
     - Composers
   * - ``genres.png``
     - Genres
   * - ``playlists.png``
     - Playlists
   * - ``recently-added.png``
     - Recently added
   * - ``favorites.png``
     - Favorites (star; artwork only, no favorites feature is added)
   * - ``computer.png``
     - PC Directory
   * - ``directory.png``
     - Directories
   * - ``photos.png``
     - Photos
   * - ``videos.png``
     - Video
   * - ``date.png``
     - Date
   * - ``year.png``
     - Year
   * - ``recently-modified.png``
     - Recently modified

Enable or customize
-------------------

Select either theme directly when generating an advanced configuration:

.. code-block:: console

    gerbera --create-advanced-config=gerbera > config-gerbera.xml
    gerbera --create-advanced-config=techno > config-techno.xml

Without a theme argument, ``--create-advanced-config`` selects ``Gerbera``.

The advanced configuration includes the installed ``gerbera`` directory,
type-qualified category matching, cover filenames, class fallbacks and the
three dynamic-container image paths.
``--create-config`` and ``--create-example-config`` retain their existing
artwork settings. Existing configurations are not changed automatically.

Both themes are installed. In Gerbera's server configuration, change the
``location`` of ``import/resources/container`` and the three dynamic-container
``image`` paths under ``server/containers``. These are existing configuration
settings; there is no additional script to install.

.. list-table:: Theme selection (example installation prefix)
   :header-rows: 1

   * - Setting
     - Gerbera (default)
     - Techno
   * - Container resource location
     - ``/usr/local/share/gerbera/container-icons/gerbera``
     - ``/usr/local/share/gerbera/container-icons``
   * - ``/LastAdded`` image, relative to that location
     - ``recently-added.png``
     - ``recently-added.png``
   * - ``/LastModified`` image, relative to that location
     - ``recently-modified.png``
     - ``recently-modified.png``
   * - ``/LastPlayed`` image, relative to that location
     - ``recently-played.png``
     - ``recently-played.png``

The dynamic-container image values must contain the complete directory and
filename, for example
``/usr/local/share/gerbera/container-icons/gerbera/recently-played.png``.
The installation prefix is selected by the generator; distribution packages
may use ``/usr/share/gerbera`` instead of ``/usr/local/share/gerbera``.

For an existing configuration, the installed ``container-icons.xml`` is a
ready-to-merge fragment selecting ``Gerbera``; ``container-icons-techno.xml``
selects ``Techno``. Preserve other resource
entries, custom dynamic-container filters, sort order and titles when merging it.
The fragment is not a complete configuration or an XML include.

Users can continue supplying their own icons: change ``location`` to a readable
directory containing their images, and change dynamic-container ``image`` paths
as needed. Copying this set to a user-controlled directory and replacing selected
files is another option. Do not edit packaged files if changes must survive
package upgrades. Restart Gerbera after configuration changes and refresh/rescan
the affected containers as appropriate for the chosen import mode and layout.
Existing stored artwork paths may retain the previous directory; configuration
changes do not rewrite existing resource metadata.
Clients may cache the old artwork.

Matching and limitations
------------------------

``%filename%`` expands to the container title; ``%upnpclass%`` expands to its
UPnP class. The advanced preset uses ``%upnpclass%-%filename%.png`` for named
categories. This keeps an album named Audio separate from the Audio category.
Renamed or translated categories can have correspondingly named files.

An ``add-file`` entry containing exactly ``%upnpclass%.png`` selects a deferred
fallback. Local artwork and existing referenced covers are tried first. If no
cover is available, the handler tries the container's class and its parent
classes: album containers receive single-album artwork; generic containers
receive a folder. Playlist containers can receive the playlist artwork through
their class. This does not change the hierarchy or remove real filesystem
levels. The three dynamic categories use their explicit image paths, so the
Recently Played clock is not a generic audio or folder fallback.

Existing configurations without the new class placeholder keep their previous
matching behavior. Existing stored artwork references are not automatically
migrated, and the production catalogue maintenance operations used while
developing this theme are not shipped as database migration scripts.

The favorites star is included even though Gerbera does not create a favorites
container through this contribution. It is available for user-defined layouts
or clients that expose such a category.

The PNGs are 1254 x 1254 pixels with transparency. The supplied originals are
preserved byte for byte.
