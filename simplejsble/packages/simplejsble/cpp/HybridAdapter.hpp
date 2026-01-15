#pragma once

#include "HybridAdapterSpec.hpp"
#include "HybridPeripheral.hpp"
#include <simpleble/SimpleBLE.h>
#include <functional>
#include <string>
#include <vector>
#include <memory>

namespace margelo::nitro::simplejsble {

class HybridAdapter : public HybridAdapterSpec {
  public:
    HybridAdapter() : HybridObject(TAG) {}
    explicit HybridAdapter(SimpleBLE::Adapter adapter)
        : HybridObject(TAG), _adapter(std::move(adapter)) {}

    bool bluetooth_enabled() override;
    std::vector<std::shared_ptr<HybridAdapterSpec>> get_adapters() override;

    bool initialized() override;
    std::string identifier() override;
    std::string address() override;

    bool is_powered() override;

    void scan_start() override;
    void scan_stop() override;
    void scan_for(double timeout_ms) override;
    bool scan_is_active() override;
    std::vector<std::shared_ptr<HybridPeripheralSpec>> scan_get_results() override;
    void set_callback_on_scan_start(const std::function<void()>& callback) override;
    void set_callback_on_scan_stop(const std::function<void()>& callback) override;
    void set_callback_on_scan_updated(
        const std::function<void(const std::shared_ptr<HybridPeripheralSpec>&)>& callback) override;
    void set_callback_on_scan_found(
        const std::function<void(const std::shared_ptr<HybridPeripheralSpec>&)>& callback) override;

    std::vector<std::shared_ptr<HybridPeripheralSpec>> get_paired_peripherals() override;
    std::vector<std::shared_ptr<HybridPeripheralSpec>> get_connected_peripherals() override;

  private:
    SimpleBLE::Adapter _adapter;

    std::function<void()> _onScanStart;
    std::function<void()> _onScanStop;
    std::function<void(const std::shared_ptr<HybridPeripheralSpec>&)> _onScanUpdated;
    std::function<void(const std::shared_ptr<HybridPeripheralSpec>&)> _onScanFound;
};

}  // namespace margelo::nitro::simplejsble
