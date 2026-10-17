#include "BackendWinRT.h"

#include "AdapterWindows.h"
#include "CommonUtils.h"

#include "winrt/Windows.Devices.Bluetooth.h"
#include "winrt/Windows.Devices.Enumeration.h"
#include "winrt/Windows.Devices.Radios.h"
#include "winrt/Windows.Foundation.Collections.h"

#include "simpleble/Config.h"
#include "Utils.h"
#include "MtaManager.h"

namespace SimpleBLE {

using namespace SimpleBLE::WinRT;

std::shared_ptr<BackendBase> BACKEND_WINDOWS() { return BackendWinRT::get(); }

BackendWinRT::BackendWinRT(buildToken) {
    if (Config::WinRT::experimental_reinitialize_winrt_apartment_on_main_thread) {
        initialize_winrt();
    }
 }

bool BackendWinRT::bluetooth_enabled() {
    return MtaManager::get().execute_sync<bool>([this]() {
        bool has_bluetooth_radio = false;
        auto radio_collection = async_get_radio([]() { return Radio::GetRadiosAsync(); });
        if (radio_collection) {
            for (const auto& radio : radio_collection) {
                if (radio.Kind() != RadioKind::Bluetooth) {
                    continue;
                }
                has_bluetooth_radio = true;
                if (radio.State() == RadioState::On) {
                    return true;
                }
            }
        }

        // Preserve a known disabled state. An unavailable radio API (or an empty
        // WoW64 radio list) is not evidence that an existing BLE adapter is off.
        if (has_bluetooth_radio) return false;
        auto device_information_collection = async_get(
            Devices::Enumeration::DeviceInformation::FindAllAsync(BluetoothAdapter::GetDeviceSelector()));
        return device_information_collection.Size() > 0;
    });
}

SharedPtrVector<AdapterBase> BackendWinRT::adapters() {
    return MtaManager::get().execute_sync<SharedPtrVector<AdapterBase>>([this]() {
        auto device_selector = BluetoothAdapter::GetDeviceSelector();
        auto device_information_collection = async_get(
            Devices::Enumeration::DeviceInformation::FindAllAsync(device_selector));

        SharedPtrVector<AdapterBase> adapter_list;
        for (uint32_t i = 0; i < device_information_collection.Size(); i++) {
            auto dev_info = device_information_collection.GetAt(i);
            adapter_list.push_back(std::make_shared<AdapterWindows>(winrt::to_string(dev_info.Id())));
        }
        return adapter_list;
    });
}

std::string BackendWinRT::identifier() const noexcept { return "WinRT"; }

};  // namespace SimpleBLE
