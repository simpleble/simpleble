#include <simplecble/backend.h>
#include <simplecble/error.h>

#include <simpleble/Backend.h>
#include <simpleble/Exceptions.h>

#include <cstdlib>
#include <cstring>

#include "list_handle.hpp"

using SimpleBLE::Error;
using SimpleBLE::ErrorCode;

size_t simpleble_backend_get_backends(simpleble_backend_list_t* out_list, simpleble_error_t** out_error) {
    return SimpleCBLE::List::read(out_list, out_error, []() { return SimpleBLE::Backend::get_backends(); });
}

size_t simpleble_backend_list_count(simpleble_backend_list_t list) {
    return SimpleCBLE::List::count<SimpleBLE::Backend>(list);
}

simpleble_backend_t simpleble_backend_list_get(simpleble_backend_list_t list, size_t index,
                                               simpleble_error_t** out_error) {
    return SimpleCBLE::List::get<SimpleBLE::Backend>(list, index, out_error);
}

void simpleble_backend_list_release(simpleble_backend_list_t list) {
    SimpleCBLE::List::release<SimpleBLE::Backend>(list);
}

// Wraps one of the named backends in an owned handle.
static simpleble_backend_t named_backend_handle(SimpleBLE::Backend (*get_backend)(), simpleble_error_t** out_error) {
    simpleble_error_release(out_error);

    try {
        return (simpleble_backend_t) new SimpleBLE::Backend(get_backend());
    } catch (const SimpleBLE::Exception::BaseException& e) {
        *out_error = e.make_error().release();
    } catch (const std::exception& e) {
        *out_error = new Error(ErrorCode::UNCLASSIFIED_EXCEPTION, e.what());
    } catch (...) {
        *out_error = new Error(ErrorCode::UNCLASSIFIED_EXCEPTION, "Unknown exception");
    }

    return nullptr;
}

simpleble_backend_t simpleble_backend_get_native(simpleble_error_t** out_error) {
    return named_backend_handle(SimpleBLE::Backend::get_native, out_error);
}

simpleble_backend_t simpleble_backend_get_dongl(simpleble_error_t** out_error) {
    return named_backend_handle(SimpleBLE::Backend::get_dongl, out_error);
}

simpleble_backend_t simpleble_backend_get_simulator(simpleble_error_t** out_error) {
    return named_backend_handle(SimpleBLE::Backend::get_simulator, out_error);
}

void simpleble_backend_release_handle(simpleble_backend_t handle) {
    if (handle == nullptr) {
        return;
    }

    SimpleBLE::Backend* backend = (SimpleBLE::Backend*)handle;
    delete backend;
}

char* simpleble_backend_identifier(simpleble_backend_t handle, simpleble_error_t** out_error) {
    simpleble_error_release(out_error);

    if (handle == nullptr) {
        *out_error = new Error(ErrorCode::INVALID_ARGUMENT, "handle is NULL");
        return nullptr;
    }

    SimpleBLE::Backend* backend = (SimpleBLE::Backend*)handle;
    try {
        std::string identifier = backend->identifier();
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

bool simpleble_backend_is_bluetooth_enabled(simpleble_backend_t handle, simpleble_error_t** out_error) {
    simpleble_error_release(out_error);

    if (handle == nullptr) {
        *out_error = new Error(ErrorCode::INVALID_ARGUMENT, "handle is NULL");
        return false;
    }

    SimpleBLE::Backend* backend = (SimpleBLE::Backend*)handle;
    try {
        return backend->bluetooth_enabled();
    } catch (const SimpleBLE::Exception::BaseException& e) {
        *out_error = e.make_error().release();
    } catch (const std::exception& e) {
        *out_error = new Error(ErrorCode::UNCLASSIFIED_EXCEPTION, e.what());
    } catch (...) {
        *out_error = new Error(ErrorCode::UNCLASSIFIED_EXCEPTION, "Unknown exception");
    }

    return false;
}

size_t simpleble_backend_get_adapters(simpleble_backend_t handle, simpleble_adapter_list_t* out_list,
                                      simpleble_error_t** out_error) {
    return SimpleCBLE::List::read<SimpleBLE::Backend>(handle, out_list, out_error,
                                                      [](SimpleBLE::Backend& backend) { return backend.adapters(); });
}
