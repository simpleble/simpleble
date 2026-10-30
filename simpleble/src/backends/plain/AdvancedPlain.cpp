#include <simpleble/Advanced.h>
#include <simpleble/Exceptions.h>

// NOTE: PLAIN replaces the native backend, so it provides the platform functions that Advanced.h declares, and none of
// them is supported.

#if defined(__APPLE__) && TARGET_OS_OSX
namespace SimpleBLE::Advanced::MacOS {

void set_advertisement_local_name(Local::Peripheral& peripheral, std::optional<std::string> local_name) {
    throw Exception::OperationNotSupported();
}

std::vector<Peripheral> retrieve_cached_peripherals(Adapter& adapter,
                                                    const std::vector<BluetoothAddress>& identifiers) {
    throw Exception::OperationNotSupported();
}

}  // namespace SimpleBLE::Advanced::MacOS

#endif

#if defined(__APPLE__) && TARGET_OS_IOS
namespace SimpleBLE::Advanced::iOS {

void set_advertisement_local_name(Local::Peripheral& peripheral, std::optional<std::string> local_name) {
    throw Exception::OperationNotSupported();
}

std::vector<Peripheral> retrieve_cached_peripherals(Adapter& adapter,
                                                    const std::vector<BluetoothAddress>& identifiers) {
    throw Exception::OperationNotSupported();
}

}  // namespace SimpleBLE::Advanced::iOS

#endif

#if defined(__linux__) && !defined(__ANDROID__)
namespace SimpleBLE::Advanced::Linux {

void set_advertisement_local_name(Local::Peripheral& peripheral, std::optional<std::string> local_name) {
    throw Exception::OperationNotSupported();
}

void set_passkey_request_callback(Adapter& adapter,
                                  const std::function<std::optional<std::string>(BluetoothAddress peer)>& callback) {
    throw Exception::OperationNotSupported();
}

void set_passkey_display_callback(Adapter& adapter,
                                  const std::function<void(BluetoothAddress peer, const std::string& passkey)>& callback) {
    throw Exception::OperationNotSupported();
}

void set_numeric_comparison_callback(
    Adapter& adapter, const std::function<bool(BluetoothAddress peer, const std::string& passkey)>& callback) {
    throw Exception::OperationNotSupported();
}

void set_pairing_authorization_callback(Adapter& adapter, const std::function<bool(BluetoothAddress peer)>& callback) {
    throw Exception::OperationNotSupported();
}

}  // namespace SimpleBLE::Advanced::Linux

#endif
