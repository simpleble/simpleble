#include <simplecble/error.h>
#include <simplecble/peripheral.h>

#include <simpleble/Exceptions.h>
#include <simpleble/Peripheral.h>

#include <climits>
#include <cstdlib>
#include <cstring>
#include <map>

using SimpleBLE::Error;
using SimpleBLE::ErrorCode;

void simpleble_peripheral_release_handle(simpleble_peripheral_t handle) {
    if (handle == nullptr) {
        return;
    }

    SimpleBLE::Peripheral* peripheral = (SimpleBLE::Peripheral*)handle;
    delete peripheral;
}

void* simpleble_peripheral_underlying(simpleble_peripheral_t handle, simpleble_error_t** out_error) {
    simpleble_error_release(out_error);

    if (handle == nullptr) {
        *out_error = new Error(ErrorCode::INVALID_ARGUMENT, "handle is NULL");
        return nullptr;
    }

    SimpleBLE::Peripheral* peripheral = (SimpleBLE::Peripheral*)handle;
    try {
        return peripheral->underlying();
    } catch (const SimpleBLE::Exception::BaseException& e) {
        *out_error = e.make_error().release();
    } catch (const std::exception& e) {
        *out_error = new Error(ErrorCode::UNCLASSIFIED_EXCEPTION, e.what());
    } catch (...) {
        *out_error = new Error(ErrorCode::UNCLASSIFIED_EXCEPTION, "Unknown exception");
    }

    return nullptr;
}

char* simpleble_peripheral_identifier(simpleble_peripheral_t handle, simpleble_error_t** out_error) {
    simpleble_error_release(out_error);

    if (handle == nullptr) {
        *out_error = new Error(ErrorCode::INVALID_ARGUMENT, "handle is NULL");
        return nullptr;
    }

    SimpleBLE::Peripheral* peripheral = (SimpleBLE::Peripheral*)handle;
    try {
        std::string identifier = peripheral->identifier();
        char* c_identifier = static_cast<char*>(std::malloc(identifier.size() + 1));
        std::strcpy(c_identifier, identifier.c_str());
        return c_identifier;
    } catch (const SimpleBLE::Exception::BaseException& e) {
        *out_error = e.make_error().release();
    } catch (const std::exception& e) {
        *out_error = new Error(ErrorCode::UNCLASSIFIED_EXCEPTION, e.what());
    } catch (...) {
        *out_error = new Error(ErrorCode::UNCLASSIFIED_EXCEPTION, "Unknown exception");
    }

    return nullptr;
}

char* simpleble_peripheral_address(simpleble_peripheral_t handle, simpleble_error_t** out_error) {
    simpleble_error_release(out_error);

    if (handle == nullptr) {
        *out_error = new Error(ErrorCode::INVALID_ARGUMENT, "handle is NULL");
        return nullptr;
    }

    SimpleBLE::Peripheral* peripheral = (SimpleBLE::Peripheral*)handle;
    try {
        std::string address = peripheral->address();
        char* c_address = static_cast<char*>(std::malloc(address.size() + 1));
        std::strcpy(c_address, address.c_str());
        return c_address;
    } catch (const SimpleBLE::Exception::BaseException& e) {
        *out_error = e.make_error().release();
    } catch (const std::exception& e) {
        *out_error = new Error(ErrorCode::UNCLASSIFIED_EXCEPTION, e.what());
    } catch (...) {
        *out_error = new Error(ErrorCode::UNCLASSIFIED_EXCEPTION, "Unknown exception");
    }

    return nullptr;
}

