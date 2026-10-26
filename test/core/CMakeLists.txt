# ~~~
# *GRB*
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
# ~~~

include(GoogleTest)
generate_compile_info()

add_executable(
    testcore
    main.cc #
    test_bpm_metadata.cc #
    test_ffmpeg_cache_paths.cc #
    test_searchhandler.cc #
    test_server.cc #
    test_upnp_map.cc #
    test_upnp_xml.cc #
    test_url_utils.cc #
    test_video_metadata.cc #
)

if(NOT TARGET GTest::gmock)
    target_link_libraries(testcore PRIVATE libgerbera GTest::GTest)
else()
    target_link_libraries(testcore PRIVATE libgerbera GTest::gmock)
endif()

target_compile_definitions(testcore PRIVATE CMAKE_BINARY_DIR="$<TARGET_FILE_DIR:gerbera>")
target_compile_definitions(testcore PRIVATE COMPILE_INFO="${COMPILE_INFO}")
target_compile_definitions(testcore PRIVATE GIT_BRANCH="${GIT_BRANCH}")
target_compile_definitions(testcore PRIVATE GIT_COMMIT_HASH="${GIT_COMMIT_HASH}")
add_dependencies(testcore libgerbera gerbera)

gtest_discover_tests(testcore DISCOVERY_TIMEOUT 60 TEST_LIST GRB_CORE_TESTS)

add_test(
    NAME coreFixtures
    COMMAND
        ${CMAKE_COMMAND} -E copy_directory ${CMAKE_CURRENT_SOURCE_DIR}/fixtures
        ${CMAKE_CURRENT_BINARY_DIR}/fixtures
        # DEPENDS fixtures/*
)
set_tests_properties(coreFixtures PROPERTIES FIXTURES_SETUP GrbCore)

if(CMAKE_SYSTEM_NAME STREQUAL "Linux")
    find_package(Python3 QUIET COMPONENTS Interpreter)
    if(Python3_Interpreter_FOUND)
        add_test(NAME ShutdownDuringStartup
                 COMMAND ${Python3_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/test_shutdown.py
                         --binary $<TARGET_FILE:gerbera> --source-root ${CMAKE_SOURCE_DIR})
        set_tests_properties(ShutdownDuringStartup PROPERTIES RUN_SERIAL TRUE TIMEOUT 240)
    endif()
endif()

set_property(
    DIRECTORY
    APPEND
    PROPERTY TEST_INCLUDE_FILES ${CMAKE_CURRENT_LIST_DIR}/CTestManip.cmake)
