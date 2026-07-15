#include <simpleble/Peripheral.h>

#import "AdapterBaseMacOS.h"
#import "AdapterMac.h"
#import "BuilderBase.h"
#import "CommonUtils.h"
#import "PeripheralMac.h"

#include <fmt/core.h>

#include <chrono>
#include <memory>
#include <thread>

using namespace SimpleBLE;

AdapterMac::AdapterMac() {
    // Cast the Objective-C++ object using __bridge_retained, which will signal ARC to increase
    // the reference count. This means that AdapterMac will be responsible for releasing the
    // Objective-C++ object in the destructor.
    opaque_internal_ = (__bridge_retained void*)[[AdapterBaseMacOS alloc] init:this];
}

AdapterMac::~AdapterMac() {
    // Cast the opaque pointer back to the Objective-C++ object and release it.
    // This will signal ARC to decrease the reference count.
    // NOTE: This is equivalent to calling [opaque_internal_ release] in Objective-C++.
    AdapterBaseMacOS* internal = (__bridge_transfer AdapterBaseMacOS*)opaque_internal_;
    internal = nil;
}

bool AdapterMac::bluetooth_enabled() {
    AdapterBaseMacOS* internal = (__bridge AdapterBaseMacOS*)opaque_internal_;
    return [internal isBluetoothEnabled];
}

void* AdapterMac::underlying() const {
    AdapterBaseMacOS* internal = (__bridge AdapterBaseMacOS*)opaque_internal_;

    return [internal underlying];
}

std::string AdapterMac::identifier() { return fmt::format("Default Adapter [{}]", this->address()); }

BluetoothAddress AdapterMac::address() const {
    AdapterBaseMacOS* internal = (__bridge AdapterBaseMacOS*)opaque_internal_;

    return [[internal address] UTF8String];
}

void AdapterMac::power_on() {
    AdapterBaseMacOS* internal = (__bridge AdapterBaseMacOS*)opaque_internal_;
    [internal powerOn];
}

void AdapterMac::power_off() {
    AdapterBaseMacOS* internal = (__bridge AdapterBaseMacOS*)opaque_internal_;
    [internal powerOff];
}

bool AdapterMac::is_powered() {
    AdapterBaseMacOS* internal = (__bridge AdapterBaseMacOS*)opaque_internal_;
    return [internal isPowered];
}

BluetoothAddress AdapterMac::address() { return const_cast<const AdapterMac*>(this)->address(); }

void AdapterMac::scan_start() {
    {
        std::scoped_lock lock(peripherals_mutex_);
        this->seen_peripherals_.clear();
    }

    AdapterBaseMacOS* internal = (__bridge AdapterBaseMacOS*)opaque_internal_;
    [internal scanStart];

    SAFE_CALLBACK_CALL(this->_callback_on_scan_start);
}

void AdapterMac::scan_stop() {
    AdapterBaseMacOS* internal = (__bridge AdapterBaseMacOS*)opaque_internal_;
    [internal scanStop];

    SAFE_CALLBACK_CALL(this->_callback_on_scan_stop);
}

void AdapterMac::scan_for(int timeout_ms) {
    this->scan_start();
    std::this_thread::sleep_for(std::chrono::milliseconds(timeout_ms));
    this->scan_stop();
}

bool AdapterMac::scan_is_active() {
    AdapterBaseMacOS* internal = (__bridge AdapterBaseMacOS*)opaque_internal_;
    return [internal scanIsActive];
}

SharedPtrVector<PeripheralBase> AdapterMac::scan_get_results() {
    std::scoped_lock lock(peripherals_mutex_);
    SharedPtrVector<PeripheralBase> peripherals = Util::values(seen_peripherals_);
    return peripherals;
}

SharedPtrVector<PeripheralBase> AdapterMac::get_paired_peripherals() { return {}; }

