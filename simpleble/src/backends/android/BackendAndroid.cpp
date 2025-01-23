#include "BackendAndroid.h"
#include "AdapterAndroid.h"
#include "android/BluetoothAdapter.h"
#include "BuildVec.h"
#include "CommonUtils.h"

#include <android/log.h>
#include <fmt/core.h>

#include <string>

namespace SimpleBLE {

std::shared_ptr<BackendAndroid> BACKEND_ANDROID() { return BackendAndroid::get(); }

BackendAndroid::BackendAndroid(buildToken) {}

std::string BackendAndroid::name() const noexcept { return "Android"; }

SharedPtrVector<AdapterBase> BackendAndroid::get_adapters() {
    SharedPtrVector<AdapterBase> adapters;
    adapters.push_back(std::make_shared<AdapterAndroid>(shared_from_this()));

    return adapters;
}

bool BackendAndroid::bluetooth_enabled() {
    Android::BluetoothAdapter btAdapter = Android::BluetoothAdapter::getDefaultAdapter();

    bool isEnabled = btAdapter.isEnabled();
    int bluetoothState = btAdapter.getState();
    __android_log_write(ANDROID_LOG_INFO, "SimpleBLE", fmt::format("Bluetooth state: {}", bluetoothState).c_str());

    return isEnabled;  // bluetoothState == 12;
}

}  // namespace SimpleBLE
