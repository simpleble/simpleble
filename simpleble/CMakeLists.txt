cmake_minimum_required(VERSION 3.21)

include(${CMAKE_CURRENT_SOURCE_DIR}/../cmake/prelude.cmake)

project(
    simpleble
    VERSION ${SIMPLEBLE_VERSION}
    DESCRIPTION "The ultimate fully-fledged cross-platform library for Bluetooth Low Energy (BLE)."
    HOMEPAGE_URL "https://github.com/simpleble/simpleble"
    LANGUAGES CXX
)

include(${CMAKE_CURRENT_SOURCE_DIR}/../cmake/epilogue.cmake)

include(GenerateExportHeader)
include(GNUInstallDirs)

option(SIMPLEBLE_PLAIN "Use plain version of SimpleBLE" OFF)
option(SIMPLEBLE_EXCLUDE_C "Exclude C bindings from SimpleBLE" OFF)

if(NOT TARGET fmt::fmt-header-only)
    option(LIBFMT_VENDORIZE "Enable vendorized libfmt" ON)
    find_package(fmt REQUIRED)

    if(TARGET fmt)
        set_target_properties(fmt PROPERTIES EXCLUDE_FROM_ALL TRUE)
    endif()
endif()

if(SIMPLEBLE_TEST)
    message(STATUS "Building tests requires plain version of SimpleBLE")
    set(SIMPLEBLE_PLAIN ON)
endif()

set(SIMPLEBLE_PRIVATE_INCLUDES
    ${CMAKE_CURRENT_SOURCE_DIR}/src
    ${CMAKE_CURRENT_SOURCE_DIR}/include
    ${CMAKE_CURRENT_SOURCE_DIR}/src/builders
    ${CMAKE_CURRENT_SOURCE_DIR}/src/external
    ${CMAKE_CURRENT_SOURCE_DIR}/src/backends/common
    ${CMAKE_CURRENT_SOURCE_DIR}/src/frontends/safe
    ${CMAKE_CURRENT_SOURCE_DIR}/../external/include)

set(SIMPLEBLE_SRC
    ${CMAKE_CURRENT_SOURCE_DIR}/src/frontends/base/Adapter.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/frontends/base/Peripheral.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/frontends/base/Service.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/frontends/base/Characteristic.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/frontends/base/Descriptor.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/frontends/base/Backend.cpp

    ${CMAKE_CURRENT_SOURCE_DIR}/src/backends/common/ServiceBase.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/backends/common/CharacteristicBase.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/backends/common/DescriptorBase.cpp

    ${CMAKE_CURRENT_SOURCE_DIR}/src/Exceptions.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/Utils.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/Logging.cpp

    ${CMAKE_CURRENT_SOURCE_DIR}/src/frontends/safe/AdapterSafe.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/frontends/safe/PeripheralSafe.cpp)

# Define simpleble target
add_library(simpleble ${SIMPLEBLE_SRC})
add_library(simpleble::simpleble ALIAS simpleble)

set_target_properties(simpleble PROPERTIES
    CXX_VISIBILITY_PRESET hidden
    VISIBILITY_INLINES_HIDDEN YES
    CXX_STANDARD 17
    CXX_STANDARD_REQUIRED YES
    CXX_EXTENSIONS NO
    POSITION_INDEPENDENT_CODE ON
    VERSION "${PROJECT_VERSION}"
    SOVERSION "${PROJECT_VERSION_MAJOR}"
    EXPORT_NAME simpleble
    OUTPUT_NAME simpleble
    RELEASE_POSTFIX ""
    RELWITHDEBINFO_POSTFIX "-relwithdebinfo"
    MINSIZEREL_POSTFIX "-minsizerel"
    DEBUG_POSTFIX "-debug")

generate_export_header(
    simpleble
    BASE_NAME simpleble
    EXPORT_FILE_NAME export/simpleble/export.h
)

# Configure include directories for simpleble
target_include_directories(simpleble PRIVATE ${SIMPLEBLE_PRIVATE_INCLUDES})
target_include_directories(simpleble INTERFACE
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/../external/include>
    $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>)
