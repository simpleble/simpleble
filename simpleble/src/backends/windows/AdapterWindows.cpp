// This weird pragma is required for the compiler to properly include the necessary namespaces.
#pragma comment(lib, "windowsapp")
#include "AdapterWindows.h"
#include "BackendWinRT.h"

#include "BuilderBase.h"
#include "CommonUtils.h"
#include "LoggingInternal.h"
#include "PeripheralWindows.h"
#include "Utils.h"
#include "MtaManager.h"

#include "winrt/Windows.Devices.Bluetooth.h"
#include "winrt/Windows.Devices.Enumeration.h"
#include "winrt/Windows.Foundation.Collections.h"
#include "winrt/Windows.Foundation.h"
#include "winrt/base.h"

#include <algorithm>
#include <iostream>
#include <iterator>
#include <memory>
#include <sstream>
#include <string>
#include <utility>
#include <vector>

using namespace SimpleBLE;
using namespace SimpleBLE::WinRT;
using namespace std::chrono_literals;

AdapterWindows::AdapterWindows(std::string device_id)
    : adapter_(async_get(BluetoothAdapter::FromIdAsync(winrt::to_hstring(device_id)))) {
    // IMPORTANT NOTE: This function must be executed in the MTA context. In this case, this is managed by the BackendWinRT class.
    auto device_information = async_get(
        Devices::Enumeration::DeviceInformation::CreateFromIdAsync(winrt::to_hstring(device_id)));
    identifier_ = winrt::to_string(device_information.Name());

    // Configure the scanner object
    scanner_ = Advertisement::BluetoothLEAdvertisementWatcher();

    // Register member functions directly as callback handlers
    scanner_stopped_token_ = scanner_.Stopped({this, &AdapterWindows::_on_scanner_stopped});
    scanner_received_token_ = scanner_.Received({this, &AdapterWindows::_on_scanner_received});
}

AdapterWindows::~AdapterWindows() {
    callback_on_scan_stop_.unload();

    MtaManager::get().execute_sync([this]() {
        scanner_.Stop();

        if (scanner_received_token_) {
            scanner_.Received(scanner_received_token_);
        }

        if (scanner_stopped_token_) {
            scanner_.Stopped(scanner_stopped_token_);
        }
    });
}

// FIXME: this should return wether this particular adapter is enabled, not if any adapter is enabled.
bool AdapterWindows::bluetooth_enabled() { return BackendWinRT::get()->bluetooth_enabled(); }

void* AdapterWindows::underlying() const { return reinterpret_cast<void*>(const_cast<BluetoothAdapter*>(&adapter_)); }

std::string AdapterWindows::identifier() { return identifier_; }

BluetoothAddress AdapterWindows::address() {
    return MtaManager::get().execute_sync<BluetoothAddress>([this]() {
        return _mac_address_to_str(adapter_.BluetoothAddress());
    });
}

void AdapterWindows::scan_start() {
    this->seen_peripherals_.clear();

    MtaManager::get().execute_sync([this]() {
        scanner_.ScanningMode(Advertisement::BluetoothLEScanningMode::Active);
        scan_is_active_ = true;
        scanner_.Start();
    });

    SAFE_CALLBACK_CALL(this->callback_on_scan_start_);
}

void AdapterWindows::scan_stop() {
    MtaManager::get().execute_sync([this]() {
        scanner_.Stop();
    });

    std::unique_lock<std::mutex> lock(scan_stop_mutex_);
    if (scan_stop_cv_.wait_for(lock, 1s, [=] { return !this->scan_is_active_; })) {
        // Scan stopped
    } else {
        // Scan did not stop, this can be because some other process
        // is using the adapter.
    }
}

void AdapterWindows::scan_for(int timeout_ms) {
    scan_start();
    std::this_thread::sleep_for(std::chrono::milliseconds(timeout_ms));
    scan_stop();
}

bool AdapterWindows::scan_is_active() { return scan_is_active_; }

SharedPtrVector<PeripheralBase> AdapterWindows::scan_get_results() { return Util::values(seen_peripherals_); }

SharedPtrVector<PeripheralBase> AdapterWindows::get_paired_peripherals() { return {}; }

void AdapterWindows::set_callback_on_scan_start(std::function<void()> on_scan_start) {
    if (on_scan_start) {
        callback_on_scan_start_.load(on_scan_start);
    } else {
        callback_on_scan_start_.unload();
    }
}

void AdapterWindows::set_callback_on_scan_stop(std::function<void()> on_scan_stop) {
    if (on_scan_stop) {
        callback_on_scan_stop_.load(on_scan_stop);
    } else {
        callback_on_scan_stop_.unload();
    }
}

void AdapterWindows::set_callback_on_scan_updated(std::function<void(Peripheral)> on_scan_updated) {
    if (on_scan_updated) {
        callback_on_scan_updated_.load(on_scan_updated);
    } else {
        callback_on_scan_updated_.unload();
    }
}

void AdapterWindows::set_callback_on_scan_found(std::function<void(Peripheral)> on_scan_found) {
    if (on_scan_found) {
        callback_on_scan_found_.load(on_scan_found);
    } else {
        callback_on_scan_found_.unload();
    }
}

// Private functions

void AdapterWindows::_scan_stopped_callback() {
    std::lock_guard<std::mutex> lock(scan_update_mutex_);
    scan_is_active_ = false;
    scan_stop_cv_.notify_all();

    SAFE_CALLBACK_CALL(this->callback_on_scan_stop_);
}

