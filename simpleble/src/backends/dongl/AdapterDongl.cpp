#include <simpleble/Config.h>
#include <simpleble/Peripheral.h>

#include "AdapterDongl.h"
#include "BuilderBase.h"
#include "CommonUtils.h"
#include "PeripheralDongl.h"
#include "protocol/simpleble.pb.h"
#include "serial/Protocol.h"
#include "Firmware.h"

#include <memory>
#include <thread>

using namespace SimpleBLE;

// Forward declarations for decoded data structures
struct DecodedManufacturerData {
    std::map<uint16_t, ByteArray> data;
};

struct DecodedServiceData {
    std::map<BluetoothUUID, ByteArray> data;
};

bool AdapterDongl::bluetooth_enabled() { return true; }

AdapterDongl::AdapterDongl(const std::string& device_path)
    : _serial_protocol(std::make_shared<Dongl::Serial::Protocol>(device_path)) {
    fmt::print("Dongl adapter created with device path: {}\n", device_path);

    _serial_protocol->set_event_callback([this](const dongl_Event& event) {
        switch (event.which_evt) {
            case dongl_Event_simpleble_tag:
                _on_simpleble_event(event.evt.simpleble);
                break;
            default:
                break;
        }
    });

    // _serial_protocol->set_response_callback([this](const dongl_Response& response) {
    //     fmt::print("Received response: {} bytes\n", response.size());

    //     // TODO: Process the received response
    //     // auto event = Dongl::CMD::UartEvent::from_bytes(response);
    //     // fmt::print("Received event: {}\n", event->to_string());
    // });

    // _serial_protocol->set_event_callback([this](const std::vector<uint8_t>& event) {
    //     fmt::print("Received event: {} bytes\n", event.size());

    //     // TODO: Process the received event
    // });

    // _serial_protocol->set_error_callback([this](const std::vector<uint8_t>& error) {
    //     fmt::print("Protocol error: {} bytes\n", error.size());

    //     // TODO: Handle protocol errors
    // });


    auto response_whoami = _serial_protocol->basic_whoami();
    fmt::print("Whoami: version {}\n", response_whoami.version);

    auto response_init = _serial_protocol->simpleble_init();
    fmt::print("SimpleBLE init: {}\n", response_init.ret_code);

    if (Config::Dongl::auto_update) {
        if (SimpleBLE::Dongl::Firmware::FIRMWARE_VERSION > response_whoami.version) {
            try {
                fmt::print("New firmware available (v{}). Updating...\n", SimpleBLE::Dongl::Firmware::FIRMWARE_VERSION);
                _update_firmware();
            } catch (const Exception::BaseException& e) {
                fmt::print("Auto-update failed: {}\n", e.what());
            }
        } else {
            fmt::print("Firmware is up to date (v{}).\n", response_whoami.version);
        }
    }

    std::this_thread::sleep_for(std::chrono::milliseconds(100));
}

AdapterDongl::~AdapterDongl() {}

void* AdapterDongl::underlying() const { return nullptr; }

std::string AdapterDongl::identifier() { return "Dongl Adapter"; }

BluetoothAddress AdapterDongl::address() { return "AA:BB:CC:DD:EE:FF"; }

void AdapterDongl::power_on() { _serial_protocol->basic_power_on(); }

void AdapterDongl::power_off() { _serial_protocol->basic_power_off(); }

bool AdapterDongl::is_powered() { return true; }

void AdapterDongl::scan_start() {
    auto response = _serial_protocol->simpleble_scan_start();

    fmt::print("Scan start: {}\n", response.ret_code);
}

void AdapterDongl::scan_stop() {
    auto response = _serial_protocol->simpleble_scan_stop();
    fmt::print("Scan stop: {}\n", response.ret_code);
}

void AdapterDongl::scan_for(int timeout_ms) {
    scan_start();
    std::this_thread::sleep_for(std::chrono::milliseconds(timeout_ms));
    scan_stop();
}

bool AdapterDongl::scan_is_active() { return false; }

SharedPtrVector<PeripheralBase> AdapterDongl::scan_get_results() { return Util::values(seen_peripherals_); }

SharedPtrVector<PeripheralBase> AdapterDongl::get_paired_peripherals() { return {}; }

void AdapterDongl::_scan_received_callback(advertising_data_t data) {
    if (this->peripherals_.count(data.mac_address) == 0) {
        // If the incoming peripheral has never been seen before, create and save a reference to it.
        auto base_peripheral = std::make_shared<PeripheralDongl>(_serial_protocol, data);
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
        SAFE_CALLBACK_CALL(this->_callback_on_scan_found, peripheral);
    } else {
        SAFE_CALLBACK_CALL(this->_callback_on_scan_updated, peripheral);
    }
}