target_include_directories(simpleble SYSTEM PUBLIC
    $<BUILD_INTERFACE:${PROJECT_BINARY_DIR}/export>)

# Configure linked libraries for simpleble
target_link_libraries(simpleble PRIVATE $<BUILD_INTERFACE:fmt::fmt-header-only>)

append_sanitize_options("${SIMPLEBLE_SANITIZE}")

if(NOT SIMPLEBLE_LOG_LEVEL)
    set(SIMPLEBLE_LOG_LEVEL "INFO")
endif()

list(APPEND PRIVATE_COMPILE_DEFINITIONS SIMPLEBLE_LOG_LEVEL=SIMPLEBLE_LOG_LEVEL_${SIMPLEBLE_LOG_LEVEL})
list(APPEND PRIVATE_COMPILE_DEFINITIONS SIMPLEBLE_VERSION="${PROJECT_VERSION}")

if(NOT SIMPLEBLE_TEST AND NOT SIMPLEBLE_PLAIN)
    if(CMAKE_SYSTEM_NAME STREQUAL "Linux")
        set(SIMPLEBLE_BACKEND_LINUX ON)
    else()
        set(SIMPLEBLE_BACKEND_LINUX OFF)
    endif()

    if(CMAKE_SYSTEM_NAME STREQUAL "Windows")
        set(SIMPLEBLE_BACKEND_WINDOWS ON)
    else()
        set(SIMPLEBLE_BACKEND_WINDOWS OFF)
    endif()

    if(CMAKE_SYSTEM_NAME STREQUAL "Darwin" OR CMAKE_SYSTEM_NAME STREQUAL "iOS")
        set(SIMPLEBLE_BACKEND_MACOS ON)
    else()
        set(SIMPLEBLE_BACKEND_MACOS OFF)
    endif()

    if(CMAKE_SYSTEM_NAME STREQUAL "Android")
        set(SIMPLEBLE_BACKEND_ANDROID ON)
    else()
        set(SIMPLEBLE_BACKEND_ANDROID OFF)
    endif()
else()
    set(SIMPLEBLE_BACKEND_LINUX OFF)
    set(SIMPLEBLE_BACKEND_WINDOWS OFF)
    set(SIMPLEBLE_BACKEND_MACOS OFF)
    set(SIMPLEBLE_BACKEND_ANDROID OFF)
endif()

add_compile_definitions(SIMPLEBLE_BACKEND_PLAIN=$<BOOL:${SIMPLEBLE_PLAIN}>
                        SIMPLEBLE_BACKEND_LINUX=$<BOOL:${SIMPLEBLE_BACKEND_LINUX}>
                        SIMPLEBLE_BACKEND_WINDOWS=$<BOOL:${SIMPLEBLE_BACKEND_WINDOWS}>
                        SIMPLEBLE_BACKEND_MACOS=$<BOOL:${SIMPLEBLE_BACKEND_MACOS}>
                        SIMPLEBLE_BACKEND_ANDROID=$<BOOL:${SIMPLEBLE_BACKEND_ANDROID}>)

# Detect the operating system and load the necessary dependencies
if(SIMPLEBLE_PLAIN)
    message(STATUS "Plain Flavor Requested")

    target_sources(simpleble PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/src/backends/plain/AdapterPlain.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/src/backends/plain/PeripheralPlain.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/src/backends/plain/BackendPlain.cpp)
