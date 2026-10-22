#include "AdapterWrapper.h"
#include "Cache.h"

#include <memory>
#include <string>

AdapterWrapper::AdapterWrapper(const SimpleBLE::Adapter& adapter) : _adapter(adapter) {}

AdapterWrapper::~AdapterWrapper() {
    _adapter.set_callback_on_power_on(nullptr);
    _adapter.set_callback_on_power_off(nullptr);
    _adapter.set_callback_on_scan_found(nullptr);
    _adapter.set_callback_on_scan_updated(nullptr);
    _adapter.set_callback_on_scan_start(nullptr);
    _adapter.set_callback_on_scan_stop(nullptr);
}

SimpleBLE::Adapter& AdapterWrapper::get() { return _adapter; }

const SimpleBLE::Adapter& AdapterWrapper::get() const { return _adapter; }

void AdapterWrapper::setCallback(jobject callback) {
    // NOTE: SimpleBLE guards replacing and calling these callbacks, so they can hold the Java callback directly.
    auto adapter_callback = std::make_shared<Org::SimpleJavaBLE::AdapterCallback>(callback);
    const int64_t adapter_id = getHash(_adapter);

    _adapter.set_callback_on_power_on([adapter_callback]() { adapter_callback->on_power_on(); });

    _adapter.set_callback_on_power_off([adapter_callback]() { adapter_callback->on_power_off(); });

    _adapter.set_callback_on_scan_found([adapter_id, adapter_callback](SimpleBLE::Peripheral peripheral) {
        adapter_callback->on_scan_found(Cache::get().addPeripheral(adapter_id, peripheral));
    });

    _adapter.set_callback_on_scan_updated([adapter_id, adapter_callback](SimpleBLE::Peripheral peripheral) {
        // NOTE: Java might see a peripheral for the first time through an update, so we need to cache it here too.
        adapter_callback->on_scan_updated(Cache::get().addPeripheral(adapter_id, peripheral));
    });

    _adapter.set_callback_on_scan_start([adapter_callback]() { adapter_callback->on_scan_start(); });

    _adapter.set_callback_on_scan_stop([adapter_callback]() { adapter_callback->on_scan_stop(); });
}

int64_t AdapterWrapper::getHash(SimpleBLE::Adapter& adapter) {
    return static_cast<int64_t>(std::hash<std::string>{}(adapter.identifier() + "|" + adapter.address()));
}
