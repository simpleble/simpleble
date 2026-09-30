#pragma once

#include <simpleble/Types.h>
#include <simpleble/simulation/Types.h>

#include <chrono>
#include <cstdint>
#include <limits>
#include <map>
#include <memory>
#include <mutex>
#include <optional>
#include <set>
#include <string>
#include <utility>
#include <vector>

#include "SimulatorUtils.h"

namespace SimpleBLE::Simulation {
class Device;
}

namespace SimpleBLE::Simulation::Internal {

class ConnectionCore;
class EnvironmentCore;

struct CharacteristicData {
    BluetoothUUID uuid;
    std::set<Property> properties;
    ByteArray value;
    std::vector<std::pair<BluetoothUUID, ByteArray>> descriptors;

    bool has(Property property) const { return properties.count(property) > 0; }
    bool subscribable() const { return has(Property::NOTIFY) || has(Property::INDICATE); }
    ByteArray* descriptor(const BluetoothUUID& uuid);
};

struct ServiceData {
    BluetoothUUID uuid;
    std::vector<CharacteristicData> characteristics;

    CharacteristicData* characteristic(const BluetoothUUID& uuid);
};

/** What a device puts on air during one advertising event. The RSSI is set by the receiving link. */
struct Advertisement {
    std::string name;
    BluetoothAddress address;
    bool connectable = true;
    int16_t tx_power = std::numeric_limits<int16_t>::min();
    int16_t rssi = 0;
    std::vector<BluetoothUUID> services;
    std::map<uint16_t, ByteArray> manufacturer_data;
    std::map<BluetoothUUID, ByteArray> service_data;
};

/** Link-layer configuration of a device's controller and stack. */
struct PeripheralSettings {
    /** Interval range the device's stack asks for after connecting, if any. */
    std::optional<std::pair<Clock::duration, Clock::duration>> preferred_interval;
    uint16_t max_mtu = 247;
    uint16_t max_data_length = 27;
    Clock::duration max_event_length = std::chrono::microseconds(7500);
    size_t tx_buffers = 4;
    size_t max_connections = 1;
};

class DeviceCore {
  public:
    DeviceCore(std::string name, BluetoothAddress address);

    mutable std::mutex mutex;

    std::string name;
    BluetoothAddress address;
    Clock::duration advertising_interval = std::chrono::milliseconds(100);
    Advertisement advertisement;
    PeripheralSettings settings;
    std::vector<ServiceData> services;
    std::map<int, std::pair<uint64_t, Clock::duration>> timers;  // id -> (generation, period)
    uint64_t next_timer_generation = 1;
    uint64_t advertising_generation = 0;

    std::weak_ptr<Device> self;
    std::weak_ptr<EnvironmentCore> environment;
    std::vector<std::weak_ptr<ConnectionCore>> connections;

    // Callers hold `mutex`.
    ServiceData* service(const BluetoothUUID& uuid);
    CharacteristicData* characteristic(const BluetoothUUID& service, const BluetoothUUID& characteristic);
};

}  // namespace SimpleBLE::Simulation::Internal
