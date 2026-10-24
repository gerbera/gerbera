Container icon theme
====================

This proposed stock theme supplies 15 original transparent RGBA PNGs for
DLNA/UPnP container artwork. It reuses Gerbera's existing ``ContainerArt``
handler, ``%filename%`` expansion, and dynamic-container ``image`` attribute.
No database migration, new JavaScript, or additional matching engine is needed.

Installation also supplies filename aliases for the unmodified English
category titles, such as ``Audio.png``, ``Albums.png``, and ``PC Directory.png``.
The original PNGs remain available under the names below. ``manifest.json``
records their SHA-256 checksums; see ``LICENSE.txt`` for license and provenance.

All 15 icons, including the favorites star, were generated using OpenAI image
generation. They are freely available under GPLv2 (GPL-2.0-only), with no
additional restrictions imposed by the contributor beyond the license itself.

.. list-table:: Complete artwork set
   :header-rows: 1

   * - File
     - Intended category
   * - ``artists.png``
     - Artists / album artists
   * - ``albums.png``
     - Albums
   * - ``tracks.png``
     - Audio / tracks / recently played
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

The installed ``container-icons.xml`` is a configuration fragment, not a complete
configuration or an XML include. Merge its ``import/resources/container`` section
into your existing configuration. Preserve your other resource entries. Merge
the three ``server/containers/container`` image attributes into the matching
existing dynamic containers; preserve any custom filters, sort order and titles.

The installation prefix is substituted when CMake configures the fragment.
With the usual prefix, the location is
``/usr/local/share/gerbera/container-icons``. Distribution packages may use
``/usr/share/gerbera/container-icons`` instead.

Existing configurations and generated defaults are intentionally unchanged in
this draft proposal. The assets and ready-to-merge configuration allow the
maintainers to review the proposed stock appearance before deciding whether to
enable it in newly generated configurations.

Users can continue supplying their own icons: change ``location`` to a readable
directory containing their images, and change dynamic-container ``image`` paths
as needed. Copying this set to a user-controlled directory and replacing selected
files is another option. Do not edit packaged files if changes must survive
package upgrades. Restart Gerbera after configuration changes and refresh/rescan
the affected containers as appropriate for the chosen import mode and layout.
Clients may cache the old artwork.

Matching and limitations
------------------------

``%filename%`` expands to the container title. Renamed or translated categories
need correspondingly named PNGs in the configured directory. The supplied
aliases cover the stock English layout; this is not a language-independent
mapping by category ID or UPnP class. Paths and matching use existing Gerbera
resource semantics; no new configuration attributes are introduced.

Only explicitly named categories receive these icons. Unmatched individual
albums retain Gerbera's normal cover-art lookup and inheritance. There is no
catch-all folder icon that would replace every album cover. Folder names equal
to a category alias will also match that alias under the existing title-based
mechanism; use a tailored user directory if such collisions matter.

``Playlists.png`` and ``All Playlists.png`` style the playlist categories.
They do not automatically match every individually named playlist. For a
playlist named ``Evening Mix``, a user can provide ``Evening Mix.png`` when that
layout processes container artwork for playlist entries. This draft does not
add a generic per-playlist fallback or claim that every existing playlist
parser will attach a thumbnail on import. No production database corrections
from the deployment example are part of this package.

The favorites star is included even though Gerbera does not create a favorites
container through this contribution. It is available for user-defined layouts
or clients that expose such a category.

The PNGs are 1254 x 1254 pixels with transparency. Runtime-sized derivatives
and the choice of default activation can be reviewed independently; this draft
preserves the supplied originals byte for byte.
