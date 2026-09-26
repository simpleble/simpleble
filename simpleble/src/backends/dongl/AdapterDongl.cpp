#include <simpleble/Config.h>
#include <simpleble/Peripheral.h>

#include "AdapterDongl.h"
#include "BuilderBase.h"
#include "CommonUtils.h"
#include "LoggingInternal.h"
#include "PeripheralDongl.h"
#include "Utils.h"
#include "protocol/simpleble.pb.h"
#include "serial/Protocol.h"

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
    _serial_protocol->set_event_callback([this](const dongl_Event& event) {
        switch (event.which_evt) {
            case dongl_Event_simpleble_tag:
                _on_simpleble_event(event.evt.simpleble);
                break;
            default:
                break;
        }
    });

    auto response_whoami = _serial_protocol->basic_whoami();
    _identifier = std::string(response_whoami.identifier);
    _address = std::string(response_whoami.mac_address);
    SIMPLEBLE_LOG_DEBUG(fmt::format("Dongl adapter initialized: {} (firmware {})", _identifier,
                                    response_whoami.version));

    auto response_init = _serial_protocol->simpleble_init();
    if (response_init.ret_code != 0) {
        SIMPLEBLE_LOG_ERROR(fmt::format("Failed to initialize Dongl adapter: {}", response_init.ret_code));
    }

    std::this_thread::sleep_for(std::chrono::milliseconds(100));
}

AdapterDongl::~AdapterDongl() {
    // Detach the serial callback before destroying the peripheral maps it uses.
    _serial_protocol->set_event_callback({});
    seen_peripherals_.clear();
    peripherals_.clear();
    _serial_protocol.reset();
}

void* AdapterDongl::underlying() const { return nullptr; }

std::string AdapterDongl::identifier() { return _identifier; }

BluetoothAddress AdapterDongl::address() { return _address; }

void AdapterDongl::power_on() { _serial_protocol->basic_power_on(); }

void AdapterDongl::power_off() { _serial_protocol->basic_power_off(); }

bool AdapterDongl::is_powered() { return _serial_protocol->basic_is_powered().is_powered; }

void AdapterDongl::scan_start() {
    seen_peripherals_.clear();
    auto response = _serial_protocol->simpleble_scan_start();
    if (response.ret_code != 0) {
        SIMPLEBLE_LOG_ERROR(fmt::format("Failed to start Dongl scan: {}", response.ret_code));
    }
    SAFE_CALLBACK_CALL(this->_callback_on_scan_start);
}

void AdapterDongl::scan_stop() {
    auto response = _serial_protocol->simpleble_scan_stop();
    if (response.ret_code != 0) {
        SIMPLEBLE_LOG_ERROR(fmt::format("Failed to stop Dongl scan: {}", response.ret_code));
    }
    SAFE_CALLBACK_CALL(this->_callback_on_scan_stop);
}

void AdapterDongl::scan_for(int timeout_ms) {
    scan_start();
    std::this_thread::sleep_for(std::chrono::milliseconds(timeout_ms));
    scan_stop();
}

bool AdapterDongl::scan_is_active() { return _serial_protocol->simpleble_scan_is_active().is_active; }

SharedPtrVector<PeripheralBase> AdapterDongl::scan_get_results() { return Util::values(seen_peripherals_); }

SharedPtrVector<PeripheralBase> AdapterDongl::get_paired_peripherals() {
    SharedPtrVector<PeripheralBase> paired_peripherals;
    constexpr uint16_t kMaxPeerIds = 256;
    const auto count_response = _serial_protocol->simpleble_get_paired_peripheral_count();
    const uint16_t paired_count = count_response.count > kMaxPeerIds ? kMaxPeerIds : count_response.count;
    paired_peripherals.reserve(paired_count);

    for (uint16_t index = 0; index < paired_count; index++) {
        auto response = _serial_protocol->simpleble_get_paired_peripheral(index);
        if (!response.found) {
            break;
        }

        BluetoothAddress address = std::string(response.address);
        auto peripheral = peripherals_.find(address);
        if (peripheral == peripherals_.end()) {
            Dongl::advertising_data_t data{};
            data.identifier = address;
            data.address_type = static_cast<BluetoothAddressType>(response.address_type);
            data.mac_address = address;
            data.connectable = true;
            peripheral = peripherals_.emplace(address, std::make_shared<PeripheralDongl>(_serial_protocol, data)).first;
        }
        paired_peripherals.push_back(peripheral->second);
    }

    return paired_peripherals;
}

void AdapterDongl::_scan_received_callback(Dongl::advertising_data_t data) {
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

void AdapterDongl::_on_simpleble_event(const simpleble_Event& event) {
    switch (event.which_evt) {
        case simpleble_Event_adv_evt_tag: {
            Dongl::advertising_data_t data = Dongl::advertising_data_t();
            data.mac_address = std::string(event.evt.adv_evt.address);
            data.address_type = static_cast<SimpleBLE::BluetoothAddressType>(event.evt.adv_evt.address_type);
            data.identifier = std::string(event.evt.adv_evt.identifier);
            data.identifier_complete = event.evt.adv_evt.identifier_complete;
            data.connectable = event.evt.adv_evt.connectable;
            data.scan_response = event.evt.adv_evt.scan_response;
            data.scannable = event.evt.adv_evt.scannable;
            data.rssi = event.evt.adv_evt.rssi;
            data.tx_power = event.evt.adv_evt.tx_power;
            // Extract decoded manufacturer and service data
            for (int i = 0; i < event.evt.adv_evt.manufacturer_data_count; i++) {
                ByteArray manufacturer_data(event.evt.adv_evt.manufacturer_data[i].data.bytes,
                                            event.evt.adv_evt.manufacturer_data[i].data.size);
                data.manufacturer_data[event.evt.adv_evt.manufacturer_data[i].company_id] = manufacturer_data;
            }

            for (int i = 0; i < event.evt.adv_evt.service_data_count; i++) {
                if (!event.evt.adv_evt.service_data[i].has_uuid) {
                    continue;
                }
                ByteArray service_data(event.evt.adv_evt.service_data[i].data.bytes,
                                       event.evt.adv_evt.service_data[i].data.size);
                data.service_data[Dongl::uuid_from_proto(event.evt.adv_evt.service_data[i].uuid)] = service_data;
            }

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

        case simpleble_Event_connect_complete_evt_tag: {
            // Routed by address: failed attempts have no connection handle.
            for (auto& [address, peripheral] : this->peripherals_) {
                if (peripheral->address() == std::string(event.evt.connect_complete_evt.address)) {
                    peripheral->notify_connect_complete(event.evt.connect_complete_evt);
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

        case simpleble_Event_passkey_display_evt_tag: {
            for (auto& [address, peripheral] : this->peripherals_) {
                if (peripheral->conn_handle() == event.evt.passkey_display_evt.conn_handle) {
                    peripheral->notify_passkey_display(event.evt.passkey_display_evt);
                    break;
                }
            }
            break;
        }

        case simpleble_Event_auth_key_request_evt_tag: {
            for (auto& [address, peripheral] : this->peripherals_) {
                if (peripheral->conn_handle() == event.evt.auth_key_request_evt.conn_handle) {
                    peripheral->notify_auth_key_request(event.evt.auth_key_request_evt);
                    break;
                }
            }
            break;
        }
    }
}
