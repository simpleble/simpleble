#include <simplecble/error.h>
#include <simplecble/local/peripheral.h>

#include <simpleble/Exceptions.h>
#include <simpleble/local/Peripheral.h>

#include "list_handle.hpp"

using SimpleBLE::Error;
using SimpleBLE::ErrorCode;

void simpleble_local_peripheral_release_handle(simpleble_local_peripheral_t handle) {
    delete (SimpleBLE::Local::Peripheral*)handle;
}

void* simpleble_local_peripheral_underlying(simpleble_local_peripheral_t handle, simpleble_error_t** out_error) {
    simpleble_error_release(out_error);

    if (handle == nullptr) {
        *out_error = new Error(ErrorCode::INVALID_ARGUMENT, "handle is NULL");
        return nullptr;
    }

    auto* peripheral = (SimpleBLE::Local::Peripheral*)handle;
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

void simpleble_local_peripheral_add_advertised_service(simpleble_local_peripheral_t handle, simpleble_uuid_t service,
                                                       simpleble_error_t** out_error) {
    simpleble_error_release(out_error);

    if (handle == nullptr) {
        *out_error = new Error(ErrorCode::INVALID_ARGUMENT, "handle is NULL");
        return;
    }

    auto* peripheral = (SimpleBLE::Local::Peripheral*)handle;
    try {
        peripheral->add_advertised_service(SimpleBLE::BluetoothUUID(service.value));
    } catch (const SimpleBLE::Exception::BaseException& e) {
        *out_error = e.make_error().release();
    } catch (const std::exception& e) {
        *out_error = new Error(ErrorCode::UNCLASSIFIED_EXCEPTION, e.what());
    } catch (...) {
        *out_error = new Error(ErrorCode::UNCLASSIFIED_EXCEPTION, "Unknown exception");
    }
}

void simpleble_local_peripheral_set_advertised_manufacturer_data(simpleble_local_peripheral_t handle,
                                                                 uint16_t company_id, const uint8_t* data,
                                                                 size_t data_length, simpleble_error_t** out_error) {
    simpleble_error_release(out_error);

    if (handle == nullptr) {
        *out_error = new Error(ErrorCode::INVALID_ARGUMENT, "handle is NULL");
        return;
    }
    if (data == nullptr && data_length != 0) {
        *out_error = new Error(ErrorCode::INVALID_ARGUMENT, "data is NULL");
        return;
    }

    auto* peripheral = (SimpleBLE::Local::Peripheral*)handle;
    try {
        peripheral->set_advertised_manufacturer_data(
            company_id,
            data_length == 0 ? SimpleBLE::ByteArray{} : SimpleBLE::ByteArray((const char*)data, data_length));
    } catch (const SimpleBLE::Exception::BaseException& e) {
        *out_error = e.make_error().release();
    } catch (const std::exception& e) {
        *out_error = new Error(ErrorCode::UNCLASSIFIED_EXCEPTION, e.what());
    } catch (...) {
        *out_error = new Error(ErrorCode::UNCLASSIFIED_EXCEPTION, "Unknown exception");
    }
}

void simpleble_local_peripheral_set_connectable(simpleble_local_peripheral_t handle, bool connectable,
                                                simpleble_error_t** out_error) {
    simpleble_error_release(out_error);

    if (handle == nullptr) {
        *out_error = new Error(ErrorCode::INVALID_ARGUMENT, "handle is NULL");
        return;
    }

    auto* peripheral = (SimpleBLE::Local::Peripheral*)handle;
    try {
        peripheral->set_connectable(connectable);
    } catch (const SimpleBLE::Exception::BaseException& e) {
        *out_error = e.make_error().release();
    } catch (const std::exception& e) {
        *out_error = new Error(ErrorCode::UNCLASSIFIED_EXCEPTION, e.what());
    } catch (...) {
        *out_error = new Error(ErrorCode::UNCLASSIFIED_EXCEPTION, "Unknown exception");
    }
}

simpleble_local_service_t simpleble_local_peripheral_add_service(simpleble_local_peripheral_t handle,
                                                                 simpleble_uuid_t uuid, simpleble_error_t** out_error) {
    simpleble_error_release(out_error);

    if (handle == nullptr) {
        *out_error = new Error(ErrorCode::INVALID_ARGUMENT, "handle is NULL");
        return nullptr;
    }

    auto* peripheral = (SimpleBLE::Local::Peripheral*)handle;
    try {
        return new SimpleBLE::Local::Service(peripheral->add_service(SimpleBLE::BluetoothUUID(uuid.value)));
    } catch (const SimpleBLE::Exception::BaseException& e) {
        *out_error = e.make_error().release();
    } catch (const std::exception& e) {
        *out_error = new Error(ErrorCode::UNCLASSIFIED_EXCEPTION, e.what());
    } catch (...) {
        *out_error = new Error(ErrorCode::UNCLASSIFIED_EXCEPTION, "Unknown exception");
    }

    return nullptr;
}

size_t simpleble_local_peripheral_services(simpleble_local_peripheral_t handle,
                                           simpleble_local_service_list_t* out_list, simpleble_error_t** out_error) {
    return SimpleCBLE::List::read<SimpleBLE::Local::Peripheral>(
        handle, out_list, out_error, [](SimpleBLE::Local::Peripheral& peripheral) { return peripheral.services(); });
}

void simpleble_local_peripheral_remove_all_services(simpleble_local_peripheral_t handle,
                                                    simpleble_error_t** out_error) {
    simpleble_error_release(out_error);

    if (handle == nullptr) {
        *out_error = new Error(ErrorCode::INVALID_ARGUMENT, "handle is NULL");
        return;
    }

    auto* peripheral = (SimpleBLE::Local::Peripheral*)handle;
    try {
        peripheral->remove_all_services();
    } catch (const SimpleBLE::Exception::BaseException& e) {
        *out_error = e.make_error().release();
    } catch (const std::exception& e) {
        *out_error = new Error(ErrorCode::UNCLASSIFIED_EXCEPTION, e.what());
    } catch (...) {
        *out_error = new Error(ErrorCode::UNCLASSIFIED_EXCEPTION, "Unknown exception");
    }
}

void simpleble_local_peripheral_start(simpleble_local_peripheral_t handle, simpleble_error_t** out_error) {
    simpleble_error_release(out_error);

    if (handle == nullptr) {
        *out_error = new Error(ErrorCode::INVALID_ARGUMENT, "handle is NULL");
        return;
    }

    auto* peripheral = (SimpleBLE::Local::Peripheral*)handle;
    try {
        peripheral->start();
    } catch (const SimpleBLE::Exception::BaseException& e) {
        *out_error = e.make_error().release();
    } catch (const std::exception& e) {
        *out_error = new Error(ErrorCode::UNCLASSIFIED_EXCEPTION, e.what());
    } catch (...) {
        *out_error = new Error(ErrorCode::UNCLASSIFIED_EXCEPTION, "Unknown exception");
    }
}

void simpleble_local_peripheral_stop(simpleble_local_peripheral_t handle, simpleble_error_t** out_error) {
    simpleble_error_release(out_error);

    if (handle == nullptr) {
        *out_error = new Error(ErrorCode::INVALID_ARGUMENT, "handle is NULL");
        return;
    }

    auto* peripheral = (SimpleBLE::Local::Peripheral*)handle;
    try {
        peripheral->stop();
    } catch (const SimpleBLE::Exception::BaseException& e) {
        *out_error = e.make_error().release();
    } catch (const std::exception& e) {
        *out_error = new Error(ErrorCode::UNCLASSIFIED_EXCEPTION, e.what());
    } catch (...) {
        *out_error = new Error(ErrorCode::UNCLASSIFIED_EXCEPTION, "Unknown exception");
    }
}

bool simpleble_local_peripheral_is_started(simpleble_local_peripheral_t handle, simpleble_error_t** out_error) {
    simpleble_error_release(out_error);

    if (handle == nullptr) {
        *out_error = new Error(ErrorCode::INVALID_ARGUMENT, "handle is NULL");
        return false;
    }

    auto* peripheral = (SimpleBLE::Local::Peripheral*)handle;
    try {
        return peripheral->is_started();
    } catch (const SimpleBLE::Exception::BaseException& e) {
        *out_error = e.make_error().release();
    } catch (const std::exception& e) {
        *out_error = new Error(ErrorCode::UNCLASSIFIED_EXCEPTION, e.what());
    } catch (...) {
        *out_error = new Error(ErrorCode::UNCLASSIFIED_EXCEPTION, "Unknown exception");
    }

    return false;
}

bool simpleble_local_peripheral_is_advertising(simpleble_local_peripheral_t handle, simpleble_error_t** out_error) {
    simpleble_error_release(out_error);

    if (handle == nullptr) {
        *out_error = new Error(ErrorCode::INVALID_ARGUMENT, "handle is NULL");
        return false;
    }

    auto* peripheral = (SimpleBLE::Local::Peripheral*)handle;
    try {
        return peripheral->is_advertising();
    } catch (const SimpleBLE::Exception::BaseException& e) {
        *out_error = e.make_error().release();
    } catch (const std::exception& e) {
        *out_error = new Error(ErrorCode::UNCLASSIFIED_EXCEPTION, e.what());
    } catch (...) {
        *out_error = new Error(ErrorCode::UNCLASSIFIED_EXCEPTION, "Unknown exception");
    }

    return false;
}

void simpleble_local_peripheral_set_callback_on_client_connected(
    simpleble_local_peripheral_t handle,
    void (*callback)(simpleble_local_peripheral_t handle, const char* client_address, void* userdata), void* userdata) {
    auto* peripheral = (SimpleBLE::Local::Peripheral*)handle;
    if (callback == nullptr) {
        peripheral->set_callback_on_client_connected(nullptr);
    } else {
        peripheral->set_callback_on_client_connected(
            [=](SimpleBLE::BluetoothAddress address) { callback(handle, address.c_str(), userdata); });
    }
}

void simpleble_local_peripheral_set_callback_on_client_disconnected(
    simpleble_local_peripheral_t handle,
    void (*callback)(simpleble_local_peripheral_t handle, const char* client_address, void* userdata), void* userdata) {
    auto* peripheral = (SimpleBLE::Local::Peripheral*)handle;
    if (callback == nullptr) {
        peripheral->set_callback_on_client_disconnected(nullptr);
    } else {
        peripheral->set_callback_on_client_disconnected(
            [=](SimpleBLE::BluetoothAddress address) { callback(handle, address.c_str(), userdata); });
    }
}
