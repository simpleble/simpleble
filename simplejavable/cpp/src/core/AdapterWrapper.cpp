#include "AdapterWrapper.h"
#include "PeripheralWrapper.h"
#include "Cache.h"

AdapterWrapper::AdapterWrapper(const SimpleBLE::Adapter& adapter) : _adapter(adapter) {}

AdapterWrapper::~AdapterWrapper() {
    _adapter.set_callback_on_scan_found(nullptr);
    _adapter.set_callback_on_scan_updated(nullptr);
    _adapter.set_callback_on_scan_start(nullptr);
    _adapter.set_callback_on_scan_stop(nullptr);
}

SimpleBLE::Adapter& AdapterWrapper::getAdapter() {
    return _adapter;
}

const SimpleBLE::Adapter& AdapterWrapper::getAdapter() const {
    return _adapter;
}

void AdapterWrapper::setCallback(Org::SimpleJavaBLE::AdapterCallback& callback) {
    _callback = std::move(callback);

    _adapter.set_callback_on_scan_found([this](const SimpleBLE::Peripheral& peripheral) {
        PeripheralWrapper peripheral_wrapper(peripheral);
        Cache::get().addPeripheral(getHash(), peripheral_wrapper.getHash(), peripheral_wrapper);
        _callback.on_scan_found(peripheral_wrapper.getHash());
    });

    _adapter.set_callback_on_scan_updated([this](const SimpleBLE::Peripheral& peripheral) {
        PeripheralWrapper peripheral_wrapper(peripheral);
        _callback.on_scan_updated(peripheral_wrapper.getHash());
    });

    _adapter.set_callback_on_scan_start([this]() {
        _callback.on_scan_start();
    });

    _adapter.set_callback_on_scan_stop([this]() {
        _callback.on_scan_stop();
    });
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

void AdapterWrapper::scanStart() {
    _adapter.scan_start();
}

void AdapterWrapper::scanStop() {
    _adapter.scan_stop();
}

void AdapterWrapper::scanFor(int timeout) {
    _adapter.scan_for(timeout);
}
