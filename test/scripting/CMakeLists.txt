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

add_executable(testscripting
    main.cc
    mock/duk_helper.h
    mock/duk_helper.cc
    mock/common_script_mock.h
    mock/script_test_fixture.h
    mock/script_test_fixture.cc
    test_common_script.cc
    test_external_m3u_playlist.cc
    test_external_pls_playlist.cc
    test_external_asx_playlist.cc
    test_import_script.cc
    test_import_initials_script.cc
    test_import_struct_script.cc
    test_internal_m3u_playlist.cc
    test_internal_m3u8_playlist.cc
    test_internal_pls_playlist.cc
    test_nfo_metafile.cc
    test_runtime.cc
)

if (NOT TARGET GTest::gmock)
    target_link_libraries(testscripting PRIVATE
        libgerbera
        GTest::GTest
    )
else()
    target_link_libraries(testscripting PRIVATE
        libgerbera
        GTest::gmock
    )
endif()

target_compile_definitions(testscripting PRIVATE SCRIPTS_DIR="${CMAKE_SOURCE_DIR}/scripts")
add_dependencies(testscripting libgerbera gerbera)

include(GoogleTest)
gtest_discover_tests(testscripting DISCOVERY_TIMEOUT 60 TEST_LIST GRB_SCRIPTING_TESTS)

add_test(NAME scriptingFixtures
    COMMAND ${CMAKE_COMMAND} -E copy_directory ${CMAKE_CURRENT_SOURCE_DIR}/fixtures ${CMAKE_CURRENT_BINARY_DIR}/fixtures
    # DEPENDS fixtures/*
)
set_tests_properties(scriptingFixtures PROPERTIES FIXTURES_SETUP GrbScripting)

set_property(DIRECTORY APPEND PROPERTY
    TEST_INCLUDE_FILES ${CMAKE_CURRENT_LIST_DIR}/CTestManip.cmake
)
