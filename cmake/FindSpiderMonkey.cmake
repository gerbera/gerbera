find_package(PkgConfig REQUIRED)

PKG_SEARCH_MODULE( JS REQUIRED mozjs185 )

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args( JS DEFAULT_MSG JS_INCLUDE_DIRS )

MARK_AS_ADVANCED( JS_LIBRARIES JS_INCLUDE_DIRS )