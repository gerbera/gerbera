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

add_executable(testdb
    main.cc
    test_database.cc
    test_sql_generators.cc
    mysql_config_fake.h
    sqlite_config_fake.h)

if (NOT TARGET GTest::gmock)
    target_link_libraries(testdb PRIVATE
        libgerbera
        GTest::GTest
    )
else()
    target_link_libraries(testdb PRIVATE
        libgerbera
        GTest::gmock
    )
endif()
add_dependencies(testdb libgerbera gerbera)

include(GoogleTest)
gtest_discover_tests(testdb DISCOVERY_TIMEOUT 60 TEST_LIST GRB_DB_TESTS)

add_test(NAME dbFixturesSqLite
    COMMAND ${CMAKE_COMMAND} -E copy ${PROJECT_SOURCE_DIR}/src/database/sqlite3/sqlite3.sql ${CMAKE_CURRENT_BINARY_DIR}
    # DEPENDS fixtures/*
)
add_test(NAME dbFixturesSqliteUpgrade
    COMMAND ${CMAKE_COMMAND} -E copy ${PROJECT_SOURCE_DIR}/src/database/sqlite3/sqlite3-upgrade.xml ${CMAKE_CURRENT_BINARY_DIR}
    # DEPENDS fixtures/*
)
add_test(NAME dbFixturesMySql
    COMMAND ${CMAKE_COMMAND} -E copy ${PROJECT_SOURCE_DIR}/src/database/mysql/mysql.sql ${CMAKE_CURRENT_BINARY_DIR}
    # DEPENDS fixtures/*
)
add_test(NAME dbFixtureMySqlUpgrade
    COMMAND ${CMAKE_COMMAND} -E copy ${PROJECT_SOURCE_DIR}/src/database/mysql/mysql-upgrade.xml ${CMAKE_CURRENT_BINARY_DIR}
    # DEPENDS fixtures/*
)
set_tests_properties(dbFixturesSqLite dbFixturesSqliteUpgrade dbFixturesMySql dbFixtureMySqlUpgrade PROPERTIES FIXTURES_SETUP GrbDb)

set_property(DIRECTORY APPEND PROPERTY
    TEST_INCLUDE_FILES ${CMAKE_CURRENT_LIST_DIR}/CTestManip.cmake
)
