#include "HybridPeripheral.hpp"
#include "HybridService.hpp"

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

std::shared_ptr<Promise<void>> HybridPeripheral::connect() {
    return Promise<void>::async([this]() {
        _peripheral.connect();
    });
}

std::shared_ptr<Promise<void>> HybridPeripheral::disconnect() {
    return Promise<void>::async([this]() {
        _peripheral.disconnect();
    });
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

std::vector<std::shared_ptr<HybridServiceSpec>> HybridPeripheral::services() {
    std::vector<SimpleBLE::Service> peripheral_services = _peripheral.services();
    std::vector<std::shared_ptr<HybridServiceSpec>> hybrid_services;
    hybrid_services.reserve(peripheral_services.size());
    
    for (auto& service : peripheral_services) {
        hybrid_services.push_back(std::make_shared<HybridService>(std::move(service)));
    }
    
    return hybrid_services;
}

std::unordered_map<std::string, std::shared_ptr<ArrayBuffer>> HybridPeripheral::manufacturer_data() {
    std::map<uint16_t, SimpleBLE::ByteArray> peripheral_manufacturer_data = _peripheral.manufacturer_data();
    std::unordered_map<std::string, std::shared_ptr<ArrayBuffer>> manufacturer_array_buffer_map;

    for (const auto& pair : peripheral_manufacturer_data) {
        manufacturer_array_buffer_map[std::to_string(pair.first)] = toArrayBuffer(pair.second);
    }

    return manufacturer_array_buffer_map;
}

std::shared_ptr<Promise<std::shared_ptr<ArrayBuffer>>> HybridPeripheral::read(const std::string& service, const std::string& characteristic) {
    return Promise<std::shared_ptr<ArrayBuffer>>::async([this, service, characteristic]() {
        SimpleBLE::ByteArray peripheral_read_data = _peripheral.read(service, characteristic);
        return toArrayBuffer(peripheral_read_data);
    });
}

std::shared_ptr<Promise<void>> HybridPeripheral::write_request(const std::string& service, const std::string& characteristic, const std::shared_ptr<ArrayBuffer>& data) {
    SimpleBLE::ByteArray bytes = fromArrayBuffer(data);
    return Promise<void>::async([this, service, characteristic, bytes]() {
        _peripheral.write_request(service, characteristic, bytes);
    });
}

void HybridPeripheral::write_command(const std::string& service, const std::string& characteristic, const std::shared_ptr<ArrayBuffer>& data) {
    SimpleBLE::ByteArray bytes = fromArrayBuffer(data);
    _peripheral.write_command(service, characteristic, bytes);
}

std::shared_ptr<Promise<void>> HybridPeripheral::notify(const std::string& service, const std::string& characteristic, 
                              const std::function<void(const std::shared_ptr<ArrayBuffer>&)>& callback) {
    auto key = std::make_pair(service, characteristic);
    _notifyCallbacks[key] = callback;
    
    return Promise<void>::async([this, service, characteristic, key]() {
        _peripheral.notify(service, characteristic, [this, key](SimpleBLE::ByteArray payload) {
            auto it = _notifyCallbacks.find(key);
            if (it != _notifyCallbacks.end() && it->second) {
                it->second(toArrayBuffer(payload));
            }
        });
    });
}

std::shared_ptr<Promise<void>> HybridPeripheral::indicate(const std::string& service, const std::string& characteristic, 
                                const std::function<void(const std::shared_ptr<ArrayBuffer>&)>& callback) {
    // Store callback before entering async
    auto key = std::make_pair(service, characteristic);
    _notifyCallbacks[key] = callback; //@alejo: verify if its necessary to store our own reference to the callback
    
    return Promise<void>::async([this, service, characteristic, key]() {
        _peripheral.indicate(service, characteristic, [this, key](SimpleBLE::ByteArray payload) {
            auto it = _notifyCallbacks.find(key);
            if (it != _notifyCallbacks.end() && it->second) {
                it->second(toArrayBuffer(payload));
            }
        });
    });
}

std::shared_ptr<Promise<void>> HybridPeripheral::unsubscribe(const std::string& service, const std::string& characteristic) {
    auto key = std::make_pair(service, characteristic);
    _notifyCallbacks.erase(key);
    return Promise<void>::async([this, service, characteristic]() {
        _peripheral.unsubscribe(service, characteristic);
    });
}

std::shared_ptr<Promise<std::shared_ptr<ArrayBuffer>>> HybridPeripheral::read_descriptor(const std::string& service, const std::string& characteristic, 
                                              const std::string& descriptor) {
    return Promise<std::shared_ptr<ArrayBuffer>>::async([this, service, characteristic, descriptor]() {
        SimpleBLE::ByteArray peripheral_descriptor_data = _peripheral.read(service, characteristic, descriptor);
        return toArrayBuffer(peripheral_descriptor_data);
    });
}

std::shared_ptr<Promise<void>> HybridPeripheral::write_descriptor(const std::string& service, const std::string& characteristic, 
                                       const std::string& descriptor, const std::shared_ptr<ArrayBuffer>& data) {
    SimpleBLE::ByteArray bytes = fromArrayBuffer(data);
    return Promise<void>::async([this, service, characteristic, descriptor, bytes]() {
        _peripheral.write(service, characteristic, descriptor, bytes);
    });
}

// Helper methods to cast between SimpleBLE::ByteArray and ArrayBuffer (NitroModules native type)
std::shared_ptr<ArrayBuffer> HybridPeripheral::toArrayBuffer(const SimpleBLE::ByteArray& data) {
    return ArrayBuffer::copy(reinterpret_cast<const uint8_t*>(data.data()), data.size());
}

SimpleBLE::ByteArray HybridPeripheral::fromArrayBuffer(const std::shared_ptr<ArrayBuffer>& buffer) {
    return SimpleBLE::ByteArray(
        static_cast<const uint8_t*>(buffer->data()), 
        buffer->size()
    );
}

}  // namespace margelo::nitro::simplejsble
