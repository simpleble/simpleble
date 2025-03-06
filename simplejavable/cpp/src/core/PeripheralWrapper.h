#pragma once
#include <simpleble/SimpleBLE.h>
#include "org/simplejavable/PeripheralCallback.h"

class PeripheralWrapper {
private:
    SimpleBLE::Peripheral _peripheral; // The wrapped object
    Org::SimpleJavaBLE::PeripheralCallback _callback;
public:
    // Constructor takes a SimpleBLE::Peripheral
    explicit PeripheralWrapper(const SimpleBLE::Peripheral& peripheral);

    // Accessor to the underlying peripheral
    SimpleBLE::Peripheral& getPeripheral();
    const SimpleBLE::Peripheral& getPeripheral() const;

    void setCallback(Org::SimpleJavaBLE::PeripheralCallback& callback);

    size_t getHash();

    std::string identifier();
    SimpleBLE::BluetoothAddress address();

    // Add more functionality here as needed, e.g.:
    // void connect();
    // std::string getName() const;
};