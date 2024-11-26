#*GRB*
#
#  Gerbera - https://gerbera.io/
#
#  CMakeLists.txt - this file is part of Gerbera.
#
#  Copyright (C) 2016-2024 Gerbera Contributors
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

add_executable(testconfig
    main.cc
    test_configgenerator.cc
    test_configmanager.cc
)

if (NOT TARGET GTest::gmock)
    target_link_libraries(testconfig PRIVATE
        libgerbera
        GTest::GTest
    )
else()
    target_link_libraries(testconfig PRIVATE
        libgerbera
        GTest::gmock
    )
endif()

target_compile_definitions(testconfig PRIVATE CMAKE_BINARY_DIR="${CMAKE_BINARY_DIR}")
add_dependencies(testconfig libgerbera gerbera)

include(GoogleTest)

gtest_discover_tests(testconfig DISCOVERY_TIMEOUT 60 TEST_LIST GRB_CONFIG_TESTS)

add_test(NAME configFixtures
    COMMAND ${CMAKE_COMMAND} -E copy_directory ${CMAKE_CURRENT_SOURCE_DIR}/fixtures ${CMAKE_CURRENT_BINARY_DIR}/fixtures
    # DEPENDS fixtures/*
)
set_tests_properties(configFixtures PROPERTIES FIXTURES_SETUP GrbConfig)

set_property(DIRECTORY APPEND PROPERTY
    TEST_INCLUDE_FILES ${CMAKE_CURRENT_LIST_DIR}/CTestManip.cmake
)
