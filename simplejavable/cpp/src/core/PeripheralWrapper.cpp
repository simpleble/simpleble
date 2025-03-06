#include "PeripheralWrapper.h"

PeripheralWrapper::PeripheralWrapper(const SimpleBLE::Peripheral& peripheral) : _peripheral(peripheral) {}

SimpleBLE::Peripheral& PeripheralWrapper::getPeripheral() {
    return _peripheral;
}

const SimpleBLE::Peripheral& PeripheralWrapper::getPeripheral() const {
    return _peripheral;
}

void PeripheralWrapper::setCallback(Org::SimpleJavaBLE::PeripheralCallback& callback) {
    _callback = std::move(callback);

    _peripheral.set_callback_on_connected([this]() {
        _callback.on_connected();
    });

    _peripheral.set_callback_on_disconnected([this]() {
        _callback.on_disconnected();
    });
}

size_t PeripheralWrapper::getHash() {
    return std::hash<std::string>{}(_peripheral.address());
}

std::string PeripheralWrapper::identifier() {
    return _peripheral.identifier();
}

SimpleBLE::BluetoothAddress PeripheralWrapper::address() {
    return _peripheral.address();
}
