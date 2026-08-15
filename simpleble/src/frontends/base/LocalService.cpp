#include <simpleble/local/Service.h>

#include <utility>

#include "BuildVec.h"
#include "LocalServiceBase.h"

using namespace SimpleBLE;
using namespace SimpleBLE::Local;

bool Service::initialized() const { return internal_ != nullptr; }

ServiceBase* Service::operator->() {
    if (!initialized()) throw Exception::NotInitialized();

    return internal_.get();
}

const ServiceBase* Service::operator->() const {
    if (!initialized()) throw Exception::NotInitialized();

    return internal_.get();
}

BluetoothUUID Service::uuid() { return (*this)->uuid(); }

Characteristic Service::add_characteristic(BluetoothUUID uuid, std::set<CharacteristicCapability> capabilities) {
    return Factory::build((*this)->add_characteristic(std::move(uuid), std::move(capabilities)));
}

std::vector<Characteristic> Service::characteristics() { return Factory::vector((*this)->characteristics()); }
