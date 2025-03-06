#pragma once
#include <simpleble/SimpleBLE.h>
#include "org/simplejavable/AdapterCallback.h"

class AdapterWrapper {
private:
    SimpleBLE::Adapter _adapter; // The wrapped object
    Org::SimpleJavaBLE::AdapterCallback _callback;
public:
    // Constructor takes a SimpleBLE::Adapter
    explicit AdapterWrapper(const SimpleBLE::Adapter& adapter);

    ~AdapterWrapper();

    // Accessor to the underlying adapter
    SimpleBLE::Adapter& getAdapter();
    const SimpleBLE::Adapter& getAdapter() const;

    void setCallback(Org::SimpleJavaBLE::AdapterCallback& callback);

    size_t getHash();

    std::string identifier();
    SimpleBLE::BluetoothAddress address();

    void scanStart();
    void scanStop();
    void scanFor(int timeout);

    // Add more functionality here as needed, e.g.:
    // void setActive(bool active);
    // bool isConnected() const;
};