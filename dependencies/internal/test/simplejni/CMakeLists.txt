cmake_minimum_required(VERSION 3.21)

set(PROJECT_ROOT_DIR ${CMAKE_CURRENT_SOURCE_DIR}/../../../..)

include(${PROJECT_ROOT_DIR}/cmake/prelude.cmake)

project(simplejni_test LANGUAGES CXX)

include(${PROJECT_ROOT_DIR}/cmake/epilogue.cmake)

find_package(Java 17 COMPONENTS Development REQUIRED)
find_package(JNI REQUIRED)

enable_testing()

set(SIMPLEJNI_TEST_CLASSES_DIR ${CMAKE_CURRENT_BINARY_DIR}/test-classes)
set(SIMPLEJNI_TEST_JAVA_SOURCE
    ${CMAKE_CURRENT_SOURCE_DIR}/java/org/simplejni/TestTarget.java)
set(SIMPLEJNI_TEST_JAVA_STAMP ${SIMPLEJNI_TEST_CLASSES_DIR}/.compiled)

add_custom_command(
    OUTPUT ${SIMPLEJNI_TEST_JAVA_STAMP}
    COMMAND ${CMAKE_COMMAND} -E make_directory ${SIMPLEJNI_TEST_CLASSES_DIR}
    COMMAND ${Java_JAVAC_EXECUTABLE}
        --release 17
        -d ${SIMPLEJNI_TEST_CLASSES_DIR}
        ${SIMPLEJNI_TEST_JAVA_SOURCE}
    COMMAND ${CMAKE_COMMAND} -E touch ${SIMPLEJNI_TEST_JAVA_STAMP}
    DEPENDS ${SIMPLEJNI_TEST_JAVA_SOURCE}
    VERBATIM)

add_executable(simplejni_test
    src/test_simplejni.cpp
    ${SIMPLEJNI_SOURCES})

target_compile_features(simplejni_test PRIVATE cxx_std_17)
target_include_directories(simplejni_test PRIVATE
    ${PROJECT_ROOT_DIR}/dependencies/external
    ${PROJECT_ROOT_DIR}/dependencies/internal/include
    ${JNI_INCLUDE_DIRS})
target_link_libraries(simplejni_test PRIVATE ${JNI_LIBRARIES})

add_custom_target(simplejni_test_java DEPENDS ${SIMPLEJNI_TEST_JAVA_STAMP})
add_dependencies(simplejni_test simplejni_test_java)

add_test(NAME simplejni COMMAND simplejni_test ${SIMPLEJNI_TEST_CLASSES_DIR})
