#include <vector>

#include <simpleble/Adapter.h>
#include <simpleble/Exceptions.h>

#include "BackendBase.h"
#include "BuildVec.h"
#include "CommonUtils.h"

#include <simpleble/Backend.h>

using namespace SimpleBLE;

namespace SimpleBLE {

using BackendPtr = std::shared_ptr<BackendBase>(void);

// NOTE: Plain builds replace the operating system's backend, and leave out the Dongl and Simulator backends.
static std::shared_ptr<BackendBase> _native_backend() {
    if constexpr (SIMPLEBLE_BACKEND_LINUX) {
        extern BackendPtr BACKEND_LINUX;
        return BACKEND_LINUX();
    } else if constexpr (SIMPLEBLE_BACKEND_WINDOWS) {
        extern BackendPtr BACKEND_WINDOWS;
        return BACKEND_WINDOWS();
    } else if constexpr (SIMPLEBLE_BACKEND_ANDROID) {
        extern BackendPtr BACKEND_ANDROID;
        return BACKEND_ANDROID();
    } else if constexpr (SIMPLEBLE_BACKEND_MACOS || SIMPLEBLE_BACKEND_IOS) {
        extern BackendPtr BACKEND_MACOS;
        return BACKEND_MACOS();
    } else if constexpr (SIMPLEBLE_BACKEND_PLAIN) {
        extern BackendPtr BACKEND_PLAIN;
        return BACKEND_PLAIN();
    } else {
        throw Exception::OperationNotSupported();
    }
}

static std::shared_ptr<BackendBase> _dongl_backend() {
    if constexpr (SIMPLEBLE_BACKEND_PLAIN) {
        throw Exception::OperationNotSupported();
    } else {
        extern BackendPtr BACKEND_DONGL;
        return BACKEND_DONGL();
    }
}

static std::shared_ptr<BackendBase> _simulator_backend() {
    if constexpr (SIMPLEBLE_BACKEND_PLAIN) {
        throw Exception::OperationNotSupported();
    } else {
        extern BackendPtr BACKEND_SIMULATOR;
        return BACKEND_SIMULATOR();
    }
}

static SharedPtrVector<BackendBase> _get_backends() {
    SharedPtrVector<BackendBase> backends;

    auto add_backend = [&](std::shared_ptr<BackendBase> backend) {
        if (backend->is_active()) {
            backends.push_back(backend);
        }
    };

    add_backend(_native_backend());
    if constexpr (!SIMPLEBLE_BACKEND_PLAIN) {
        add_backend(_dongl_backend());
        add_backend(_simulator_backend());
    }

    return backends;
}

}  // namespace SimpleBLE

std::vector<Backend> Backend::get_backends() { return Factory::vector(_get_backends()); }

Backend Backend::get_native() { return Factory::build(_native_backend()); }

Backend Backend::get_dongl() { return Factory::build(_dongl_backend()); }

Backend Backend::get_simulator() { return Factory::build(_simulator_backend()); }

bool Backend::initialized() const { return internal_ != nullptr; }

BackendBase* Backend::operator->() {
    if (!initialized()) {
        throw Exception::NotInitialized();
    }
    return internal_.get();
}

const BackendBase* Backend::operator->() const {
    if (!initialized()) {
        throw Exception::NotInitialized();
    }
    return internal_.get();
}

std::vector<Adapter> Backend::adapters() { return Factory::vector((*this)->adapters()); }

bool Backend::bluetooth_enabled() { return (*this)->bluetooth_enabled(); }

std::string Backend::identifier() const noexcept { return (*this)->identifier(); }
