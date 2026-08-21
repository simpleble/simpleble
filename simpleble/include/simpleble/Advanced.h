#pragma once

#include <functional>
#include <optional>
#include <string>
#include <vector>

#include <simpleble/Adapter.h>
#include <simpleble/export.h>

#if __APPLE__
#include "TargetConditionals.h"
#endif

namespace SimpleBLE::Advanced::Dongl {

/**
 * Register a callback for entering a passkey requested by the peer.
 *
 * The callback must return exactly six decimal digits, including leading zeroes.
 * Return std::nullopt to reject the pairing request. An absent callback, an
 * invalid passkey, or an exception from the callback also rejects the request.
 *
 * Set this callback before connecting because pairing may begin immediately.
 * The callback runs on an internal worker thread and may block while obtaining
 * the passkey from the user. Passing an empty callback unregisters it.
 */
void SIMPLEBLE_EXPORT set_passkey_request_callback(Peripheral& peripheral,
                                                   const std::function<std::optional<std::string>()>& callback);

/**
 * Register a callback for a passkey that the peer must enter.
 *
 * The callback receives the passkey as exactly six decimal digits, including
 * leading zeroes. Display it to the user without modification. This event does
 * not require a reply; an absent callback simply ignores the event.
 *
 * Set this callback before connecting because pairing may begin immediately.
 * The callback runs on an internal worker thread. Passing an empty callback
 * unregisters it.
 */
void SIMPLEBLE_EXPORT set_passkey_display_callback(Peripheral& peripheral,
                                                   const std::function<void(const std::string& passkey)>& callback);

/**
 * Register a callback for numeric comparison during pairing.
 *
 * The callback receives the number as exactly six decimal digits, including
 * leading zeroes. Return true only after the user confirms that the peer shows
 * the same number; return false to reject pairing. An absent callback or an
 * exception from the callback rejects the request.
 *
 * Set this callback before connecting because pairing may begin immediately.
 * The callback runs on an internal worker thread and may block while obtaining
 * confirmation from the user. Passing an empty callback unregisters it.
 */
void SIMPLEBLE_EXPORT set_numeric_comparison_callback(Peripheral& peripheral,
                                                      const std::function<bool(const std::string& passkey)>& callback);

}  // namespace SimpleBLE::Advanced::Dongl

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

/**
 * Provide the Android application context used to host local GATT services.
 *
 * SimpleBLE retains a global reference to the supplied context. Pass an
 * application context rather than an Activity to avoid extending an Activity
 * lifecycle. SimpleDroidBLE configures this automatically when
 * Adapter.createLocalPeripheral(context) is called.
 */
void SIMPLEBLE_EXPORT set_context(jobject context);

}  // namespace SimpleBLE::Advanced::Android

#endif

#if defined(__linux__) && !defined(__ANDROID__)
namespace SimpleBLE::Advanced::Linux {}

#endif
