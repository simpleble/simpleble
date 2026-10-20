#include <simplecble/adapter.h>
#include <simplecble/error.h>

#include <simpleble/Adapter.h>
#include <simpleble/Exceptions.h>

#include <cstdlib>
#include <cstring>
#include <vector>

#include "list_handle.hpp"

using SimpleBLE::Error;
using SimpleBLE::ErrorCode;

bool simpleble_adapter_is_bluetooth_enabled(simpleble_error_t** out_error) {
    simpleble_error_release(out_error);

    try {
        return SimpleBLE::Adapter::bluetooth_enabled();
    } catch (const SimpleBLE::Exception::BaseException& e) {
        *out_error = e.make_error().release();
    } catch (const std::exception& e) {
        *out_error = new Error(ErrorCode::UNCLASSIFIED_EXCEPTION, e.what());
    } catch (...) {
        *out_error = new Error(ErrorCode::UNCLASSIFIED_EXCEPTION, "Unknown exception");
    }

    return false;
}

size_t simpleble_adapter_get_adapters(simpleble_adapter_list_t* out_list, simpleble_error_t** out_error) {
    return SimpleCBLE::List::read(out_list, out_error, []() { return SimpleBLE::Adapter::get_adapters(); });
}

size_t simpleble_adapter_list_count(simpleble_adapter_list_t list) {
    return SimpleCBLE::List::count<SimpleBLE::Adapter>(list);
}

simpleble_adapter_t simpleble_adapter_list_get(simpleble_adapter_list_t list, size_t index,
                                               simpleble_error_t** out_error) {
    return SimpleCBLE::List::get<SimpleBLE::Adapter>(list, index, out_error);
}

void simpleble_adapter_list_release(simpleble_adapter_list_t list) {
    SimpleCBLE::List::release<SimpleBLE::Adapter>(list);
}

void simpleble_adapter_release_handle(simpleble_adapter_t handle) {
    if (handle == nullptr) {
        return;
    }

    SimpleBLE::Adapter* adapter = (SimpleBLE::Adapter*)handle;
    delete adapter;
}

void* simpleble_adapter_underlying(simpleble_adapter_t handle, simpleble_error_t** out_error) {
    simpleble_error_release(out_error);

    if (handle == nullptr) {
        *out_error = new Error(ErrorCode::INVALID_ARGUMENT, "handle is NULL");
        return nullptr;
    }

    SimpleBLE::Adapter* adapter = (SimpleBLE::Adapter*)handle;
    try {
        return adapter->underlying();
    } catch (const SimpleBLE::Exception::BaseException& e) {
        *out_error = e.make_error().release();
    } catch (const std::exception& e) {
        *out_error = new Error(ErrorCode::UNCLASSIFIED_EXCEPTION, e.what());
    } catch (...) {
        *out_error = new Error(ErrorCode::UNCLASSIFIED_EXCEPTION, "Unknown exception");
    }

    return nullptr;
}

