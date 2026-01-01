#*GRB*
#
#  Gerbera - https://gerbera.io/
#
#  CMakeLists.txt - this file is part of Gerbera.
#
#  Copyright (C) 2016-2026 Gerbera Contributors
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

find_package(GTest REQUIRED)

# CMake can't fix gmock support. https://gitlab.kitware.com/cmake/cmake/-/issues/17365
# So for non-conan build we need to find it separately.
if (NOT "${GTest_LIBRARY_LIST}" MATCHES "gmock")
    if (NOT TARGET GTest::gmock)
        message ("No GMock in GTest")
        find_package(GMock REQUIRED)
        set_property(TARGET GTest::GTest APPEND PROPERTY INTERFACE_LINK_LIBRARIES ${GMOCK_LIBRARIES})
    endif()
endif()

## Add tests below
add_subdirectory(config)
add_subdirectory(content)
add_subdirectory(core)
if(WITH_JS)
    add_subdirectory(scripting)
endif()
add_subdirectory(database)
add_subdirectory(util)
