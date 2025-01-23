#include "AdapterAndroid.h"
#include "BackendAndroid.h"
#include "BuilderBase.h"
#include "CommonUtils.h"
#include "PeripheralAndroid.h"
#include "simpleble/Peripheral.h"

#include <android/BluetoothDevice.h>
#include <android/ScanResult.h>
#include <android/log.h>
#include <fmt/core.h>
#include <jni.h>
#include <thread>

using namespace SimpleBLE;

bool AdapterAndroid::bluetooth_enabled() { return backend_->bluetooth_enabled(); }

AdapterAndroid::AdapterAndroid(std::shared_ptr<BackendAndroid> backend) : backend_(backend) {
    _btScanCallback.set_callback_onScanResult([this](Android::ScanResult scan_result) {
        std::string address = scan_result.getDevice().getAddress();

        if (this->peripherals_.count(address) == 0) {
            // If the incoming peripheral has never been seen before, create and save a reference to it.
            auto base_peripheral = std::make_shared<PeripheralAndroid>(scan_result);
            this->peripherals_.insert(std::make_pair(address, base_peripheral));
        }

        // Update the received advertising data.
        auto base_peripheral = this->peripherals_.at(address);
        base_peripheral->update_advertising_data(scan_result);

        // Convert the base object into an external-facing Peripheral object
        Peripheral peripheral = Factory::build(base_peripheral);

        // Check if the device has been seen before, to forward the correct call to the user.
        if (this->seen_peripherals_.count(address) == 0) {
            // Store it in our table of seen peripherals
            this->seen_peripherals_.insert(std::make_pair(address, base_peripheral));
            SAFE_CALLBACK_CALL(this->callback_on_scan_found_, peripheral);
        } else {
            SAFE_CALLBACK_CALL(this->callback_on_scan_updated_, peripheral);
        }
    });
}

AdapterAndroid::~AdapterAndroid() {}

void* AdapterAndroid::underlying() const { return nullptr; }

std::string AdapterAndroid::identifier() {
    return _btAdapter.getName();
}

BluetoothAddress AdapterAndroid::address() {
    return BluetoothAddress(_btAdapter.getAddress());
}

void AdapterAndroid::scan_start() {
    seen_peripherals_.clear();
    _btScanner.startScan(_btScanCallback);
    scanning_ = true;
    SAFE_CALLBACK_CALL(this->callback_on_scan_start_);
}

void AdapterAndroid::scan_stop() {
    _btScanner.stopScan(_btScanCallback);
    scanning_ = false;
    SAFE_CALLBACK_CALL(this->callback_on_scan_stop_);
}

void AdapterAndroid::scan_for(int timeout_ms) {
    scan_start();
    std::this_thread::sleep_for(std::chrono::milliseconds(timeout_ms));
    scan_stop();
}

bool AdapterAndroid::scan_is_active() { return scanning_; }

SharedPtrVector<PeripheralBase> AdapterAndroid::scan_get_results() { return Util::values(seen_peripherals_); }

SharedPtrVector<PeripheralBase> AdapterAndroid::get_paired_peripherals() {
    // TODO: In order to implement this, we should implement a dedicated class for btAdapter, instead
    // of making direct calls to the Java objects.
    return {};
}

void AdapterAndroid::set_callback_on_scan_start(std::function<void()> on_scan_start) {
    if (on_scan_start) {
        callback_on_scan_start_.load(on_scan_start);
    } else {
        callback_on_scan_start_.unload();
    }
}

void AdapterAndroid::set_callback_on_scan_stop(std::function<void()> on_scan_stop) {
    if (on_scan_stop) {
        callback_on_scan_stop_.load(on_scan_stop);
    } else {
        callback_on_scan_stop_.unload();
    }
}

void AdapterAndroid::set_callback_on_scan_updated(std::function<void(Peripheral)> on_scan_updated) {
    if (on_scan_updated) {
        callback_on_scan_updated_.load(on_scan_updated);
    } else {
        callback_on_scan_updated_.unload();
    }
}

void AdapterAndroid::set_callback_on_scan_found(std::function<void(Peripheral)> on_scan_found) {
    if (on_scan_found) {
        callback_on_scan_found_.load(on_scan_found);
    } else {
        callback_on_scan_found_.unload();
    }
}