simpleble_address_type_t simpleble_peripheral_address_type(simpleble_peripheral_t handle,
                                                           simpleble_error_t** out_error) {
    simpleble_error_release(out_error);

    if (handle == nullptr) {
        *out_error = new Error(ErrorCode::INVALID_ARGUMENT, "handle is NULL");
        return SIMPLEBLE_ADDRESS_TYPE_UNSPECIFIED;
    }

    SimpleBLE::Peripheral* peripheral = (SimpleBLE::Peripheral*)handle;
    try {
        SimpleBLE::BluetoothAddressType address_type = peripheral->address_type();
        return (simpleble_address_type_t)address_type;
    } catch (const SimpleBLE::Exception::BaseException& e) {
        *out_error = e.make_error().release();
    } catch (const std::exception& e) {
        *out_error = new Error(ErrorCode::UNCLASSIFIED_EXCEPTION, e.what());
    } catch (...) {
        *out_error = new Error(ErrorCode::UNCLASSIFIED_EXCEPTION, "Unknown exception");
    }

    return SIMPLEBLE_ADDRESS_TYPE_UNSPECIFIED;
}

int16_t simpleble_peripheral_rssi(simpleble_peripheral_t handle, simpleble_error_t** out_error) {
    simpleble_error_release(out_error);

    if (handle == nullptr) {
        *out_error = new Error(ErrorCode::INVALID_ARGUMENT, "handle is NULL");
        return INT16_MIN;
    }

    SimpleBLE::Peripheral* peripheral = (SimpleBLE::Peripheral*)handle;
    try {
        return peripheral->rssi();
    } catch (const SimpleBLE::Exception::BaseException& e) {
        *out_error = e.make_error().release();
    } catch (const std::exception& e) {
        *out_error = new Error(ErrorCode::UNCLASSIFIED_EXCEPTION, e.what());
    } catch (...) {
        *out_error = new Error(ErrorCode::UNCLASSIFIED_EXCEPTION, "Unknown exception");
    }

    return INT16_MIN;
}

int16_t simpleble_peripheral_tx_power(simpleble_peripheral_t handle, simpleble_error_t** out_error) {
    simpleble_error_release(out_error);

    if (handle == nullptr) {
        *out_error = new Error(ErrorCode::INVALID_ARGUMENT, "handle is NULL");
        return INT16_MIN;
    }

    SimpleBLE::Peripheral* peripheral = (SimpleBLE::Peripheral*)handle;
    try {
        return peripheral->tx_power();
    } catch (const SimpleBLE::Exception::BaseException& e) {
        *out_error = e.make_error().release();
    } catch (const std::exception& e) {
        *out_error = new Error(ErrorCode::UNCLASSIFIED_EXCEPTION, e.what());
    } catch (...) {
        *out_error = new Error(ErrorCode::UNCLASSIFIED_EXCEPTION, "Unknown exception");
    }

    return INT16_MIN;
}

uint16_t simpleble_peripheral_mtu(simpleble_peripheral_t handle, simpleble_error_t** out_error) {
    simpleble_error_release(out_error);

    if (handle == nullptr) {
        *out_error = new Error(ErrorCode::INVALID_ARGUMENT, "handle is NULL");
        return 0;
    }

    SimpleBLE::Peripheral* peripheral = (SimpleBLE::Peripheral*)handle;
    try {
        return peripheral->mtu();
    } catch (const SimpleBLE::Exception::BaseException& e) {
        *out_error = e.make_error().release();
    } catch (const std::exception& e) {
        *out_error = new Error(ErrorCode::UNCLASSIFIED_EXCEPTION, e.what());
    } catch (...) {
        *out_error = new Error(ErrorCode::UNCLASSIFIED_EXCEPTION, "Unknown exception");
    }

    return 0;
}

void simpleble_peripheral_connect(simpleble_peripheral_t handle, simpleble_error_t** out_error) {
    simpleble_error_release(out_error);

    if (handle == nullptr) {
        *out_error = new Error(ErrorCode::INVALID_ARGUMENT, "handle is NULL");
        return;
    }

    SimpleBLE::Peripheral* peripheral = (SimpleBLE::Peripheral*)handle;
    try {
        peripheral->connect();
    } catch (const SimpleBLE::Exception::BaseException& e) {
        *out_error = e.make_error().release();
    } catch (const std::exception& e) {
        *out_error = new Error(ErrorCode::UNCLASSIFIED_EXCEPTION, e.what());
    } catch (...) {
        *out_error = new Error(ErrorCode::UNCLASSIFIED_EXCEPTION, "Unknown exception");
    }
}

