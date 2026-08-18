#include "LocalCharacteristicWindows.h"

#include <algorithm>
#include <utility>

#include <simpleble/Exceptions.h>

#include "CommonUtils.h"
#include "LoggingInternal.h"
#include "MtaManager.h"
#include "Utils.h"

namespace SimpleBLE::Local {

using namespace winrt::Windows::Devices::Bluetooth;
using namespace winrt::Windows::Devices::Bluetooth::GenericAttributeProfile;

namespace {

constexpr uint8_t ATT_ERROR_INVALID_OFFSET = 0x07;
constexpr uint8_t ATT_ERROR_UNLIKELY = 0x0e;

GattCharacteristicProperties properties_from_capabilities(const std::set<CharacteristicCapability>& capabilities) {
    GattCharacteristicProperties properties = GattCharacteristicProperties::None;
    for (const auto capability : capabilities) {
        switch (capability) {
            case CharacteristicCapability::READ:
                properties = properties | GattCharacteristicProperties::Read;
                break;
            case CharacteristicCapability::WRITE_REQUEST:
                properties = properties | GattCharacteristicProperties::Write;
                break;
            case CharacteristicCapability::WRITE_COMMAND:
                properties = properties | GattCharacteristicProperties::WriteWithoutResponse;
                break;
            case CharacteristicCapability::NOTIFY:
                properties = properties | GattCharacteristicProperties::Notify;
                break;
            case CharacteristicCapability::INDICATE:
                properties = properties | GattCharacteristicProperties::Indicate;
                break;
        }
    }
    return properties;
}

}  // namespace

CharacteristicWindows::CharacteristicWindows(const GattLocalService& service, BluetoothUUID uuid,
                                             std::set<CharacteristicCapability> capabilities,
                                             SessionObserver session_observer, ActivityObserver activity_observer,
                                             CallbackObserver callback_observer)
    : _uuid(std::move(uuid)),
      _capabilities(std::move(capabilities)),
      _session_observer(std::move(session_observer)),
      _activity_observer(std::move(activity_observer)),
      _callback_observer(std::move(callback_observer)) {
    if (_capabilities.empty()) {
        throw Exception::OperationFailed("A local characteristic requires at least one capability.");
    }

    auto parameters = GattLocalCharacteristicParameters();
    parameters.CharacteristicProperties(properties_from_capabilities(_capabilities));
    if (_capabilities.count(CharacteristicCapability::READ) != 0) {
        parameters.ReadProtectionLevel(GattProtectionLevel::Plain);
    }
    if (_capabilities.count(CharacteristicCapability::WRITE_REQUEST) != 0 ||
        _capabilities.count(CharacteristicCapability::WRITE_COMMAND) != 0) {
        parameters.WriteProtectionLevel(GattProtectionLevel::Plain);
    }

    auto result = WinRT::MtaManager::get().execute_sync<GattLocalCharacteristicResult>(
        [&]() { return async_get(service.CreateCharacteristicAsync(uuid_to_guid(_uuid), parameters)); });
    if (result.Error() != BluetoothError::Success || !result.Characteristic()) {
        throw Exception::OperationFailed(fmt::format("Failed to create local characteristic {} (WinRT error {}).",
                                                     _uuid, static_cast<int32_t>(result.Error())));
    }

    _characteristic = result.Characteristic();
}

void CharacteristicWindows::initialize_handlers() {
    auto weak_self = weak_from_this();
    _read_requested_token_ = _characteristic.ReadRequested(
        [weak_self](const GattLocalCharacteristic& sender, const GattReadRequestedEventArgs& args) {
            if (auto self = weak_self.lock()) {
                self->_on_read_requested(sender, args);
            }
        });
    _write_requested_token_ = _characteristic.WriteRequested(
        [weak_self](const GattLocalCharacteristic& sender, const GattWriteRequestedEventArgs& args) {
            if (auto self = weak_self.lock()) {
                self->_on_write_requested(sender, args);
            }
        });
    _subscribed_clients_changed_token_ = _characteristic.SubscribedClientsChanged(
        [weak_self](const GattLocalCharacteristic& sender, const winrt::Windows::Foundation::IInspectable& args) {
            if (auto self = weak_self.lock()) {
                self->_on_subscribed_clients_changed(sender, args);
            }
        });
}

CharacteristicWindows::~CharacteristicWindows() {
    _callback_on_read.unload();
    _callback_on_write.unload();
    _callback_on_subscribed.unload();
    _callback_on_unsubscribed.unload();

    if (_characteristic) {
        if (_read_requested_token_) {
            try {
                _characteristic.ReadRequested(_read_requested_token_);
            } catch (...) {
            }
        }
        if (_write_requested_token_) {
            try {
                _characteristic.WriteRequested(_write_requested_token_);
            } catch (...) {
            }
        }
        if (_subscribed_clients_changed_token_) {
            try {
                _characteristic.SubscribedClientsChanged(_subscribed_clients_changed_token_);
            } catch (...) {
            }
        }
    }
}

BluetoothUUID CharacteristicWindows::uuid() { return _uuid; }

std::set<CharacteristicCapability> CharacteristicWindows::capabilities() { return _capabilities; }

ByteArray CharacteristicWindows::value() {
    std::scoped_lock lock(_value_mutex);
    return _value;
}

void CharacteristicWindows::set_value(ByteArray value) {
    ByteArray value_snapshot;
    {
        std::scoped_lock lock(_value_mutex);
        _value = std::move(value);
        value_snapshot = _value;
    }

    const bool can_publish = _capabilities.count(CharacteristicCapability::NOTIFY) != 0 ||
                             _capabilities.count(CharacteristicCapability::INDICATE) != 0;
    if (!can_publish) {
        return;
    }

    uint64_t subscription_generation;
    {
        std::scoped_lock lock(_subscription_mutex);
        if (_subscribed_clients == 0) {
            return;
        }
        subscription_generation = _subscription_generation;
    }
    if (!_activity_observer || _activity_observer() != subscription_generation) {
        return;
    }

    try {
        WinRT::MtaManager::get().execute_sync([this, value_snapshot]() {
            async_get(_characteristic.NotifyValueAsync(bytearray_to_ibuffer(value_snapshot)));
        });
    } catch (const std::exception& ex) {
        SIMPLEBLE_LOG_WARN(fmt::format("Failed to publish local characteristic {}: {}", _uuid, ex.what()));
    }
}

void CharacteristicWindows::set_callback_on_read(std::function<ByteArray()> on_read) {
    if (on_read) {
        _callback_on_read.load(std::move(on_read));
    } else {
        _callback_on_read.unload();
    }
}

void CharacteristicWindows::set_callback_on_write(std::function<void(ByteArray)> on_write) {
    if (on_write) {
        _callback_on_write.load(std::move(on_write));
    } else {
        _callback_on_write.unload();
    }
}

void CharacteristicWindows::set_callback_on_subscribed(std::function<void()> on_subscribed) {
    if (on_subscribed) {
        _callback_on_subscribed.load(std::move(on_subscribed));
    } else {
        _callback_on_subscribed.unload();
    }
}

void CharacteristicWindows::set_callback_on_unsubscribed(std::function<void()> on_unsubscribed) {
    if (on_unsubscribed) {
        _callback_on_unsubscribed.load(std::move(on_unsubscribed));
    } else {
        _callback_on_unsubscribed.unload();
    }
}

void CharacteristicWindows::reset_subscriptions() {
    std::scoped_lock lock(_subscription_mutex);
    _subscribed_clients = 0;
    _subscription_generation = 0;
    _subscription_callbacks_enabled = false;
}

void CharacteristicWindows::reconcile_subscriptions(uint64_t generation) noexcept {
    // Re-read the native CCCD state after every start. WinRT may retain subscribed clients across
    // StopAdvertising/StartAdvertising without emitting another change event.
    _on_subscribed_clients_changed(_characteristic, nullptr, generation);
}

void CharacteristicWindows::enable_subscription_callbacks(uint64_t generation) {
    bool notify_subscribed = false;
    {
        std::scoped_lock lock(_subscription_mutex);
        if (_subscription_generation == generation && !_subscription_callbacks_enabled && _callback_observer &&
            _callback_observer(generation)) {
            _subscription_callbacks_enabled = true;
            notify_subscribed = _subscribed_clients > 0;
        }
    }
    if (notify_subscribed) {
        SAFE_CALLBACK_CALL(_callback_on_subscribed);
    }
}

void CharacteristicWindows::_on_read_requested(const GattLocalCharacteristic&, const GattReadRequestedEventArgs& args) {
    auto deferral = args.GetDeferral();
    GattReadRequest request{nullptr};
    try {
        if (_session_observer) {
            _session_observer(args.Session(), 0);
        }

        request = async_get(args.GetRequestAsync());
        if (!request) {
            deferral.Complete();
            return;
        }

        const uint64_t generation = _activity_observer ? _activity_observer() : 0;
        if (generation == 0) {
            request.RespondWithProtocolError(ATT_ERROR_UNLIKELY);
            deferral.Complete();
            return;
        }

        ByteArray response;
        if (_activity_observer && _activity_observer() == generation && _callback_on_read) {
            response = _callback_on_read();
            std::scoped_lock lock(_value_mutex);
            _value = response;
        } else {
            std::scoped_lock lock(_value_mutex);
            response = _value;
        }

        const size_t offset = request.Offset();
        if (offset > response.size()) {
            request.RespondWithProtocolError(ATT_ERROR_INVALID_OFFSET);
        } else {
            const size_t response_size = std::min(response.size() - offset, static_cast<size_t>(request.Length()));
            request.RespondWithValue(bytearray_to_ibuffer(response.slice(offset, offset + response_size)));
        }
    } catch (const std::exception& ex) {
        SIMPLEBLE_LOG_ERROR(fmt::format("Exception while handling local characteristic {} read: {}", _uuid, ex.what()));
        try {
            if (request) {
                request.RespondWithProtocolError(ATT_ERROR_UNLIKELY);
            }
        } catch (...) {
        }
    } catch (...) {
        SIMPLEBLE_LOG_ERROR(fmt::format("Unknown exception while handling local characteristic {} read", _uuid));
        try {
            if (request) {
                request.RespondWithProtocolError(ATT_ERROR_UNLIKELY);
            }
        } catch (...) {
        }
    }
    deferral.Complete();
}

void CharacteristicWindows::_on_write_requested(const GattLocalCharacteristic&,
                                                const GattWriteRequestedEventArgs& args) {
    auto deferral = args.GetDeferral();
    GattWriteRequest request{nullptr};
    bool should_respond = false;
    try {
        if (_session_observer) {
            _session_observer(args.Session(), 0);
        }

        request = async_get(args.GetRequestAsync());
        if (!request) {
            deferral.Complete();
            return;
        }

        should_respond = request.Option() == GattWriteOption::WriteWithResponse;
        const uint64_t generation = _activity_observer ? _activity_observer() : 0;
        if (generation == 0) {
            if (should_respond) {
                request.RespondWithProtocolError(ATT_ERROR_UNLIKELY);
            }
            deferral.Complete();
            return;
        }
        const ByteArray incoming = ibuffer_to_bytearray(request.Value());
        const size_t offset = request.Offset();
        ByteArray updated;
        {
            std::scoped_lock lock(_value_mutex);
            if (offset > _value.size()) {
                if (should_respond) {
                    request.RespondWithProtocolError(ATT_ERROR_INVALID_OFFSET);
                }
                deferral.Complete();
                return;
            }

            if (offset == 0) {
                updated = incoming;
            } else {
                updated = _value;
                updated.resize(std::max(updated.size(), offset + incoming.size()));
                std::copy(incoming.begin(), incoming.end(), updated.begin() + offset);
            }
            _value = updated;
        }

        if (_activity_observer && _activity_observer() == generation) {
            SAFE_CALLBACK_CALL(_callback_on_write, updated);
        }
        if (should_respond) {
            request.Respond();
        }
    } catch (const std::exception& ex) {
        SIMPLEBLE_LOG_ERROR(
            fmt::format("Exception while handling local characteristic {} write: {}", _uuid, ex.what()));
        try {
            if (request && should_respond) {
                request.RespondWithProtocolError(ATT_ERROR_UNLIKELY);
            }
        } catch (...) {
        }
    } catch (...) {
        SIMPLEBLE_LOG_ERROR(fmt::format("Unknown exception while handling local characteristic {} write", _uuid));
        try {
            if (request && should_respond) {
                request.RespondWithProtocolError(ATT_ERROR_UNLIKELY);
            }
        } catch (...) {
        }
    }
    deferral.Complete();
}

void CharacteristicWindows::_on_subscribed_clients_changed(const GattLocalCharacteristic& sender,
                                                           const winrt::Windows::Foundation::IInspectable&,
                                                           uint64_t expected_generation) {
    try {
        const uint64_t generation = _activity_observer ? _activity_observer() : 0;
        if (generation == 0 || (expected_generation != 0 && generation != expected_generation)) {
            return;
        }

        const auto clients = sender.SubscribedClients();
        for (const auto& client : clients) {
            if (_session_observer) {
                _session_observer(client.Session(), generation);
            }
        }

        bool notify = false;
        bool subscribed = false;
        {
            std::scoped_lock lock(_subscription_mutex);
            if (!_activity_observer || _activity_observer() != generation ||
                (expected_generation != 0 && generation != expected_generation)) {
                return;
            }

            const bool was_subscribed = _subscription_generation == generation && _subscribed_clients > 0;
            _subscribed_clients = clients.Size();
            _subscription_generation = generation;
            subscribed = _subscribed_clients > 0;
            notify = _subscription_callbacks_enabled && _callback_observer && _callback_observer(generation) &&
                     was_subscribed != subscribed;
        }

        if (notify) {
            if (subscribed) {
                SAFE_CALLBACK_CALL(_callback_on_subscribed);
            } else {
                SAFE_CALLBACK_CALL(_callback_on_unsubscribed);
            }
        }
    } catch (const std::exception& ex) {
        SIMPLEBLE_LOG_ERROR(
            fmt::format("Exception while handling local characteristic {} subscriptions: {}", _uuid, ex.what()));
    } catch (...) {
        SIMPLEBLE_LOG_ERROR(
            fmt::format("Unknown exception while handling local characteristic {} subscriptions", _uuid));
    }
}

}  // namespace SimpleBLE::Local
