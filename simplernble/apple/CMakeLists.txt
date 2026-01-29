cmake_minimum_required(VERSION 3.21)

# This is used by the podspec's prepare_command when building the XCFramework
set(PROJECT_ROOT_DIR ${CMAKE_CURRENT_SOURCE_DIR}/..)

set(BUILD_SHARED_LIBS OFF)

include(${PROJECT_ROOT_DIR}/cmake/prelude.cmake)

project(simpleble-ios VERSION ${SIMPLEBLE_VERSION_BASE} LANGUAGES CXX C OBJCXX)

include(${PROJECT_ROOT_DIR}/cmake/epilogue.cmake)

add_subdirectory(${PROJECT_ROOT_DIR}/simpleble ${CMAKE_BINARY_DIR}/simpleble)
