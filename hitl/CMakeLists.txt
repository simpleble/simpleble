cmake_minimum_required(VERSION 3.21)

# The SDK package supplies the toolchain, so import it before project().
include(cmake/simpleembed.cmake)
project(simpleble_hitl LANGUAGES C ASM)

if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE MinSizeRel CACHE STRING "Build type" FORCE)
endif()

add_subdirectory("${HITL_SDK_ROOT}" simpleembed)

add_custom_target(import_sdk
    COMMAND ${sdk_import_command}
    COMMENT "Packaging and importing SimpleEmbed; rebuild to use the updated manifest"
    VERBATIM
)

set(HITL_FIRMWARE_SOURCES
    firmware/main.c
    firmware/peripheral.c
    firmware/peripheral.h
    firmware/gatt.c
    firmware/gatt.h
    firmware/control.c
    firmware/control.h
    firmware/sdk_config.h
)

add_executable(simpleble_hitl ${HITL_FIRMWARE_SOURCES})
set_target_properties(simpleble_hitl PROPERTIES
    SUFFIX ".elf"
    C_STANDARD 11
)

target_include_directories(simpleble_hitl PRIVATE
    firmware
    "${CMAKE_CURRENT_BINARY_DIR}"
)
target_compile_definitions(simpleble_hitl PRIVATE DEBUG)
target_link_libraries(simpleble_hitl PRIVATE
    simpleembed::nrf52840
    simpleembed::sdh
    simpleembed::hal_timer
    simpleembed::delay
    segger::rtt
)
target_link_options(simpleble_hitl PRIVATE
    "-L${HITL_SDK_ROOT}/src/simpleembed/nrf52/device/linker"
    "-T${CMAKE_CURRENT_SOURCE_DIR}/firmware/linker.ld"
    "-Wl,-Map=${CMAKE_CURRENT_BINARY_DIR}/simpleble_hitl.map"
)

# Identify the firmware sources, build configuration, and exact SDK package.
set(build_inputs "${sdk_sha256}")
foreach(input IN ITEMS
    ${HITL_FIRMWARE_SOURCES}
    firmware/linker.ld
    CMakeLists.txt
    cmake/simpleembed.cmake
)
    set(input_path "${CMAKE_CURRENT_SOURCE_DIR}/${input}")
    set_property(DIRECTORY APPEND PROPERTY CMAKE_CONFIGURE_DEPENDS "${input_path}")
    file(SHA256 "${input_path}" input_sha256)
    string(APPEND build_inputs "${input_sha256}")
endforeach()

string(SHA256 build_id "${build_inputs}")
file(WRITE "${CMAKE_CURRENT_BINARY_DIR}/build_id.h" "#define BUILD_ID \"${build_id}\"\n")
configure_file("${HITL_SDK_MANIFEST}" sdk-manifest.json COPYONLY)

add_custom_command(TARGET simpleble_hitl POST_BUILD
    COMMAND "${CMAKE_OBJCOPY}" -O ihex "$<TARGET_FILE:simpleble_hitl>" "${CMAKE_CURRENT_BINARY_DIR}/simpleble_hitl.hex"
    COMMAND "${CMAKE_SIZE}" "$<TARGET_FILE:simpleble_hitl>"
)
