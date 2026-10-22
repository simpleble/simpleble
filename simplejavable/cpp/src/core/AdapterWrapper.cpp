#include "AdapterWrapper.h"
#include "Cache.h"
#include "CallbackRunner.h"

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
    // NOTE: SimpleBLE's threads only queue the events. The callback runner calls Java.
    auto adapter_callback = make_java_callback<Org::SimpleJavaBLE::AdapterCallback>(callback);
    const int64_t adapter_id = getHash(_adapter);

    _adapter.set_callback_on_power_on([adapter_callback]() {
        callback_runner().enqueue([adapter_callback]() { adapter_callback->on_power_on(); });
    });

    _adapter.set_callback_on_power_off([adapter_callback]() {
        callback_runner().enqueue([adapter_callback]() { adapter_callback->on_power_off(); });
    });

    _adapter.set_callback_on_scan_found([adapter_id, adapter_callback](SimpleBLE::Peripheral peripheral) {
        const int64_t peripheral_id = Cache::get().addPeripheral(adapter_id, peripheral);
        callback_runner().enqueue(
            [adapter_callback, peripheral_id]() { adapter_callback->on_scan_found(peripheral_id); });
    });

    _adapter.set_callback_on_scan_updated([adapter_id, adapter_callback](SimpleBLE::Peripheral peripheral) {
        // NOTE: Java might see a peripheral for the first time through an update, so we need to cache it here too.
        const int64_t peripheral_id = Cache::get().addPeripheral(adapter_id, peripheral);
        callback_runner().enqueue(
            [adapter_callback, peripheral_id]() { adapter_callback->on_scan_updated(peripheral_id); });
    });

    _adapter.set_callback_on_scan_start([adapter_callback]() {
        callback_runner().enqueue([adapter_callback]() { adapter_callback->on_scan_start(); });
    });

    _adapter.set_callback_on_scan_stop([adapter_callback]() {
        callback_runner().enqueue([adapter_callback]() { adapter_callback->on_scan_stop(); });
    });
}

int64_t AdapterWrapper::getHash(SimpleBLE::Adapter& adapter) {
    return static_cast<int64_t>(std::hash<std::string>{}(adapter.identifier() + "|" + adapter.address()));
}
