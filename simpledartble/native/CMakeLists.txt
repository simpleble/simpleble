cmake_minimum_required(VERSION 3.24)

set(PROJECT_ROOT_DIR ${CMAKE_CURRENT_SOURCE_DIR}/../..)

include(${PROJECT_ROOT_DIR}/cmake/prelude.cmake)

project(
    "simpledartble"
    VERSION ${SIMPLEBLE_VERSION_BASE}
    LANGUAGES CXX
)

include(${PROJECT_ROOT_DIR}/cmake/epilogue.cmake)

if(NOT DART_SDK_INCLUDE_DIR)
    message(FATAL_ERROR "Set DART_SDK_INCLUDE_DIR to the include directory of the Dart SDK")
endif()

# SimpleDartBLE calls SimpleCBLE through dart:ffi. Its one shared library holds SimpleCBLE, SimpleBLE and the
# functions that hand SimpleBLE's callbacks to Dart.
set(BUILD_SHARED_LIBS OFF)
add_subdirectory(${PROJECT_ROOT_DIR}/simplecble ${CMAKE_BINARY_DIR}/simplecble)

add_library(simpledartble SHARED
    ${CMAKE_CURRENT_SOURCE_DIR}/src/events.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/android.cpp)

set_target_properties(simpledartble PROPERTIES
    CXX_VISIBILITY_PRESET hidden
    VISIBILITY_INLINES_HIDDEN YES
    CXX_STANDARD 17
    CXX_STANDARD_REQUIRED YES
    CXX_EXTENSIONS NO)

# NOTE: This library exports SimpleCBLE's functions. A static SimpleCBLE's export macro is empty, which MSVC
# doesn't export, so it is predefined there. Other compilers export SimpleCBLE's default-visibility functions.
if(MSVC)
    target_compile_definitions(simplecble PRIVATE "SIMPLECBLE_EXPORT=__declspec(dllexport)")
    target_compile_definitions(simpledartble PRIVATE "SIMPLECBLE_EXPORT=__declspec(dllexport)")
endif()

target_include_directories(simpledartble PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/include)
target_include_directories(simpledartble SYSTEM PRIVATE ${DART_SDK_INCLUDE_DIR})

# Every SimpleCBLE function stays in the library, whether the shim calls it or not.
target_link_libraries(simpledartble PRIVATE "$<LINK_LIBRARY:WHOLE_ARCHIVE,simplecble>" simpleble::simpleble)

if(ANDROID)
    if(NOT SIMPLEBLE_PLAIN)
        target_compile_definitions(simpledartble PRIVATE SIMPLEDARTBLE_BACKEND_ANDROID=1)
    endif()
    # The C++ runtime is linked statically, and only the C functions are exported.
    target_link_options(simpledartble PRIVATE "-Wl,--exclude-libs,libc++_static.a" "-Wl,--exclude-libs,libc++abi.a")
endif()