void simpleble_peripheral_disconnect(simpleble_peripheral_t handle, simpleble_error_t** out_error) {
    simpleble_error_release(out_error);

    if (handle == nullptr) {
        *out_error = new Error(ErrorCode::INVALID_ARGUMENT, "handle is NULL");
        return;
    }

    SimpleBLE::Peripheral* peripheral = (SimpleBLE::Peripheral*)handle;
    try {
        peripheral->disconnect();
    } catch (const SimpleBLE::Exception::BaseException& e) {
        *out_error = e.make_error().release();
    } catch (const std::exception& e) {
        *out_error = new Error(ErrorCode::UNCLASSIFIED_EXCEPTION, e.what());
    } catch (...) {
        *out_error = new Error(ErrorCode::UNCLASSIFIED_EXCEPTION, "Unknown exception");
    }
}

bool simpleble_peripheral_is_connected(simpleble_peripheral_t handle, simpleble_error_t** out_error) {
    simpleble_error_release(out_error);

    if (handle == nullptr) {
        *out_error = new Error(ErrorCode::INVALID_ARGUMENT, "handle is NULL");
        return false;
    }

    SimpleBLE::Peripheral* peripheral = (SimpleBLE::Peripheral*)handle;
    try {
        return peripheral->is_connected();
    } catch (const SimpleBLE::Exception::BaseException& e) {
        *out_error = e.make_error().release();
    } catch (const std::exception& e) {
        *out_error = new Error(ErrorCode::UNCLASSIFIED_EXCEPTION, e.what());
    } catch (...) {
        *out_error = new Error(ErrorCode::UNCLASSIFIED_EXCEPTION, "Unknown exception");
    }

    return false;
}

bool simpleble_peripheral_is_connectable(simpleble_peripheral_t handle, simpleble_error_t** out_error) {
    simpleble_error_release(out_error);

    if (handle == nullptr) {
        *out_error = new Error(ErrorCode::INVALID_ARGUMENT, "handle is NULL");
        return false;
    }

    SimpleBLE::Peripheral* peripheral = (SimpleBLE::Peripheral*)handle;
    try {
        return peripheral->is_connectable();
    } catch (const SimpleBLE::Exception::BaseException& e) {
        *out_error = e.make_error().release();
    } catch (const std::exception& e) {
        *out_error = new Error(ErrorCode::UNCLASSIFIED_EXCEPTION, e.what());
    } catch (...) {
        *out_error = new Error(ErrorCode::UNCLASSIFIED_EXCEPTION, "Unknown exception");
    }

    return false;
}

bool simpleble_peripheral_is_paired(simpleble_peripheral_t handle, simpleble_error_t** out_error) {
    simpleble_error_release(out_error);

    if (handle == nullptr) {
        *out_error = new Error(ErrorCode::INVALID_ARGUMENT, "handle is NULL");
        return false;
    }

    SimpleBLE::Peripheral* peripheral = (SimpleBLE::Peripheral*)handle;
    try {
        return peripheral->is_paired();
    } catch (const SimpleBLE::Exception::BaseException& e) {
        *out_error = e.make_error().release();
    } catch (const std::exception& e) {
        *out_error = new Error(ErrorCode::UNCLASSIFIED_EXCEPTION, e.what());
    } catch (...) {
        *out_error = new Error(ErrorCode::UNCLASSIFIED_EXCEPTION, "Unknown exception");
    }

    return false;
}

