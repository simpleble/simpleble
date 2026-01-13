# CMake Install Options Reference

## Install Target Destinations

| Destination | Purpose | File Types |
|-------------|---------|------------|
| `ARCHIVE` | Static libraries and import libraries | `.a`, `.lib` (Windows import libs) |
| `LIBRARY` | Shared libraries | `.so`, `.dylib` |
| `RUNTIME` | Executables and DLLs | `.exe`, `.dll` |

## Recommended Variables (via `GNUInstallDirs`)

```cmake
include(GNUInstallDirs)
```

| Variable | Default Value | Purpose |
|----------|---------------|---------|
| `CMAKE_INSTALL_LIBDIR` | `lib` | Library installation directory |
| `CMAKE_INSTALL_INCLUDEDIR` | `include` | Header installation directory |
| `CMAKE_INSTALL_BINDIR` | `bin` | Binary/executable installation directory |

## Current iOS CMakeLists.txt

```cmake
install(TARGETS simpleble ARCHIVE DESTINATION lib)

install(DIRECTORY ${PROJECT_ROOT_DIR}/simpleble/include/simpleble/ DESTINATION include/simpleble)
install(DIRECTORY ${CMAKE_BINARY_DIR}/simpleble/export/simpleble/ DESTINATION include/simpleble)
install(FILES ${PROJECT_ROOT_DIR}/dependencies/external/kvn/kvn_bytearray.h DESTINATION include/simpleble/kvn)
```

## Recommended Update (matching main simpleble pattern)

```cmake
include(GNUInstallDirs)

install(TARGETS simpleble ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR})

install(DIRECTORY ${PROJECT_ROOT_DIR}/simpleble/include/simpleble/ DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/simpleble)
install(DIRECTORY ${CMAKE_BINARY_DIR}/simpleble/export/simpleble/ DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/simpleble)
install(FILES ${PROJECT_ROOT_DIR}/dependencies/external/kvn/kvn_bytearray.h DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/simpleble/kvn)
```

## Notes

- For iOS static builds (`BUILD_SHARED_LIBS OFF`), only `ARCHIVE` is needed
- `LIBRARY` is unnecessary since we're not building shared libraries
- Using CMake variables instead of hardcoded paths improves portability
- Header installation is required separately from library installation for XCFramework builds
