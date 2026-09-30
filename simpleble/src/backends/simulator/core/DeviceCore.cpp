#include "DeviceCore.h"

namespace SimpleBLE::Simulation::Internal {

ByteArray* CharacteristicData::descriptor(const BluetoothUUID& uuid) {
    for (auto& [descriptor_uuid, value] : descriptors) {
        if (descriptor_uuid == uuid) return &value;
    }
    return nullptr;
}

CharacteristicData* ServiceData::characteristic(const BluetoothUUID& uuid) {
    for (auto& characteristic : characteristics) {
        if (characteristic.uuid == uuid) return &characteristic;
    }
    return nullptr;
}

DeviceCore::DeviceCore(std::string name_, BluetoothAddress address_)
    : name(std::move(name_)), address(std::move(address_)) {
    advertisement.name = name;
    advertisement.address = address;
}

ServiceData* DeviceCore::service(const BluetoothUUID& uuid) {
    for (auto& service : services) {
        if (service.uuid == uuid) return &service;
    }
    return nullptr;
}

CharacteristicData* DeviceCore::characteristic(const BluetoothUUID& service_uuid,
                                               const BluetoothUUID& characteristic_uuid) {
    auto* service_data = service(service_uuid);
    return service_data ? service_data->characteristic(characteristic_uuid) : nullptr;
}

}  // namespace SimpleBLE::Simulation::Internal
