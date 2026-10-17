#include "HybridAdapterHandle.hpp"

#include <cmath>
#include <cstdint>
#include <stdexcept>

#include <simpleble/Advanced.h>

#include "HybridPeripheralHandle.hpp"
#include "LocalHandles.hpp"

namespace margelo::nitro::simplernble {

namespace {

std::vector<std::shared_ptr<HybridPeripheralHandleSpec>> wrap(std::vector<SimpleBLE::Peripheral> peripherals) {
    std::vector<std::shared_ptr<HybridPeripheralHandleSpec>> handles;
    handles.reserve(peripherals.size());
    for (auto& peripheral : peripherals) handles.push_back(intern<HybridPeripheralHandle>(peripheral));
    return handles;
}

}  // namespace

HybridAdapterHandle::HybridAdapterHandle(SimpleBLE::Adapter adapter) : HybridObject(TAG), adapter_(std::move(adapter)) {}

std::string HybridAdapterHandle::identifier() {
    return guard([&] { return adapter_.identifier(); });
}

std::string HybridAdapterHandle::address() {
    return guard([&] { return adapter_.address(); });
}

std::shared_ptr<Promise<bool>> HybridAdapterHandle::isPowered() {
    auto adapter = adapter_;
    return run<bool>(strand_, [adapter]() mutable { return adapter.is_powered(); });
}

std::shared_ptr<Promise<void>> HybridAdapterHandle::powerOn() {
    auto adapter = adapter_;
    return run<void>(strand_, [adapter]() mutable { adapter.power_on(); });
}

std::shared_ptr<Promise<void>> HybridAdapterHandle::powerOff() {
    auto adapter = adapter_;
    return run<void>(strand_, [adapter]() mutable { adapter.power_off(); });
}

std::shared_ptr<Promise<void>> HybridAdapterHandle::scanStart() {
    auto adapter = adapter_;
    return run<void>(strand_, [adapter]() mutable { adapter.scan_start(); });
}

std::shared_ptr<Promise<void>> HybridAdapterHandle::scanStop() {
    auto adapter = adapter_;
    return run<void>(strand_, [adapter]() mutable { adapter.scan_stop(); });
}

std::shared_ptr<Promise<void>> HybridAdapterHandle::scanFor(double timeoutMs) {
    if (!(timeoutMs >= 0 && timeoutMs <= INT32_MAX) || std::floor(timeoutMs) != timeoutMs) {
        throw std::invalid_argument("timeoutMs must be an integer from 0 to 2147483647");
    }
    auto adapter = adapter_;
    const int timeout = static_cast<int>(timeoutMs);
    return run<void>(strand_, [adapter, timeout]() mutable { adapter.scan_for(timeout); });
}

std::shared_ptr<Promise<bool>> HybridAdapterHandle::scanIsActive() {
    auto adapter = adapter_;
    return run<bool>(strand_, [adapter]() mutable { return adapter.scan_is_active(); });
}

std::vector<std::shared_ptr<HybridPeripheralHandleSpec>> HybridAdapterHandle::scanGetResults() {
    return guard([&] { return wrap(adapter_.scan_get_results()); });
}

std::shared_ptr<Promise<std::vector<std::shared_ptr<HybridPeripheralHandleSpec>>>> HybridAdapterHandle::getPairedPeripherals() {
    auto adapter = adapter_;
    return run<std::vector<std::shared_ptr<HybridPeripheralHandleSpec>>>(
        strand_, [adapter]() mutable { return wrap(adapter.get_paired_peripherals()); });
}

std::shared_ptr<Promise<std::vector<std::shared_ptr<HybridPeripheralHandleSpec>>>> HybridAdapterHandle::getConnectedPeripherals() {
    auto adapter = adapter_;
    return run<std::vector<std::shared_ptr<HybridPeripheralHandleSpec>>>(
        strand_, [adapter]() mutable { return wrap(adapter.get_connected_peripherals()); });
}

// NOTE: Only CoreBluetooth on iOS keeps a cache of known peripherals.
std::shared_ptr<Promise<std::vector<std::shared_ptr<HybridPeripheralHandleSpec>>>> HybridAdapterHandle::retrieveCachedPeripherals(
    const std::vector<std::string>& identifiers) {
    auto adapter = adapter_;
    return run<std::vector<std::shared_ptr<HybridPeripheralHandleSpec>>>(strand_, [adapter, identifiers]() mutable {
#if SIMPLEBLE_BACKEND_IOS
        return wrap(SimpleBLE::Advanced::iOS::retrieve_cached_peripherals(adapter, identifiers));
#else
        (void)adapter;
        (void)identifiers;
        throw SimpleBLE::Exception::OperationNotSupported();
        return std::vector<std::shared_ptr<HybridPeripheralHandleSpec>>();
#endif
    });
}

std::shared_ptr<HybridLocalPeripheralHandleSpec> HybridAdapterHandle::createLocalPeripheral() {
    return guard([&] {
        return std::static_pointer_cast<HybridLocalPeripheralHandleSpec>(
            intern<HybridLocalPeripheralHandle>(adapter_.create_local_peripheral()));
    });
}

// NOTE: The callbacks stay with the native adapter, which outlives this object.
// The listener runs on the JavaScript thread, in the order the events happened.
void HybridAdapterHandle::subscribe(
    const std::function<void(AdapterEvent, const std::optional<std::shared_ptr<HybridPeripheralHandleSpec>>&)>& listener) {
    auto emit = listener;
    guard([&] {
        adapter_.set_callback_on_scan_start([emit] { emit(AdapterEvent::SCANSTART, std::nullopt); });
        adapter_.set_callback_on_scan_stop([emit] { emit(AdapterEvent::SCANSTOP, std::nullopt); });
        adapter_.set_callback_on_power_on([emit] { emit(AdapterEvent::POWERON, std::nullopt); });
        adapter_.set_callback_on_power_off([emit] { emit(AdapterEvent::POWEROFF, std::nullopt); });
        adapter_.set_callback_on_scan_found([emit](SimpleBLE::Peripheral peripheral) {
            emit(AdapterEvent::SCANFOUND, intern<HybridPeripheralHandle>(peripheral));
        });
        adapter_.set_callback_on_scan_updated([emit](SimpleBLE::Peripheral peripheral) {
            emit(AdapterEvent::SCANUPDATED, intern<HybridPeripheralHandle>(peripheral));
        });
    });
}

}  // namespace margelo::nitro::simplernble
