cmake_minimum_required(VERSION 3.16)
project(prometheus_client_cxx_tests LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(GTest CONFIG REQUIRED)
enable_testing()

file(GLOB TEST_SOURCES "*.cpp")

add_executable(runTests ${TEST_SOURCES})

target_link_libraries(runTests
    PRIVATE
        GTest::gtest
        GTest::gtest_main
        GTest::gmock
)

include(GoogleTest)
gtest_discover_tests(runTests)
