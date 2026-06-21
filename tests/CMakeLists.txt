cmake_minimum_required(VERSION 3.16)
project(prometheus_client_cxx_tests LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_COMPILE_WARNING_AS_ERROR ON)

find_package(GTest CONFIG REQUIRED)
enable_testing()

file(GLOB TEST_SOURCES "test_*.cpp")

add_executable(runTests ${TEST_SOURCES})
add_executable(runTsan tsan.cpp)
add_executable(runAsan tsan.cpp)

target_link_libraries(runTests
    PRIVATE
        GTest::gtest
        GTest::gtest_main
        GTest::gmock
)

target_compile_options(runTsan PRIVATE -fsanitize=thread -g -O0)
target_link_options(runTsan PRIVATE -fsanitize=thread)

target_compile_options(runAsan PRIVATE -fsanitize=address -g -O0)
target_link_options(runAsan PRIVATE -fsanitize=address)

include(GoogleTest)
gtest_discover_tests(runTests)
