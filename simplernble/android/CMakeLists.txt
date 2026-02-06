cmake_minimum_required(VERSION 3.9.0)

set(PROJECT_ROOT_DIR ${CMAKE_SOURCE_DIR}/..)

project(NitroSimplernble LANGUAGES CXX)

set(CMAKE_VERBOSE_MAKEFILE ON)
set(CMAKE_CXX_STANDARD 20)

set(SIMPLEBLE_PREBUILT_DIR "${CMAKE_SOURCE_DIR}/prebuilt/${ANDROID_ABI}")
set(SIMPLEBLE_PREBUILT_LIB "${SIMPLEBLE_PREBUILT_DIR}/lib/libsimpleble.a")

add_library(simpleble STATIC IMPORTED)
set_target_properties(simpleble PROPERTIES
    IMPORTED_LOCATION "${SIMPLEBLE_PREBUILT_LIB}"
)

target_include_directories(simpleble INTERFACE
    "${SIMPLEBLE_PREBUILT_DIR}/include"
)

add_library(simpleble::simpleble ALIAS simpleble)

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
    nativehelper # Required for JNI (JNI_GetCreatedJavaVMs)
    -ldl
)