SharedPtrVector<PeripheralBase> AdapterMac::get_peripherals_by_identifiers(const std::vector<BluetoothAddress>& identifiers) {
    AdapterBaseMacOS* internal = (__bridge AdapterBaseMacOS*)opaque_internal_;
    void* opaque_adapter = [internal underlying];

    NSMutableArray<NSString*>* uuid_strings = [NSMutableArray arrayWithCapacity:identifiers.size()];
    for (const auto& identifier : identifiers) {
        [uuid_strings addObject:[NSString stringWithUTF8String:identifier.c_str()]];
    }

    NSArray<CBPeripheral*>* retrieved = [internal retrievePeripheralsWithIdentifiers:uuid_strings];

    SharedPtrVector<PeripheralBase> result;
    for (CBPeripheral* cb_peripheral in retrieved) {
        void* opaque_peripheral = (__bridge void*)cb_peripheral;

        // A retrieved peripheral carries no advertising data (it was not
        // scanned), so synthesise a connectable-by-default record. Register it
        // in the same tables as a scanned peripheral so connect/GATT delegate
        // callbacks route back to it.
        advertising_data_t advertising_data{};
        advertising_data.connectable = true;
        advertising_data.rssi = INT16_MIN;
        advertising_data.tx_power = INT16_MIN;

        std::shared_ptr<PeripheralMac> base_peripheral;
        {
            std::scoped_lock lock(peripherals_mutex_);
            if (this->peripherals_.count(opaque_peripheral) == 0) {
                base_peripheral = std::make_shared<PeripheralMac>(opaque_peripheral, opaque_adapter, advertising_data);
                this->peripherals_.insert(std::make_pair(opaque_peripheral, base_peripheral));
            } else {
                base_peripheral = this->peripherals_.at(opaque_peripheral);
            }
            this->seen_peripherals_[opaque_peripheral] = base_peripheral;
        }
        result.push_back(base_peripheral);
    }

    return result;
}

// Delegate methods passed for AdapterBaseMacOS

void AdapterMac::delegate_did_discover_peripheral(void* opaque_peripheral, void* opaque_adapter, advertising_data_t advertising_data) {
    std::shared_ptr<PeripheralMac> base_peripheral;
    bool is_new_peripheral = false;

    {
        std::scoped_lock lock(peripherals_mutex_);
        if (this->peripherals_.count(opaque_peripheral) == 0) {
            // If the incoming peripheral has never been seen before, create and save a reference to it.
            base_peripheral = std::make_shared<PeripheralMac>(opaque_peripheral, opaque_adapter, advertising_data);
            this->peripherals_.insert(std::make_pair(opaque_peripheral, base_peripheral));
        } else {
            base_peripheral = this->peripherals_.at(opaque_peripheral);
        }
    }

    // Update the received advertising data.
    base_peripheral->update_advertising_data(advertising_data);

    // Convert the base object into an external-facing Peripheral object
    Peripheral peripheral = Factory::build(base_peripheral);

    // Check if the device has been seen before, to forward the correct call to the user.
    {
        std::scoped_lock lock(peripherals_mutex_);
        is_new_peripheral = this->seen_peripherals_.count(opaque_peripheral) == 0;
        if (is_new_peripheral) {
            // Store it in our table of seen peripherals
            this->seen_peripherals_.insert(std::make_pair(opaque_peripheral, base_peripheral));
        }
    }

    if (is_new_peripheral) {
        SAFE_CALLBACK_CALL(this->_callback_on_scan_found, peripheral);
    } else {
        SAFE_CALLBACK_CALL(this->_callback_on_scan_updated, peripheral);
    }
}

void AdapterMac::delegate_did_connect_peripheral(void* opaque_peripheral) {
    std::shared_ptr<PeripheralMac> base_peripheral;
    {
        std::scoped_lock lock(peripherals_mutex_);
        if (this->peripherals_.count(opaque_peripheral) == 0) {
            throw Exception::InvalidReference();
        }
        // Load the existing PeripheralBase object
        base_peripheral = this->peripherals_.at(opaque_peripheral);
    }

    base_peripheral->delegate_did_connect();
}

void AdapterMac::delegate_did_fail_to_connect_peripheral(void* opaque_peripheral, void* opaque_error) {
    std::shared_ptr<PeripheralMac> base_peripheral;
    {
        std::scoped_lock lock(peripherals_mutex_);
        if (this->peripherals_.count(opaque_peripheral) == 0) {
            throw Exception::InvalidReference();
        }
        // Load the existing PeripheralBase object
        base_peripheral = this->peripherals_.at(opaque_peripheral);
    }

    base_peripheral->delegate_did_fail_to_connect(opaque_error);
}

void AdapterMac::delegate_did_disconnect_peripheral(void* opaque_peripheral, void* opaque_error) {
    std::shared_ptr<PeripheralMac> base_peripheral;
    {
        std::scoped_lock lock(peripherals_mutex_);
        if (this->peripherals_.count(opaque_peripheral) == 0) {
            throw Exception::InvalidReference();
        }
        // Load the existing PeripheralBase object
        base_peripheral = this->peripherals_.at(opaque_peripheral);
    }

    base_peripheral->delegate_did_disconnect(opaque_error);
}

void AdapterMac::delegate_did_power_on() { SAFE_CALLBACK_CALL(this->_callback_on_power_on); }

void AdapterMac::delegate_did_power_off() { SAFE_CALLBACK_CALL(this->_callback_on_power_off); }
