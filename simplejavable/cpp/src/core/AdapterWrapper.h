#pragma once
#include <simpleble/SimpleBLE.h>

class AdapterWrapper {
private:
    SimpleBLE::Adapter _adapter; // The wrapped object

public:
    // Constructor takes a SimpleBLE::Adapter
    explicit AdapterWrapper(const SimpleBLE::Adapter& adapter);

    // Accessor to the underlying adapter
    SimpleBLE::Adapter& getAdapter();
    const SimpleBLE::Adapter& getAdapter() const;

    size_t getHash();

    std::string identifier();
    SimpleBLE::BluetoothAddress address();

    // Add more functionality here as needed, e.g.:
    // void setActive(bool active);
    // bool isConnected() const;
};