#include "BackendAndroid.h"
#include "AdapterAndroid.h"
#include "BuildVec.h"
#include "CommonUtils.h"
#include "types/android/bluetooth/BluetoothAdapter.h"
#include "simplejni/Registry.hpp"

#include <android/log.h>
#include <fmt/core.h>

#include <mutex>
#include <stdexcept>
#include <string>

namespace SimpleBLE {
namespace {

std::mutex context_mutex;
SimpleJNI::Object<SimpleJNI::GlobalRef, jobject> stored_application_context;

}  // namespace

std::shared_ptr<BackendAndroid> BACKEND_ANDROID() { return BackendAndroid::get(); }

BackendAndroid::BackendAndroid(buildToken) {
    SimpleJNI::Registrar::get().preload(SimpleJNI::VM::env());

    _adapter = std::make_shared<AdapterAndroid>();
}

std::string BackendAndroid::identifier() const noexcept { return "Android"; }

SharedPtrVector<AdapterBase> BackendAndroid::adapters() {
    SharedPtrVector<AdapterBase> adapter_list;
    adapter_list.push_back(_adapter);
    return adapter_list;
}

bool BackendAndroid::bluetooth_enabled() {
    Android::BluetoothAdapter btAdapter = Android::BluetoothAdapter::getDefaultAdapter();

    bool isEnabled = btAdapter.isEnabled();
    int bluetoothState = btAdapter.getState();
    __android_log_write(ANDROID_LOG_INFO, "SimpleBLE", fmt::format("Bluetooth state: {}", bluetoothState).c_str());

    return isEnabled;  // bluetoothState == 12;
}

void BackendAndroid::set_application_context(jobject context) {
    if (context == nullptr) {
        throw std::invalid_argument("Android application context cannot be null.");
    }

    std::scoped_lock lock(context_mutex);
    stored_application_context = SimpleJNI::Object<SimpleJNI::GlobalRef, jobject>(context);
}

Android::Context BackendAndroid::application_context() {
    std::scoped_lock lock(context_mutex);
    if (!stored_application_context) {
        throw std::runtime_error(
            "Android peripheral mode requires an application context. "
            "Call SimpleBLE::Advanced::Android::set_context() before creating a local peripheral.");
    }
    return Android::Context(stored_application_context);
}

}  // namespace SimpleBLE
