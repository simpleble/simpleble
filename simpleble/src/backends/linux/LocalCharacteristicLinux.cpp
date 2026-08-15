#include "LocalCharacteristicLinux.h"

#include <utility>

#include "CommonUtils.h"

namespace SimpleBLE::Local {

CharacteristicLinux::CharacteristicLinux(std::shared_ptr<SimpleBluez::Characteristic> characteristic,
                                         BluetoothUUID uuid, std::set<CharacteristicCapability> capabilities)
    : _characteristic(std::move(characteristic)), _uuid(std::move(uuid)), _capabilities(std::move(capabilities)) {
    if (_capabilities.empty()) {
        throw Exception::OperationFailed("A local characteristic requires at least one capability.");
    }

    _characteristic->uuid(_uuid);
    _characteristic->flags(_flags_from_capabilities(_capabilities));

    _characteristic->set_on_read_value([this](SimpleBluez::Characteristic::ValueOptions) {
        if (!_callback_on_read) {
            return;
        }

        try {
            _characteristic->value(_callback_on_read());
        } catch (const std::exception& ex) {
            SIMPLEBLE_LOG_ERROR(fmt::format("Exception during local characteristic read callback: {}", ex.what()));
        } catch (...) {
            SIMPLEBLE_LOG_ERROR("Unknown exception during local characteristic read callback");
        }
    });

    _characteristic->set_on_write_value(
        [this](const SimpleBluez::ByteArray& value, SimpleBluez::Characteristic::ValueOptions) {
            SAFE_CALLBACK_CALL(_callback_on_write, value);
        });

    _characteristic->set_on_notify([this](bool subscribed) {
        const bool was_subscribed = _subscribed.exchange(subscribed);
        if (subscribed == was_subscribed) {
            return;
        }

        if (subscribed) {
            SAFE_CALLBACK_CALL(_callback_on_subscribed);
        } else {
            SAFE_CALLBACK_CALL(_callback_on_unsubscribed);
        }
    });
}

CharacteristicLinux::~CharacteristicLinux() {
    _callback_on_read.unload();
    _callback_on_write.unload();
    _callback_on_subscribed.unload();
    _callback_on_unsubscribed.unload();

    _characteristic->clear_on_read_value();
    _characteristic->clear_on_write_value();
    _characteristic->clear_on_notify();
}

BluetoothUUID CharacteristicLinux::uuid() { return _uuid; }

std::set<CharacteristicCapability> CharacteristicLinux::capabilities() { return _capabilities; }

ByteArray CharacteristicLinux::value() { return _characteristic->value(); }

void CharacteristicLinux::set_value(ByteArray value) { _characteristic->value(std::move(value)); }

void CharacteristicLinux::set_callback_on_read(std::function<ByteArray()> on_read) {
    if (on_read) {
        _callback_on_read.load(std::move(on_read));
    } else {
        _callback_on_read.unload();
    }
}

void CharacteristicLinux::set_callback_on_write(std::function<void(ByteArray value)> on_write) {
    if (on_write) {
        _callback_on_write.load(std::move(on_write));
    } else {
        _callback_on_write.unload();
    }
}

void CharacteristicLinux::set_callback_on_subscribed(std::function<void()> on_subscribed) {
    if (on_subscribed) {
        _callback_on_subscribed.load(std::move(on_subscribed));
    } else {
        _callback_on_subscribed.unload();
    }
}

void CharacteristicLinux::set_callback_on_unsubscribed(std::function<void()> on_unsubscribed) {
    if (on_unsubscribed) {
        _callback_on_unsubscribed.load(std::move(on_unsubscribed));
    } else {
        _callback_on_unsubscribed.unload();
    }
}

std::vector<std::string> CharacteristicLinux::_flags_from_capabilities(
    const std::set<CharacteristicCapability>& capabilities) {
    std::vector<std::string> flags;

    for (const auto capability : capabilities) {
        switch (capability) {
            case CharacteristicCapability::READ:
                flags.emplace_back("read");
                break;
            case CharacteristicCapability::WRITE_REQUEST:
                flags.emplace_back("write");
                break;
            case CharacteristicCapability::WRITE_COMMAND:
                flags.emplace_back("write-without-response");
                break;
            case CharacteristicCapability::NOTIFY:
                flags.emplace_back("notify");
                break;
            case CharacteristicCapability::INDICATE:
                flags.emplace_back("indicate");
                break;
        }
    }

    return flags;
}

}  // namespace SimpleBLE::Local
