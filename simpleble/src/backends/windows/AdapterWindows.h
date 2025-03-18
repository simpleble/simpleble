#pragma once

#include <simpleble/Exceptions.h>
#include <simpleble/Peripheral.h>
#include <simpleble/Types.h>

#include "AdapterBase.h"
#include "AdapterBaseTypes.h"

#include <kvn_safe_callback.hpp>

#include "winrt/Windows.Devices.Bluetooth.Advertisement.h"
#include "winrt/Windows.Devices.Bluetooth.h"

#include <atomic>
#include <condition_variable>
#include <functional>
#include <memory>
#include <string>
#include <vector>

using namespace winrt::Windows;
using namespace winrt::Windows::Devices;
using namespace winrt::Windows::Devices::Bluetooth;
using namespace winrt::Windows::Devices::Radios;

namespace SimpleBLE {

class PeripheralWindows;
class PeripheralBase;

class AdapterWindows : public AdapterBase {
  public:
    AdapterWindows(std::string device_id);
    virtual ~AdapterWindows();

    virtual void* underlying() const override;

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
    BluetoothAdapter adapter_;
    std::string identifier_;

    struct Advertisement::BluetoothLEAdvertisementWatcher scanner_;
    winrt::event_token scanner_received_token_;
    winrt::event_token scanner_stopped_token_;

    std::atomic_bool scan_is_active_{false};
    std::condition_variable scan_stop_cv_;
    std::mutex scan_stop_mutex_;
    std::mutex scan_update_mutex_;
    std::map<BluetoothAddress, std::shared_ptr<PeripheralWindows>> peripherals_;
    std::map<BluetoothAddress, std::shared_ptr<PeripheralBase>> seen_peripherals_;

    // Event handler methods for scanner events
    void _on_scanner_stopped(
        const Advertisement::BluetoothLEAdvertisementWatcher& watcher,
        const Advertisement::BluetoothLEAdvertisementWatcherStoppedEventArgs args);

    void _on_scanner_received(
        const Advertisement::BluetoothLEAdvertisementWatcher& watcher,
        const Advertisement::BluetoothLEAdvertisementReceivedEventArgs args);

    void _scan_stopped_callback();
    void _scan_received_callback(advertising_data_t data);

    kvn::safe_callback<void()> callback_on_scan_start_;
    kvn::safe_callback<void()> callback_on_scan_stop_;
    kvn::safe_callback<void(Peripheral)> callback_on_scan_updated_;
    kvn::safe_callback<void(Peripheral)> callback_on_scan_found_;
};

}  // namespace SimpleBLE
