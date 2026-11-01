# ~~~
# *GRB*
#
#  Gerbera - https://gerbera.io/
#
#  CMakeLists.txt - this file is part of Gerbera.
#
#  Copyright (C) 2016-2026 Gerbera Contributors
#
#  Gerbera is free software; you can redistribute it and/or modify
#  it under the terms of the GNU General Public License version 2
#  as published by the Free Software Foundation.
#
#  Gerbera is distributed in the hope that it will be useful,
#  but WITHOUT ANY WARRANTY; without even the implied warranty of
#  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
#  GNU General Public License for more details.
#
#  You should have received a copy of the GNU General Public License
#  along with Gerbera.  If not, see <http://www.gnu.org/licenses/>.
#
#  $Id$
# ~~~

# Install the original artwork plus filename aliases used by the stock layout. Keep this usable on
# its own for a staged installation check.
install(
    DIRECTORY "${CMAKE_CURRENT_LIST_DIR}/"
    DESTINATION "${CMAKE_INSTALL_DATADIR}/container-icons"
    COMPONENT ContainerIcons
    FILES_MATCHING
    PATTERN "*.png"
    PATTERN "LICENSE.txt"
    PATTERN "README.rst"
    PATTERN "manifest.json")

foreach(_container_icon_theme IN ITEMS "." "gerbera")
    set(_container_icon_destination "${CMAKE_INSTALL_DATADIR}/container-icons")
    if(NOT _container_icon_theme STREQUAL ".")
        string(APPEND _container_icon_destination "/${_container_icon_theme}")
    endif()
    foreach(
        _container_icon_alias IN
        ITEMS "computer;PC Directory"
              "audio;Audio"
              "photos;Photos"
              "videos;Video"
              "playlists;Playlists"
              "albums;Albums"
              "albums;Alben"
              "artists;Artists"
              "composers;Composers"
              "genres;Genres"
              "all-audio;All Audio"
              "all-full-name;All - full name"
              "tracks;All Songs"
              "photos;All Photos"
              "videos;All Video"
              "playlists;All Playlists"
              "directory;Directories"
              "directory;content"
              "tracks;titel"
              "year;Year"
              "date;Date"
              "favorites;Favorites")
        list(GET _container_icon_alias 0 _container_icon_source)
        list(GET _container_icon_alias 1 _container_icon_name)
        install(
            FILES "${CMAKE_CURRENT_LIST_DIR}/${_container_icon_theme}/${_container_icon_source}.png"
            DESTINATION "${_container_icon_destination}"
            RENAME "${_container_icon_name}.png"
            COMPONENT ContainerIcons)
        install(
            FILES "${CMAKE_CURRENT_LIST_DIR}/${_container_icon_theme}/${_container_icon_source}.png"
            DESTINATION "${_container_icon_destination}"
            RENAME "object.container-${_container_icon_name}.png"
            COMPONENT ContainerIcons)
    endforeach()
    foreach(_container_icon_album_class IN
            ITEMS "object.container.album" "object.container.album.musicAlbum"
                  "object.container.album.photoAlbum")
        foreach(_container_icon_album_name IN ITEMS "Albums" "Alben")
            install(
                FILES "${CMAKE_CURRENT_LIST_DIR}/${_container_icon_theme}/albums.png"
                DESTINATION "${_container_icon_destination}"
                RENAME "${_container_icon_album_class}-${_container_icon_album_name}.png"
                COMPONENT ContainerIcons)
        endforeach()
    endforeach()
    foreach(_container_icon_fallback IN
            ITEMS "directory;object.container" "album-single;object.container.album"
                  "playlists;object.container.playlistContainer")
        list(GET _container_icon_fallback 0 _container_icon_source)
        list(GET _container_icon_fallback 1 _container_icon_name)
        install(
            FILES "${CMAKE_CURRENT_LIST_DIR}/${_container_icon_theme}/${_container_icon_source}.png"
            DESTINATION "${_container_icon_destination}"
            RENAME "${_container_icon_name}.png"
            COMPONENT ContainerIcons)
    endforeach()
endforeach()

foreach(_container_icon_fragment IN ITEMS "container-icons.xml;gerbera"
                                          "container-icons-techno.xml;.")
    list(GET _container_icon_fragment 0 _container_icon_config_name)
    list(GET _container_icon_fragment 1 CONTAINER_ICON_THEME_DIR)
    configure_file("${PROJECT_SOURCE_DIR}/config/container-icons.xml.in"
                   "${PROJECT_BINARY_DIR}/${_container_icon_config_name}" @ONLY)
    install(
        FILES "${PROJECT_BINARY_DIR}/${_container_icon_config_name}"
        DESTINATION "${CMAKE_INSTALL_DATADIR}/container-icons"
        COMPONENT ContainerIcons)
endforeach()
