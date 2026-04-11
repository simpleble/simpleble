#include <vector>

#include <simpleble/Adapter.h>
#include <simpleble/Config.h>
#include <simpleble/Exceptions.h>

#include "BackendBase.h"
#include "BuildVec.h"
#include "CommonUtils.h"

#include <simpleble/Backend.h>

using namespace SimpleBLE;

namespace SimpleBLE {

static SharedPtrVector<BackendBase> _get_backends() {
    SharedPtrVector<BackendBase> backends;
    using BackendPtr = std::shared_ptr<BackendBase>(void);

    auto add_backend = [&](std::shared_ptr<BackendBase> backend) {
        if (backend->is_active()) {
            backends.push_back(backend);
        }
    };

    if constexpr (SIMPLEBLE_BACKEND_LINUX) {
        extern BackendPtr BACKEND_LINUX;
        extern BackendPtr BACKEND_LINUX_LEGACY;
        add_backend(BACKEND_LINUX());
        add_backend(BACKEND_LINUX_LEGACY());
    } else if constexpr (SIMPLEBLE_BACKEND_WINDOWS) {
        extern BackendPtr BACKEND_WINDOWS;
        add_backend(BACKEND_WINDOWS());
    } else if constexpr (SIMPLEBLE_BACKEND_ANDROID) {
        extern BackendPtr BACKEND_ANDROID;
        add_backend(BACKEND_ANDROID());
    } else if constexpr (SIMPLEBLE_BACKEND_MACOS) {
        extern BackendPtr BACKEND_MACOS;
        add_backend(BACKEND_MACOS());
    } else if constexpr (SIMPLEBLE_BACKEND_IOS) {
        extern BackendPtr BACKEND_MACOS;
        add_backend(BACKEND_MACOS());
    } else if constexpr (SIMPLEBLE_BACKEND_PLAIN) {
        extern BackendPtr BACKEND_PLAIN;
        add_backend(BACKEND_PLAIN());
    }

    extern BackendPtr BACKEND_DONGL;
    add_backend(BACKEND_DONGL());

    return backends;
}

}  // namespace SimpleBLE

std::vector<Backend> Backend::get_backends() { return Factory::vector(_get_backends()); }

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
