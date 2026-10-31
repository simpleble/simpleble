#include "AdapterWrapper.h"
#include "Cache.h"
#include "CallbackRunner.h"

#include <simpleble/Advanced.h>

#include <memory>
#include <string>

AdapterWrapper::AdapterWrapper(const SimpleBLE::Adapter& adapter) : _adapter(adapter) {}

AdapterWrapper::~AdapterWrapper() {
    _adapter.set_callback_on_power_on(nullptr);
    _adapter.set_callback_on_power_off(nullptr);
    _adapter.set_callback_on_scan_found(nullptr);
    _adapter.set_callback_on_scan_updated(nullptr);
    _adapter.set_callback_on_scan_start(nullptr);
    _adapter.set_callback_on_scan_stop(nullptr);
}

SimpleBLE::Adapter& AdapterWrapper::get() { return _adapter; }

const SimpleBLE::Adapter& AdapterWrapper::get() const { return _adapter; }

void AdapterWrapper::setCallback(jobject callback) {
    // NOTE: SimpleBLE's threads only queue the events. The callback runner calls Java.
    auto adapter_callback = make_java_callback<Org::SimpleJavaBLE::AdapterCallback>(callback);
    _callback = adapter_callback;
    const int64_t adapter_id = getHash(_adapter);

    _adapter.set_callback_on_power_on([adapter_callback]() {
        callback_runner().enqueue([adapter_callback]() { adapter_callback->on_power_on(); });
    });

    _adapter.set_callback_on_power_off([adapter_callback]() {
        callback_runner().enqueue([adapter_callback]() { adapter_callback->on_power_off(); });
    });

    _adapter.set_callback_on_scan_found([adapter_id, adapter_callback](SimpleBLE::Peripheral peripheral) {
        const int64_t peripheral_id = Cache::get().addPeripheral(adapter_id, peripheral);
        callback_runner().enqueue(
            [adapter_callback, peripheral_id]() { adapter_callback->on_scan_found(peripheral_id); });
    });

    _adapter.set_callback_on_scan_updated([adapter_id, adapter_callback](SimpleBLE::Peripheral peripheral) {
        // NOTE: Java might see a peripheral for the first time through an update, so we need to cache it here too.
        const int64_t peripheral_id = Cache::get().addPeripheral(adapter_id, peripheral);
        callback_runner().enqueue(
            [adapter_callback, peripheral_id]() { adapter_callback->on_scan_updated(peripheral_id); });
    });

    _adapter.set_callback_on_scan_start([adapter_callback]() {
        callback_runner().enqueue([adapter_callback]() { adapter_callback->on_scan_start(); });
    });

    _adapter.set_callback_on_scan_stop([adapter_callback]() {
        callback_runner().enqueue([adapter_callback]() { adapter_callback->on_scan_stop(); });
    });
}

#if defined(__linux__) && !defined(__ANDROID__)
// NOTE: SimpleBLE's BlueZ event thread waits for these answers, so they call Java directly instead of going through
// the callback runner.
void AdapterWrapper::setPasskeyRequestCallback(bool enabled) {
    if (!enabled) {
        SimpleBLE::Advanced::Linux::set_passkey_request_callback(_adapter, nullptr);
        return;
    }

    SimpleBLE::Advanced::Linux::set_passkey_request_callback(
        _adapter, [adapter_callback = _callback](SimpleBLE::BluetoothAddress peer) {
            return adapter_callback->on_passkey_request(peer);
        });
}

void AdapterWrapper::setPasskeyDisplayCallback(bool enabled) {
    if (!enabled) {
        SimpleBLE::Advanced::Linux::set_passkey_display_callback(_adapter, nullptr);
        return;
    }

    SimpleBLE::Advanced::Linux::set_passkey_display_callback(
        _adapter, [adapter_callback = _callback](SimpleBLE::BluetoothAddress peer, const std::string& passkey) {
            adapter_callback->on_passkey_display(peer, passkey);
        });
}

void AdapterWrapper::setNumericComparisonCallback(bool enabled) {
    if (!enabled) {
        SimpleBLE::Advanced::Linux::set_numeric_comparison_callback(_adapter, nullptr);
        return;
    }

    SimpleBLE::Advanced::Linux::set_numeric_comparison_callback(
        _adapter, [adapter_callback = _callback](SimpleBLE::BluetoothAddress peer, const std::string& passkey) {
            return adapter_callback->on_numeric_comparison(peer, passkey);
        });
}

void AdapterWrapper::setPairingAuthorizationCallback(bool enabled) {
    if (!enabled) {
        SimpleBLE::Advanced::Linux::set_pairing_authorization_callback(_adapter, nullptr);
        return;
    }

    SimpleBLE::Advanced::Linux::set_pairing_authorization_callback(
        _adapter, [adapter_callback = _callback](SimpleBLE::BluetoothAddress peer) {
            return adapter_callback->on_pairing_authorization(peer);
        });
}
#endif

int64_t AdapterWrapper::getHash(SimpleBLE::Adapter& adapter) {
    return static_cast<int64_t>(std::hash<std::string>{}(adapter.identifier() + "|" + adapter.address()));
}
