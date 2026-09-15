#include <simplecble/backend.h>
#include <simplecble/error.h>

#include <simpleble/Backend.h>
#include <simpleble/Exceptions.h>

#include <cstdlib>
#include <cstring>

using SimpleBLE::Error;
using SimpleBLE::ErrorCode;

size_t simpleble_backend_get_count(simpleble_error_t** out_error) {
    simpleble_error_release(out_error);

    try {
        return SimpleBLE::Backend::get_backends().size();
    } catch (const SimpleBLE::Exception::BaseException& e) {
        *out_error = e.make_error().release();
    } catch (const std::exception& e) {
        *out_error = new Error(ErrorCode::UNCLASSIFIED_EXCEPTION, e.what());
    } catch (...) {
        *out_error = new Error(ErrorCode::UNCLASSIFIED_EXCEPTION, "Unknown exception");
    }

    return 0;
}

simpleble_backend_t simpleble_backend_get_handle(size_t index, simpleble_error_t** out_error) {
    simpleble_error_release(out_error);

    try {
        auto backend_list = SimpleBLE::Backend::get_backends();

        if (index >= backend_list.size()) {
            *out_error = new Error(ErrorCode::INVALID_ARGUMENT, "index is out of range");
            return nullptr;
        }

        SimpleBLE::Backend* handle = new SimpleBLE::Backend(backend_list[index]);
        return (simpleble_backend_t)handle;
    } catch (const SimpleBLE::Exception::BaseException& e) {
        *out_error = e.make_error().release();
    } catch (const std::exception& e) {
        *out_error = new Error(ErrorCode::UNCLASSIFIED_EXCEPTION, e.what());
    } catch (...) {
        *out_error = new Error(ErrorCode::UNCLASSIFIED_EXCEPTION, "Unknown exception");
    }

    return nullptr;
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

size_t simpleble_backend_get_adapters_count(simpleble_backend_t handle, simpleble_error_t** out_error) {
    simpleble_error_release(out_error);

    if (handle == nullptr) {
        *out_error = new Error(ErrorCode::INVALID_ARGUMENT, "handle is NULL");
        return 0;
    }

    SimpleBLE::Backend* backend = (SimpleBLE::Backend*)handle;
    try {
        return backend->adapters().size();
    } catch (const SimpleBLE::Exception::BaseException& e) {
        *out_error = e.make_error().release();
    } catch (const std::exception& e) {
        *out_error = new Error(ErrorCode::UNCLASSIFIED_EXCEPTION, e.what());
    } catch (...) {
        *out_error = new Error(ErrorCode::UNCLASSIFIED_EXCEPTION, "Unknown exception");
    }

    return 0;
}

simpleble_adapter_t simpleble_backend_get_adapters_handle(simpleble_backend_t handle, size_t index,
                                                          simpleble_error_t** out_error) {
    simpleble_error_release(out_error);

    if (handle == nullptr) {
        *out_error = new Error(ErrorCode::INVALID_ARGUMENT, "handle is NULL");
        return nullptr;
    }

    SimpleBLE::Backend* backend = (SimpleBLE::Backend*)handle;
    try {
        auto results = backend->adapters();

        if (index >= results.size()) {
            *out_error = new Error(ErrorCode::INVALID_ARGUMENT, "index is out of range");
            return nullptr;
        }

        SimpleBLE::Adapter* adapter_handle = new SimpleBLE::Adapter(results[index]);
        return (simpleble_adapter_t)adapter_handle;
    } catch (const SimpleBLE::Exception::BaseException& e) {
        *out_error = e.make_error().release();
    } catch (const std::exception& e) {
        *out_error = new Error(ErrorCode::UNCLASSIFIED_EXCEPTION, e.what());
    } catch (...) {
        *out_error = new Error(ErrorCode::UNCLASSIFIED_EXCEPTION, "Unknown exception");
    }

    return nullptr;
}
