#include "AdapterSimulator.h"

#include <simpleble/Exceptions.h>
#include <simpleble/Peripheral.h>

#include <chrono>
#include <thread>

#include "BuilderBase.h"
#include "CommonUtils.h"
#include "PeripheralSimulator.h"

using namespace SimpleBLE;

AdapterSimulator::AdapterSimulator(std::weak_ptr<Simulation::Internal::EnvironmentCore> environment,
                                   std::string identifier, BluetoothAddress address)
    : environment_(std::move(environment)),
      identifier_(std::move(identifier)),
      address_(std::move(address)),
      executor_(std::make_shared<kvn::thread_runner>()) {}

AdapterSimulator::~AdapterSimulator() { executor_->stop(); }

AdapterSimulator& AdapterSimulator::from(const std::shared_ptr<AdapterBase>& adapter) {
    if (!adapter) throw Exception::NotInitialized();
    return static_cast<AdapterSimulator&>(*adapter);
}

void* AdapterSimulator::underlying() const { return nullptr; }

std::string AdapterSimulator::identifier() { return identifier_; }

BluetoothAddress AdapterSimulator::address() { return address_; }

void AdapterSimulator::power_on() {
    powered_ = true;
    executor_->enqueue([this]() { SAFE_CALLBACK_CALL(this->_callback_on_power_on); });
}

void AdapterSimulator::power_off() {
    powered_ = false;
    scanning_ = false;
    executor_->enqueue([this]() { SAFE_CALLBACK_CALL(this->_callback_on_power_off); });
}

bool AdapterSimulator::is_powered() { return powered_; }

void AdapterSimulator::scan_start() {
    {
        std::lock_guard<std::mutex> lock(mutex_);
        seen_.clear();
    }
    scanning_ = true;
    executor_->enqueue([this]() { SAFE_CALLBACK_CALL(this->_callback_on_scan_start); });
}

void AdapterSimulator::scan_stop() {
    scanning_ = false;
    executor_->enqueue([this]() { SAFE_CALLBACK_CALL(this->_callback_on_scan_stop); });
}

void AdapterSimulator::scan_for(int timeout_ms) {
    scan_start();
    std::this_thread::sleep_for(std::chrono::milliseconds(timeout_ms));
    scan_stop();
}

bool AdapterSimulator::scan_is_active() { return scanning_; }

SharedPtrVector<PeripheralBase> AdapterSimulator::scan_get_results() {
    std::lock_guard<std::mutex> lock(mutex_);
    SharedPtrVector<PeripheralBase> results;
    for (auto& address : seen_) {
        results.push_back(peripherals_.at(address));
    }
    return results;
}

SharedPtrVector<PeripheralBase> AdapterSimulator::get_paired_peripherals() { return {}; }

SharedPtrVector<PeripheralBase> AdapterSimulator::get_connected_peripherals() {
    std::lock_guard<std::mutex> lock(mutex_);
    SharedPtrVector<PeripheralBase> results;
    for (auto& [address, peripheral] : peripherals_) {
        if (!peripheral->is_connected()) continue;
        results.push_back(peripheral);
    }
    return results;
}

bool AdapterSimulator::bluetooth_enabled() { return powered_; }

Simulation::Internal::CentralPolicy AdapterSimulator::policy() const {
    std::lock_guard<std::mutex> lock(policy_mutex_);
    return policy_;
}

void AdapterSimulator::update_policy(const std::function<void(Simulation::Internal::CentralPolicy&)>& change) {
    std::lock_guard<std::mutex> lock(policy_mutex_);
    change(policy_);
}

std::shared_ptr<kvn::thread_runner> AdapterSimulator::executor() const { return executor_; }

void AdapterSimulator::receive_advertisement(const Simulation::Internal::Advertisement& advertisement,
                                             std::weak_ptr<Simulation::Internal::DeviceCore> device) {
    if (!scanning_) return;

    std::shared_ptr<PeripheralSimulator> base_peripheral;
    bool first_seen;
    {
        std::lock_guard<std::mutex> lock(mutex_);
        auto& slot = peripherals_[advertisement.address];
        if (!slot) {
            slot = std::make_shared<PeripheralSimulator>(environment_, weak_from_this(), device, advertisement);
        } else {
            slot->update_advertisement(advertisement);
        }
        base_peripheral = slot;
        first_seen = seen_.insert(advertisement.address).second;
    }

    Peripheral peripheral = Factory::build(base_peripheral);
    if (first_seen) {
        SAFE_CALLBACK_CALL(this->_callback_on_scan_found, peripheral);
    } else {
        SAFE_CALLBACK_CALL(this->_callback_on_scan_updated, peripheral);
    }
}

void AdapterSimulator::shutdown() {
    scanning_ = false;
    executor_->stop();
}
