#include <simplecble/error.h>
#include <simplecble/local/characteristic.h>

#include <simpleble/Exceptions.h>
#include <simpleble/local/Characteristic.h>

#include <cstdlib>
#include <cstring>

using SimpleBLE::Error;
using SimpleBLE::ErrorCode;

void simpleble_local_characteristic_release_handle(simpleble_local_characteristic_t handle) {
    delete (SimpleBLE::Local::Characteristic*)handle;
}

void simpleble_local_characteristic_uuid(simpleble_local_characteristic_t handle, simpleble_uuid_t* out_uuid,
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

    auto* characteristic = (SimpleBLE::Local::Characteristic*)handle;
    try {
        auto uuid = characteristic->uuid();
        std::strncpy(out_uuid->value, uuid.c_str(), SIMPLEBLE_UUID_STR_LEN - 1);
    } catch (const SimpleBLE::Exception::BaseException& e) {
        *out_error = e.make_error().release();
    } catch (const std::exception& e) {
        *out_error = new Error(ErrorCode::UNCLASSIFIED_EXCEPTION, e.what());
    } catch (...) {
        *out_error = new Error(ErrorCode::UNCLASSIFIED_EXCEPTION, "Unknown exception");
    }
}

uint32_t simpleble_local_characteristic_capabilities(simpleble_local_characteristic_t handle,
                                                     simpleble_error_t** out_error) {
    simpleble_error_release(out_error);

    if (handle == nullptr) {
        *out_error = new Error(ErrorCode::INVALID_ARGUMENT, "handle is NULL");
        return 0;
    }

    auto* characteristic = (SimpleBLE::Local::Characteristic*)handle;
    try {
        uint32_t result = 0;
        for (auto capability : characteristic->capabilities()) {
            switch (capability) {
                case SimpleBLE::Local::CharacteristicCapability::READ:
                    result |= SIMPLEBLE_LOCAL_CHARACTERISTIC_READ;
                    break;
                case SimpleBLE::Local::CharacteristicCapability::WRITE_REQUEST:
                    result |= SIMPLEBLE_LOCAL_CHARACTERISTIC_WRITE_REQUEST;
                    break;
                case SimpleBLE::Local::CharacteristicCapability::WRITE_COMMAND:
                    result |= SIMPLEBLE_LOCAL_CHARACTERISTIC_WRITE_COMMAND;
                    break;
                case SimpleBLE::Local::CharacteristicCapability::NOTIFY:
                    result |= SIMPLEBLE_LOCAL_CHARACTERISTIC_NOTIFY;
                    break;
                case SimpleBLE::Local::CharacteristicCapability::INDICATE:
                    result |= SIMPLEBLE_LOCAL_CHARACTERISTIC_INDICATE;
                    break;
            }
        }
        return result;
    } catch (const SimpleBLE::Exception::BaseException& e) {
        *out_error = e.make_error().release();
    } catch (const std::exception& e) {
        *out_error = new Error(ErrorCode::UNCLASSIFIED_EXCEPTION, e.what());
    } catch (...) {
        *out_error = new Error(ErrorCode::UNCLASSIFIED_EXCEPTION, "Unknown exception");
    }

    return 0;
}

uint8_t* simpleble_local_characteristic_value(simpleble_local_characteristic_t handle, size_t* data_length,
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

    auto* characteristic = (SimpleBLE::Local::Characteristic*)handle;
    try {
        auto value = characteristic->value();
        if (value.empty()) return nullptr;
        auto* data = static_cast<uint8_t*>(std::malloc(value.size()));
        std::memcpy(data, value.data(), value.size());
        *data_length = value.size();
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

void simpleble_local_characteristic_set_value(simpleble_local_characteristic_t handle, const uint8_t* data,
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

    auto* characteristic = (SimpleBLE::Local::Characteristic*)handle;
    try {
        characteristic->set_value(data_length == 0 ? SimpleBLE::ByteArray{}
                                                   : SimpleBLE::ByteArray((const char*)data, data_length));
    } catch (const SimpleBLE::Exception::BaseException& e) {
        *out_error = e.make_error().release();
    } catch (const std::exception& e) {
        *out_error = new Error(ErrorCode::UNCLASSIFIED_EXCEPTION, e.what());
    } catch (...) {
        *out_error = new Error(ErrorCode::UNCLASSIFIED_EXCEPTION, "Unknown exception");
    }
}

void simpleble_local_characteristic_set_callback_on_read(
    simpleble_local_characteristic_t handle,
    const uint8_t* (*callback)(simpleble_local_characteristic_t handle, size_t* data_length, void* userdata),
    void* userdata) {
    auto* characteristic = (SimpleBLE::Local::Characteristic*)handle;
    if (callback == nullptr) {
        characteristic->set_callback_on_read(nullptr);
    } else {
        characteristic->set_callback_on_read([=]() {
            size_t data_length = 0;
            const uint8_t* data = callback(handle, &data_length, userdata);
            if (data == nullptr || data_length == 0) return SimpleBLE::ByteArray{};
            return SimpleBLE::ByteArray((const char*)data, data_length);
        });
    }
}

void simpleble_local_characteristic_set_callback_on_write(simpleble_local_characteristic_t handle,
                                                          void (*callback)(simpleble_local_characteristic_t handle,
                                                                           const uint8_t* data, size_t data_length,
                                                                           void* userdata),
                                                          void* userdata) {
    auto* characteristic = (SimpleBLE::Local::Characteristic*)handle;
    if (callback == nullptr) {
        characteristic->set_callback_on_write(nullptr);
    } else {
        characteristic->set_callback_on_write([=](SimpleBLE::ByteArray value) {
            callback(handle, reinterpret_cast<const uint8_t*>(value.data()), value.size(), userdata);
        });
    }
}

void simpleble_local_characteristic_set_callback_on_subscribed(simpleble_local_characteristic_t handle,
                                                               void (*callback)(simpleble_local_characteristic_t handle,
                                                                                void* userdata),
                                                               void* userdata) {
    auto* characteristic = (SimpleBLE::Local::Characteristic*)handle;
    if (callback == nullptr) {
        characteristic->set_callback_on_subscribed(nullptr);
    } else {
        characteristic->set_callback_on_subscribed([=]() { callback(handle, userdata); });
    }
}

void simpleble_local_characteristic_set_callback_on_unsubscribed(
    simpleble_local_characteristic_t handle, void (*callback)(simpleble_local_characteristic_t handle, void* userdata),
    void* userdata) {
    auto* characteristic = (SimpleBLE::Local::Characteristic*)handle;
    if (callback == nullptr) {
        characteristic->set_callback_on_unsubscribed(nullptr);
    } else {
        characteristic->set_callback_on_unsubscribed([=]() { callback(handle, userdata); });
    }
}
