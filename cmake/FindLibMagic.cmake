INCLUDE (FindPackageHandleStandardArgs)

FIND_PATH(MAGIC_INCLUDE_DIR magic.h)

FIND_LIBRARY(MAGIC_LIBRARIES NAMES magic)

# handle the QUIETLY and REQUIRED arguments and set LIBMAGIC_FOUND to TRUE
include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(MAGIC DEFAULT_MSG MAGIC_INCLUDE_DIR)

MARK_AS_ADVANCED( MAGIC_LIBRARIES MAGIC_INCLUDE_DIRS )