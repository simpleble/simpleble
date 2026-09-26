set(HITL_SDK_SOURCE "${CMAKE_CURRENT_LIST_DIR}/../../../simpleembed-nrf"
    CACHE PATH "SimpleEmbed checkout used by import_sdk"
)
set(HITL_SDK_MANIFEST "${CMAKE_CURRENT_LIST_DIR}/../../.sdk/manifest.json"
    CACHE FILEPATH "Imported SDK manifest"
)

find_package(Python3 3.11 REQUIRED COMPONENTS Interpreter)
set(sdk_import_command
    "${Python3_EXECUTABLE}" "${CMAKE_CURRENT_LIST_DIR}/../import_sdk.py"
    --source "${HITL_SDK_SOURCE}"
    --manifest "${HITL_SDK_MANIFEST}"
)

if(NOT EXISTS "${HITL_SDK_MANIFEST}")
    execute_process(
        COMMAND ${sdk_import_command}
        COMMAND_ERROR_IS_FATAL ANY
    )
endif()

file(READ "${HITL_SDK_MANIFEST}" sdk_manifest)
get_filename_component(sdk_cache "${HITL_SDK_MANIFEST}" DIRECTORY)
string(JSON sdk_root GET "${sdk_manifest}" root)
string(JSON sdk_archive GET "${sdk_manifest}" archive)
string(JSON sdk_sha256 GET "${sdk_manifest}" sha256)

file(SHA256 "${sdk_cache}/${sdk_archive}" actual_sha256)
if(NOT actual_sha256 STREQUAL sdk_sha256)
    message(FATAL_ERROR "SDK archive checksum mismatch: ${sdk_cache}/${sdk_archive}")
endif()

get_filename_component(HITL_SDK_ROOT "${sdk_cache}/${sdk_root}" ABSOLUTE)
set(CMAKE_TOOLCHAIN_FILE "${HITL_SDK_ROOT}/cmake/arm-none-eabi.cmake"
    CACHE FILEPATH "SimpleEmbed cross-compilation toolchain"
)
