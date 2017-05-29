# - Try to find duktape
# Once done this will define
#
#  DUKTAPE_FOUND - system has Duktape
#  DUKTAPE_INCLUDE_DIRS - the Duktape include directory
#  DUKTAPE_LIBRARIES - Link these to use DUKTAPE
#  DUKTAPE_DEFINITIONS - Compiler switches required for using Duktape
#

INCLUDE (FindPackageHandleStandardArgs)

find_package( PkgConfig REQUIRED )
PKG_SEARCH_MODULE( DUKTAPE REQUIRED duktape libduktape )

find_package_handle_standard_args(DUKTAPE DEFAULT_MSG DUKTAPE_FOUND)

MARK_AS_ADVANCED( DUKTAPE_LIBRARIES DUKTAPE_INCLUDE_DIRS )
