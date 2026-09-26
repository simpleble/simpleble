cmake_minimum_required(VERSION 3.21)

include(${CMAKE_CURRENT_SOURCE_DIR}/../../cmake/prelude.cmake)
project(simpleble_hitl_host LANGUAGES CXX)
include(${CMAKE_CURRENT_SOURCE_DIR}/../../cmake/epilogue.cmake)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/../../simpleble simpleble)
find_package(Threads REQUIRED)
find_package(googletest REQUIRED)

add_library(hitl_support
    fixture.cpp
    rtt.cpp
)
target_include_directories(hitl_support PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(hitl_support
    PUBLIC simpleble::simpleble Threads::Threads
    PRIVATE ${CMAKE_DL_LIBS}
)

add_executable(hitl_basic scenarios/basic.cpp)
target_link_libraries(hitl_basic PRIVATE hitl_support)

add_executable(hitl_tests tests.cpp)
target_link_libraries(hitl_tests PRIVATE hitl_support GTest::gtest)

# Backend headers are available for implementation-specific regression tests.
target_include_directories(hitl_tests PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/../../simpleble/src)

enable_testing()

add_test(NAME hitl
    COMMAND hitl_tests --gtest_filter=Mtu/* --gtest_output=xml:hitl-results.xml
)
set_tests_properties(hitl PROPERTIES TIMEOUT 1800 RUN_SERIAL TRUE)

add_test(NAME hitl_control_encoding
    COMMAND hitl_tests --gtest_filter=ControlEncoding.*
)
set_tests_properties(hitl_control_encoding PROPERTIES TIMEOUT 10)
