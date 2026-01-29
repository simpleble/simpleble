cmake_minimum_required(VERSION 3.9.0)

set(PROJECT_ROOT_DIR ${CMAKE_SOURCE_DIR}/..)
if(NOT EXISTS "${PROJECT_ROOT_DIR}/cmake/prelude.cmake")
    message(FATAL_ERROR "Cannot find bundled SimpleBLE cmake files at ${PROJECT_ROOT_DIR}/cmake. Make sure you're using the npm package contents.")
endif()

file(READ "${PROJECT_ROOT_DIR}/VERSION" VERSION_FILE_CONTENTS)
string(STRIP "${VERSION_FILE_CONTENTS}" SIMPLEBLE_VERSION_BASE)

include(${PROJECT_ROOT_DIR}/cmake/prelude.cmake)

project(NitroSimplernble VERSION ${SIMPLEBLE_VERSION_BASE} LANGUAGES CXX)

include(${PROJECT_ROOT_DIR}/cmake/epilogue.cmake)

set(CMAKE_VERBOSE_MAKEFILE ON)
set(CMAKE_CXX_STANDARD 20)

set(BUILD_SHARED_LIBS OFF)
add_subdirectory(${PROJECT_ROOT_DIR}/simpleble ${CMAKE_BINARY_DIR}/simpleble)
set(BUILD_SHARED_LIBS ON)

file(GLOB_RECURSE SHARED_CPP_FILES "../cpp/*.cpp")

# Define C++ library and add all sources - Nitro generated
add_library(NitroSimplernble SHARED
    src/main/cpp/cpp-adapter.cpp
    ${SHARED_CPP_FILES}
)

# Add Nitrogen specs - Nitro generated
include(${CMAKE_SOURCE_DIR}/../nitrogen/generated/android/NitroSimplernble+autolinking.cmake)

# Set up local includes - Nitro generated
target_include_directories(NitroSimplernble PRIVATE
    src/main/cpp
    ../cpp
)

# Nitro generated 
find_library(LOG_LIB log)

# Link all libraries together
target_link_libraries(
    NitroSimplernble
    ${LOG_LIB}
    simpleble::simpleble # SimpleBLE library
    android     # Android core - Nitro generated
    -ldl
)