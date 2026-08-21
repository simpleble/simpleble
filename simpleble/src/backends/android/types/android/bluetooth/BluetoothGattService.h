#pragma once

#include <string>
#include <vector>
#include "BluetoothGattCharacteristic.h"
#include "simplejni/Common.hpp"
#include "simplejni/Registry.hpp"

namespace SimpleBLE {
namespace Android {

class BluetoothGattService {
  public:
    BluetoothGattService();
    explicit BluetoothGattService(const std::string& uuid);
    BluetoothGattService(SimpleJNI::Object<SimpleJNI::GlobalRef, jobject> obj);

    bool addCharacteristic(const BluetoothGattCharacteristic& characteristic);
    //    bool addService(BluetoothGattService service);
    //    BluetoothGattCharacteristic getCharacteristic(std::string uuid);
    std::vector<BluetoothGattCharacteristic> getCharacteristics();
    //    std::vector<BluetoothGattService> getIncludedServices(); // TODO: This might be necessary if we don't see the
    //    secondary services in some other way.
    int getInstanceId();
    int getType();
    std::string getUuid();

    SimpleJNI::Object<SimpleJNI::GlobalRef, jobject> getObject() const { return _obj; }

  private:
    SimpleJNI::Object<SimpleJNI::GlobalRef, jobject> _obj;
    static SimpleJNI::GlobalRef<jclass> _cls;
    static jmethodID _method_addCharacteristic;
    static jmethodID _method_addService;
    static jmethodID _method_getCharacteristic;
    static jmethodID _method_getCharacteristics;
    static jmethodID _method_getIncludedServices;
    static jmethodID _method_getInstanceId;
    static jmethodID _method_getType;
    static jmethodID _method_getUuid;
    static jmethodID _constructor;

    static constexpr int SERVICE_TYPE_PRIMARY = 0;

    // JNI descriptor for auto-registration
    static const SimpleJNI::JNIDescriptor descriptor;
    static const SimpleJNI::AutoRegister<BluetoothGattService> registrar;
};

}  // namespace Android
}  // namespace SimpleBLE
