#include "BluetoothLeAdvertiser.h"

#include <stdexcept>

namespace SimpleBLE::Android {

SimpleJNI::GlobalRef<jclass> BluetoothLeAdvertiser::_cls;
jmethodID BluetoothLeAdvertiser::_start_advertising = nullptr;
jmethodID BluetoothLeAdvertiser::_stop_advertising = nullptr;

const SimpleJNI::JNIDescriptor BluetoothLeAdvertiser::descriptor{
    "android/bluetooth/le/BluetoothLeAdvertiser",
    &_cls,
    {{"startAdvertising",
      "(Landroid/bluetooth/le/AdvertiseSettings;Landroid/bluetooth/le/AdvertiseData;Landroid/bluetooth/le/"
      "AdvertiseData;Landroid/bluetooth/le/AdvertiseCallback;)V",
      &_start_advertising},
     {"stopAdvertising", "(Landroid/bluetooth/le/AdvertiseCallback;)V", &_stop_advertising}}};
const SimpleJNI::AutoRegister<BluetoothLeAdvertiser> BluetoothLeAdvertiser::registrar{&descriptor};

void BluetoothLeAdvertiser::startAdvertising(const AdvertiseSettings& settings, const AdvertiseData& data,
                                             const AdvertiseData& scan_response, Bridge::AdvertiseCallback& callback) {
    if (!_obj) throw std::runtime_error("Bluetooth LE advertising is not supported by this Android device.");
    _obj.call_void_method(_start_advertising, settings.get(), data.get(), scan_response.get(), callback.get());
}

void BluetoothLeAdvertiser::stopAdvertising(Bridge::AdvertiseCallback& callback) {
    if (!_obj) throw std::runtime_error("BluetoothLeAdvertiser is not initialized");
    _obj.call_void_method(_stop_advertising, callback.get());
}

}  // namespace SimpleBLE::Android
