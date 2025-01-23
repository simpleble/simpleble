#include <simpleble/Peripheral.h>

#include "AdapterPlain.h"
#include "BuilderBase.h"
#include "CommonUtils.h"
#include "PeripheralBase.h"
#include "PeripheralPlain.h"

#include <memory>
#include <thread>

using namespace SimpleBLE;

bool AdapterPlain::bluetooth_enabled() { return true; }

AdapterPlain::AdapterPlain() {}

AdapterPlain::~AdapterPlain() {}

void* AdapterPlain::underlying() const { return nullptr; }

std::string AdapterPlain::identifier() { return "Plain Adapter"; }

BluetoothAddress AdapterPlain::address() { return "AA:BB:CC:DD:EE:FF"; }

void AdapterPlain::scan_start() {
    is_scanning_ = true;
    SAFE_CALLBACK_CALL(this->callback_on_scan_start_);

    Peripheral peripheral = Factory::build(std::make_shared<PeripheralPlain>());
    SAFE_CALLBACK_CALL(this->callback_on_scan_found_, peripheral);
    SAFE_CALLBACK_CALL(this->callback_on_scan_updated_, peripheral);
}

void AdapterPlain::scan_stop() {
    is_scanning_ = false;
    SAFE_CALLBACK_CALL(this->callback_on_scan_stop_);
}

void AdapterPlain::scan_for(int timeout_ms) {
    scan_start();
    std::this_thread::sleep_for(std::chrono::milliseconds(timeout_ms));
    scan_stop();
}

bool AdapterPlain::scan_is_active() { return is_scanning_; }
SharedPtrVector<PeripheralBase> AdapterPlain::scan_get_results() {
    SharedPtrVector<PeripheralBase> peripherals;
    peripherals.push_back(std::make_shared<PeripheralPlain>());

    return peripherals;
}

SharedPtrVector<PeripheralBase> AdapterPlain::get_paired_peripherals() {
    SharedPtrVector<PeripheralBase> peripherals;
    peripherals.push_back(std::make_shared<PeripheralPlain>());

    return peripherals;
}

void AdapterPlain::set_callback_on_scan_start(std::function<void()> on_scan_start) {
    if (on_scan_start) {
        callback_on_scan_start_.load(std::move(on_scan_start));
    } else {
        callback_on_scan_start_.unload();
    }
}

void AdapterPlain::set_callback_on_scan_stop(std::function<void()> on_scan_stop) {
    if (on_scan_stop) {
        callback_on_scan_stop_.load(std::move(on_scan_stop));
    } else {
        callback_on_scan_stop_.unload();
    }
}

void AdapterPlain::set_callback_on_scan_updated(std::function<void(Peripheral)> on_scan_updated) {
    if (on_scan_updated) {
        callback_on_scan_updated_.load(std::move(on_scan_updated));
    } else {
        callback_on_scan_updated_.unload();
    }
}

void AdapterPlain::set_callback_on_scan_found(std::function<void(Peripheral)> on_scan_found) {
    if (on_scan_found) {
        callback_on_scan_found_.load(std::move(on_scan_found));
    } else {
        callback_on_scan_found_.unload();
    }
}
