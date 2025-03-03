#include "AdapterWrapper.h"

AdapterWrapper::AdapterWrapper(const SimpleBLE::Adapter& adapter) : _adapter(adapter) {}

SimpleBLE::Adapter& AdapterWrapper::getAdapter() {
    return _adapter;
}

const SimpleBLE::Adapter& AdapterWrapper::getAdapter() const {
    return _adapter;
}

size_t AdapterWrapper::getHash() {
    return std::hash<std::string>{}(_adapter.identifier());
}

std::string AdapterWrapper::identifier() {
    return _adapter.identifier();
}

SimpleBLE::BluetoothAddress AdapterWrapper::address() {
    return _adapter.address();
}
