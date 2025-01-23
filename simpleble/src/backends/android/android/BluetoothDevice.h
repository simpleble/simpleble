#pragma once

#include "BluetoothGatt.h"

#include "bridge/BluetoothGattCallback.h"
#include "jni/Common.hpp"

namespace SimpleBLE {
namespace Android {

class BluetoothDevice {
  public:
    BluetoothDevice(JNI::Object obj);

    std::string getAddress();
    int getAddressType();
    std::string getName();
    int getBondState();

    BluetoothGatt connectGatt(bool autoConnect, Bridge::BluetoothGattCallback& callback);

    static const int BOND_NONE = 10;
    static const int BOND_BONDING = 11;
    static const int BOND_BONDED = 12;

    static const int ADDRESS_TYPE_ANONYMOUS = 255;
    static const int ADDRESS_TYPE_PUBLIC = 0;
    static const int ADDRESS_TYPE_RANDOM = 1;

  private:
    static JNI::Class _cls;
    static jmethodID _method_getAddress;
    static jmethodID _method_getAddressType;
    static jmethodID _method_getName;
    static jmethodID _method_getBondState;
    static jmethodID _method_connectGatt;

    static void initialize();
    void check_initialized() const;
    JNI::Object _obj;
};

}  // namespace Android
}  // namespace SimpleBLE
