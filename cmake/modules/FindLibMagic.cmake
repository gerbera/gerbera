include(FindPackageHandleStandardArgs)

find_package(PkgConfig QUIET)

pkg_search_module(PC_MAGIC QUIET libmagic)

find_path(MAGIC_INCLUDE_DIR magic.h
    HINTS ${PC_MAGIC_INCLUDEDIR} ${PC_MAGIC_INCLUDE_DIRS})
find_library(MAGIC_LIBRARY NAMES magic
    HINTS ${PC_MAGIC_LIBDIR} ${PC_MAGIC_LIBRARY_DIRS})

# handle the QUIETLY and REQUIRED arguments and set MAGIC_FOUND to TRUE
find_package_handle_standard_args(LibMagic DEFAULT_MSG MAGIC_LIBRARY MAGIC_LIBRARY)

if (LibMagic_FOUND)
    set(LibMagic_LIBRARIES ${MAGIC_LIBRARY})
    set(LibMagic_INCLUDE_DIRS ${MAGIC_INCLUDE_DIR})
endif()

MARK_AS_ADVANCED(
        LibMagic_LIBRARIES
        LibMagic_INCLUDE_DIRS
)