elseif(SIMPLEBLE_BACKEND_LINUX)
    message(STATUS "Linux Host Detected")

    find_package(DBus1 REQUIRED)

    if(NOT SIMPLEDBUS_LOG_LEVEL)
        set(SIMPLEDBUS_LOG_LEVEL "FATAL")
    endif()

    if(NOT SIMPLEBLUEZ_LOG_LEVEL)
        set(SIMPLEBLUEZ_LOG_LEVEL "FATAL")
    endif()

    list(APPEND PRIVATE_COMPILE_DEFINITIONS SIMPLEDBUS_LOG_LEVEL=${SIMPLEDBUS_LOG_LEVEL})
    list(APPEND PRIVATE_COMPILE_DEFINITIONS SIMPLEBLUEZ_LOG_LEVEL=${SIMPLEBLUEZ_LOG_LEVEL})

    if(SIMPLEBLE_USE_SESSION_DBUS)
        list(APPEND PRIVATE_COMPILE_DEFINITIONS SIMPLEBLUEZ_USE_SESSION_DBUS)
    endif()

    target_sources(simpleble PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/src/backends/linux/AdapterLinux.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/src/backends/linux/PeripheralLinux.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/src/backends/linux/BackendBluez.cpp

        ${CMAKE_CURRENT_SOURCE_DIR}/../simplebluez/src/ProxyOrg.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/../simplebluez/src/Logging.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/../simplebluez/src/Agent.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/../simplebluez/src/Device.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/../simplebluez/src/Characteristic.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/../simplebluez/src/Exceptions.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/../simplebluez/src/ProxyOrgBluez.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/../simplebluez/src/Service.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/../simplebluez/src/Adapter.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/../simplebluez/src/Bluez.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/../simplebluez/src/Descriptor.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/../simplebluez/src/interfaces/Adapter1.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/../simplebluez/src/interfaces/Agent1.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/../simplebluez/src/interfaces/GattDescriptor1.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/../simplebluez/src/interfaces/GattCharacteristic1.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/../simplebluez/src/interfaces/GattService1.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/../simplebluez/src/interfaces/Device1.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/../simplebluez/src/interfaces/Battery1.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/../simplebluez/src/interfaces/AgentManager1.cpp

        ${CMAKE_CURRENT_SOURCE_DIR}/../simpledbus/src/advanced/Interface.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/../simpledbus/src/advanced/Proxy.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/../simpledbus/src/base/Connection.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/../simpledbus/src/base/Exceptions.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/../simpledbus/src/base/Holder.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/../simpledbus/src/base/Logging.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/../simpledbus/src/base/Message.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/../simpledbus/src/base/Path.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/../simpledbus/src/interfaces/ObjectManager.cpp
    )

    target_link_libraries(simpleble PUBLIC pthread)
    target_link_libraries(simpleble PRIVATE ${DBus1_LIBRARIES})

    target_include_directories(simpleble PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/../simplebluez/include
        ${CMAKE_CURRENT_SOURCE_DIR}/../simpledbus/include)

    set_property(TARGET simpleble PROPERTY INSTALL_RPATH $ORIGIN)

elseif(SIMPLEBLE_BACKEND_WINDOWS)
    message(STATUS "Windows Host Detected")

    set(WINVERSION_CODE 0x0A00) # Selected Windows 10 based on https://docs.microsoft.com/en-us/cpp/porting/modifying-winver-and-win32-winnt

    if (CMAKE_CXX_COMPILER_ID STREQUAL "MSVC")
        # Add all the special definitions for Visual Studio C++

        # /D_WIN32_WINNT -> Specifies the minimum version of Windows that the application is compatible with.
        list(APPEND PRIVATE_COMPILE_DEFINITIONS "/D_WIN32_WINNT=${WINVERSION_CODE}")
        # /D_USE_MATH_DEFINES -> Specifies that the math.h header file should be included.
        list(APPEND PRIVATE_COMPILE_DEFINITIONS "/D_USE_MATH_DEFINES")

        # /utf-8 -> Set source and executable character sets to utf-8. https://learn.microsoft.com/en-us/cpp/build/reference/utf-8-set-source-and-executable-character-sets-to-utf-8
        list(APPEND PRIVATE_COMPILE_OPTIONS "/utf-8")
        # /Gd -> Use __cdecl as the default calling convention. https://docs.microsoft.com/en-us/cpp/cpp/cdecl
        list(APPEND PRIVATE_COMPILE_OPTIONS "/Gd")
        # /EHsc -> Use the standard C++ exception handling model. https://learn.microsoft.com/en-us/cpp/build/reference/eh-exception-handling-model
        list(APPEND PRIVATE_COMPILE_OPTIONS "/EHsc")
        # /WX -> Treats all warnings as errors.
        list(APPEND PRIVATE_COMPILE_OPTIONS "/WX")
        # /W1 -> Use the lowest level of warnings, as there are some unsafe functions that MSVC doesn't like.
        # TODO: This should be removed once the warnings are fixed.
        list(APPEND PRIVATE_COMPILE_OPTIONS "/W1")
    endif()

    target_sources(simpleble PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/src/backends/windows/AdapterWindows.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/src/backends/windows/PeripheralWindows.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/src/backends/windows/BackendWinRT.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/src/backends/windows/MtaManager.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/src/backends/windows/Utils.cpp)

