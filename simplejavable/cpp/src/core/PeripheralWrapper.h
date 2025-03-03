#pragma once
#include <simpleble/SimpleBLE.h>

class PeripheralWrapper {
private:
    SimpleBLE::Peripheral peripheral_; // The wrapped object

public:
    // Constructor takes a SimpleBLE::Peripheral
    explicit PeripheralWrapper(const SimpleBLE::Peripheral& peripheral);

    // Accessor to the underlying peripheral
    SimpleBLE::Peripheral& getPeripheral();
    const SimpleBLE::Peripheral& getPeripheral() const;

    // Add more functionality here as needed, e.g.:
    // void connect();
    // std::string getName() const;
};