add_executable(testdb
        main.cc
        import.cc
        mysql_config_fake.h)

add_compile_definitions(SPDLOG_ACTIVE_LEVEL=SPDLOG_LEVEL_DEBUG)

target_link_libraries(testdb PUBLIC
        libgerbera
        GTest::GTest
)

add_test(NAME testdb COMMAND testdb)
