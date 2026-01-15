#include "HybridAdapter.hpp"

namespace margelo::nitro::simplejsble {

bool HybridAdapter::bluetooth_enabled() {
    return SimpleBLE::Adapter::bluetooth_enabled();
}

std::vector<std::shared_ptr<HybridAdapterSpec>> HybridAdapter::get_adapters() {
    std::vector<std::shared_ptr<HybridAdapterSpec>> result;
    for (auto& adapter : SimpleBLE::Adapter::get_adapters()) {
        result.push_back(std::make_shared<HybridAdapter>(std::move(adapter)));
    }
    return result;
}

bool HybridAdapter::initialized() {
    return _adapter.initialized();
}

std::string HybridAdapter::identifier() {
    return _adapter.identifier();
}

std::string HybridAdapter::address() {
    return _adapter.address();
}

bool HybridAdapter::is_powered() {
    return _adapter.is_powered();
}

void HybridAdapter::scan_start() {
    _adapter.scan_start();
}

void HybridAdapter::scan_stop() {
    _adapter.scan_stop();
}

void HybridAdapter::scan_for(double timeout_ms) {
    _adapter.scan_for(static_cast<int>(timeout_ms));
}

bool HybridAdapter::scan_is_active() {
    return _adapter.scan_is_active();
}

std::vector<std::shared_ptr<HybridPeripheralSpec>> HybridAdapter::scan_get_results() {
    std::vector<std::shared_ptr<HybridPeripheralSpec>> result;
    for (auto& peripheral : _adapter.scan_get_results()) {
        result.push_back(std::make_shared<HybridPeripheral>(std::move(peripheral)));
    }
    return result;
}

void HybridAdapter::set_callback_on_scan_start(const std::function<void()>& callback) {
    _onScanStart = callback;
    _adapter.set_callback_on_scan_start([this]() {
        if (_onScanStart) {
            _onScanStart();
        }
    });
}

void HybridAdapter::set_callback_on_scan_stop(const std::function<void()>& callback) {
    _onScanStop = callback;
    _adapter.set_callback_on_scan_stop([this]() {
        if (_onScanStop) {
            _onScanStop();
        }
    });
}

void HybridAdapter::set_callback_on_scan_updated(
    const std::function<void(const std::shared_ptr<HybridPeripheralSpec>&)>& callback) {
    _onScanUpdated = callback;
    _adapter.set_callback_on_scan_updated([this](SimpleBLE::Peripheral peripheral) {
        if (_onScanUpdated) {
            auto hybridPeripheral = std::make_shared<HybridPeripheral>(std::move(peripheral));
            _onScanUpdated(hybridPeripheral);
        }
    });
}

void HybridAdapter::set_callback_on_scan_found(
    const std::function<void(const std::shared_ptr<HybridPeripheralSpec>&)>& callback) {
    _onScanFound = callback;
    _adapter.set_callback_on_scan_found([this](SimpleBLE::Peripheral peripheral) {
        if (_onScanFound) {
            auto hybridPeripheral = std::make_shared<HybridPeripheral>(std::move(peripheral));
            _onScanFound(hybridPeripheral);
        }
    });
}

std::vector<std::shared_ptr<HybridPeripheralSpec>> HybridAdapter::get_paired_peripherals() {
    std::vector<std::shared_ptr<HybridPeripheralSpec>> result;
    for (auto& peripheral : _adapter.get_paired_peripherals()) {
        result.push_back(std::make_shared<HybridPeripheral>(std::move(peripheral)));
    }
    return result;
}

std::vector<std::shared_ptr<HybridPeripheralSpec>> HybridAdapter::get_connected_peripherals() {
    std::vector<std::shared_ptr<HybridPeripheralSpec>> result;
    for (auto& peripheral : _adapter.get_connected_peripherals()) {
        result.push_back(std::make_shared<HybridPeripheral>(std::move(peripheral)));
    }
    return result;
}

}  // namespace margelo::nitro::simplejsble
