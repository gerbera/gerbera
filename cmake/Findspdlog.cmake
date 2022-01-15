# This is a wrapper for spdlog to cover case when spdlog cmake config
# is not installed (on some Joyent systems).
include(FindPackageHandleStandardArgs)

find_package(spdlog QUIET NO_MODULE)

if(spdlog_FOUND)
    find_package_handle_standard_args(spdlog CONFIG_MODE)

    if(NOT SPDLOG_FMT_EXTERNAL AND NOT spdlog_COMPILE_DEFINITIONS MATCHES "SPDLOG_FMT_EXTERNAL")
        message(WARNING [=[
spdlog is built without SPDLOG_FMT_EXTERNAL.
Since Gerbera uses fmt library internally the spdlog's bundled version wlll be in conflict
It is strongly recommended to rebuild spdlog without bundled fmt]=])
        if(spdlog_VERSION VERSION_GREATER_EQUAL "1.4.0" AND fmt_VERSION VERSION_LESS "6.0.0")
            message(FATAL_ERROR [=[
The version of spdlog >= 1.4 bundles fmt version 6, but an older version of fmt was found.
The current combination won't link.
Please upgrade fmt or build spdlog with SPDLOG_FMT_EXTERNAL=ON]=])
        endif()
    endif()
else()
    # See if it's just installed as a library, as not all installs have .cmake files
    find_library(spdlog_LIBRARY spdlog)
    find_path(spdlog_INCLUDE_DIR spdlog/spdlog.h)

    find_package_handle_standard_args(spdlog REQUIRED_VARS spdlog_LIBRARY spdlog_INCLUDE_DIR)

    if(spdlog_FOUND)
        set(spdlog_LIBRARIES ${spdlog_LIBRARY})
        set(spdlog_INCLUDE_DIRS ${spdlog_INCLUDE_DIR})

        if(NOT TARGET spdlog::spdlog)
            add_library(spdlog::spdlog INTERFACE IMPORTED)
            set_target_properties(spdlog::spdlog PROPERTIES INTERFACE_INCLUDE_DIRECTORIES "${spdlog_INCLUDE_DIRS}")
            set_property(TARGET spdlog::spdlog PROPERTY INTERFACE_LINK_LIBRARIES "${spdlog_LIBRARIES}")
        endif()
    endif()

    mark_as_advanced(spdlog_LIBRARY spdlog_INCLUDE_DIR)
endif()
