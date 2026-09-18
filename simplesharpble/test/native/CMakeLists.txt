cmake_minimum_required(VERSION 3.21)
project(simplesharpble_test_native LANGUAGES C CXX)
get_filename_component(REPO_ROOT "${CMAKE_CURRENT_SOURCE_DIR}/../../.." ABSOLUTE)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin")
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/lib")
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/lib")
set(SIMPLEBLE_PLAIN ON CACHE BOOL "No hardware in managed tests" FORCE)
set(BUILD_SHARED_LIBS ON)
add_subdirectory("${REPO_ROOT}/simplecble" "${CMAKE_BINARY_DIR}/simplecble")
add_library(simplesharpble_test_native SHARED fixture.cpp
    "${REPO_ROOT}/simpleble/src/backends/common/ServiceBase.cpp"
    "${REPO_ROOT}/simpleble/src/backends/common/CharacteristicBase.cpp"
    "${REPO_ROOT}/simpleble/src/backends/common/DescriptorBase.cpp")
target_compile_features(simplesharpble_test_native PRIVATE cxx_std_17)
target_include_directories(simplesharpble_test_native PRIVATE
    "${REPO_ROOT}/simpleble/src/backends/common" "${REPO_ROOT}/simpleble/src/builders"
    "${REPO_ROOT}/simpleble/src" "${REPO_ROOT}/dependencies/internal/include"
    "${REPO_ROOT}/dependencies/external")
target_link_libraries(simplesharpble_test_native PRIVATE simplecble simpleble)
