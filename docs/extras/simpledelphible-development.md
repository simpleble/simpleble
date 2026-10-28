# SimpleDelphiBLE development

This is contributor documentation for SimpleDelphiBLE, the Delphi and Free Pascal bindings. It is kept outside
`content/docs` and is not included in the published documentation site.

## Layout

```text
simpledelphible/
├── source/
│   ├── SimpleDelphiBLE.pas                   Public API: the only unit applications list
│   ├── SimpleDelphiBLE.Support.inc           Conversions, atomics and errors
│   ├── SimpleDelphiBLE.Events.inc            Event delivery and the dispatcher
│   ├── SimpleDelphiBLE.Callbacks.inc         The native callbacks
│   ├── SimpleDelphiBLE.Backend.inc           TBleBackend
│   ├── SimpleDelphiBLE.Adapter.inc           TBleAdapter
│   ├── SimpleDelphiBLE.Peripheral.inc        TBlePeripheral
│   ├── SimpleDelphiBLE.LocalPeripheral.inc   TBleLocalPeripheral, TBleLocalService, TBleLocalCharacteristic
│   ├── SimpleDelphiBLE.Config.inc            TBleConfig, TBleLogging and TSimpleDelphiBLE
│   ├── SimpleDelphiBLE.Native.pas            One cdecl external per SimpleCBLE function
│   └── SimpleDelphiBLE.inc                   Compiler settings for both units
├── tests/SimpleDelphiBLETests.dpr            Binding tests against PLAIN
├── SimpleDelphiBLEPackage.lpk                Lazarus runtime package
├── SimpleDelphiBLEPackage.pas                Its main unit, written by Lazarus
└── README.md
examples/simpledelphible/                     Interactive console programs, one per .dpr
utils/build_simpledelphible.sh                Builds PLAIN and runs the tests with Free Pascal
utils/package_simpledelphible.sh              Assembles the distribution
```

The code compiles with Delphi and with Free Pascal 3.2.2 in `{$MODE DELPHIUNICODE}`. It stays within what Free Pascal
3.2.2 has: no anonymous methods, inline variables or `[weak]`, so events are `procedure ... of object`. It uses
nothing newer than Delphi XE4 (`{$IF}` closed by `{$ENDIF}`, `AtomicIncrement`).

The classes refer to each other, which Pascal allows only within one unit, so `SimpleDelphiBLE.pas` declares them all
and includes the rest of its implementation from the `.inc` files. A routine outside a class must come before its
callers, so the include order matters.

## How a call reaches SimpleBLE

Every class wraps a SimpleCBLE handle. A method call goes through `TNativeCall`: `Enter` masks floating-point
exceptions, the C function runs with `Call.Error` as its `out_error`, and `Leave` restores the mask and raises the
`EBleError` subclass for the error code. Free Pascal unmasks invalid-operation, divide-by-zero and overflow exceptions
on x86, and Delphi applications can unmask them too, while SimpleBLE's C++ code runs with the C defaults.

`simpleble_uuid_t` is passed by value. Records passed to `cdecl` externals without `const` follow each platform's C
ABI in both compilers; the GATT tests cover it, in both directions, since notification callbacks receive UUIDs by
value too.

SimpleCBLE defines the platform-specific advanced functions (`simpleble_advanced_{macos,ios,linux}_*`) in every build
for their platform, PLAIN included, where they report `OPERATION_NOT_SUPPORTED`. A missing `external` stops the
program from loading, so `SimpleDelphiBLE.inc` defines `SIMPLEDELPHIBLE_MACOS` and `SIMPLEDELPHIBLE_LINUX`, and
the units declare and call each platform's functions only under its symbol. Elsewhere, `RetrieveCachedPeripherals`
and `SetAdvertisementLocalName` raise `EBleOperationNotSupported` without calling SimpleCBLE. The iOS and Android
functions wait for mobile support.

## Objects and identity

SimpleDelphiBLE keeps one object per SimpleBLE object:

- Adapters, in a unit-wide dictionary keyed by `simpleble_adapter_identity`.
- Peripherals, per adapter, keyed by `simpleble_peripheral_identity`.
- Local services and characteristics, per parent, keyed by their identity functions.
- Backends, by identifier.

The first handle is kept, so the identity stays valid, and later handles are released. SimpleDelphiBLE owns all of
these and frees them in the unit's finalization. `TBleNativeObject.BeforeDestruction` raises `EInvalidOperation`
unless the owner is freeing the object. The application owns `TBleLocalPeripheral`, which owns its services and
characteristics, including the ones `RemoveAllServices` removed.

## Callbacks and events

A native callback's userdata is the object's token, an integer the unit's `Targets` dictionary maps to the object,
never a pointer to it. A callback that arrives after its object is gone finds nothing.

Callbacks only convert their arguments and queue a `TBleEvent`. The dispatcher has two queues: one drained by
SimpleDelphiBLE's event thread, and one the main thread drains through `TThread.Queue`. The event thread schedules
the main thread's drains, so a drain never runs inside a callback: `TThread.Queue` runs a method at once when called
on the main thread. `edAuto` uses the main thread when `Classes.WakeMainThread` is set: VCL, FMX and the LCL's win32,
gtk2, gtk3, qt5 and cocoa widgetsets set it.

`OnReadRequest`, `OnPasskeyRequest` and `OnNumericComparison` must answer, so they run on the SimpleBLE thread.

Freeing a local peripheral unregisters its tokens and purges its queued events under the unit's lock, and callbacks
look up their object and queue their event under the same lock. So an event is either purged or never queued.

The lock is never held while calling SimpleBLE: SimpleBLE's adapter and peripheral callbacks hold a
`kvn::safe_callback` mutex while they run, and may be waiting for the lock.

## Shutdown

