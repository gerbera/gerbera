#*GRB*
#
#  Gerbera - https://gerbera.io/
#
#  generateDoc.cmake - this file is part of Gerbera.
#
#  Copyright (C) 2024-2026 Gerbera Contributors
#
#  Gerbera is free software; you can redistribute it and/or modify
#  it under the terms of the GNU General Public License version 2
#  as published by the Free Software Foundation.
#
#  Gerbera is distributed in the hope that it will be useful,
#  but WITHOUT ANY WARRANTY; without even the implied warranty of
#  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
#  GNU General Public License for more details.
#
#  You should have received a copy of the GNU General Public License
#  along with Gerbera.  If not, see <http://www.gnu.org/licenses/>.
#
#  $Id$

# helper macro to add a "doc" target with CMake build system.
# and configure doxy.config.in to doxy.config
#
# target "doc" allows building the documentation with doxygen/dot on Linux and Mac

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
    add_custom_target(doc
	    COMMAND ${DOXYGEN_EXECUTABLE} ${DOXY_CONFIG}
	    OUTPUT ${PROJECT_SOURCE_DIR}/doc/_build/html
    )

    set_property(DIRECTORY APPEND PROPERTY ADDITIONAL_MAKE_CLEAN_FILES doc)
endmacro()