char* simpleble_adapter_identifier(simpleble_adapter_t handle, simpleble_error_t** out_error) {
    simpleble_error_release(out_error);

    if (handle == nullptr) {
        *out_error = new Error(ErrorCode::INVALID_ARGUMENT, "handle is NULL");
        return nullptr;
    }

    SimpleBLE::Adapter* adapter = (SimpleBLE::Adapter*)handle;
    try {
        std::string identifier = adapter->identifier();
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

char* simpleble_adapter_address(simpleble_adapter_t handle, simpleble_error_t** out_error) {
    simpleble_error_release(out_error);

    if (handle == nullptr) {
        *out_error = new Error(ErrorCode::INVALID_ARGUMENT, "handle is NULL");
        return nullptr;
    }

    SimpleBLE::Adapter* adapter = (SimpleBLE::Adapter*)handle;
    try {
        std::string address = adapter->address();
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

void simpleble_adapter_power_on(simpleble_adapter_t handle, simpleble_error_t** out_error) {
    simpleble_error_release(out_error);

    if (handle == nullptr) {
        *out_error = new Error(ErrorCode::INVALID_ARGUMENT, "handle is NULL");
        return;
    }

    SimpleBLE::Adapter* adapter = (SimpleBLE::Adapter*)handle;
    try {
        adapter->power_on();
    } catch (const SimpleBLE::Exception::BaseException& e) {
        *out_error = e.make_error().release();
    } catch (const std::exception& e) {
        *out_error = new Error(ErrorCode::UNCLASSIFIED_EXCEPTION, e.what());
    } catch (...) {
        *out_error = new Error(ErrorCode::UNCLASSIFIED_EXCEPTION, "Unknown exception");
    }
}

void simpleble_adapter_power_off(simpleble_adapter_t handle, simpleble_error_t** out_error) {
    simpleble_error_release(out_error);

    if (handle == nullptr) {
        *out_error = new Error(ErrorCode::INVALID_ARGUMENT, "handle is NULL");
        return;
    }

    SimpleBLE::Adapter* adapter = (SimpleBLE::Adapter*)handle;
    try {
        adapter->power_off();
    } catch (const SimpleBLE::Exception::BaseException& e) {
        *out_error = e.make_error().release();
    } catch (const std::exception& e) {
        *out_error = new Error(ErrorCode::UNCLASSIFIED_EXCEPTION, e.what());
    } catch (...) {
        *out_error = new Error(ErrorCode::UNCLASSIFIED_EXCEPTION, "Unknown exception");
    }
}

bool simpleble_adapter_is_powered(simpleble_adapter_t handle, simpleble_error_t** out_error) {
    simpleble_error_release(out_error);

    if (handle == nullptr) {
        *out_error = new Error(ErrorCode::INVALID_ARGUMENT, "handle is NULL");
        return false;
    }

    SimpleBLE::Adapter* adapter = (SimpleBLE::Adapter*)handle;
    try {
        return adapter->is_powered();
    } catch (const SimpleBLE::Exception::BaseException& e) {
        *out_error = e.make_error().release();
    } catch (const std::exception& e) {
        *out_error = new Error(ErrorCode::UNCLASSIFIED_EXCEPTION, e.what());
    } catch (...) {
        *out_error = new Error(ErrorCode::UNCLASSIFIED_EXCEPTION, "Unknown exception");
    }

    return false;
}

void simpleble_adapter_set_callback_on_power_on(simpleble_adapter_t handle,
                                                void (*callback)(simpleble_adapter_t, void*), void* userdata) {
    SimpleBLE::Adapter* adapter = (SimpleBLE::Adapter*)handle;
    if (callback == nullptr) {
        adapter->set_callback_on_power_on(nullptr);
    } else {
        adapter->set_callback_on_power_on([=]() { callback(handle, userdata); });
    }
}

void simpleble_adapter_set_callback_on_power_off(simpleble_adapter_t handle,
                                                 void (*callback)(simpleble_adapter_t, void*), void* userdata) {
    SimpleBLE::Adapter* adapter = (SimpleBLE::Adapter*)handle;
    if (callback == nullptr) {
        adapter->set_callback_on_power_off(nullptr);
    } else {
        adapter->set_callback_on_power_off([=]() { callback(handle, userdata); });
    }
}

void simpleble_adapter_scan_start(simpleble_adapter_t handle, simpleble_error_t** out_error) {
    simpleble_error_release(out_error);

    if (handle == nullptr) {
        *out_error = new Error(ErrorCode::INVALID_ARGUMENT, "handle is NULL");
        return;
    }

    SimpleBLE::Adapter* adapter = (SimpleBLE::Adapter*)handle;
    try {
        adapter->scan_start();
    } catch (const SimpleBLE::Exception::BaseException& e) {
        *out_error = e.make_error().release();
    } catch (const std::exception& e) {
        *out_error = new Error(ErrorCode::UNCLASSIFIED_EXCEPTION, e.what());
    } catch (...) {
        *out_error = new Error(ErrorCode::UNCLASSIFIED_EXCEPTION, "Unknown exception");
    }
}

void simpleble_adapter_scan_stop(simpleble_adapter_t handle, simpleble_error_t** out_error) {
    simpleble_error_release(out_error);

    if (handle == nullptr) {
        *out_error = new Error(ErrorCode::INVALID_ARGUMENT, "handle is NULL");
        return;
    }

    SimpleBLE::Adapter* adapter = (SimpleBLE::Adapter*)handle;
    try {
        adapter->scan_stop();
    } catch (const SimpleBLE::Exception::BaseException& e) {
        *out_error = e.make_error().release();
    } catch (const std::exception& e) {
        *out_error = new Error(ErrorCode::UNCLASSIFIED_EXCEPTION, e.what());
    } catch (...) {
        *out_error = new Error(ErrorCode::UNCLASSIFIED_EXCEPTION, "Unknown exception");
    }
}

bool simpleble_adapter_scan_is_active(simpleble_adapter_t handle, simpleble_error_t** out_error) {
    simpleble_error_release(out_error);

    if (handle == nullptr) {
        *out_error = new Error(ErrorCode::INVALID_ARGUMENT, "handle is NULL");
        return false;
    }

    SimpleBLE::Adapter* adapter = (SimpleBLE::Adapter*)handle;
    try {
        return adapter->scan_is_active();
    } catch (const SimpleBLE::Exception::BaseException& e) {
        *out_error = e.make_error().release();
    } catch (const std::exception& e) {
        *out_error = new Error(ErrorCode::UNCLASSIFIED_EXCEPTION, e.what());
    } catch (...) {
        *out_error = new Error(ErrorCode::UNCLASSIFIED_EXCEPTION, "Unknown exception");
    }

    return false;
}

void simpleble_adapter_scan_for(simpleble_adapter_t handle, int timeout_ms, simpleble_error_t** out_error) {
    simpleble_error_release(out_error);

    if (handle == nullptr) {
        *out_error = new Error(ErrorCode::INVALID_ARGUMENT, "handle is NULL");
        return;
    }

    SimpleBLE::Adapter* adapter = (SimpleBLE::Adapter*)handle;
    try {
        adapter->scan_for(timeout_ms);
    } catch (const SimpleBLE::Exception::BaseException& e) {
        *out_error = e.make_error().release();
    } catch (const std::exception& e) {
        *out_error = new Error(ErrorCode::UNCLASSIFIED_EXCEPTION, e.what());
    } catch (...) {
        *out_error = new Error(ErrorCode::UNCLASSIFIED_EXCEPTION, "Unknown exception");
    }
}

size_t simpleble_adapter_scan_get_results(simpleble_adapter_t handle, simpleble_peripheral_list_t* out_list,
                                          simpleble_error_t** out_error) {
    return SimpleCBLE::List::read<SimpleBLE::Adapter>(
        handle, out_list, out_error, [](SimpleBLE::Adapter& adapter) { return adapter.scan_get_results(); });
}

size_t simpleble_adapter_get_paired_peripherals(simpleble_adapter_t handle, simpleble_peripheral_list_t* out_list,
                                                simpleble_error_t** out_error) {
    return SimpleCBLE::List::read<SimpleBLE::Adapter>(
        handle, out_list, out_error, [](SimpleBLE::Adapter& adapter) { return adapter.get_paired_peripherals(); });
}

size_t simpleble_adapter_get_connected_peripherals(simpleble_adapter_t handle, simpleble_peripheral_list_t* out_list,
                                                   simpleble_error_t** out_error) {
    return SimpleCBLE::List::read<SimpleBLE::Adapter>(
        handle, out_list, out_error, [](SimpleBLE::Adapter& adapter) { return adapter.get_connected_peripherals(); });
}

void simpleble_adapter_set_callback_on_scan_start(simpleble_adapter_t handle,
                                                  void (*callback)(simpleble_adapter_t, void*), void* userdata) {
    SimpleBLE::Adapter* adapter = (SimpleBLE::Adapter*)handle;
    if (callback == nullptr) {
        adapter->set_callback_on_scan_start(nullptr);
    } else {
        adapter->set_callback_on_scan_start([=]() { callback(handle, userdata); });
    }
}

void simpleble_adapter_set_callback_on_scan_stop(simpleble_adapter_t handle,
                                                 void (*callback)(simpleble_adapter_t, void*), void* userdata) {
    SimpleBLE::Adapter* adapter = (SimpleBLE::Adapter*)handle;
    if (callback == nullptr) {
        adapter->set_callback_on_scan_stop(nullptr);
    } else {
        adapter->set_callback_on_scan_stop([=]() { callback(handle, userdata); });
    }
}

void simpleble_adapter_set_callback_on_scan_updated(
    simpleble_adapter_t handle, void (*callback)(simpleble_adapter_t, simpleble_peripheral_t, void*), void* userdata) {
    SimpleBLE::Adapter* adapter = (SimpleBLE::Adapter*)handle;
    if (callback == nullptr) {
        adapter->set_callback_on_scan_updated(nullptr);
    } else {
        adapter->set_callback_on_scan_updated([=](SimpleBLE::Peripheral peripheral) {
            SimpleBLE::Peripheral* peripheral_handle = new SimpleBLE::Peripheral(peripheral);
            callback(handle, (simpleble_peripheral_t)peripheral_handle, userdata);
        });
    }
}

void simpleble_adapter_set_callback_on_scan_found(simpleble_adapter_t handle,
                                                  void (*callback)(simpleble_adapter_t, simpleble_peripheral_t, void*),
                                                  void* userdata) {
    SimpleBLE::Adapter* adapter = (SimpleBLE::Adapter*)handle;
    if (callback == nullptr) {
        adapter->set_callback_on_scan_found(nullptr);
    } else {
        adapter->set_callback_on_scan_found([=](SimpleBLE::Peripheral peripheral) {
            SimpleBLE::Peripheral* peripheral_handle = new SimpleBLE::Peripheral(peripheral);
            callback(handle, (simpleble_peripheral_t)peripheral_handle, userdata);
        });
    }
}

simpleble_local_peripheral_t simpleble_adapter_create_local_peripheral(simpleble_adapter_t handle,
                                                                       simpleble_error_t** out_error) {
    simpleble_error_release(out_error);

    if (handle == nullptr) {
        *out_error = new Error(ErrorCode::INVALID_ARGUMENT, "handle is NULL");
        return nullptr;
    }

    SimpleBLE::Adapter* adapter = (SimpleBLE::Adapter*)handle;
    try {
        return new SimpleBLE::Local::Peripheral(adapter->create_local_peripheral());
    } catch (const SimpleBLE::Exception::BaseException& e) {
        *out_error = e.make_error().release();
    } catch (const std::exception& e) {
        *out_error = new Error(ErrorCode::UNCLASSIFIED_EXCEPTION, e.what());
    } catch (...) {
        *out_error = new Error(ErrorCode::UNCLASSIFIED_EXCEPTION, "Unknown exception");
    }

    return nullptr;
}