void simpleble_peripheral_unpair(simpleble_peripheral_t handle, simpleble_error_t** out_error) {
    simpleble_error_release(out_error);

    if (handle == nullptr) {
        *out_error = new Error(ErrorCode::INVALID_ARGUMENT, "handle is NULL");
        return;
    }

    SimpleBLE::Peripheral* peripheral = (SimpleBLE::Peripheral*)handle;
    try {
        peripheral->unpair();
    } catch (const SimpleBLE::Exception::BaseException& e) {
        *out_error = e.make_error().release();
    } catch (const std::exception& e) {
        *out_error = new Error(ErrorCode::UNCLASSIFIED_EXCEPTION, e.what());
    } catch (...) {
        *out_error = new Error(ErrorCode::UNCLASSIFIED_EXCEPTION, "Unknown exception");
    }
}

size_t simpleble_peripheral_services_count(simpleble_peripheral_t handle, simpleble_error_t** out_error) {
    simpleble_error_release(out_error);

    if (handle == nullptr) {
        *out_error = new Error(ErrorCode::INVALID_ARGUMENT, "handle is NULL");
        return 0;
    }

    SimpleBLE::Peripheral* peripheral = (SimpleBLE::Peripheral*)handle;
    try {
        return peripheral->services().size();
    } catch (const SimpleBLE::Exception::BaseException& e) {
        *out_error = e.make_error().release();
    } catch (const std::exception& e) {
        *out_error = new Error(ErrorCode::UNCLASSIFIED_EXCEPTION, e.what());
    } catch (...) {
        *out_error = new Error(ErrorCode::UNCLASSIFIED_EXCEPTION, "Unknown exception");
    }

    return 0;
}

void simpleble_peripheral_services_get(simpleble_peripheral_t handle, size_t index, simpleble_service_t* out_service,
                                       simpleble_error_t** out_error) {
    simpleble_error_release(out_error);

    if (handle == nullptr) {
        *out_error = new Error(ErrorCode::INVALID_ARGUMENT, "handle is NULL");
        return;
    }
    if (out_service == nullptr) {
        *out_error = new Error(ErrorCode::INVALID_ARGUMENT, "out_service is NULL");
        return;
    }
    *out_service = {};

    SimpleBLE::Peripheral* peripheral = (SimpleBLE::Peripheral*)handle;
    try {
        auto peripheral_services = peripheral->services();

        if (index >= peripheral_services.size()) {
            *out_error = new Error(ErrorCode::INVALID_ARGUMENT, "index is out of range");
            return;
        }

        SimpleBLE::Service service = peripheral_services[index];

        strncpy(out_service->uuid.value, service.uuid().c_str(), SIMPLEBLE_UUID_STR_LEN - 1);

        // TODO: Support advertisement payloads larger than the fixed data buffer.
        const size_t copy_len = std::min(service.data().size(), sizeof(out_service->data));
        out_service->data_length = copy_len;
        memcpy(out_service->data, service.data().data(), copy_len);

        out_service->characteristic_count = service.characteristics().size();
        if (out_service->characteristic_count > SIMPLEBLE_CHARACTERISTIC_MAX_COUNT) {
            out_service->characteristic_count = SIMPLEBLE_CHARACTERISTIC_MAX_COUNT;
        }

        for (size_t i = 0; i < out_service->characteristic_count; i++) {
            SimpleBLE::Characteristic characteristic = service.characteristics()[i];

            out_service->characteristics[i].can_read = characteristic.can_read();
            out_service->characteristics[i].can_write_request = characteristic.can_write_request();
            out_service->characteristics[i].can_write_command = characteristic.can_write_command();
            out_service->characteristics[i].can_notify = characteristic.can_notify();
            out_service->characteristics[i].can_indicate = characteristic.can_indicate();

            strncpy(out_service->characteristics[i].uuid.value, characteristic.uuid().c_str(),
                    SIMPLEBLE_UUID_STR_LEN - 1);
            out_service->characteristics[i].descriptor_count = characteristic.descriptors().size();

            if (out_service->characteristics[i].descriptor_count > SIMPLEBLE_DESCRIPTOR_MAX_COUNT) {
                out_service->characteristics[i].descriptor_count = SIMPLEBLE_DESCRIPTOR_MAX_COUNT;
            }

            for (size_t j = 0; j < out_service->characteristics[i].descriptor_count; j++) {
                SimpleBLE::Descriptor descriptor = characteristic.descriptors()[j];

                strncpy(out_service->characteristics[i].descriptors[j].uuid.value, descriptor.uuid().c_str(),
                        SIMPLEBLE_UUID_STR_LEN - 1);
            }
        }
    } catch (const SimpleBLE::Exception::BaseException& e) {
        *out_error = e.make_error().release();
    } catch (const std::exception& e) {
        *out_error = new Error(ErrorCode::UNCLASSIFIED_EXCEPTION, e.what());
    } catch (...) {
        *out_error = new Error(ErrorCode::UNCLASSIFIED_EXCEPTION, "Unknown exception");
    }
}

