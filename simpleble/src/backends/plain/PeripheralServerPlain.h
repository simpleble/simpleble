#pragma once

#include "PeripheralServerBase.h"

#include <atomic>
#include <mutex>
#include <optional>
#include <vector>

namespace SimpleBLE {

class PeripheralServerPlain : public PeripheralServerBase {
  public:
    PeripheralServerPlain();
    virtual ~PeripheralServerPlain();

    virtual void* underlying() const override;

    virtual void add_service(ServiceDefinition service) override;
    virtual void remove_service(BluetoothUUID const& service) override;
    virtual std::vector<ServiceDefinition> services() override;

    virtual void start() override;
    virtual void stop() override;
    virtual bool is_active() override;

    virtual void start_advertising(AdvertisingData advertising_data) override;
    virtual void stop_advertising() override;
    virtual bool is_advertising() override;

    virtual void notify(BluetoothUUID const& service, BluetoothUUID const& characteristic, ByteArray const& data) override;
    virtual void notify(Central const& central, BluetoothUUID const& service, BluetoothUUID const& characteristic,
                        ByteArray const& data) override;
    virtual void indicate(BluetoothUUID const& service, BluetoothUUID const& characteristic, ByteArray const& data) override;
    virtual void indicate(Central const& central, BluetoothUUID const& service, BluetoothUUID const& characteristic,
                          ByteArray const& data) override;

    virtual std::vector<Central> centrals() override;

    std::optional<AdvertisingData> advertising_data();
    std::vector<ByteArray> notifications();
    std::vector<ByteArray> indications();

  private:
    void _ensure_characteristic_exists(BluetoothUUID const& service, BluetoothUUID const& characteristic);

    std::atomic_bool active_{false};
    std::atomic_bool advertising_{false};
    std::mutex mutex_;

    std::vector<ServiceDefinition> services_;
    std::optional<AdvertisingData> advertising_data_;
    std::vector<Central> centrals_;
    std::vector<ByteArray> notifications_;
    std::vector<ByteArray> indications_;
};

}  // namespace SimpleBLE
