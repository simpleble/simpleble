#include "PeripheralServerPlain.h"

#include <algorithm>

#include "CommonUtils.h"

using namespace SimpleBLE;

PeripheralServerPlain::PeripheralServerPlain() {}

PeripheralServerPlain::~PeripheralServerPlain() {}

void* PeripheralServerPlain::underlying() const { return nullptr; }

void PeripheralServerPlain::add_service(ServiceDefinition service) {
    std::lock_guard<std::mutex> lock(mutex_);

    auto it = std::find_if(services_.begin(), services_.end(),
                           [&](const ServiceDefinition& existing) { return existing.uuid == service.uuid; });
    if (it != services_.end()) {
        *it = std::move(service);
    } else {
        services_.push_back(std::move(service));
    }
}

void PeripheralServerPlain::remove_service(BluetoothUUID const& service) {
    std::lock_guard<std::mutex> lock(mutex_);

    services_.erase(std::remove_if(services_.begin(), services_.end(),
                                   [&](const ServiceDefinition& existing) { return existing.uuid == service; }),
                    services_.end());
}

std::vector<ServiceDefinition> PeripheralServerPlain::services() {
    std::lock_guard<std::mutex> lock(mutex_);
    return services_;
}

void PeripheralServerPlain::start() {
    const bool was_active = active_.exchange(true);
    if (!was_active) {
        Central central("plain-central", "00:00:00:00:00:00", BluetoothAddressType::PUBLIC, 247);
        {
            std::lock_guard<std::mutex> lock(mutex_);
            centrals_ = {central};
        }

        SAFE_CALLBACK_CALL(this->_callback_on_started);
        SAFE_CALLBACK_CALL(this->_callback_on_central_connected, central);
    }
}

void PeripheralServerPlain::stop() {
    const bool was_advertising = advertising_.exchange(false);
    const bool was_active = active_.exchange(false);

    if (was_advertising) {
        SAFE_CALLBACK_CALL(this->_callback_on_advertising_stopped);
    }

    if (was_active) {
        std::vector<Central> centrals;
        {
            std::lock_guard<std::mutex> lock(mutex_);
            centrals = centrals_;
            centrals_.clear();
        }

        for (auto& central : centrals) {
            SAFE_CALLBACK_CALL(this->_callback_on_central_disconnected, central);
        }
        SAFE_CALLBACK_CALL(this->_callback_on_stopped);
    }
}

bool PeripheralServerPlain::is_active() { return active_; }

void PeripheralServerPlain::start_advertising(AdvertisingData advertising_data) {
    if (!is_active()) {
        start();
    }

    {
        std::lock_guard<std::mutex> lock(mutex_);
        advertising_data_ = std::move(advertising_data);
    }

    const bool was_advertising = advertising_.exchange(true);
    if (!was_advertising) {
        SAFE_CALLBACK_CALL(this->_callback_on_advertising_started);
    }
}

void PeripheralServerPlain::stop_advertising() {
    const bool was_advertising = advertising_.exchange(false);
    if (was_advertising) {
        SAFE_CALLBACK_CALL(this->_callback_on_advertising_stopped);
    }
}

bool PeripheralServerPlain::is_advertising() { return advertising_; }

void PeripheralServerPlain::notify(BluetoothUUID const& service, BluetoothUUID const& characteristic, ByteArray const& data) {
    _ensure_characteristic_exists(service, characteristic);

    std::lock_guard<std::mutex> lock(mutex_);
    notifications_.push_back(data);
}

void PeripheralServerPlain::notify(Central const& central, BluetoothUUID const& service, BluetoothUUID const& characteristic,
                                   ByteArray const& data) {
    (void)central;
    notify(service, characteristic, data);
}

void PeripheralServerPlain::indicate(BluetoothUUID const& service, BluetoothUUID const& characteristic, ByteArray const& data) {
    _ensure_characteristic_exists(service, characteristic);

    std::lock_guard<std::mutex> lock(mutex_);
    indications_.push_back(data);
}

void PeripheralServerPlain::indicate(Central const& central, BluetoothUUID const& service,
                                     BluetoothUUID const& characteristic, ByteArray const& data) {
    (void)central;
    indicate(service, characteristic, data);
}

std::vector<Central> PeripheralServerPlain::centrals() {
    std::lock_guard<std::mutex> lock(mutex_);
    return centrals_;
}

std::optional<AdvertisingData> PeripheralServerPlain::advertising_data() {
    std::lock_guard<std::mutex> lock(mutex_);
    return advertising_data_;
}

std::vector<ByteArray> PeripheralServerPlain::notifications() {
    std::lock_guard<std::mutex> lock(mutex_);
    return notifications_;
}

std::vector<ByteArray> PeripheralServerPlain::indications() {
    std::lock_guard<std::mutex> lock(mutex_);
    return indications_;
}

void PeripheralServerPlain::_ensure_characteristic_exists(BluetoothUUID const& service, BluetoothUUID const& characteristic) {
    std::lock_guard<std::mutex> lock(mutex_);

    auto service_it = std::find_if(services_.begin(), services_.end(),
                                   [&](const ServiceDefinition& existing) { return existing.uuid == service; });
    if (service_it == services_.end()) {
        throw Exception::ServiceNotFound(service);
    }

    auto characteristic_it =
        std::find_if(service_it->characteristics.begin(), service_it->characteristics.end(),
                     [&](const CharacteristicDefinition& existing) { return existing.uuid == characteristic; });
    if (characteristic_it == service_it->characteristics.end()) {
        throw Exception::CharacteristicNotFound(characteristic);
    }
}
