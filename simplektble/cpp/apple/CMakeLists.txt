cmake_minimum_required(VERSION 3.21)

set(PROJECT_ROOT_DIR ${CMAKE_CURRENT_SOURCE_DIR}/../../..)

include(${PROJECT_ROOT_DIR}/cmake/prelude.cmake)

project(
    "simplektble-apple"
    VERSION ${SIMPLEBLE_VERSION_BASE}
    LANGUAGES CXX
)

include(${PROJECT_ROOT_DIR}/cmake/epilogue.cmake)

# SimpleKtBLE calls SimpleCBLE on Apple platforms, plus the identity, snapshot and advanced functions it lacks.
add_subdirectory(${PROJECT_ROOT_DIR}/simplecble ${CMAKE_BINARY_DIR}/simplecble)

add_library(simplektble-apple STATIC Advanced.cpp Identity.cpp Logging.cpp Snapshot.cpp)

set_target_properties(
    simplektble-apple PROPERTIES
    CXX_STANDARD 17
    CXX_STANDARD_REQUIRED YES
    CXX_EXTENSIONS NO
)

target_include_directories(simplektble-apple PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)
target_link_libraries(simplektble-apple PRIVATE simplecble::simplecble simpleble::simpleble)
