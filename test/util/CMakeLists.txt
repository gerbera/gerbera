#*GRB*
#
#  Gerbera - https://gerbera.io/
#
#  CMakeLists.txt - this file is part of Gerbera.
#
#  Copyright (C) 2016-2025 Gerbera Contributors
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

add_executable(testutil
    main.cc
    test_tools.cc
    test_upnp_clients.cc
    test_upnp_headers.cc
    test_jpeg_res.cc
)

if (NOT TARGET GTest::gmock)
    target_link_libraries(testutil PRIVATE
        libgerbera
        GTest::GTest
    )
else()
    target_link_libraries(testutil PRIVATE
        libgerbera
        GTest::gmock
    )
endif()

add_dependencies(testutil libgerbera gerbera)

include(GoogleTest)
gtest_discover_tests(testutil DISCOVERY_TIMEOUT 60 TEST_LIST GRB_UTIL_TESTS)

add_test(NAME utilFixtures
    COMMAND ${CMAKE_COMMAND} -E copy_directory ${CMAKE_CURRENT_SOURCE_DIR}/testdata ${CMAKE_CURRENT_BINARY_DIR}/testdata
    # DEPENDS testdata/*
)
set_tests_properties(utilFixtures PROPERTIES FIXTURES_SETUP GrbUtil)

set_property(DIRECTORY APPEND PROPERTY
    TEST_INCLUDE_FILES ${CMAKE_CURRENT_LIST_DIR}/CTestManip.cmake
)