size_t simpleble_peripheral_manufacturer_data_count(simpleble_peripheral_t handle, simpleble_error_t** out_error) {
    simpleble_error_release(out_error);

    if (handle == nullptr) {
        *out_error = new Error(ErrorCode::INVALID_ARGUMENT, "handle is NULL");
        return 0;
    }

    SimpleBLE::Peripheral* peripheral = (SimpleBLE::Peripheral*)handle;
    try {
        return peripheral->manufacturer_data().size();
    } catch (const SimpleBLE::Exception::BaseException& e) {
        *out_error = e.make_error().release();
    } catch (const std::exception& e) {
        *out_error = new Error(ErrorCode::UNCLASSIFIED_EXCEPTION, e.what());
    } catch (...) {
        *out_error = new Error(ErrorCode::UNCLASSIFIED_EXCEPTION, "Unknown exception");
    }

    return 0;
}

void simpleble_peripheral_manufacturer_data_get(simpleble_peripheral_t handle, size_t index,
                                                simpleble_manufacturer_data_t* out_data,
                                                simpleble_error_t** out_error) {
    simpleble_error_release(out_error);

    if (handle == nullptr) {
        *out_error = new Error(ErrorCode::INVALID_ARGUMENT, "handle is NULL");
        return;
    }
    if (out_data == nullptr) {
        *out_error = new Error(ErrorCode::INVALID_ARGUMENT, "out_data is NULL");
        return;
    }
    *out_data = {};

    SimpleBLE::Peripheral* peripheral = (SimpleBLE::Peripheral*)handle;
    try {
        auto peripheral_manufacturer_data = peripheral->manufacturer_data();

        if (index >= peripheral_manufacturer_data.size()) {
            *out_error = new Error(ErrorCode::INVALID_ARGUMENT, "index is out of range");
            return;
        }

        // Build an iterator and advance to the expected element
        std::map<uint16_t, SimpleBLE::ByteArray>::iterator it = peripheral_manufacturer_data.begin();
        for (size_t i = 0; i < index; i++) {
            it++;
        }

        auto& selected_manufacturer_data = *it;
        out_data->manufacturer_id = selected_manufacturer_data.first;
        // TODO: Support advertisement payloads larger than the fixed data buffer.
        const size_t copy_len = std::min(selected_manufacturer_data.second.size(), sizeof(out_data->data));
        out_data->data_length = copy_len;
        memcpy(out_data->data, selected_manufacturer_data.second.data(), copy_len);
    } catch (const SimpleBLE::Exception::BaseException& e) {
        *out_error = e.make_error().release();
    } catch (const std::exception& e) {
        *out_error = new Error(ErrorCode::UNCLASSIFIED_EXCEPTION, e.what());
    } catch (...) {
        *out_error = new Error(ErrorCode::UNCLASSIFIED_EXCEPTION, "Unknown exception");
    }
}