elseif(SIMPLEBLE_BACKEND_MACOS)
    message(STATUS "Darwin / iOS Host Detected")

    list(APPEND PRIVATE_COMPILE_OPTIONS -fobjc-arc)

    target_link_libraries(simpleble PUBLIC "-framework Foundation" "-framework CoreBluetooth" ObjC)
    target_sources(simpleble PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/src/backends/macos/Utils.mm
        ${CMAKE_CURRENT_SOURCE_DIR}/src/backends/macos/AdapterMac.mm
        ${CMAKE_CURRENT_SOURCE_DIR}/src/backends/macos/AdapterBaseMacOS.mm
        ${CMAKE_CURRENT_SOURCE_DIR}/src/backends/macos/PeripheralMac.mm
        ${CMAKE_CURRENT_SOURCE_DIR}/src/backends/macos/PeripheralBaseMacOS.mm
        ${CMAKE_CURRENT_SOURCE_DIR}/src/backends/macos/BackendCoreBluetooth.mm)

    set_property(TARGET simpleble PROPERTY INSTALL_RPATH @loader_path)

elseif(SIMPLEBLE_BACKEND_ANDROID)
    message(STATUS "Configuring for Android")

    include_directories(${ANDROID_NDK}/sysroot/usr/include)
    include_directories(${CMAKE_CURRENT_SOURCE_DIR}/src/backends/android)

    add_compile_options(-DANDROID -D__ANDROID_API__=${ANDROID_NATIVE_API_LEVEL})

    target_sources(simpleble PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/src/backends/android/AdapterAndroid.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/src/backends/android/PeripheralAndroid.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/src/backends/android/BackendAndroid.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/src/backends/android/android/ClassHandler.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/src/backends/android/android/BluetoothAdapter.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/src/backends/android/android/BluetoothDevice.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/src/backends/android/android/BluetoothGatt.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/src/backends/android/android/BluetoothGattService.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/src/backends/android/android/BluetoothGattCharacteristic.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/src/backends/android/android/BluetoothGattDescriptor.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/src/backends/android/android/BluetoothScanner.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/src/backends/android/android/UUID.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/src/backends/android/android/ParcelUUID.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/src/backends/android/android/ScanRecord.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/src/backends/android/android/ScanResult.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/src/backends/android/android/SparseArray.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/src/backends/android/bridge/ClassHandler.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/src/backends/android/bridge/BluetoothGattCallback.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/src/backends/android/bridge/ScanCallback.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/src/backends/android/jni/Iterator.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/src/backends/android/jni/List.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/src/backends/android/jni/Set.cpp)
    target_link_libraries(simpleble PUBLIC android nativehelper log)
endif()

apply_build_options(simpleble
    "${PRIVATE_COMPILE_DEFINITIONS}"
    "${PRIVATE_COMPILE_OPTIONS}"
    "${PRIVATE_LINK_OPTIONS}"
    "${PUBLIC_LINK_OPTIONS}")

configure_file(${CMAKE_CURRENT_SOURCE_DIR}/cmake/simpleble.pc.in
            ${CMAKE_CURRENT_BINARY_DIR}/simpleble.pc @ONLY)

install(
    TARGETS simpleble
    EXPORT simpleble-config
    ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})

install(
    EXPORT simpleble-config
    NAMESPACE simpleble::
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/simpleble)

install(
    DIRECTORY ${CMAKE_CURRENT_LIST_DIR}/include/simpleble/
    DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/simpleble)

