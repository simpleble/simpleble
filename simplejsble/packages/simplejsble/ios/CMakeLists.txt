cmake_minimum_required(VERSION 3.21)

# Determine if we're in the SimpleBLE monorepo or npm package
# In monorepo: packages/simplejsble/ios → ../../../.. has cmake/
# In npm: node_modules/simplejsble/ios → .. has cmake/ (bundled)
# This is used by the podspec's prepare_command when building the XCFramework
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/../cmake/prelude.cmake")
    # npm package - bundled cmake
    set(PROJECT_ROOT_DIR ${CMAKE_CURRENT_SOURCE_DIR}/..)
elseif(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/../../../../cmake/prelude.cmake")
    # Monorepo
    set(PROJECT_ROOT_DIR ${CMAKE_CURRENT_SOURCE_DIR}/../../../..)
else()
    message(FATAL_ERROR "Cannot find SimpleBLE cmake files. Checked:\n  ${CMAKE_CURRENT_SOURCE_DIR}/../cmake/prelude.cmake\n  ${CMAKE_CURRENT_SOURCE_DIR}/../../../../cmake/prelude.cmake")
endif()

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
