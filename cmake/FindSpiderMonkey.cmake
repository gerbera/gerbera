INCLUDE( FindPkgConfig )

PKG_SEARCH_MODULE( JS ${_pkgconfig_REQUIRED} mozjs185 )

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(JS DEFAULT_MSG LIBMAGIC_INCLUDE_DIR)

MARK_AS_ADVANCED( JS_LIBRARIES JS_INCLUDE_DIRS )