install(
    DIRECTORY ${CMAKE_CURRENT_LIST_DIR}/../external/include/
    DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/simpleble)

install(
    DIRECTORY ${PROJECT_BINARY_DIR}/export/simpleble/
    DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/simpleble)

install(
    FILES ${CMAKE_CURRENT_BINARY_DIR}/simpleble.pc
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/pkgconfig)

if(SIMPLEBLE_TEST)
    message(STATUS "Building Tests")
    find_package(GTest REQUIRED)

    add_executable(simpleble_test
        ${CMAKE_CURRENT_SOURCE_DIR}/test/src/main.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/test/src/test_utils.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/test/src/test_bytearray.cpp)
    set_target_properties(simpleble_test PROPERTIES
        CXX_VISIBILITY_PRESET hidden
        VISIBILITY_INLINES_HIDDEN YES
        CXX_STANDARD 17
        POSITION_INDEPENDENT_CODE ON
        WINDOWS_EXPORT_ALL_SYMBOLS ON)

    target_link_libraries(simpleble_test PRIVATE simpleble::simpleble GTest::gtest)
endif()

# *** SIMPLEBLE C BINDINGS ***

if(NOT SIMPLEBLE_EXCLUDE_C)
    set(SIMPLEBLE_C_SRC
        ${CMAKE_CURRENT_SOURCE_DIR}/src_c/simpleble.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/src_c/adapter.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/src_c/peripheral.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/src_c/logging.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/src_c/utils.cpp)

    add_library(simpleble-c ${SIMPLEBLE_C_SRC})
    add_library(simpleble::simpleble-c ALIAS simpleble-c)

    set_target_properties(simpleble-c PROPERTIES
        C_VISIBILITY_PRESET hidden
        VISIBILITY_INLINES_HIDDEN YES
        CXX_STANDARD 17
        CXX_STANDARD_REQUIRED YES
        CXX_EXTENSIONS NO
        POSITION_INDEPENDENT_CODE ON
        DEFINE_SYMBOL simpleble_EXPORTS
        VERSION "${PROJECT_VERSION}"
        SOVERSION "${PROJECT_VERSION_MAJOR}"
        EXPORT_NAME simpleble-c
        OUTPUT_NAME simpleble-c
        RELEASE_POSTFIX ""
        RELWITHDEBINFO_POSTFIX "-relwithdebinfo"
        MINSIZEREL_POSTFIX "-minsizerel"
        DEBUG_POSTFIX "-debug")

    # Configure include directories for simpleble-c
    target_include_directories(simpleble-c PRIVATE ${SIMPLEBLE_PRIVATE_INCLUDES})
    target_include_directories(simpleble-c INTERFACE
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>)
    target_include_directories(simpleble-c SYSTEM PUBLIC
        $<BUILD_INTERFACE:${PROJECT_BINARY_DIR}/export>)

    # Configure linked libraries for simpleble-c
    target_link_libraries(simpleble-c PRIVATE $<BUILD_INTERFACE:fmt::fmt-header-only>)
    target_link_libraries(simpleble-c PRIVATE simpleble::simpleble)

    # Apply OS-specific settings for simpleble-c
    if(SIMPLEBLE_BACKEND_LINUX)
        set_property(TARGET simpleble-c PROPERTY INSTALL_RPATH $ORIGIN)
    elseif(SIMPLEBLE_BACKEND_MACOS)
        set_property(TARGET simpleble-c PROPERTY INSTALL_RPATH @loader_path)
    endif()

    apply_build_options(simpleble-c
        "${PRIVATE_COMPILE_DEFINITIONS}"
        "${PRIVATE_COMPILE_OPTIONS}"
        "${PRIVATE_LINK_OPTIONS}"
        "${PUBLIC_LINK_OPTIONS}")

    install(
        TARGETS simpleble-c
        EXPORT simpleble-config
        ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
        LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
        RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})

    install(
        DIRECTORY ${CMAKE_CURRENT_LIST_DIR}/include/simpleble_c/
        DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/simpleble_c)
endif()
