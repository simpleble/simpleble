#pragma once

#include <vector>

#include <simpleble/Adapter.h>
#include <simpleble/export.h>

#if __APPLE__
#include "TargetConditionals.h"
#endif

/**
 * Advanced Features
 *
 * The functions presented in this namespace are OS-specific backdoors that are
 * not part of the standard SimpleBLE API, which allow the user to access
 * low-level details of the implementation for advanced use cases.
 *
 * These functions should be used with caution.
 */

#if defined(_WIN32)
namespace SimpleBLE::Advanced::Windows {}

#endif

#if TARGET_OS_OSX
namespace SimpleBLE::Advanced::MacOS {

/**
 * Retrieve peripherals that CoreBluetooth can resolve from its system cache.
 *
 * This does not scan or guarantee that a returned peripheral is reachable.
 * Invalid identifiers and identifiers unknown to CoreBluetooth are omitted.
 * The adapter must be powered on.
 *
 * NOTE: "Cached" refers to CoreBluetooth's system cache, not SimpleBLE's
 * in-memory cache.
 */
std::vector<Peripheral> SIMPLEBLE_EXPORT retrieve_cached_peripherals(Adapter& adapter,
                                                                     const std::vector<BluetoothAddress>& identifiers);

}  // namespace SimpleBLE::Advanced::MacOS

#endif

#if TARGET_OS_IOS
namespace SimpleBLE::Advanced::iOS {

/**
 * Retrieve peripherals that CoreBluetooth can resolve from its system cache.
 *
 * This does not scan or guarantee that a returned peripheral is reachable.
 * Invalid identifiers and identifiers unknown to CoreBluetooth are omitted.
 * The adapter must be powered on.
 *
 * NOTE: "Cached" refers to CoreBluetooth's system cache, not SimpleBLE's
 * in-memory cache.
 */
std::vector<Peripheral> SIMPLEBLE_EXPORT retrieve_cached_peripherals(Adapter& adapter,
                                                                     const std::vector<BluetoothAddress>& identifiers);

}  // namespace SimpleBLE::Advanced::iOS

#endif

#if defined(__ANDROID__)

#include <jni.h>

namespace SimpleBLE::Advanced::Android {

JavaVM* SIMPLEBLE_EXPORT get_jvm();
void SIMPLEBLE_EXPORT set_jvm(JavaVM* jvm);

}  // namespace SimpleBLE::Advanced::Android

#endif

#if defined(__linux__) && !defined(__ANDROID__)
namespace SimpleBLE::Advanced::Linux {}

#endif
