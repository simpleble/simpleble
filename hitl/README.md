# SimpleBLE hardware tests

C++ development scenarios and regression tests against an nRF52840 peripheral.
Firmware controls set attribute values, generate notifications and indications,
and schedule disconnects or resets. Controls use either the BLE connection under
test or SEGGER RTT through a J-Link probe.

| Path | Purpose |
| --- | --- |
| `host/scenarios/` | Editable C++ operation sequences for backend development |
| `host/tests.cpp` | GoogleTest regression suite linked to SimpleBLE |
| `host/fixture.*`, `host/rtt.cpp` | Board discovery and BLE/RTT control clients |
| `firmware/main.c` | Firmware startup and polling loop |
| `firmware/peripheral.c` | Advertising, connection state, scheduled actions, and packet delivery |
| `firmware/gatt.c` | Service definitions, ATT authorization, and BLE event dispatch |
| `firmware/control.c` | Command parsing, replies, and BLE/RTT transport framing |
| `tools/` | Python flashing utility and probe-selection tests |
| [PROTOCOL.md](PROTOCOL.md) | GATT layout, commands, transport framing, and events |

## Build the host tools

Requirements: CMake 3.21+, a C++17 compiler, and the platform dependencies used
by SimpleBLE. Run from the repository root:

```sh
cmake -S hitl/host -B hitl/_build/host -DCMAKE_BUILD_TYPE=Debug
cmake --build hitl/_build/host --parallel
```

The host build compiles SimpleBLE from this checkout and uses the repository's
GoogleTest dependency. The DK must be running the firmware described below.

## Develop a backend

Edit `host/scenarios/basic.cpp` and run:

```sh
cmake --build hitl/_build/host --target hitl_basic
hitl/_build/host/bin/hitl_basic
```

The example connects, sets a readable value, reads it through SimpleBLE, writes
a binary value, queries the firmware's write history, and disconnects.
`Fixture::peer` exposes the SimpleBLE peripheral directly. `Fixture::control()`
sends individual firmware commands. Construction selects the adapter and control
transport; scanning, connection, and reset are explicit calls.

For another scenario, add a C++ source under `host/scenarios/` and an executable
linked to `hitl_support` in `host/CMakeLists.txt`. A reproduced failure can then
be expressed as a GoogleTest case in `host/tests.cpp`.

| Argument | Default | Behavior |
| --- | --- | --- |
| `--control=ble` | BLE | Uses write requests and reads on the same connection as the operations under test |
| `--control=rtt` | | Uses the attached J-Link; commands also work while BLE is disconnected |
| `--board=<id>` | Automatic | Selects a board by its full 16-digit FICR ID |
| `--probe=<serial>` | Automatic | Selects a J-Link when several are connected; requires RTT |
| `--adapter=<index>` | 0 | Selects the Dongl adapter; the fixture currently always tests through a Dongl |

BLE control uses writes of at most 20 bytes and response reads of at most 20
bytes. It requires connection, characteristic discovery, reads, and write
requests. RTT is useful while those backend operations are being implemented:

```sh
hitl/_build/host/bin/hitl_basic --control=rtt
```

RTT requires SEGGER J-Link software. The client loads `libjlinkarm.dylib` from
`/Applications/SEGGER/JLink` on macOS, `libjlinkarm.so` from the loader path on
Linux, or `JLink_x64.dll` on Windows. Set `JLINK_LIBRARY` to override the library
path. Use one RTT reader at a time; readers consume shared records. Resume a
halted target before attaching.

## Run regression tests

```sh
ctest --test-dir hitl/_build/host --output-on-failure
```

The hardware suite runs each case with MTU ceilings 23 and 247. Each case resets
the firmware and establishes a new connection. Tests cover discovery, binary
reads, writes with and without response, descriptors, errors, notifications,
indications, Battery Level, reconnects, and disconnect/reset during streaming.
The `Advertising` cases run once, at MTU 247: they check each advertising
profile as seen by a scan, and that connecting to a nonconnectable advertiser
fails within one bounded attempt.

Select tests or repeat a case with GoogleTest options:

```sh
hitl/_build/host/bin/hitl_tests --gtest_list_tests
hitl/_build/host/bin/hitl_tests --gtest_filter='*Reads*' --control=rtt
hitl/_build/host/bin/hitl_tests --gtest_filter='*Reconnect*' --gtest_repeat=10
hitl/_build/host/bin/hitl_tests --gtest_output=xml:hitl/_build/results.xml
```

CTest limits the hardware run to 1800 seconds and writes `hitl-results.xml` in
the host build directory. Individual failures appear in the test output and XML;
GoogleTest continues with the remaining cases. Direct binary execution permits
debugger breakpoints without a process deadline. XML properties include the
board ID, firmware build ID, SimpleBLE version, transport, and negotiated MTU.
Commands and replies are printed to standard output; RTT mode also prints events.

Host-only control framing and encoding tests run without a DK:

```sh
ctest --test-dir hitl/_build/host -R hitl_control_encoding --output-on-failure
```

## Build the firmware

Requirements: Python 3.11+, CMake 3.21+, and a `simpleembed-nrf` checkout beside
this repository or an imported SDK package.

```sh
cmake -S hitl -B hitl/_build/firmware
cmake --build hitl/_build/firmware --parallel
```

CMake invokes SimpleEmbed's packager when `.sdk/manifest.json` is absent. It
stores the ZIP and extracted SDK in `.sdk/<sha256>/`, verifies the archive
checksum, and loads the packaged CMake targets. The packaged toolchain downloads
Arm GNU 10.3-2021.10 on first use. Existing configurations reuse the selected SDK.

After modifying SimpleEmbed:

```sh
cmake --build hitl/_build/firmware --target import_sdk
cmake --build hitl/_build/firmware --parallel
```

| CMake variable | Default | Purpose |
| --- | --- | --- |
| `HITL_SDK_SOURCE` | `../simpleembed-nrf` relative to the repository root | Checkout to package |
| `HITL_SDK_MANIFEST` | `.sdk/manifest.json` | Selected package; imports are stored beside it |

Set these with `-D<variable>=<absolute-path>`. Imports include uncommitted SDK
changes. The manifest records the version, archive checksum, source commit, and
working-tree status. Existing packages support builds without the source
checkout. Use a fresh build directory when changing toolchain versions.

The firmware build produces ELF, HEX, MAP, `build_id.h`, and a manifest copy.
SDK files and `hitl/_build/` are ignored by Git.

## Flash the firmware

Connect the DK over USB:

```sh
python3 -m venv hitl/_build/venv
hitl/_build/venv/bin/python -m pip install -r hitl/tools/requirements.txt
hitl/_build/venv/bin/python hitl/tools/flash.py
```

The flash tool discovers the J-Link probe, verifies the nRF52840 part, reads its
board ID, erases flash, and programs S140 and the application. With multiple
probes, use `--probe <serial>`; `--board <id>` optionally checks the target ID.
Use `--build <directory>` for another firmware build and `--sdk-cache <directory>`
for a custom SDK manifest location. The flash report contains probe and board
identifiers and the programmed image hashes.
