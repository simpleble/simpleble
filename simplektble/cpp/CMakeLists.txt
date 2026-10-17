cmake_minimum_required(VERSION 3.21)

set(PROJECT_ROOT_DIR ${CMAKE_CURRENT_SOURCE_DIR}/../..)

include(${PROJECT_ROOT_DIR}/cmake/prelude.cmake)

project(
    "simplektble-jni"
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
    org/simplektble/AdapterCallback.cpp
    org/simplektble/Characteristic.cpp
    org/simplektble/DataCallback.cpp
    org/simplektble/Descriptor.cpp
    org/simplektble/PeripheralCallback.cpp
    org/simplektble/Service.cpp
    org/simplektble/SimpleKtBleException.cpp
    simplektble.cpp
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
    ${PROJECT_ROOT_DIR}/dependencies/internal/include
    ${PROJECT_ROOT_DIR}/dependencies/external
)

if(ANDROID)
    target_link_libraries(${CMAKE_PROJECT_NAME} android log simpleble::simpleble)
else()
    find_package(JNI REQUIRED COMPONENTS JVM)
    target_include_directories(${CMAKE_PROJECT_NAME} PRIVATE ${JNI_INCLUDE_DIRS})
    # NOTE: Only libjvm. Nothing here uses AWT, and runtimes built without java.desktop have no libjawt.
    target_link_libraries(${CMAKE_PROJECT_NAME} simpleble::simpleble ${JAVA_JVM_LIBRARY})
endif()

if(CMAKE_SYSTEM_NAME STREQUAL "Windows")
    # NOTE: SimpleBLE is embedded statically, so both targets must share one MSVC runtime.
    if(MSVC AND NOT DEFINED CMAKE_MSVC_RUNTIME_LIBRARY)
        set_property(TARGET ${CMAKE_PROJECT_NAME} simpleble PROPERTY
            MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
    endif()

    target_link_options(${CMAKE_PROJECT_NAME} PRIVATE "/DELAYLOAD:jvm.dll")
endif()

target_compile_definitions(${CMAKE_PROJECT_NAME} PRIVATE FMT_HEADER_ONLY FMT_UNICODE=0)
target_compile_definitions(
    ${CMAKE_PROJECT_NAME} PRIVATE SIMPLEKTBLE_PLAIN=$<BOOL:${SIMPLEBLE_PLAIN}>
)
