cmake_minimum_required(VERSION 3.21)

include(${CMAKE_CURRENT_SOURCE_DIR}/../cmake/prelude.cmake)

project(
    simplecble
    VERSION ${SIMPLEBLE_VERSION_BASE}
    DESCRIPTION "The ultimate fully-fledged cross-platform library for Bluetooth Low Energy (BLE)."
    HOMEPAGE_URL "https://github.com/simpleble/simpleble"
    LANGUAGES CXX
)

include(${CMAKE_CURRENT_SOURCE_DIR}/../cmake/epilogue.cmake)

include(GenerateExportHeader)
include(GNUInstallDirs)
include(CMakePackageConfigHelpers)

if(SIMPLECBLE_TEST)
    message(STATUS "Building tests requires plain version of SimpleBLE")
    set(SIMPLEBLE_PLAIN ON)
endif()

add_subdirectory(${CMAKE_CURRENT_LIST_DIR}/../simpleble ${CMAKE_BINARY_DIR}/simpleble)

set(SIMPLEBLE_C_SRC
    ${CMAKE_CURRENT_SOURCE_DIR}/src/simplecble.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/adapter.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/advanced.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/advanced_dongl.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/backend.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/config.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/error.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/peripheral.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/logging.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/local_peripheral.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/local_service.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/local_characteristic.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/utils.cpp)

add_library(simplecble ${SIMPLEBLE_C_SRC})
add_library(simplecble::simplecble ALIAS simplecble)

# NOTE: This is a temporary alias to ensure backwards compatibility with the old C bindings.
add_library(simpleble-c ALIAS simplecble)
add_library(simpleble::simpleble-c ALIAS simplecble)

set_target_properties(simplecble PROPERTIES
    C_VISIBILITY_PRESET hidden
    VISIBILITY_INLINES_HIDDEN YES
    CXX_STANDARD 17
    CXX_STANDARD_REQUIRED YES
    CXX_EXTENSIONS NO
    POSITION_INDEPENDENT_CODE ON
    VERSION "${PROJECT_VERSION}"
    SOVERSION "${PROJECT_VERSION_MAJOR}"
    EXPORT_NAME simplecble
    OUTPUT_NAME simplecble
    RELEASE_POSTFIX ""
    RELWITHDEBINFO_POSTFIX "-relwithdebinfo"
    MINSIZEREL_POSTFIX "-minsizerel"
    DEBUG_POSTFIX "-debug")

# TODO: We need to migrate everything internally to use the new export header.
generate_export_header(
    simplecble
    BASE_NAME simplecble
    EXPORT_FILE_NAME export/simplecble/export.h
)

# Configure include directories for simplecble
target_include_directories(simplecble PRIVATE ${SIMPLEBLE_PRIVATE_INCLUDES} ${CMAKE_CURRENT_SOURCE_DIR}/include)
target_include_directories(simplecble INTERFACE
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>)
target_include_directories(simplecble SYSTEM PUBLIC
    $<BUILD_INTERFACE:${PROJECT_BINARY_DIR}/export>)

# Configure linked libraries for simplecble
target_link_libraries(simplecble PRIVATE simpleble::simpleble)
target_compile_definitions(simplecble PRIVATE SIMPLEBLE_VERSION="${PROJECT_VERSION}")

# Apply OS-specific settings for simplecble
if(CMAKE_SYSTEM_NAME STREQUAL "Linux")
    set_property(TARGET simplecble PROPERTY INSTALL_RPATH $ORIGIN)
elseif(CMAKE_SYSTEM_NAME STREQUAL "Darwin")
    set_property(TARGET simplecble PROPERTY INSTALL_RPATH @loader_path)
endif()

apply_build_options(simplecble
    "${PRIVATE_COMPILE_DEFINITIONS}"
    "${PRIVATE_COMPILE_OPTIONS}"
    "${PRIVATE_LINK_OPTIONS}"
    "${PUBLIC_LINK_OPTIONS}")

set(SIMPLECBLE_CMAKE_INSTALL_DIR ${CMAKE_INSTALL_LIBDIR}/cmake/simplecble)

configure_package_config_file(
    ${CMAKE_CURRENT_SOURCE_DIR}/cmake/simplecble-config.cmake.in
    ${CMAKE_CURRENT_BINARY_DIR}/simplecble-config.cmake
    INSTALL_DESTINATION ${SIMPLECBLE_CMAKE_INSTALL_DIR})

write_basic_package_version_file(
    ${CMAKE_CURRENT_BINARY_DIR}/simplecble-config-version.cmake
    VERSION ${PROJECT_VERSION}
    COMPATIBILITY SameMajorVersion)

install(
    TARGETS simplecble
    EXPORT simplecble-targets
    ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})

install(
    EXPORT simplecble-targets
    FILE simplecble-targets.cmake
    NAMESPACE simplecble::
    DESTINATION ${SIMPLECBLE_CMAKE_INSTALL_DIR})

install(
    FILES
        ${CMAKE_CURRENT_BINARY_DIR}/simplecble-config.cmake
        ${CMAKE_CURRENT_BINARY_DIR}/simplecble-config-version.cmake
    DESTINATION ${SIMPLECBLE_CMAKE_INSTALL_DIR})

install(
    DIRECTORY ${CMAKE_CURRENT_LIST_DIR}/include/simplecble/
    DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/simplecble)

install(
    FILES ${PROJECT_BINARY_DIR}/export/simplecble/export.h
    DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/simplecble)

if(SIMPLECBLE_TEST)
    message(STATUS "Building tests for simplecble")
    find_package(GTest REQUIRED)

    add_executable(simplecble_test
        ${CMAKE_CURRENT_SOURCE_DIR}/test/src/test_buffer_overflow.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/test/src/test_config.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/test/src/test_logging.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/test/src/test_utils.cpp)
    set_target_properties(simplecble_test PROPERTIES
        CXX_VISIBILITY_PRESET hidden
        VISIBILITY_INLINES_HIDDEN YES
        CXX_STANDARD 17
        POSITION_INDEPENDENT_CODE ON
        WINDOWS_EXPORT_ALL_SYMBOLS ON)

    target_include_directories(simplecble_test PRIVATE 
        ${CMAKE_CURRENT_SOURCE_DIR}/include
        ${CMAKE_CURRENT_SOURCE_DIR}/../simpleble/include
        ${CMAKE_CURRENT_SOURCE_DIR}/../simpleble/src
        ${CMAKE_CURRENT_SOURCE_DIR}/../simpleble/src/backends/dongl
        ${CMAKE_CURRENT_SOURCE_DIR}/../dependencies/internal/include)
    
    target_link_libraries(simplecble_test PRIVATE simplecble simpleble::simpleble GTest::gtest GTest::gtest_main)
endif()
