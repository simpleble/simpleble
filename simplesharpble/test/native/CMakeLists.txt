cmake_minimum_required(VERSION 3.21)
project(simplesharpble_test_native LANGUAGES C CXX)
get_filename_component(REPO_ROOT "${CMAKE_CURRENT_SOURCE_DIR}/../../.." ABSOLUTE)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin")
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/lib")
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/lib")
set(SIMPLEBLE_PLAIN ON CACHE BOOL "No hardware in managed tests" FORCE)
set(BUILD_SHARED_LIBS ON)
add_subdirectory("${REPO_ROOT}/simplecble" "${CMAKE_BINARY_DIR}/simplecble")
# NOTE: SimpleBLE is built into simplecble, so the fixture goes into the same library to reach the same SimpleBLE.
target_sources(simplecble PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/fixture.cpp")
target_include_directories(simplecble PRIVATE
    "${REPO_ROOT}/simpleble/src/backends/common" "${REPO_ROOT}/simpleble/src/builders"
    "${REPO_ROOT}/simpleble/src" "${REPO_ROOT}/dependencies/internal/include"
    "${REPO_ROOT}/dependencies/external")
if(MSVC)
    # The fixture includes simpleble headers that use fmt, which requires UTF-8.
    target_compile_options(simplecble PRIVATE "/utf-8")
endif()
