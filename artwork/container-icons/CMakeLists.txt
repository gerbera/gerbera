# Install the original artwork plus filename aliases used by the stock layout.
# Keep this usable on its own for a staged installation check.
install(
  DIRECTORY "${CMAKE_CURRENT_LIST_DIR}/"
  DESTINATION "${CMAKE_INSTALL_DATADIR}/container-icons"
  FILES_MATCHING
  PATTERN "*.png"
  PATTERN "LICENSE.txt"
  PATTERN "README.rst"
  PATTERN "manifest.json")

foreach(
  _container_icon_alias IN
  ITEMS "computer;PC Directory"
        "tracks;Audio"
        "photos;Photos"
        "videos;Video"
        "playlists;Playlists"
        "albums;Albums"
        "artists;Artists"
        "composers;Composers"
        "genres;Genres"
        "tracks;All Audio"
        "tracks;All - full name"
        "tracks;All Songs"
        "photos;All Photos"
        "videos;All Video"
        "playlists;All Playlists"
        "directory;Directories"
        "year;Year"
        "date;Date"
        "favorites;Favorites")
  list(GET _container_icon_alias 0 _container_icon_source)
  list(GET _container_icon_alias 1 _container_icon_name)
  install(
    FILES "${CMAKE_CURRENT_LIST_DIR}/${_container_icon_source}.png"
    DESTINATION "${CMAKE_INSTALL_DATADIR}/container-icons"
    RENAME "${_container_icon_name}.png")
endforeach()

configure_file("${PROJECT_SOURCE_DIR}/config/container-icons.xml.in"
               "${PROJECT_BINARY_DIR}/container-icons.xml" @ONLY)
install(FILES "${PROJECT_BINARY_DIR}/container-icons.xml"
        DESTINATION "${CMAKE_INSTALL_DATADIR}/container-icons")
