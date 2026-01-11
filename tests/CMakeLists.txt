# SODA Player Tests

find_package(GTest QUIET)

if(GTest_FOUND)
    enable_testing()

    add_executable(soda_tests
        test_main.cpp
        test_string_utils.cpp
        test_queue.cpp
        test_playlist.cpp
    )

    target_link_libraries(soda_tests
        soda_core
        GTest::gtest
        GTest::gtest_main
    )

    include(GoogleTest)
    gtest_discover_tests(soda_tests)
else()
    message(STATUS "GTest not found, tests will not be built")
endif()
