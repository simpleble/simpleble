#include "HybridPeripheral.hpp"

namespace margelo::nitro::simplejsble {

bool HybridPeripheral::initialized() {
    return _peripheral.initialized();
}

std::string HybridPeripheral::identifier() {
    return _peripheral.identifier();
}

std::string HybridPeripheral::address() {
    return _peripheral.address();
}

BluetoothAddressType HybridPeripheral::address_type() {
    auto type = _peripheral.address_type();
    switch (type) {
        case SimpleBLE::BluetoothAddressType::PUBLIC:
            return BluetoothAddressType::PUBLIC;
        case SimpleBLE::BluetoothAddressType::RANDOM:
            return BluetoothAddressType::RANDOM;
        default:
            return BluetoothAddressType::UNSPECIFIED;
    }
}

double HybridPeripheral::rssi() {
    return static_cast<double>(_peripheral.rssi());
}

double HybridPeripheral::tx_power() {
    return static_cast<double>(_peripheral.tx_power());
}

double HybridPeripheral::mtu() {
    return static_cast<double>(_peripheral.mtu());
}

void HybridPeripheral::connect() {
    _peripheral.connect();
}

void HybridPeripheral::disconnect() {
    _peripheral.disconnect();
}

bool HybridPeripheral::is_connected() {
    return _peripheral.is_connected();
}

bool HybridPeripheral::is_connectable() {
    return _peripheral.is_connectable();
}

bool HybridPeripheral::is_paired() {
    return _peripheral.is_paired();
}

void HybridPeripheral::unpair() {
    _peripheral.unpair();
}

void HybridPeripheral::set_callback_on_connected(const std::function<void()>& callback) {
    _onConnected = callback;
    _peripheral.set_callback_on_connected([this]() {
        if (_onConnected) {
            _onConnected();
        }
    });
}

void HybridPeripheral::set_callback_on_disconnected(const std::function<void()>& callback) {
    _onDisconnected = callback;
    _peripheral.set_callback_on_disconnected([this]() {
        if (_onDisconnected) {
            _onDisconnected();
        }
    });
}

}  // namespace margelo::nitro::simplejsble
