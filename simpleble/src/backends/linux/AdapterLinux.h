#pragma once

#include <simpleble/Exceptions.h>
#include <simpleble/Types.h>

#include "../common/AdapterBase.h"

#include <kvn_safe_callback.hpp>

#include <simplebluez/Adapter.h>

#include <atomic>
#include <functional>
#include <memory>
#include <string>
#include <vector>

namespace SimpleBLE {

class PeripheralLinux;

class AdapterLinux : public SimpleBLE::AdapterBase {
  public:
    AdapterLinux(std::shared_ptr<SimpleBluez::Adapter> adapter);
    virtual ~AdapterLinux();

    void* underlying() const override;

    virtual std::string identifier() override;
    virtual BluetoothAddress address() override;

    virtual void scan_start() override;
    virtual void scan_stop() override;
    virtual void scan_for(int timeout_ms) override;
    virtual bool scan_is_active() override;
    virtual std::vector<std::shared_ptr<PeripheralBase>> scan_get_results() override;

    virtual void set_callback_on_scan_start(std::function<void()> on_scan_start) override;
    virtual void set_callback_on_scan_stop(std::function<void()> on_scan_stop) override;
    virtual void set_callback_on_scan_updated(std::function<void(Peripheral)> on_scan_updated) override;
    virtual void set_callback_on_scan_found(std::function<void(Peripheral)> on_scan_found) override;

    virtual std::vector<std::shared_ptr<PeripheralBase>> get_paired_peripherals() override;

    virtual bool bluetooth_enabled() override;

  private:
    std::shared_ptr<SimpleBluez::Adapter> adapter_;

    std::atomic_bool is_scanning_;

    std::map<BluetoothAddress, std::shared_ptr<PeripheralLinux>> peripherals_;
    std::map<BluetoothAddress, std::shared_ptr<PeripheralLinux>> seen_peripherals_;

    kvn::safe_callback<void()> callback_on_scan_start_;
    kvn::safe_callback<void()> callback_on_scan_stop_;
    kvn::safe_callback<void(Peripheral)> callback_on_scan_updated_;
    kvn::safe_callback<void(Peripheral)> callback_on_scan_found_;
};

}  // namespace SimpleBLE