uint8_t* simpleble_peripheral_read(simpleble_peripheral_t handle, simpleble_uuid_t service,
                                   simpleble_uuid_t characteristic, size_t* data_length,
                                   simpleble_error_t** out_error) {
    simpleble_error_release(out_error);

    if (handle == nullptr) {
        *out_error = new Error(ErrorCode::INVALID_ARGUMENT, "handle is NULL");
        return nullptr;
    }
    if (data_length == nullptr) {
        *out_error = new Error(ErrorCode::INVALID_ARGUMENT, "data_length is NULL");
        return nullptr;
    }
    *data_length = 0;

    SimpleBLE::Peripheral* peripheral = (SimpleBLE::Peripheral*)handle;
    try {
        SimpleBLE::ByteArray read_data = peripheral->read(SimpleBLE::BluetoothUUID(service.value),
                                                          SimpleBLE::BluetoothUUID(characteristic.value));

        if (read_data.empty()) return nullptr;

        auto* data = static_cast<uint8_t*>(malloc(read_data.size()));
        memcpy(data, read_data.data(), read_data.size());
        *data_length = read_data.size();

        return data;
    } catch (const SimpleBLE::Exception::BaseException& e) {
        *out_error = e.make_error().release();
    } catch (const std::exception& e) {
        *out_error = new Error(ErrorCode::UNCLASSIFIED_EXCEPTION, e.what());
    } catch (...) {
        *out_error = new Error(ErrorCode::UNCLASSIFIED_EXCEPTION, "Unknown exception");
    }

    return nullptr;
}

void simpleble_peripheral_write_request(simpleble_peripheral_t handle, simpleble_uuid_t service,
                                        simpleble_uuid_t characteristic, const uint8_t* data, size_t data_length,
                                        simpleble_error_t** out_error) {
    simpleble_error_release(out_error);

    if (handle == nullptr) {
        *out_error = new Error(ErrorCode::INVALID_ARGUMENT, "handle is NULL");
        return;
    }
    if (data == nullptr) {
        *out_error = new Error(ErrorCode::INVALID_ARGUMENT, "data is NULL");
        return;
    }

    SimpleBLE::Peripheral* peripheral = (SimpleBLE::Peripheral*)handle;
    try {
        peripheral->write_request(SimpleBLE::BluetoothUUID(service.value),
                                  SimpleBLE::BluetoothUUID(characteristic.value),
                                  SimpleBLE::ByteArray((const char*)data, data_length));
    } catch (const SimpleBLE::Exception::BaseException& e) {
        *out_error = e.make_error().release();
    } catch (const std::exception& e) {
        *out_error = new Error(ErrorCode::UNCLASSIFIED_EXCEPTION, e.what());
    } catch (...) {
        *out_error = new Error(ErrorCode::UNCLASSIFIED_EXCEPTION, "Unknown exception");
    }
}

void simpleble_peripheral_write_command(simpleble_peripheral_t handle, simpleble_uuid_t service,
                                        simpleble_uuid_t characteristic, const uint8_t* data, size_t data_length,
                                        simpleble_error_t** out_error) {
    simpleble_error_release(out_error);

    if (handle == nullptr) {
        *out_error = new Error(ErrorCode::INVALID_ARGUMENT, "handle is NULL");
        return;
    }
    if (data == nullptr) {
        *out_error = new Error(ErrorCode::INVALID_ARGUMENT, "data is NULL");
        return;
    }

    SimpleBLE::Peripheral* peripheral = (SimpleBLE::Peripheral*)handle;
    try {
        peripheral->write_command(SimpleBLE::BluetoothUUID(service.value),
                                  SimpleBLE::BluetoothUUID(characteristic.value),
                                  SimpleBLE::ByteArray((const char*)data, data_length));
    } catch (const SimpleBLE::Exception::BaseException& e) {
        *out_error = e.make_error().release();
    } catch (const std::exception& e) {
        *out_error = new Error(ErrorCode::UNCLASSIFIED_EXCEPTION, e.what());
    } catch (...) {
        *out_error = new Error(ErrorCode::UNCLASSIFIED_EXCEPTION, "Unknown exception");
    }
}