void AdapterDongl::_update_firmware() {
    const uint8_t* firmware_data = SimpleBLE::Dongl::Firmware::OBFUSCATED_FIRMWARE;
    size_t total_length = SimpleBLE::Dongl::Firmware::OBFUSCATED_FIRMWARE_LEN;
    uint32_t version = SimpleBLE::Dongl::Firmware::FIRMWARE_VERSION;
    
    if (total_length < 16) {
        throw Exception::BaseException("Firmware too small");
    }
    
    fmt::print("Starting DFU to version {} (0x{:08x}), total length: {} bytes\n", version, version, total_length);

    // 1. DFU Start
    auto start_rsp = _serial_protocol->basic_dfu_start(version, total_length);
    if (start_rsp.error != basic_DfuError_DFU_ERROR_NONE) {
        throw Exception::BaseException("DFU Start failed with error: " + std::to_string(start_rsp.error));
    }

    // 2. Sequential Chunks (up to 512 bytes each)
    uint32_t offset = 0;
    uint32_t chunk_index = 0;
    while (offset < total_length) {
        uint32_t chunk_size = std::min((uint32_t)512, (uint32_t)(total_length - offset));
        std::vector<uint8_t> chunk_data(firmware_data + offset, firmware_data + offset + chunk_size);

        uint32_t page_index = offset / 4096;
        uint32_t offset_in_page = offset % 4096;

        auto chunk_rsp = _serial_protocol->basic_dfu_chunk(page_index, offset_in_page, chunk_data);
        if (chunk_rsp.error != basic_DfuError_DFU_ERROR_NONE) {
            throw Exception::BaseException("DFU Chunk " + std::to_string(chunk_index) + " failed with error: " + std::to_string(chunk_rsp.error));
        }

        offset += chunk_size;
        chunk_index++;

        if (chunk_index % 10 == 0 || offset == total_length) {
            fmt::print("Progress: {}/{} bytes sent\r", offset, total_length);
            std::fflush(stdout);
        }
    }
    fmt::print("\nDFU chunks sent successfully. Verifying...\n");

    // 3. DFU Verify
    auto verify_rsp = _serial_protocol->basic_dfu_verify();
    if (verify_rsp.error != basic_DfuError_DFU_ERROR_NONE) {
        throw Exception::BaseException("DFU Verification failed with error: " + std::to_string(verify_rsp.error));
    }
    fmt::print("DFU verified successfully! Accepted version: {}. Resetting dongle...\n", verify_rsp.accepted_version);

    // 4. DFU Reboot
    _serial_protocol->basic_dfu_reboot();
    fmt::print("Dongle rebooting. DFU complete.\n");
}

void AdapterDongl::_on_simpleble_event(const simpleble_Event& event) {
    switch (event.which_evt) {
        case simpleble_Event_adv_evt_tag: {
            advertising_data_t data = advertising_data_t();
            data.mac_address = std::string(event.evt.adv_evt.address);
            data.address_type = static_cast<SimpleBLE::BluetoothAddressType>(event.evt.adv_evt.address_type);
            data.identifier = std::string(event.evt.adv_evt.identifier);
            data.connectable = event.evt.adv_evt.connectable;
            data.rssi = event.evt.adv_evt.rssi;
            data.tx_power = event.evt.adv_evt.tx_power;

            // Extract decoded manufacturer and service data
            for (int i = 0; i < event.evt.adv_evt.manufacturer_data_count; i++) {
                ByteArray manufacturer_data(event.evt.adv_evt.manufacturer_data[i].data.bytes,
                                            event.evt.adv_evt.manufacturer_data[i].data.size);
                data.manufacturer_data[event.evt.adv_evt.manufacturer_data[i].company_id] = manufacturer_data;
            }

            // TODO: Implement service data extraction
            // for (int i = 0; i < event.evt.adv_evt.service_data_count; i++) {
            //     ByteArray service_data(event.evt.adv_evt.service_data[i].data.bytes,
            //     event.evt.adv_evt.service_data[i].data.size);
            //     data.service_data[BluetoothUUID(event.evt.adv_evt.service_data[i].uuid)] = service_data;
            // }

            _scan_received_callback(data);
            break;
        }

        case simpleble_Event_connection_evt_tag: {
            for (auto& [address, peripheral] : this->peripherals_) {
                if (peripheral->address() == std::string(event.evt.connection_evt.address)) {
                    peripheral->notify_connected(event.evt.connection_evt.conn_handle);
                    break;
                }
            }
            break;
        }

        case simpleble_Event_disconnection_evt_tag: {
            for (auto& [address, peripheral] : this->peripherals_) {
                if (peripheral->conn_handle() == event.evt.disconnection_evt.conn_handle) {
                    peripheral->notify_disconnected();
                    break;
                }
            }
            break;
        }

        case simpleble_Event_service_discovered_evt_tag: {
            for (auto& [address, peripheral] : this->peripherals_) {
                if (peripheral->conn_handle() == event.evt.service_discovered_evt.conn_handle) {
                    peripheral->notify_service_discovered(event.evt.service_discovered_evt);
                    break;
                }
            }
            break;
        }

        case simpleble_Event_characteristic_discovered_evt_tag: {
            for (auto& [address, peripheral] : this->peripherals_) {
                if (peripheral->conn_handle() == event.evt.characteristic_discovered_evt.conn_handle) {
                    peripheral->notify_characteristic_discovered(event.evt.characteristic_discovered_evt);
                    break;
                }
            }
            break;
        }

        case simpleble_Event_descriptor_discovered_evt_tag: {
            for (auto& [address, peripheral] : this->peripherals_) {
                if (peripheral->conn_handle() == event.evt.descriptor_discovered_evt.conn_handle) {
                    peripheral->notify_descriptor_discovered(event.evt.descriptor_discovered_evt);
                    break;
                }
            }
            break;
        }

        case simpleble_Event_attribute_discovery_complete_evt_tag: {
            for (auto& [address, peripheral] : this->peripherals_) {
                if (peripheral->conn_handle() == event.evt.attribute_discovery_complete_evt.conn_handle) {
                    peripheral->notify_attribute_discovery_complete();
                    break;
                }
            }
            break;
        }

        case simpleble_Event_value_changed_evt_tag: {
            for (auto& [address, peripheral] : this->peripherals_) {
                if (peripheral->conn_handle() == event.evt.value_changed_evt.conn_handle) {
                    peripheral->notify_value_changed(event.evt.value_changed_evt);
                    break;
                }
            }
            break;
        }
    }
}
