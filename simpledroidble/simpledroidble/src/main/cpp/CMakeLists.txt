cmake_minimum_required(VERSION 3.21)

set(PROJECT_ROOT_DIR ${CMAKE_CURRENT_SOURCE_DIR}/../../../../..)

include(${PROJECT_ROOT_DIR}/cmake/prelude.cmake)

project(
    "simpleble-jni"
    VERSION ${SIMPLEBLE_VERSION_BASE}
    LANGUAGES CXX
)

include(${PROJECT_ROOT_DIR}/cmake/epilogue.cmake)

set(BUILD_SHARED_LIBS OFF)
add_subdirectory(${PROJECT_ROOT_DIR}/simpleble ${CMAKE_BINARY_DIR}/simpleble)
set(BUILD_SHARED_LIBS ON)

set(SIMPLEJNI_SOURCES
    ${PROJECT_ROOT_DIR}/dependencies/internal/src/simplejni/Common.cpp
    ${PROJECT_ROOT_DIR}/dependencies/internal/src/simplejni/java/lang/Integer.cpp
    ${PROJECT_ROOT_DIR}/dependencies/internal/src/simplejni/java/util/ArrayList.cpp
    ${PROJECT_ROOT_DIR}/dependencies/internal/src/simplejni/java/util/HashMap.cpp)

add_library(
    ${CMAKE_PROJECT_NAME} SHARED
    NativeCache.cpp
    org/simpleble/android/AdapterCallback.cpp
    org/simpleble/android/Characteristic.cpp
    org/simpleble/android/DataCallback.cpp
    org/simpleble/android/Descriptor.cpp
    org/simpleble/android/LocalCharacteristicCallback.cpp
    org/simpleble/android/LocalPeripheralCallback.cpp
    org/simpleble/android/PeripheralCallback.cpp
    org/simpleble/android/Service.cpp
    org/simpleble/android/SimpleDroidBleException.cpp
    simpleble_android.cpp
    ${SIMPLEJNI_SOURCES}
)

set_target_properties(
    ${CMAKE_PROJECT_NAME} PROPERTIES
    CXX_STANDARD 17
    CXX_STANDARD_REQUIRED YES
    CXX_EXTENSIONS NO
)

target_include_directories(
    ${CMAKE_PROJECT_NAME} PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/../../../../../dependencies/internal/include
    ${CMAKE_CURRENT_SOURCE_DIR}/../../../../../dependencies/external
)

target_link_libraries(${CMAKE_PROJECT_NAME}
    android log simpleble::simpleble
)

target_compile_definitions(${CMAKE_PROJECT_NAME} PRIVATE FMT_HEADER_ONLY FMT_UNICODE=0)
target_compile_definitions(
    ${CMAKE_PROJECT_NAME} PRIVATE SIMPLEDROIDBLE_PLAIN=$<BOOL:${SIMPLEBLE_PLAIN}>
)
