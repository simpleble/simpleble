cmake_minimum_required(VERSION 3.21)

# Path: packages/simplejsble/ios → ../../../.. → repo root (where simpleble/ lives)
set(PROJECT_ROOT_DIR ${CMAKE_CURRENT_SOURCE_DIR}/../../../..)
set(BUILD_SHARED_LIBS OFF)  # Static library for iOS

include(${PROJECT_ROOT_DIR}/cmake/prelude.cmake)

project(simpleble-ios VERSION ${SIMPLEBLE_VERSION_BASE} LANGUAGES CXX C OBJCXX)

include(${PROJECT_ROOT_DIR}/cmake/epilogue.cmake)

add_subdirectory(${PROJECT_ROOT_DIR}/simpleble ${CMAKE_BINARY_DIR}/simpleble)

# Install library
install(TARGETS simpleble ARCHIVE DESTINATION lib)

# Install headers (including generated export.h)
install(DIRECTORY ${PROJECT_ROOT_DIR}/simpleble/include/simpleble/ DESTINATION include/simpleble)
install(DIRECTORY ${CMAKE_BINARY_DIR}/simpleble/export/simpleble/ DESTINATION include/simpleble)
install(FILES ${PROJECT_ROOT_DIR}/dependencies/external/kvn/kvn_bytearray.h DESTINATION include/simpleble/kvn)
