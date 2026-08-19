#include "LocalServiceWindows.h"

#include <chrono>
#include <utility>

#include <fmt/format.h>
#include <simpleble/Exceptions.h>

#include "LocalCharacteristicWindows.h"
#include "MtaManager.h"
#include "Utils.h"

namespace SimpleBLE::Local {

using namespace winrt::Windows::Devices::Bluetooth;
using namespace winrt::Windows::Devices::Bluetooth::GenericAttributeProfile;

ServiceWindows::ServiceWindows(BluetoothUUID uuid) : _uuid(std::move(uuid)) {}

ServiceWindows::~ServiceWindows() {
    try {
        stop_advertising();
    } catch (...) {
    }
    destroy_native();
}

BluetoothUUID ServiceWindows::uuid() { return _uuid; }

std::shared_ptr<CharacteristicBase> ServiceWindows::add_characteristic(
    BluetoothUUID uuid, std::set<CharacteristicCapability> capabilities) {
    std::scoped_lock lock(_mutex);
    if (_frozen) {
        throw Exception::OperationFailed("The local service cannot be changed while its peripheral is started.");
    }

    auto characteristic = std::make_shared<CharacteristicWindows>(std::move(uuid), std::move(capabilities));
    _characteristics.push_back(characteristic);
    return characteristic;
}

std::vector<std::shared_ptr<CharacteristicBase>> ServiceWindows::characteristics() {
    std::scoped_lock lock(_mutex);
    return {_characteristics.begin(), _characteristics.end()};
}

void ServiceWindows::freeze() {
    std::scoped_lock lock(_mutex);
    _frozen = true;
}

void ServiceWindows::unfreeze() {
    std::scoped_lock lock(_mutex);
    _frozen = false;
}

void ServiceWindows::create_native(SessionObserver session_observer, ActivityObserver activity_observer) {
    std::vector<std::shared_ptr<CharacteristicWindows>> characteristics;
    {
        std::scoped_lock lock(_mutex);
        if (_native) {
            throw Exception::OperationFailed(fmt::format("Local service {} is already active.", _uuid));
        }
        characteristics = _characteristics;
    }

    auto result = WinRT::MtaManager::get().execute_sync<GattServiceProviderResult>(
        [this]() { return async_get(GattServiceProvider::CreateAsync(uuid_to_guid(_uuid))); });
    if (result.Error() != BluetoothError::Success || !result.ServiceProvider()) {
        throw Exception::OperationFailed(fmt::format("Failed to create local service {} (WinRT error {}).", _uuid,
                                                     static_cast<int32_t>(result.Error())));
    }

    auto native = std::make_shared<NativeState>();
    native->provider = result.ServiceProvider();
    native->advertisement_status = native->provider.AdvertisementStatus();
    native->activity_observer = activity_observer;

    try {
        for (const auto& characteristic : characteristics) {
            characteristic->start_native(native->provider.Service(), session_observer, activity_observer);
        }
    } catch (...) {
        for (const auto& characteristic : characteristics) {
            characteristic->stop_native();
        }
        throw;
    }

    std::scoped_lock lock(_mutex);
    _native = std::move(native);
}

void ServiceWindows::destroy_native() noexcept {
    std::shared_ptr<NativeState> native;
    {
        std::scoped_lock lock(_mutex);
        native.swap(_native);
        for (const auto& characteristic : _characteristics) {
            characteristic->stop_native();
        }
    }
    _remove_advertisement_handler(native);
}

void ServiceWindows::start_advertising() {
    const auto native = _native_snapshot();
    if (!native || !native->active()) {
        throw Exception::OperationFailed(fmt::format("Local service {} is not active.", _uuid));
    }

    auto parameters = GattServiceProviderAdvertisingParameters();
    parameters.IsConnectable(true);
    parameters.IsDiscoverable(true);
    {
        std::scoped_lock lock(native->advertisement_mutex);
        native->advertisement_status = AdvertisementStatus::Created;
        native->advertisement_error = BluetoothError::Success;
        native->advertising_requested = true;
    }

    std::weak_ptr<NativeState> weak_native = native;
    try {
        _remove_advertisement_handler(native);
        native->advertisement_status_changed_token = native->provider.AdvertisementStatusChanged(
            [weak_native](const GattServiceProvider&,
                          const GattServiceProviderAdvertisementStatusChangedEventArgs& args) {
                if (auto state = weak_native.lock()) {
                    if (!state->active()) {
                        return;
                    }
                    {
                        std::scoped_lock lock(state->advertisement_mutex);
                        state->advertisement_status = args.Status();
                        state->advertisement_error = args.Error();
                    }
                    state->advertisement_cv.notify_all();
                }
            });
        native->provider.StartAdvertising(parameters);
    } catch (...) {
        {
            std::scoped_lock lock(native->advertisement_mutex);
            native->advertising_requested = false;
        }
        _remove_advertisement_handler(native);
        throw;
    }
}

void ServiceWindows::wait_until_advertising() {
    const auto native = _native_snapshot();
    if (!native) {
        throw Exception::OperationFailed(fmt::format("Local service {} is not active.", _uuid));
    }

    std::unique_lock lock(native->advertisement_mutex);
    // WinRT can report a transient Aborted state before recovering to Started. Keep waiting for success and only
    // interpret the last observed state as a failure after the timeout.
    native->advertisement_cv.wait_for(lock, std::chrono::seconds(5), [&native]() {
        return native->advertisement_status == AdvertisementStatus::Started ||
               native->advertisement_status == AdvertisementStatus::StartedWithoutAllAdvertisementData;
    });
    if (!native->active()) {
        throw Exception::OperationFailed(fmt::format("Advertising local service {} was cancelled.", _uuid));
    }
    if (native->advertisement_status == AdvertisementStatus::Aborted) {
        const auto error = native->advertisement_error;
        throw Exception::OperationFailed(fmt::format("Windows aborted advertising local service {} (WinRT error {}).",
                                                     _uuid, static_cast<int32_t>(error)));
    }
    if (native->advertisement_status != AdvertisementStatus::Started &&
        native->advertisement_status != AdvertisementStatus::StartedWithoutAllAdvertisementData) {
        const auto status = native->advertisement_status;
        const auto error = native->advertisement_error;
        throw Exception::OperationFailed(
            fmt::format("Timed out while starting advertisement for local service {} (status {}, WinRT error {}).",
                        _uuid, static_cast<int32_t>(status), static_cast<int32_t>(error)));
    }
}

void ServiceWindows::stop_advertising() {
    const auto native = _native_snapshot();
    if (!native) {
        return;
    }
    {
        std::scoped_lock lock(native->advertisement_mutex);
        if (!native->advertising_requested) {
            return;
        }
    }

    native->provider.StopAdvertising();
    {
        std::scoped_lock lock(native->advertisement_mutex);
        native->advertisement_status = AdvertisementStatus::Stopped;
        native->advertisement_error = BluetoothError::Success;
        native->advertising_requested = false;
    }
    _remove_advertisement_handler(native);
}

bool ServiceWindows::is_advertising() const {
    const auto native = _native_snapshot();
    if (!native) {
        return false;
    }
    std::scoped_lock lock(native->advertisement_mutex);
    return native->advertisement_status == AdvertisementStatus::Started ||
           native->advertisement_status == AdvertisementStatus::StartedWithoutAllAdvertisementData;
}

std::shared_ptr<ServiceWindows::NativeState> ServiceWindows::_native_snapshot() const {
    std::scoped_lock lock(_mutex);
    return _native;
}

void ServiceWindows::_remove_advertisement_handler(const std::shared_ptr<NativeState>& native) noexcept {
    if (!native || !native->provider || !native->advertisement_status_changed_token) {
        return;
    }
    try {
        native->provider.AdvertisementStatusChanged(native->advertisement_status_changed_token);
    } catch (...) {
    }
    native->advertisement_status_changed_token = {};
}

}  // namespace SimpleBLE::Local
