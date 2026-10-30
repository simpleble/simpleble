#pragma once

#include <simpleble/Exceptions.h>
#include <simpleble/Types.h>

#include "../common/AdapterBase.h"

#include <kvn_safe_callback.hpp>

#include <simplebluez/standard/Adapter.h>

#include <atomic>
#include <functional>
#include <map>
#include <memory>
#include <mutex>
#include <optional>
#include <set>
#include <string>
#include <vector>

namespace SimpleBLE {

class PeripheralLinux;

namespace Local {
class PeripheralLinux;
}

class AdapterLinux : public SimpleBLE::AdapterBase {
  public:
    AdapterLinux(std::shared_ptr<SimpleBluez::Adapter> adapter);
    virtual ~AdapterLinux();

    void* underlying() const override;

    virtual std::string identifier() override;
    virtual BluetoothAddress address() override;

    virtual void power_on() override;
    virtual void power_off() override;
    virtual bool is_powered() override;

    virtual void scan_start() override;
    virtual void scan_stop() override;
    virtual void scan_for(int timeout_ms) override;
    virtual bool scan_is_active() override;
    virtual std::vector<std::shared_ptr<PeripheralBase>> scan_get_results() override;

    virtual std::vector<std::shared_ptr<PeripheralBase>> get_paired_peripherals() override;

    virtual std::shared_ptr<Local::PeripheralBase> create_local_peripheral() override;

    virtual bool bluetooth_enabled() override;

    void set_passkey_request_callback(std::function<std::optional<std::string>(BluetoothAddress peer)> callback);
    void set_passkey_display_callback(std::function<void(BluetoothAddress peer, const std::string& passkey)> callback);
    void set_numeric_comparison_callback(std::function<bool(BluetoothAddress peer, const std::string& passkey)> callback);
    void set_pairing_authorization_callback(std::function<bool(BluetoothAddress peer)> callback);

    // Answers to the BlueZ agent, which BackendBluez forwards to the adapter of the pairing device.
    std::optional<uint32_t> agent_request_passkey(const BluetoothAddress& peer);
    void agent_display_passkey(const BluetoothAddress& peer, uint32_t passkey);
    bool agent_request_confirmation(const BluetoothAddress& peer, uint32_t passkey);
    bool agent_request_authorization(const BluetoothAddress& peer);

  private:
    std::shared_ptr<SimpleBluez::Adapter> adapter_;

    std::atomic_bool is_scanning_{false};

    std::map<BluetoothAddress, std::shared_ptr<PeripheralLinux>> peripherals_;
    std::map<BluetoothAddress, std::shared_ptr<PeripheralLinux>> seen_peripherals_;
    std::mutex peripherals_mutex_;

    std::set<BluetoothAddress> _seen_addresses;
    std::vector<std::weak_ptr<Local::PeripheralLinux>> _local_peripherals;
    std::mutex _local_peripherals_mutex;

    kvn::safe_callback<std::optional<std::string>(BluetoothAddress)> passkey_request_callback_;
    kvn::safe_callback<void(BluetoothAddress, const std::string&)> passkey_display_callback_;
    kvn::safe_callback<bool(BluetoothAddress, const std::string&)> numeric_comparison_callback_;
    kvn::safe_callback<bool(BluetoothAddress)> pairing_authorization_callback_;

    void _on_device_connected(std::shared_ptr<SimpleBluez::Device> device, bool connected);
};

}  // namespace SimpleBLE
