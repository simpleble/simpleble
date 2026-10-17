cmake_minimum_required(VERSION 3.21)

# NOTE: In the repository, SimpleBLE's sources sit beside the package. A
# published package carries them in native/. Apps reach the package through a
# node_modules link, so the paths start from its real location.
get_filename_component(PACKAGE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/.. REALPATH)
if(EXISTS ${PACKAGE_DIR}/../simpleble/CMakeLists.txt)
    get_filename_component(SIMPLEBLE_ROOT ${PACKAGE_DIR}/.. REALPATH)
else()
    set(SIMPLEBLE_ROOT ${PACKAGE_DIR}/native)
endif()

include(${SIMPLEBLE_ROOT}/cmake/prelude.cmake)
project(SimpleRnBLE VERSION ${SIMPLEBLE_VERSION_BASE} LANGUAGES CXX)
include(${SIMPLEBLE_ROOT}/cmake/epilogue.cmake)

set(BUILD_SHARED_LIBS OFF)
add_subdirectory(${SIMPLEBLE_ROOT}/simpleble ${CMAKE_BINARY_DIR}/simpleble)

add_library(SimpleRnBLE SHARED
    src/main/cpp/OnLoad.cpp
    ../cpp/Common.cpp
    ../cpp/Executor.cpp
    ../cpp/HybridAdapterHandle.cpp
    ../cpp/HybridBackendHandle.cpp
    ../cpp/HybridPeripheralHandle.cpp
    ../cpp/HybridSimpleRnBLE.cpp
    ../cpp/LocalHandles.cpp
    ../cpp/Logging.cpp
)
set_target_properties(SimpleRnBLE PROPERTIES CXX_STANDARD 20 CXX_STANDARD_REQUIRED YES)
target_include_directories(SimpleRnBLE PRIVATE ../cpp ${SIMPLEBLE_ROOT}/dependencies/internal/include)

include(${CMAKE_CURRENT_SOURCE_DIR}/../nitrogen/generated/android/SimpleRnBLE+autolinking.cmake)

find_library(LOG_LIB log)
target_link_libraries(SimpleRnBLE ${LOG_LIB} android simpleble::simpleble)
