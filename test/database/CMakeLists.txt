add_executable(testdb
    main.cc
    test_database.cc
    test_sql_generators.cc
    mysql_config_fake.h
    sqlite_config_fake.h)

add_custom_command(TARGET testdb POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy ${PROJECT_SOURCE_DIR}/src/database/sqlite3/sqlite3.sql ${CMAKE_CURRENT_BINARY_DIR}
    COMMAND ${CMAKE_COMMAND} -E copy ${PROJECT_SOURCE_DIR}/src/database/sqlite3/sqlite3-upgrade.xml ${CMAKE_CURRENT_BINARY_DIR}
    COMMAND ${CMAKE_COMMAND} -E copy ${PROJECT_SOURCE_DIR}/src/database/mysql/mysql.sql ${CMAKE_CURRENT_BINARY_DIR}
    COMMAND ${CMAKE_COMMAND} -E copy ${PROJECT_SOURCE_DIR}/src/database/mysql/mysql-upgrade.xml ${CMAKE_CURRENT_BINARY_DIR}
    COMMENT "Copying Database Scripts"
)

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

include(GoogleTest)
gtest_discover_tests(testdb DISCOVERY_TIMEOUT 60)
