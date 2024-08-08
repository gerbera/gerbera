# generateDoc.cmake
# -helper macro to add a "doc" target with CMake build system.
# and configure doxy.config.in to doxy.config
#
# target "doc" allows building the documentation with doxygen/dot on Linux and Mac
#

find_package(Doxygen REQUIRED dot)

macro(generate_documentation DOX_CONFIG_FILE)
    if(NOT EXISTS "${DOX_CONFIG_FILE}")
        message(FATAL_ERROR "Configuration file for doxygen not found")
    endif()

    #Define variables
    get_target_property(GRB_COMPILE_DEFINITIONS libgerbera COMPILE_DEFINITIONS)
    string (REPLACE ";" " " GRB_COMPILE_DEFINITIONS_STR "${GRB_COMPILE_DEFINITIONS}")
    set(SRCDIR  "${PROJECT_SOURCE_DIR}/src")
    set(ROOTDIR "${PROJECT_SOURCE_DIR}")
    set(BINDIR  "${PROJECT_BINARY_DIR}")
    set(PREDEF  "${GRB_COMPILE_DEFINITIONS_STR}")

    configure_file(${DOX_CONFIG_FILE} ${CMAKE_CURRENT_BINARY_DIR}/doxy.config @ONLY)

    set(DOXY_CONFIG "${CMAKE_CURRENT_BINARY_DIR}/doxy.config")
    add_custom_target(doc ${DOXYGEN_EXECUTABLE} ${DOXY_CONFIG})

    set_property(DIRECTORY APPEND PROPERTY ADDITIONAL_MAKE_CLEAN_FILES doc)
endmacro()