cmake_minimum_required(VERSION 3.21)

# NOTE: Must be set before project(). The same minimum as SimplePyBLE's wheels.
set(CMAKE_OSX_DEPLOYMENT_TARGET "13.0" CACHE STRING "Minimum macOS version")

project(simplejsble LANGUAGES C CXX)

# NOTE: Node and Electron don't ship the Visual C++ runtime, so the addon and
# the SimpleBLE libraries linked into it use the static one, like node-gyp.
set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")

set(BUILD_SHARED_LIBS OFF)
set(SIMPLEBLE_INSTALL OFF CACHE BOOL "" FORCE)
add_subdirectory(../simpleble ${CMAKE_BINARY_DIR}/simpleble)

execute_process(COMMAND node -p "require('node-addon-api').include_dir"
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
    OUTPUT_VARIABLE NODE_ADDON_API_DIR OUTPUT_STRIP_TRAILING_WHITESPACE
    COMMAND_ERROR_IS_FATAL ANY)

# NOTE: This must be a SHARED library. cmake-js passes its linker flags through
# CMAKE_SHARED_LINKER_FLAGS, which CMake doesn't apply to MODULE libraries; on
# Windows that drops /DELAYLOAD:NODE.EXE and the addon can't load in Electron.
add_library(simplejsble SHARED
    src/Addon.cpp
    src/AdapterHandle.cpp
    src/BackendHandle.cpp
    src/Dispatcher.cpp
    src/Executor.cpp
    src/Module.cpp
    src/PeripheralHandle.cpp
    ${CMAKE_JS_SRC})

target_compile_features(simplejsble PRIVATE cxx_std_17)
# NOTE: A terminated worker stops running JavaScript before its event loop
# stops. Without NODE_API_SWALLOW_UNTHROWABLE_EXCEPTIONS, node-addon-api aborts
# the whole process when a call fails in that window.
target_compile_definitions(simplejsble PRIVATE
    NAPI_VERSION=8
    NAPI_CPP_EXCEPTIONS
    NODE_ADDON_API_CPP_EXCEPTIONS_ALL
    NODE_ADDON_API_DISABLE_DEPRECATED
    NODE_API_SWALLOW_UNTHROWABLE_EXCEPTIONS)
target_include_directories(simplejsble PRIVATE ${CMAKE_JS_INC} ${NODE_ADDON_API_DIR})
target_link_libraries(simplejsble PRIVATE simpleble::simpleble ${CMAKE_JS_LIB} ${CMAKE_DL_LIBS})
set_target_properties(simplejsble PROPERTIES
    PREFIX ""
    SUFFIX ".node"
    CXX_VISIBILITY_PRESET hidden
    VISIBILITY_INLINES_HIDDEN YES)

if(MSVC)
    target_compile_options(simplejsble PRIVATE /EHsc /utf-8)
    if(CMAKE_JS_NODELIB_DEF AND CMAKE_JS_NODELIB_TARGET)
        execute_process(COMMAND ${CMAKE_AR} /def:${CMAKE_JS_NODELIB_DEF} /out:${CMAKE_JS_NODELIB_TARGET} ${CMAKE_STATIC_LINKER_FLAGS}
            COMMAND_ERROR_IS_FATAL ANY)
    endif()
endif()
