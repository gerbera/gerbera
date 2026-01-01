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

add_executable(testcontent
    main.cc
    test_autoscan_list.cc
    test_resolution.cc
)

if (NOT TARGET GTest::gmock)
    target_link_libraries(testcontent PRIVATE
        libgerbera
        GTest::GTest
    )
else()
    target_link_libraries(testcontent PRIVATE
        libgerbera
        GTest::gmock
    )
endif()
add_dependencies(testcontent libgerbera gerbera)

include(GoogleTest)
gtest_discover_tests(testcontent DISCOVERY_TIMEOUT 60)
