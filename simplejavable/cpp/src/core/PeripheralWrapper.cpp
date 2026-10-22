#include "PeripheralWrapper.h"

#include <simpleble/Advanced.h>

#include <string>

#include "CallbackRunner.h"

PeripheralWrapper::PeripheralWrapper(const SimpleBLE::Peripheral& peripheral) : _peripheral(peripheral) {}

SimpleBLE::Peripheral& PeripheralWrapper::get() { return _peripheral; }

const SimpleBLE::Peripheral& PeripheralWrapper::get() const { return _peripheral; }

void PeripheralWrapper::setCallback(jobject callback) {
    // NOTE: SimpleBLE's threads only queue the events. The callback runner calls Java.
    auto peripheral_callback = make_java_callback<Org::SimpleJavaBLE::PeripheralCallback>(callback);
    _callback = peripheral_callback;

    _peripheral.set_callback_on_connected([peripheral_callback]() {
        callback_runner().enqueue([peripheral_callback]() { peripheral_callback->on_connected(); });
    });

    _peripheral.set_callback_on_disconnected([peripheral_callback]() {
        callback_runner().enqueue([peripheral_callback]() { peripheral_callback->on_disconnected(); });
    });
}

void PeripheralWrapper::setPasskeyRequestCallback(bool enabled) {
    if (!enabled) {
        SimpleBLE::Advanced::Dongl::set_passkey_request_callback(_peripheral, nullptr);
        return;
    }

    SimpleBLE::Advanced::Dongl::set_passkey_request_callback(
        _peripheral, [peripheral_callback = _callback]() { return peripheral_callback->on_passkey_request(); });
}

void PeripheralWrapper::setPasskeyDisplayCallback(bool enabled) {
    if (!enabled) {
        SimpleBLE::Advanced::Dongl::set_passkey_display_callback(_peripheral, nullptr);
        return;
    }

    SimpleBLE::Advanced::Dongl::set_passkey_display_callback(
        _peripheral, [peripheral_callback = _callback](const std::string& passkey) {
            peripheral_callback->on_passkey_display(passkey);
        });
}

void PeripheralWrapper::setNumericComparisonCallback(bool enabled) {
    if (!enabled) {
        SimpleBLE::Advanced::Dongl::set_numeric_comparison_callback(_peripheral, nullptr);
        return;
    }

    SimpleBLE::Advanced::Dongl::set_numeric_comparison_callback(
        _peripheral, [peripheral_callback = _callback](const std::string& passkey) {
            return peripheral_callback->on_numeric_comparison(passkey);
        });
}

int64_t PeripheralWrapper::getHash(SimpleBLE::Peripheral& peripheral) {
    return static_cast<int64_t>(std::hash<std::string>{}(peripheral.address()));
}
