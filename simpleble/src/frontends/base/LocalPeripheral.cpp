#include <simpleble/local/Peripheral.h>

#include <utility>

#include "BuildVec.h"
#include "LocalPeripheralBase.h"

using namespace SimpleBLE;
using namespace SimpleBLE::Local;

bool Peripheral::initialized() const { return internal_ != nullptr; }

PeripheralBase* Peripheral::operator->() {
    if (!initialized()) throw Exception::NotInitialized();

    return internal_.get();
}

const PeripheralBase* Peripheral::operator->() const {
    if (!initialized()) throw Exception::NotInitialized();

    return internal_.get();
}

void* Peripheral::underlying() const { return (*this)->underlying(); }

Advertisement Peripheral::advertisement() { return (*this)->advertisement(); }

void Peripheral::set_advertisement(Advertisement advertisement) {
    (*this)->set_advertisement(std::move(advertisement));
}

Service Peripheral::add_service(BluetoothUUID uuid) { return Factory::build((*this)->add_service(std::move(uuid))); }

std::vector<Service> Peripheral::services() { return Factory::vector((*this)->services()); }

void Peripheral::remove_all_services() { (*this)->remove_all_services(); }

void Peripheral::start() { (*this)->start(); }

void Peripheral::stop() { (*this)->stop(); }

bool Peripheral::is_started() { return (*this)->is_started(); }

bool Peripheral::is_advertising() { return (*this)->is_advertising(); }

void Peripheral::set_callback_on_client_connected(
    std::function<void(BluetoothAddress client_address)> on_client_connected) {
    (*this)->set_callback_on_client_connected(std::move(on_client_connected));
}

void Peripheral::set_callback_on_client_disconnected(
    std::function<void(BluetoothAddress client_address)> on_client_disconnected) {
    (*this)->set_callback_on_client_disconnected(std::move(on_client_disconnected));
}