void simpleble_peripheral_notify(simpleble_peripheral_t handle, simpleble_uuid_t service,
                                 simpleble_uuid_t characteristic,
                                 void (*callback)(simpleble_peripheral_t, simpleble_uuid_t, simpleble_uuid_t,
                                                  const uint8_t*, size_t, void*),
                                 void* userdata, simpleble_error_t** out_error) {
    simpleble_error_release(out_error);

    if (handle == nullptr) {
        *out_error = new Error(ErrorCode::INVALID_ARGUMENT, "handle is NULL");
        return;
    }
    if (callback == nullptr) {
        *out_error = new Error(ErrorCode::INVALID_ARGUMENT, "callback is NULL");
        return;
    }

    SimpleBLE::Peripheral* peripheral = (SimpleBLE::Peripheral*)handle;
    try {
        peripheral->notify(SimpleBLE::BluetoothUUID(service.value), SimpleBLE::BluetoothUUID(characteristic.value),
                           [=](SimpleBLE::ByteArray data) {
                               callback(handle, service, characteristic, (const uint8_t*)data.data(), data.size(),
                                        userdata);
                           });
    } catch (const SimpleBLE::Exception::BaseException& e) {
        *out_error = e.make_error().release();
    } catch (const std::exception& e) {
        *out_error = new Error(ErrorCode::UNCLASSIFIED_EXCEPTION, e.what());
    } catch (...) {
        *out_error = new Error(ErrorCode::UNCLASSIFIED_EXCEPTION, "Unknown exception");
    }
}

void simpleble_peripheral_indicate(simpleble_peripheral_t handle, simpleble_uuid_t service,
                                   simpleble_uuid_t characteristic,
                                   void (*callback)(simpleble_peripheral_t, simpleble_uuid_t, simpleble_uuid_t,
                                                    const uint8_t*, size_t, void*),
                                   void* userdata, simpleble_error_t** out_error) {
    simpleble_error_release(out_error);

    if (handle == nullptr) {
        *out_error = new Error(ErrorCode::INVALID_ARGUMENT, "handle is NULL");
        return;
    }
    if (callback == nullptr) {
        *out_error = new Error(ErrorCode::INVALID_ARGUMENT, "callback is NULL");
        return;
    }

    SimpleBLE::Peripheral* peripheral = (SimpleBLE::Peripheral*)handle;
    try {
        peripheral->indicate(SimpleBLE::BluetoothUUID(service.value), SimpleBLE::BluetoothUUID(characteristic.value),
                             [=](SimpleBLE::ByteArray data) {
                                 callback(handle, service, characteristic, (const uint8_t*)data.data(), data.size(),
                                          userdata);
                             });
    } catch (const SimpleBLE::Exception::BaseException& e) {
        *out_error = e.make_error().release();
    } catch (const std::exception& e) {
        *out_error = new Error(ErrorCode::UNCLASSIFIED_EXCEPTION, e.what());
    } catch (...) {
        *out_error = new Error(ErrorCode::UNCLASSIFIED_EXCEPTION, "Unknown exception");
    }
}

void simpleble_peripheral_unsubscribe(simpleble_peripheral_t handle, simpleble_uuid_t service,
                                      simpleble_uuid_t characteristic, simpleble_error_t** out_error) {
    simpleble_error_release(out_error);

    if (handle == nullptr) {
        *out_error = new Error(ErrorCode::INVALID_ARGUMENT, "handle is NULL");
        return;
    }

    SimpleBLE::Peripheral* peripheral = (SimpleBLE::Peripheral*)handle;
    try {
        peripheral->unsubscribe(SimpleBLE::BluetoothUUID(service.value),
                                SimpleBLE::BluetoothUUID(characteristic.value));
    } catch (const SimpleBLE::Exception::BaseException& e) {
        *out_error = e.make_error().release();
    } catch (const std::exception& e) {
        *out_error = new Error(ErrorCode::UNCLASSIFIED_EXCEPTION, e.what());
    } catch (...) {
        *out_error = new Error(ErrorCode::UNCLASSIFIED_EXCEPTION, "Unknown exception");
    }
}

