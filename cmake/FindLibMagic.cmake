INCLUDE (FindPackageHandleStandardArgs)

find_package(PkgConfig QUIET)

pkg_check_modules(PC_MAGIC QUIET libmagic)

FIND_PATH(LibMagic_INCLUDE_DIR magic.h
    HINTS ${PC_MAGIC_INCLUDEDIR} ${PC_MAGIC_INCLUDE_DIRS})
FIND_LIBRARY(LibMagic_LIBRARIES NAMES magic
    HINTS ${PC_MAGIC_LIBDIR} ${PC_MAGIC_LIBRARY_DIRS})

# handle the QUIETLY and REQUIRED arguments and set MAGIC_FOUND to TRUE
find_package_handle_standard_args(LibMagic DEFAULT_MSG LibMagic_LIBRARIES)

if (LibMagic_FOUND)
    set (LibMagic_LIBRARIES ${MAGIC_LIBRARY} ${PC_MAGIC_LIBRARIES})
    set (LibMagic_INCLUDE_DIRS ${MAGIC_INCLUDE_DIR} )
endif ()

MARK_AS_ADVANCED(
        LibMagic_LIBRARIES
        LibMagic_INCLUDE_DIRS
)