void AdapterWindows::_scan_received_callback(advertising_data_t data) {
    if (this->peripherals_.count(data.mac_address) == 0) {
        // If the incoming peripheral has never been seen before, create and save a reference to it.
        auto base_peripheral = std::make_shared<PeripheralWindows>(data);
        this->peripherals_.insert(std::make_pair(data.mac_address, base_peripheral));
    }

    // Update the received advertising data.
    auto base_peripheral = this->peripherals_.at(data.mac_address);
    base_peripheral->update_advertising_data(data);

    // Convert the base object into an external-facing Peripheral object
    Peripheral peripheral = Factory::build(base_peripheral);

    // Check if the device has been seen before, to forward the correct call to the user.
    if (this->seen_peripherals_.count(data.mac_address) == 0) {
        // Store it in our table of seen peripherals
        this->seen_peripherals_.insert(std::make_pair(data.mac_address, base_peripheral));
        SAFE_CALLBACK_CALL(this->callback_on_scan_found_, peripheral);
    } else {
        SAFE_CALLBACK_CALL(this->callback_on_scan_updated_, peripheral);
    }
}

void AdapterWindows::_on_scanner_stopped(
    const Advertisement::BluetoothLEAdvertisementWatcher& watcher,
    const Advertisement::BluetoothLEAdvertisementWatcherStoppedEventArgs args) {
    // This callback is already in the MTA context as it's called by WinRT
    this->_scan_stopped_callback();
}

void AdapterWindows::_on_scanner_received(
    const Advertisement::BluetoothLEAdvertisementWatcher& watcher,
    const Advertisement::BluetoothLEAdvertisementReceivedEventArgs args) {
    // This callback is already in the MTA context as it's called by WinRT
    std::lock_guard<std::mutex> lock(this->scan_update_mutex_);
    if (!this->scan_is_active_) return;

    advertising_data_t data;
    data.mac_address = _mac_address_to_str(args.BluetoothAddress());
    Bluetooth::BluetoothAddressType addr_type_enum = args.BluetoothAddressType();
    switch (addr_type_enum) {
        case Bluetooth::BluetoothAddressType::Public:
            data.address_type = SimpleBLE::BluetoothAddressType::PUBLIC;
            break;

        case Bluetooth::BluetoothAddressType::Random:
            data.address_type = SimpleBLE::BluetoothAddressType::RANDOM;
            break;

        case Bluetooth::BluetoothAddressType::Unspecified:
            data.address_type = SimpleBLE::BluetoothAddressType::UNSPECIFIED;
            break;
    }

    data.identifier = winrt::to_string(args.Advertisement().LocalName());
    data.connectable = args.IsConnectable();
    data.rssi = args.RawSignalStrengthInDBm();

    if (args.TransmitPowerLevelInDBm()) {
        data.tx_power = args.TransmitPowerLevelInDBm().Value();
    } else {
        data.tx_power = INT16_MIN;
    }

    // Parse manufacturer data
    auto manufacturer_data = args.Advertisement().ManufacturerData();
    for (auto& item : manufacturer_data) {
        uint16_t company_id = item.CompanyId();
        ByteArray manufacturer_data_buffer = ibuffer_to_bytearray(item.Data());
        data.manufacturer_data[company_id] = manufacturer_data_buffer;
    }

    // Parse service data.
    const auto& sections = args.Advertisement().DataSections();
    for (const auto& section : sections) {
        ByteArray section_data_buffer = ibuffer_to_bytearray(section.Data());

        std::string service_uuid;
        ByteArray service_data;

        if (section.DataType() == Advertisement::BluetoothLEAdvertisementDataTypes::ServiceData128BitUuids()) {
            service_uuid = fmt::format(
                "{:02x}{:02x}{:02x}{:02x}-{:02x}{:02x}-{:02x}{:02x}-{:02x}{:02x}-"
                "{:02x}{:02x}{:02x}{:02x}{:02x}{:02x}",
                (uint8_t)section_data_buffer[15], (uint8_t)section_data_buffer[14],
                (uint8_t)section_data_buffer[13], (uint8_t)section_data_buffer[12],
                (uint8_t)section_data_buffer[11], (uint8_t)section_data_buffer[10],
                (uint8_t)section_data_buffer[9], (uint8_t)section_data_buffer[8],
                (uint8_t)section_data_buffer[7], (uint8_t)section_data_buffer[6],
                (uint8_t)section_data_buffer[5], (uint8_t)section_data_buffer[4],
                (uint8_t)section_data_buffer[3], (uint8_t)section_data_buffer[2],
                (uint8_t)section_data_buffer[1], (uint8_t)section_data_buffer[0]);
            service_data = section_data_buffer.slice_from(16);
        }

        else if (section.DataType() ==
                    Advertisement::BluetoothLEAdvertisementDataTypes::ServiceData32BitUuids()) {
            service_uuid = fmt::format("{:02x}{:02x}{:02x}{:02x}-0000-1000-8000-00805f9b34fb",
                                        (uint8_t)section_data_buffer[3], (uint8_t)section_data_buffer[2],
                                        (uint8_t)section_data_buffer[1], (uint8_t)section_data_buffer[0]);
            service_data = section_data_buffer.slice_from(4);
        } else if (section.DataType() ==
                    Advertisement::BluetoothLEAdvertisementDataTypes::ServiceData16BitUuids()) {
            service_uuid = fmt::format("0000{:02x}{:02x}-0000-1000-8000-00805f9b34fb",
                                        (uint8_t)section_data_buffer[1], (uint8_t)section_data_buffer[0]);
            service_data = section_data_buffer.slice_from(2);
        } else {
            continue;
        }

        data.service_data.emplace(std::make_pair(service_uuid, service_data));
    }

    // Parse service uuids
    auto service_data = args.Advertisement().ServiceUuids();
    for (auto& service_guid : service_data) {
        std::string service_uuid = guid_to_uuid(service_guid);
        data.service_data.emplace(std::make_pair(service_uuid, ByteArray()));
    }

    this->_scan_received_callback(data);
}
