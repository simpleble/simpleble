# Attempt to load DBus1 as CONFIG silently
find_package(DBus1 CONFIG QUIET)

if(TARGET DBus1::DBus1)
    set(DBus1_LIBRARIES DBus1::DBus1)
    set(DBus1_FOUND TRUE)
elseif(TARGET dbus-1)
    add_library(DBus1::DBus1 INTERFACE IMPORTED)
    target_link_libraries(DBus1::DBus1 INTERFACE dbus-1)
    set(DBus1_LIBRARIES dbus-1)
    set(DBus1_FOUND TRUE)
endif()

# If DBus1 was not found, try to find it through pkg-config.
if(NOT TARGET DBus1::DBus1)
    find_package(PkgConfig REQUIRED)

    if(NOT TARGET PkgConfig::DBus1)
        pkg_check_modules(DBus1 REQUIRED IMPORTED_TARGET dbus-1)
    endif()

    if(NOT TARGET dbus-1)
        add_library(dbus-1 INTERFACE IMPORTED)
        target_link_libraries(dbus-1 INTERFACE PkgConfig::DBus1)
    endif()

    add_library(DBus1::DBus1 INTERFACE IMPORTED)
    target_link_libraries(DBus1::DBus1 INTERFACE PkgConfig::DBus1)
    set(DBus1_LIBRARIES dbus-1)
    set(DBus1_FOUND TRUE)
endif()
