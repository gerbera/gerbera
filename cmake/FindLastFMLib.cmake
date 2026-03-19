include(FindPackageHandleStandardArgs)

find_package(PkgConfig REQUIRED QUIET)
pkg_search_module(LastFMLib liblastfm liblastfmlib QUIET)

find_package_handle_standard_args(
    LastFMLib DEFAULT_MSG LastFMLib_FOUND QUIET)

mark_as_advanced(
    LastFMLib_LIBRARIES LastFMLib_INCLUDE_DIRS)
