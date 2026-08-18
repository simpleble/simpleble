#include "LocalServiceWindows.h"

#include <chrono>
#include <utility>

#include <fmt/format.h>
#include <simpleble/Exceptions.h>

#include "LocalCharacteristicWindows.h"
#include "LoggingInternal.h"
#include "MtaManager.h"
#include "Utils.h"

namespace SimpleBLE::Local {

using namespace winrt::Windows::Devices::Bluetooth;
using namespace winrt::Windows::Devices::Bluetooth::GenericAttributeProfile;

ServiceWindows::ServiceWindows(BluetoothUUID uuid, SessionObserver session_observer, ActivityObserver activity_observer,
                               CallbackObserver callback_observer)
    : _uuid(std::move(uuid)),
      _session_observer(std::move(session_observer)),
      _activity_observer(std::move(activity_observer)),
      _callback_observer(std::move(callback_observer)) {
    auto result = WinRT::MtaManager::get().execute_sync<GattServiceProviderResult>(
        [this]() { return async_get(GattServiceProvider::CreateAsync(uuid_to_guid(_uuid))); });
    if (result.Error() != BluetoothError::Success || !result.ServiceProvider()) {
        throw Exception::OperationFailed(fmt::format("Failed to create local service {} (WinRT error {}).", _uuid,
                                                     static_cast<int32_t>(result.Error())));
    }
    _provider = result.ServiceProvider();
    _advertisement_status = _provider.AdvertisementStatus();
}

void ServiceWindows::initialize_handlers() {
    auto weak_self = weak_from_this();
    _advertisement_status_changed_token = _provider.AdvertisementStatusChanged(
        [weak_self](const GattServiceProvider&, const GattServiceProviderAdvertisementStatusChangedEventArgs& args) {
            if (auto self = weak_self.lock()) {
                {
                    std::scoped_lock lock(self->_advertisement_mutex);
                    self->_advertisement_status = args.Status();
                    self->_advertisement_error = args.Error();
                }
                self->_advertisement_cv.notify_all();
            }
        });
}

ServiceWindows::~ServiceWindows() {
    try {
        if (_provider) {
            stop_advertising();
            if (_advertisement_status_changed_token) {
                _provider.AdvertisementStatusChanged(_advertisement_status_changed_token);
            }
        }
    } catch (...) {
    }
    std::scoped_lock lock(_mutex);
    _characteristics.clear();
}

BluetoothUUID ServiceWindows::uuid() { return _uuid; }

std::shared_ptr<CharacteristicBase> ServiceWindows::add_characteristic(
    BluetoothUUID uuid, std::set<CharacteristicCapability> capabilities) {
    std::scoped_lock lock(_mutex);
    if (_frozen) {
        throw Exception::OperationFailed("The local service cannot be changed while its peripheral is started.");
    }

    auto weak_self = weak_from_this();
    auto characteristic = std::make_shared<CharacteristicWindows>(
        _provider.Service(), std::move(uuid), std::move(capabilities),
        [weak_self](const GattSession& session, uint64_t expected_generation) {
            if (auto self = weak_self.lock()) {
                const uint64_t generation = self->_current_generation();
                if (generation != 0 && (expected_generation == 0 || expected_generation == generation) &&
                    self->_session_observer) {
                    self->_session_observer(session, generation);
                }
            }
        },
        [weak_self]() -> uint64_t {
            if (auto self = weak_self.lock()) {
                return self->_current_generation();
            }
            return 0;
        },
        [weak_self](uint64_t generation) {
            if (auto self = weak_self.lock()) {
                return self->_current_generation() == generation && self->_callback_observer &&
                       self->_callback_observer(generation);
            }
            return false;
        });
    characteristic->initialize_handlers();
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

void ServiceWindows::start_advertising() {
    auto parameters = GattServiceProviderAdvertisingParameters();
    parameters.IsConnectable(true);
    parameters.IsDiscoverable(true);
    {
        std::scoped_lock lock(_advertisement_mutex);
        _advertisement_status = GattServiceProviderAdvertisementStatus::Created;
        _advertisement_error = BluetoothError::Success;
        _advertising_requested = true;
    }
    try {
        _provider.StartAdvertising(parameters);
    } catch (...) {
        std::scoped_lock lock(_advertisement_mutex);
        _advertising_requested = false;
        throw;
    }
}

void ServiceWindows::activate(uint64_t generation) { _active_generation.store(generation); }

void ServiceWindows::deactivate() noexcept { _active_generation.store(0); }

void ServiceWindows::wait_until_advertising() {
    std::unique_lock lock(_advertisement_mutex);
    // WinRT can report a transient Aborted state before recovering to Started. Keep waiting for success and only
    // interpret the last observed state as a failure after the timeout.
    _advertisement_cv.wait_for(lock, std::chrono::seconds(5), [this]() {
        return _advertisement_status == GattServiceProviderAdvertisementStatus::Started ||
               _advertisement_status == GattServiceProviderAdvertisementStatus::StartedWithoutAllAdvertisementData;
    });
    if (_advertisement_status == GattServiceProviderAdvertisementStatus::Aborted) {
        const auto error = _advertisement_error;
        throw Exception::OperationFailed(fmt::format("Windows aborted advertising local service {} (WinRT error {}).",
                                                     _uuid, static_cast<int32_t>(error)));
    }
    if (_advertisement_status != GattServiceProviderAdvertisementStatus::Started &&
        _advertisement_status != GattServiceProviderAdvertisementStatus::StartedWithoutAllAdvertisementData) {
        const auto status = _advertisement_status;
        const auto error = _advertisement_error;
        throw Exception::OperationFailed(
            fmt::format("Timed out while starting advertisement for local service {} (status {}, WinRT error {}).",
                        _uuid, static_cast<int32_t>(status), static_cast<int32_t>(error)));
    }
}

void ServiceWindows::stop_advertising() {
    {
        std::scoped_lock lock(_advertisement_mutex);
        if (!_advertising_requested) {
            return;
        }
    }

    _provider.StopAdvertising();
    // StopAdvertising is a synchronous void operation. Some Windows stacks do not emit a follow-up Stopped status
    // event, so a successful return is the completion boundary for the local lifecycle.
    std::scoped_lock lock(_advertisement_mutex);
    _advertisement_status = GattServiceProviderAdvertisementStatus::Stopped;
    _advertisement_error = BluetoothError::Success;
    _advertising_requested = false;
}

void ServiceWindows::reset_subscriptions() {
    std::vector<std::shared_ptr<CharacteristicWindows>> characteristics;
    {
        std::scoped_lock lock(_mutex);
        characteristics = _characteristics;
    }
    for (const auto& characteristic : characteristics) {
        characteristic->reset_subscriptions();
    }
}

void ServiceWindows::reconcile_subscriptions(uint64_t generation) noexcept {
    if (_current_generation() != generation) {
        return;
    }
    try {
        std::scoped_lock lock(_mutex);
        for (const auto& characteristic : _characteristics) {
            characteristic->reconcile_subscriptions(generation);
        }
    } catch (const std::exception& ex) {
        SIMPLEBLE_LOG_WARN(fmt::format("Failed to reconcile subscriptions for Windows local service {}: {}", _uuid,
                                       ex.what()));
    } catch (...) {
        SIMPLEBLE_LOG_WARN(
            fmt::format("Failed to reconcile subscriptions for Windows local service {}", _uuid));
    }
}

void ServiceWindows::enable_subscription_callbacks(uint64_t generation) {
    if (_current_generation() != generation) {
        return;
    }
    std::vector<std::shared_ptr<CharacteristicWindows>> characteristics;
    {
        std::scoped_lock lock(_mutex);
        characteristics = _characteristics;
    }
    for (const auto& characteristic : characteristics) {
        characteristic->enable_subscription_callbacks(generation);
    }
}

bool ServiceWindows::is_advertising() const {
    std::scoped_lock lock(_advertisement_mutex);
    return _advertisement_status == GattServiceProviderAdvertisementStatus::Started ||
           _advertisement_status == GattServiceProviderAdvertisementStatus::StartedWithoutAllAdvertisementData;
}

uint64_t ServiceWindows::_current_generation() {
    const uint64_t generation = _active_generation.load();
    if (generation == 0 || !_activity_observer || _activity_observer() != generation) {
        return 0;
    }
    return generation;
}

}  // namespace SimpleBLE::Local
