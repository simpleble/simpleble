#pragma once

#include <atomic>
#include <functional>
#include <map>
#include <memory>
#include <mutex>
#include <vector>

#include <kvn_safe_callback.hpp>

#include "../common/LocalPeripheralBase.h"

namespace SimpleBLE::Local {

class CharacteristicMac;
class ServiceMac;

class PeripheralMac : public PeripheralBase, public std::enable_shared_from_this<PeripheralMac> {
  public:
    PeripheralMac();
    ~PeripheralMac() override;

    void* underlying() const override;

    Advertisement advertisement() override;
    void set_advertisement(Advertisement advertisement) override;

    std::shared_ptr<ServiceBase> add_service(BluetoothUUID uuid) override;
    std::vector<std::shared_ptr<ServiceBase>> services() override;
    void remove_all_services() override;

    void start() override;
    void stop() override;

    bool is_started() override;
    bool is_advertising() override;

    void set_callback_on_client_connected(
        std::function<void(BluetoothAddress client_address)> on_client_connected) override;
    void set_callback_on_client_disconnected(
        std::function<void(BluetoothAddress client_address)> on_client_disconnected) override;

    void register_characteristic(const std::shared_ptr<CharacteristicMac>& characteristic);
    std::shared_ptr<CharacteristicMac> characteristic_for(void* opaque_characteristic);
    void publish(void* opaque_characteristic, const ByteArray& value);
    void handle_subscribed(void* opaque_characteristic, const BluetoothAddress& client_address);
    void handle_unsubscribed(void* opaque_characteristic, const BluetoothAddress& client_address);

  private:
    void* _opaque_internal;
    Advertisement _advertisement;
    std::vector<std::shared_ptr<ServiceMac>> _services;
    std::map<void*, std::weak_ptr<CharacteristicMac>> _characteristics;
    std::map<BluetoothAddress, size_t> _client_subscriptions;
    std::atomic_bool _started{false};
    mutable std::mutex _lifecycle_mutex;
    std::mutex _characteristics_mutex;
    std::mutex _clients_mutex;

    kvn::safe_callback<void(BluetoothAddress)> _callback_on_client_connected;
    kvn::safe_callback<void(BluetoothAddress)> _callback_on_client_disconnected;

    void _ensure_mutable() const;
    void _clear_subscribers();
};

}  // namespace SimpleBLE::Local