uint8_t* simpleble_peripheral_read_descriptor(simpleble_peripheral_t handle, simpleble_uuid_t service,
                                              simpleble_uuid_t characteristic, simpleble_uuid_t descriptor,
                                              size_t* data_length, simpleble_error_t** out_error) {
    simpleble_error_release(out_error);

    if (handle == nullptr) {
        *out_error = new Error(ErrorCode::INVALID_ARGUMENT, "handle is NULL");
        return nullptr;
    }
    if (data_length == nullptr) {
        *out_error = new Error(ErrorCode::INVALID_ARGUMENT, "data_length is NULL");
        return nullptr;
    }
    *data_length = 0;

    SimpleBLE::Peripheral* peripheral = (SimpleBLE::Peripheral*)handle;
    try {
        SimpleBLE::ByteArray read_data = peripheral->read(SimpleBLE::BluetoothUUID(service.value),
                                                          SimpleBLE::BluetoothUUID(characteristic.value),
                                                          SimpleBLE::BluetoothUUID(descriptor.value));

        if (read_data.empty()) return nullptr;

        auto* data = static_cast<uint8_t*>(malloc(read_data.size()));
        memcpy(data, read_data.data(), read_data.size());
        *data_length = read_data.size();

        return data;
    } catch (const SimpleBLE::Exception::BaseException& e) {
        *out_error = e.make_error().release();
    } catch (const std::exception& e) {
        *out_error = new Error(ErrorCode::UNCLASSIFIED_EXCEPTION, e.what());
    } catch (...) {
        *out_error = new Error(ErrorCode::UNCLASSIFIED_EXCEPTION, "Unknown exception");
    }

    return nullptr;
}

void simpleble_peripheral_write_descriptor(simpleble_peripheral_t handle, simpleble_uuid_t service,
                                           simpleble_uuid_t characteristic, simpleble_uuid_t descriptor,
                                           const uint8_t* data, size_t data_length, simpleble_error_t** out_error) {
    simpleble_error_release(out_error);

    if (handle == nullptr) {
        *out_error = new Error(ErrorCode::INVALID_ARGUMENT, "handle is NULL");
        return;
    }
    if (data == nullptr) {
        *out_error = new Error(ErrorCode::INVALID_ARGUMENT, "data is NULL");
        return;
    }

    SimpleBLE::Peripheral* peripheral = (SimpleBLE::Peripheral*)handle;
    try {
        peripheral->write(SimpleBLE::BluetoothUUID(service.value), SimpleBLE::BluetoothUUID(characteristic.value),
                          SimpleBLE::BluetoothUUID(descriptor.value),
                          SimpleBLE::ByteArray((const char*)data, data_length));
    } catch (const SimpleBLE::Exception::BaseException& e) {
        *out_error = e.make_error().release();
    } catch (const std::exception& e) {
        *out_error = new Error(ErrorCode::UNCLASSIFIED_EXCEPTION, e.what());
    } catch (...) {
        *out_error = new Error(ErrorCode::UNCLASSIFIED_EXCEPTION, "Unknown exception");
    }
}

void simpleble_peripheral_set_callback_on_connected(simpleble_peripheral_t handle,
                                                    void (*callback)(simpleble_peripheral_t, void*), void* userdata) {
    SimpleBLE::Peripheral* peripheral = (SimpleBLE::Peripheral*)handle;
    if (callback == nullptr) {
        peripheral->set_callback_on_connected(nullptr);
    } else {
        peripheral->set_callback_on_connected([=]() { callback(handle, userdata); });
    }
}

void simpleble_peripheral_set_callback_on_disconnected(simpleble_peripheral_t handle,
                                                       void (*callback)(simpleble_peripheral_t, void*),
                                                       void* userdata) {
    SimpleBLE::Peripheral* peripheral = (SimpleBLE::Peripheral*)handle;
    if (callback == nullptr) {
        peripheral->set_callback_on_disconnected(nullptr);
    } else {
        peripheral->set_callback_on_disconnected([=]() { callback(handle, userdata); });
    }
}
