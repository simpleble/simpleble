cmake_minimum_required(VERSION 3.9.0)

# Determine if we're in the SimpleBLE monorepo or npm package
# In monorepo: packages/simplejsble/android → ../../../../ has cmake/
# In npm: node_modules/simplejsble/android → ../ has cmake/ (bundled)
if(EXISTS "${CMAKE_SOURCE_DIR}/../cmake/prelude.cmake")
    # npm package - bundled cmake
    set(PROJECT_ROOT_DIR ${CMAKE_SOURCE_DIR}/..)
elseif(EXISTS "${CMAKE_SOURCE_DIR}/../../../../cmake/prelude.cmake")
    # Monorepo
    set(PROJECT_ROOT_DIR ${CMAKE_SOURCE_DIR}/../../../..)
else()
    message(FATAL_ERROR "Cannot find SimpleBLE cmake files. Make sure you're using a complete package.")
endif()

# Read version from VERSION file
file(READ "${PROJECT_ROOT_DIR}/VERSION" VERSION_FILE_CONTENTS)
string(STRIP "${VERSION_FILE_CONTENTS}" SIMPLEBLE_VERSION_BASE)

# Include SimpleBLE prelude
include(${PROJECT_ROOT_DIR}/cmake/prelude.cmake)

project(NitroSimplejsble VERSION ${SIMPLEBLE_VERSION_BASE} LANGUAGES CXX)

# Include SimpleBLE epilogue
include(${PROJECT_ROOT_DIR}/cmake/epilogue.cmake)

set(CMAKE_VERBOSE_MAKEFILE ON)
set(CMAKE_CXX_STANDARD 20)

# Build SimpleBLE as static library
set(BUILD_SHARED_LIBS OFF)
add_subdirectory(${PROJECT_ROOT_DIR}/simpleble ${CMAKE_BINARY_DIR}/simpleble)
set(BUILD_SHARED_LIBS ON)

# Remove nativehelper from SimpleBLE (not available in React Native)
get_target_property(SIMPLEBLE_LINK_LIBS simpleble INTERFACE_LINK_LIBRARIES)
if(SIMPLEBLE_LINK_LIBS)
    list(REMOVE_ITEM SIMPLEBLE_LINK_LIBS nativehelper)
    set_target_properties(simpleble PROPERTIES INTERFACE_LINK_LIBRARIES "${SIMPLEBLE_LINK_LIBS}")
endif()

# Glob shared C++ files
file(GLOB_RECURSE SHARED_CPP_FILES "../cpp/*.cpp")

# Define C++ library and add all sources
add_library(NitroSimplejsble SHARED
    src/main/cpp/cpp-adapter.cpp
    ${SHARED_CPP_FILES}
)

# Add Nitrogen specs
include(${CMAKE_SOURCE_DIR}/../nitrogen/generated/android/NitroSimplejsble+autolinking.cmake)

# Set up local includes
target_include_directories(NitroSimplejsble PRIVATE
    src/main/cpp
    ../cpp
)

find_library(LOG_LIB log)

# Link all libraries together
target_link_libraries(NitroSimplejsble
    ${LOG_LIB}
    simpleble::simpleble
    android
    -ldl
)

# Allow undefined JNI symbols (resolved at runtime)
set_target_properties(NitroSimplejsble PROPERTIES
    LINK_FLAGS "-Wl,--allow-shlib-undefined -Wl,--unresolved-symbols=ignore-in-object-files"
)

# Add SimpleBLE compile definitions
target_compile_definitions(NitroSimplejsble PRIVATE FMT_HEADER_ONLY FMT_UNICODE=0)
