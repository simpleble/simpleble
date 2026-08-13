#include <simpleble/local/Characteristic.h>

#include <utility>

#include "LocalCharacteristicBase.h"

using namespace SimpleBLE;
using namespace SimpleBLE::Local;

bool Characteristic::initialized() const { return internal_ != nullptr; }

CharacteristicBase* Characteristic::operator->() {
    if (!initialized()) throw Exception::NotInitialized();

    return internal_.get();
}

const CharacteristicBase* Characteristic::operator->() const {
    if (!initialized()) throw Exception::NotInitialized();

    return internal_.get();
}

BluetoothUUID Characteristic::uuid() { return (*this)->uuid(); }

std::vector<CharacteristicCapability> Characteristic::capabilities() { return (*this)->capabilities(); }

ByteArray Characteristic::value() { return (*this)->value(); }

void Characteristic::set_value(ByteArray value) { (*this)->set_value(std::move(value)); }

void Characteristic::set_callback_on_read(std::function<ByteArray()> on_read) {
    (*this)->set_callback_on_read(std::move(on_read));
}

void Characteristic::set_callback_on_write(std::function<void(ByteArray value)> on_write) {
    (*this)->set_callback_on_write(std::move(on_write));
}

void Characteristic::set_callback_on_subscribed(std::function<void()> on_subscribed) {
    (*this)->set_callback_on_subscribed(std::move(on_subscribed));
}

void Characteristic::set_callback_on_unsubscribed(std::function<void()> on_unsubscribed) {
    (*this)->set_callback_on_unsubscribed(std::move(on_unsubscribed));
}