The unit's finalization sets `ShuttingDown`, clears every registered callback, waits for running callbacks
(`InFlight`), stops the event thread, and frees the objects it owns. Clearing an adapter or peripheral callback waits
for a running one (`kvn::safe_callback`). Notification callbacks can't be cleared without unsubscribing, so
`InFlight` covers them. When a callback doesn't return within five seconds, for example a pairing handler waiting
for the main thread, the shared state is left allocated instead of being freed under it.

## Free Pascal

- Free Pascal on macOS doesn't link the library its `external` declarations name, so the native unit adds
  `{$LINKLIB simplecble}` on Unix.
- Free Pascal 3.2.2 names the mask type `TFPUExceptionMask`, and has no `exAllArithmeticExceptions`.
- Specializing its `TDictionary` warns about abstract methods in its own enumerators (4046), and private constructors
  warn (3018); `SimpleDelphiBLE.pas` turns both off.
- In `{$MODE DELPHIUNICODE}`, Free Pascal's RTL still returns `AnsiString`, so application code like the examples
  gets implicit-conversion warnings.
- `Output` is per thread in Free Pascal, and buffered when it isn't a terminal. Piped output of an example shows the
  event thread's lines when the program ends; in a terminal they appear as they happen.
- A thread must be created through Free Pascal before foreign threads call in, on Windows. The event thread starts
  before the first callback is registered.

## Testing

The tests check the binding layer against PLAIN's fixture (`plain-backend.md`): wiring, conversions, events crossing
threads in each delivery mode, identity, ownership, error mapping, the local GATT loopback and exiting while
callbacks arrive. On macOS or Linux, with Free Pascal installed:

```bash
utils/build_simpledelphible.sh
```

With Delphi on Windows, build SimpleCBLE with `-DSIMPLEBLE_PLAIN=ON -DBUILD_SHARED_LIBS=ON` for each architecture,
then compile the test program with the source folder on the unit and include paths, and run it with the two DLLs next
to it. The units list RTL units without their `System.` scope, as Free Pascal names them, so the command line passes
the scope that IDE projects set by default:

```bat
dcc64 -NSSystem -U..\source -I..\source -E<out> SimpleDelphiBLETests.dpr
dcc32 -NSSystem -U..\source -I..\source -E<out> SimpleDelphiBLETests.dpr
```

## Packaging

A release holds `source`, `examples`, the README, the license, a `boss.json`, the Lazarus package, and
`lib/<platform>` with SimpleBLE built as shared libraries: `win32`, `win64`, `macos-arm64`, `macos-x64`, `linux-x64`
and `linux-arm64`. `utils/package_simpledelphible.sh <libraries> <output>` assembles it from a folder per platform and
zips it as `simpledelphible-<version>.zip`.

`SimpleDelphiBLEPackage.lpk` sits next to `source` and `lib` in the release, as in the repository. It is named apart
from the `SimpleDelphiBLE` unit because Lazarus generates a main unit with the package's name. It compiles the units
into `units/<cpu>-<os>`, and its conditionals add `lib/<platform>` to the library path of projects that require it on
macOS and Linux, where Free Pascal links `libsimplecble`. Windows programs need no library path. A new unit or include
file goes in the `.lpk` too; Lazarus rewrites `SimpleDelphiBLEPackage.pas` when the units change, so commit it with the
`.lpk`. Check the package with:

```bash
lazbuild simpledelphible/SimpleDelphiBLEPackage.lpk
```

The libraries are SimpleCBLE's shared build with three changes:

- The Windows DLLs link the MSVC runtime statically (`CMAKE_MSVC_RUNTIME_LIBRARY=MultiThreaded`), so applications
  need no redistributable.
- The macOS and Linux libraries are unversioned (`CMAKE_PLATFORM_NO_VERSIONED_SONAME=ON`). A versioned build
  identifies itself as `libsimplecble.1.dylib` or `libsimplecble.so.1`, so an application linked against
  `libsimplecble.dylib` would need that other file at run time.
- `libsimplecble` names itself relative to the program that loads it: its install name is
  `@executable_path/libsimplecble.dylib` on macOS, and its SONAME is `$ORIGIN/libsimplecble.so` on Linux. A program
  records that name when it links, so it loads the library from its own folder, or an app bundle's `Contents/MacOS`,
  without an rpath. `libsimplecble` already finds `libsimpleble` next to itself (`@loader_path` and `$ORIGIN`).

## CI distribution

`ci_simpledelphible.yml`, called by `ci_main.yml`, runs the tests with Free Pascal on Linux and macOS, builds the
libraries for each platform, assembles the package, and compiles every example from it against `lib/linux-x64`.

## Versions and publication

Boss resolves versions from Git tags, so SimpleDelphiBLE is published like SimpleGoBLE. For each `main` build and each
published SimpleBLE release, `publish-simpledelphible` in `ci_main.yml` updates the generated
[simpleble/simpledelphible](https://github.com/simpleble/simpledelphible) repository using `SIMPLEDELPHIBLE_TOKEN`, a
token with read and write access to its contents:

1. It replaces the repository's files with the package, and pushes the commit to `main` and its tag `vX.Y.Z` or
   `vX.Y.Z-dev.N` in one atomic push.
2. It creates the tag's GitHub release with `simpledelphible-X.Y.Z.zip`. Development builds are prereleases.

If the tag already exists, the run only creates its release when it is missing. Never replace a published tag or its
archive.

Without a version, Boss installs the highest stable tag: its default constraint, `>0.0.0`, excludes prereleases. A
project follows a development build by naming it in its `boss.json`, such as
`"github.com/simpleble/simpledelphible": "1.3.0-dev.5"`. The package's `mainsrc` puts `source` on the project's search
path, in a `.dproj` or a Lazarus `.lpi`.
