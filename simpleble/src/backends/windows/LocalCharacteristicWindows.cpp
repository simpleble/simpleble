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

CharacteristicWindows::CharacteristicWindows(BluetoothUUID uuid, std::set<CharacteristicCapability> capabilities)
    : _uuid(std::move(uuid)), _capabilities(std::move(capabilities)) {
    if (_capabilities.empty()) {
        throw Exception::OperationFailed("A local characteristic requires at least one capability.");
    }
}

CharacteristicWindows::~CharacteristicWindows() {
    _callback_on_read.unload();
    _callback_on_write.unload();
    _callback_on_subscribed.unload();
    _callback_on_unsubscribed.unload();
    destroy_native();
}

void CharacteristicWindows::create_native(const GattLocalService& service, SessionObserver session_observer,
                                          std::shared_ptr<std::atomic_bool> active) {
    destroy_native();

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

    auto native = std::make_shared<NativeState>();
    native->characteristic = result.Characteristic();
    native->session_observer = std::move(session_observer);
    native->active = std::move(active);

    auto weak_self = weak_from_this();
    std::weak_ptr<NativeState> weak_native = native;
    try {
        native->read_requested_token = native->characteristic.ReadRequested(
            [weak_self, weak_native](const GattLocalCharacteristic&, const GattReadRequestedEventArgs& args) {
                if (auto self = weak_self.lock()) {
                    if (auto state = weak_native.lock()) {
                        self->_on_read_requested(state, args);
                    }
                }
            });
        native->write_requested_token = native->characteristic.WriteRequested(
            [weak_self, weak_native](const GattLocalCharacteristic&, const GattWriteRequestedEventArgs& args) {
                if (auto self = weak_self.lock()) {
                    if (auto state = weak_native.lock()) {
                        self->_on_write_requested(state, args);
                    }
                }
            });
        native->subscribed_clients_changed_token = native->characteristic.SubscribedClientsChanged(
            [weak_self, weak_native](const GattLocalCharacteristic&, const winrt::Windows::Foundation::IInspectable&) {
                if (auto self = weak_self.lock()) {
                    if (auto state = weak_native.lock()) {
                        self->_on_subscribed_clients_changed(state);
                    }
                }
            });
    } catch (...) {
        _revoke_handlers(native);
        throw;
    }

    std::scoped_lock lock(_native_mutex);
    _native = std::move(native);
}

void CharacteristicWindows::destroy_native() noexcept {
    std::shared_ptr<NativeState> native;
    {
        std::scoped_lock lock(_native_mutex);
        native.swap(_native);
    }
    _revoke_handlers(native);
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
    const auto native = _native_state();
    if (!can_publish || !native || !native->subscribed.load() || !native->is_active()) {
        return;
    }

    try {
        WinRT::MtaManager::get().execute_sync([native, value_snapshot]() {
            if (native->is_active()) {
                async_get(native->characteristic.NotifyValueAsync(bytearray_to_ibuffer(value_snapshot)));
            }
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

std::shared_ptr<CharacteristicWindows::NativeState> CharacteristicWindows::_native_state() {
    std::scoped_lock lock(_native_mutex);
    return _native;
}

void CharacteristicWindows::_revoke_handlers(const std::shared_ptr<NativeState>& native) noexcept {
    if (!native || !native->characteristic) {
        return;
    }
    try {
        if (native->read_requested_token) {
            native->characteristic.ReadRequested(native->read_requested_token);
        }
    } catch (...) {
    }
    try {
        if (native->write_requested_token) {
            native->characteristic.WriteRequested(native->write_requested_token);
        }
    } catch (...) {
    }
    try {
        if (native->subscribed_clients_changed_token) {
            native->characteristic.SubscribedClientsChanged(native->subscribed_clients_changed_token);
        }
    } catch (...) {
    }
}

void CharacteristicWindows::_on_read_requested(const std::shared_ptr<NativeState>& native,
                                               const GattReadRequestedEventArgs& args) {
    auto deferral = args.GetDeferral();
    GattReadRequest request{nullptr};
    try {
        if (!native->is_active()) {
            deferral.Complete();
            return;
        }
        if (native->session_observer) {
            native->session_observer(args.Session());
        }

        request = async_get(args.GetRequestAsync());
        if (!request) {
            deferral.Complete();
            return;
        }
        if (!native->is_active()) {
            request.RespondWithProtocolError(ATT_ERROR_UNLIKELY);
            deferral.Complete();
            return;
        }

        ByteArray response;
        if (_callback_on_read && native->is_active()) {
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

void CharacteristicWindows::_on_write_requested(const std::shared_ptr<NativeState>& native,
                                                const GattWriteRequestedEventArgs& args) {
    auto deferral = args.GetDeferral();
    GattWriteRequest request{nullptr};
    bool should_respond = false;
    try {
        if (!native->is_active()) {
            deferral.Complete();
            return;
        }
        if (native->session_observer) {
            native->session_observer(args.Session());
        }

        request = async_get(args.GetRequestAsync());
        if (!request) {
            deferral.Complete();
            return;
        }

        should_respond = request.Option() == GattWriteOption::WriteWithResponse;
        if (!native->is_active()) {
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

        if (native->is_active()) {
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

void CharacteristicWindows::_on_subscribed_clients_changed(const std::shared_ptr<NativeState>& native) {
    try {
        if (!native->is_active()) {
            return;
        }

        const auto clients = native->characteristic.SubscribedClients();
        for (const auto& client : clients) {
            if (native->session_observer) {
                native->session_observer(client.Session());
            }
        }
        if (!native->is_active()) {
            return;
        }

        const bool subscribed = clients.Size() > 0;
        const bool changed = native->subscribed.exchange(subscribed) != subscribed;
        if (changed && native->is_active()) {
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
