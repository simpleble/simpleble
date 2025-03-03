#include "PeripheralWrapper.h"

PeripheralWrapper::PeripheralWrapper(const SimpleBLE::Peripheral& peripheral) : peripheral_(peripheral) {}

SimpleBLE::Peripheral& PeripheralWrapper::getPeripheral() {
    return peripheral_;
}

const SimpleBLE::Peripheral& PeripheralWrapper::getPeripheral() const {
    return peripheral_;
}
