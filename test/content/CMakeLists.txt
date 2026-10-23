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

add_executable(
    testcontent
    main.cc #
    test_autoscan_list.cc #
    test_playlist_update.cc #
    test_playlist_parser_script.cc #
    test_resolution.cc #
)

if(NOT TARGET GTest::gmock)
    target_link_libraries(testcontent PRIVATE libgerbera GTest::GTest)
else()
    target_link_libraries(testcontent PRIVATE libgerbera GTest::gmock)
endif()
add_dependencies(testcontent libgerbera gerbera)

target_compile_definitions(
    testcontent
    PRIVATE XML_PLAYLIST_FIXTURES="${CMAKE_SOURCE_DIR}/test/scripting/fixtures/xml-playlists")

include(GoogleTest)
gtest_discover_tests(testcontent DISCOVERY_TIMEOUT 60)

find_package(Python3 QUIET COMPONENTS Interpreter)
if(Python3_Interpreter_FOUND)
    add_test(NAME IntegrationRuntime
             COMMAND ${Python3_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/test_integration_runtime.py)
    set_tests_properties(IntegrationRuntime PROPERTIES TIMEOUT 60)
else()
    message(STATUS "IntegrationRuntime disabled: Python 3 interpreter not found")
endif()

if(WITH_JS
   AND WITH_INOTIFY
   AND (WITH_TAGLIB OR WITH_AVCODEC)
   AND CMAKE_SYSTEM_NAME STREQUAL "Linux")
    if(Python3_Interpreter_FOUND)
        add_test(NAME PlaylistUpdateIntegration
                 COMMAND ${Python3_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/test_playlist_updates.py
                         --binary $<TARGET_FILE:gerbera> --source-root ${CMAKE_SOURCE_DIR})
        set_tests_properties(PlaylistUpdateIntegration PROPERTIES RUN_SERIAL TRUE TIMEOUT 240)
    endif()
endif()

if(WITH_JS
   AND WITH_AVCODEC
   AND CMAKE_SYSTEM_NAME STREQUAL "Linux")
    if(Python3_Interpreter_FOUND)
        add_test(NAME XmlPlaylistIntegration
                 COMMAND ${Python3_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/test_xml_playlists.py
                         --binary $<TARGET_FILE:gerbera> --source-root ${CMAKE_SOURCE_DIR})
        set_tests_properties(XmlPlaylistIntegration PROPERTIES RUN_SERIAL TRUE TIMEOUT 240)
        add_test(NAME AsxUriIntegration
                 COMMAND ${Python3_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/test_asx_uris.py
                         --binary $<TARGET_FILE:gerbera> --source-root ${CMAKE_SOURCE_DIR})
        set_tests_properties(AsxUriIntegration PROPERTIES RUN_SERIAL TRUE TIMEOUT 300)
        add_test(
            NAME AsxSemanticsIntegration
            COMMAND
                ${Python3_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/test_asx_semantics.py --binary
                $<TARGET_FILE:gerbera> --source-root ${CMAKE_SOURCE_DIR} --output
                ${CMAKE_CURRENT_BINARY_DIR}/asx-semantics.json)
        set_tests_properties(AsxSemanticsIntegration PROPERTIES RUN_SERIAL TRUE TIMEOUT 300)
    endif()
endif()
