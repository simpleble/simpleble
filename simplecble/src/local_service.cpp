#include <simplecble/error.h>
#include <simplecble/local/service.h>

#include <simpleble/Exceptions.h>
#include <simpleble/local/Service.h>

#include <cstring>
#include <set>
#include <utility>

#include "list_handle.hpp"

using SimpleBLE::Error;
using SimpleBLE::ErrorCode;

void simpleble_local_service_release_handle(simpleble_local_service_t handle) {
    delete (SimpleBLE::Local::Service*)handle;
}

size_t simpleble_local_service_list_count(simpleble_local_service_list_t list) {
    return SimpleCBLE::List::count<SimpleBLE::Local::Service>(list);
}

simpleble_local_service_t simpleble_local_service_list_get(simpleble_local_service_list_t list, size_t index,
                                                           simpleble_error_t** out_error) {
    return SimpleCBLE::List::get<SimpleBLE::Local::Service>(list, index, out_error);
}

void simpleble_local_service_list_release(simpleble_local_service_list_t list) {
    SimpleCBLE::List::release<SimpleBLE::Local::Service>(list);
}

size_t simpleble_local_service_characteristics(simpleble_local_service_t handle,
                                               simpleble_local_characteristic_list_t* out_list,
                                               simpleble_error_t** out_error) {
    return SimpleCBLE::List::read<SimpleBLE::Local::Service>(
        handle, out_list, out_error, [](SimpleBLE::Local::Service& service) { return service.characteristics(); });
}

void simpleble_local_service_uuid(simpleble_local_service_t handle, simpleble_uuid_t* out_uuid,
                                  simpleble_error_t** out_error) {
    simpleble_error_release(out_error);

    if (handle == nullptr) {
        *out_error = new Error(ErrorCode::INVALID_ARGUMENT, "handle is NULL");
        return;
    }
    if (out_uuid == nullptr) {
        *out_error = new Error(ErrorCode::INVALID_ARGUMENT, "out_uuid is NULL");
        return;
    }
    *out_uuid = {};

    auto* service = (SimpleBLE::Local::Service*)handle;
    try {
        auto uuid = service->uuid();
        std::strncpy(out_uuid->value, uuid.c_str(), SIMPLEBLE_UUID_STR_LEN - 1);
    } catch (const SimpleBLE::Exception::BaseException& e) {
        *out_error = e.make_error().release();
    } catch (const std::exception& e) {
        *out_error = new Error(ErrorCode::UNCLASSIFIED_EXCEPTION, e.what());
    } catch (...) {
        *out_error = new Error(ErrorCode::UNCLASSIFIED_EXCEPTION, "Unknown exception");
    }
}

simpleble_local_characteristic_t simpleble_local_service_add_characteristic(simpleble_local_service_t handle,
                                                                            simpleble_uuid_t uuid,
                                                                            uint32_t capabilities,
                                                                            simpleble_error_t** out_error) {
    simpleble_error_release(out_error);

    if (handle == nullptr) {
        *out_error = new Error(ErrorCode::INVALID_ARGUMENT, "handle is NULL");
        return nullptr;
    }
    if (capabilities & ~uint32_t(SIMPLEBLE_LOCAL_CHARACTERISTIC_READ | SIMPLEBLE_LOCAL_CHARACTERISTIC_WRITE_REQUEST |
                                 SIMPLEBLE_LOCAL_CHARACTERISTIC_WRITE_COMMAND | SIMPLEBLE_LOCAL_CHARACTERISTIC_NOTIFY |
                                 SIMPLEBLE_LOCAL_CHARACTERISTIC_INDICATE)) {
        *out_error = new Error(ErrorCode::INVALID_ARGUMENT, "capabilities contains unknown flags");
        return nullptr;
    }

    auto* service = (SimpleBLE::Local::Service*)handle;
    try {
        std::set<SimpleBLE::Local::CharacteristicCapability> cpp_capabilities;
        if (capabilities & SIMPLEBLE_LOCAL_CHARACTERISTIC_READ)
            cpp_capabilities.insert(SimpleBLE::Local::CharacteristicCapability::READ);
        if (capabilities & SIMPLEBLE_LOCAL_CHARACTERISTIC_WRITE_REQUEST)
            cpp_capabilities.insert(SimpleBLE::Local::CharacteristicCapability::WRITE_REQUEST);
        if (capabilities & SIMPLEBLE_LOCAL_CHARACTERISTIC_WRITE_COMMAND)
            cpp_capabilities.insert(SimpleBLE::Local::CharacteristicCapability::WRITE_COMMAND);
        if (capabilities & SIMPLEBLE_LOCAL_CHARACTERISTIC_NOTIFY)
            cpp_capabilities.insert(SimpleBLE::Local::CharacteristicCapability::NOTIFY);
        if (capabilities & SIMPLEBLE_LOCAL_CHARACTERISTIC_INDICATE)
            cpp_capabilities.insert(SimpleBLE::Local::CharacteristicCapability::INDICATE);
        return new SimpleBLE::Local::Characteristic(
            service->add_characteristic(SimpleBLE::BluetoothUUID(uuid.value), std::move(cpp_capabilities)));
    } catch (const SimpleBLE::Exception::BaseException& e) {
        *out_error = e.make_error().release();
    } catch (const std::exception& e) {
        *out_error = new Error(ErrorCode::UNCLASSIFIED_EXCEPTION, e.what());
    } catch (...) {
        *out_error = new Error(ErrorCode::UNCLASSIFIED_EXCEPTION, "Unknown exception");
    }

    return nullptr;
}
