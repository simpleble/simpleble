#pragma once

#include <atomic>
#include <memory>
#include <mutex>
#include <set>
#include <string>
#include <vector>

#include <simplebluez/standard/Adapter.h>
#include <simplebluez/standard/Advertisement.h>
#include <simplebluez/standard/CustomRoot.h>
#include <simplebluez/standard/ServiceManager.h>
#include <kvn_safe_callback.hpp>

#include "../common/LocalPeripheralBase.h"

namespace SimpleBLE::Local {

class ServiceLinux;

class PeripheralLinux : public PeripheralBase {
  public:
    PeripheralLinux(std::shared_ptr<SimpleBluez::Adapter> adapter, std::string name);
    ~PeripheralLinux() override;

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

    void handle_connected_changed(const BluetoothAddress& address, bool connected);

  private:
    std::shared_ptr<SimpleBluez::Adapter> _adapter;
    std::shared_ptr<SimpleBluez::CustomRoot> _root;
    std::shared_ptr<SimpleBluez::ServiceManager> _service_manager;
    std::shared_ptr<SimpleBluez::Advertisement> _bluez_advertisement;
    std::string _name;

    Advertisement _advertisement;
    std::vector<std::shared_ptr<ServiceLinux>> _services;
    size_t _next_service_id{0};
    std::atomic_bool _started{false};
    mutable std::mutex _lifecycle_mutex;

    std::set<BluetoothAddress> _connected_clients;
    std::mutex _clients_mutex;
    kvn::safe_callback<void(BluetoothAddress)> _callback_on_client_connected;
    kvn::safe_callback<void(BluetoothAddress)> _callback_on_client_disconnected;

    void _ensure_mutable() const;
};

}  // namespace SimpleBLE::Local
