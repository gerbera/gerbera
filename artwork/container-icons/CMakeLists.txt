# Install the original artwork plus filename aliases used by the stock layout. Keep this usable on
# its own for a staged installation check.
install(
    DIRECTORY "${CMAKE_CURRENT_LIST_DIR}/"
    DESTINATION "${CMAKE_INSTALL_DATADIR}/container-icons"
    FILES_MATCHING
    PATTERN "*.png"
    PATTERN "LICENSE.txt"
    PATTERN "README.rst"
    PATTERN "manifest.json")

install(
    FILES "${CMAKE_CURRENT_LIST_DIR}/computer.png"
    DESTINATION "${CMAKE_INSTALL_DATADIR}/container-icons"
    RENAME "PC Directory.png")

install(
    FILES "${CMAKE_CURRENT_LIST_DIR}/tracks.png"
    DESTINATION "${CMAKE_INSTALL_DATADIR}/container-icons"
    RENAME "Audio.png")

install(
    FILES "${CMAKE_CURRENT_LIST_DIR}/photos.png"
    DESTINATION "${CMAKE_INSTALL_DATADIR}/container-icons"
    RENAME "Photos.png")

install(
    FILES "${CMAKE_CURRENT_LIST_DIR}/videos.png"
    DESTINATION "${CMAKE_INSTALL_DATADIR}/container-icons"
    RENAME "Video.png")

install(
    FILES "${CMAKE_CURRENT_LIST_DIR}/playlists.png"
    DESTINATION "${CMAKE_INSTALL_DATADIR}/container-icons"
    RENAME "Playlists.png")

install(
    FILES "${CMAKE_CURRENT_LIST_DIR}/albums.png"
    DESTINATION "${CMAKE_INSTALL_DATADIR}/container-icons"
    RENAME "Albums.png")

install(
    FILES "${CMAKE_CURRENT_LIST_DIR}/artists.png"
    DESTINATION "${CMAKE_INSTALL_DATADIR}/container-icons"
    RENAME "Artists.png")

install(
    FILES "${CMAKE_CURRENT_LIST_DIR}/composers.png"
    DESTINATION "${CMAKE_INSTALL_DATADIR}/container-icons"
    RENAME "Composers.png")

install(
    FILES "${CMAKE_CURRENT_LIST_DIR}/genres.png"
    DESTINATION "${CMAKE_INSTALL_DATADIR}/container-icons"
    RENAME "Genres.png")

install(
    FILES "${CMAKE_CURRENT_LIST_DIR}/tracks.png"
    DESTINATION "${CMAKE_INSTALL_DATADIR}/container-icons"
    RENAME "All Audio.png")

install(
    FILES "${CMAKE_CURRENT_LIST_DIR}/tracks.png"
    DESTINATION "${CMAKE_INSTALL_DATADIR}/container-icons"
    RENAME "All - full name.png")

install(
    FILES "${CMAKE_CURRENT_LIST_DIR}/tracks.png"
    DESTINATION "${CMAKE_INSTALL_DATADIR}/container-icons"
    RENAME "All Songs.png")

install(
    FILES "${CMAKE_CURRENT_LIST_DIR}/photos.png"
    DESTINATION "${CMAKE_INSTALL_DATADIR}/container-icons"
    RENAME "All Photos.png")

install(
    FILES "${CMAKE_CURRENT_LIST_DIR}/videos.png"
    DESTINATION "${CMAKE_INSTALL_DATADIR}/container-icons"
    RENAME "All Video.png")

install(
    FILES "${CMAKE_CURRENT_LIST_DIR}/playlists.png"
    DESTINATION "${CMAKE_INSTALL_DATADIR}/container-icons"
    RENAME "All Playlists.png")

install(
    FILES "${CMAKE_CURRENT_LIST_DIR}/directory.png"
    DESTINATION "${CMAKE_INSTALL_DATADIR}/container-icons"
    RENAME "Directories.png")

install(
    FILES "${CMAKE_CURRENT_LIST_DIR}/year.png"
    DESTINATION "${CMAKE_INSTALL_DATADIR}/container-icons"
    RENAME "Year.png")

install(
    FILES "${CMAKE_CURRENT_LIST_DIR}/date.png"
    DESTINATION "${CMAKE_INSTALL_DATADIR}/container-icons"
    RENAME "Date.png")

install(
    FILES "${CMAKE_CURRENT_LIST_DIR}/favorites.png"
    DESTINATION "${CMAKE_INSTALL_DATADIR}/container-icons"
    RENAME "Favorites.png")

configure_file("${PROJECT_SOURCE_DIR}/config/container-icons.xml.in"
               "${PROJECT_BINARY_DIR}/container-icons.xml" @ONLY)
install(
    FILES "${PROJECT_BINARY_DIR}/container-icons.xml"
    DESTINATION "${CMAKE_INSTALL_DATADIR}/container-icons")
