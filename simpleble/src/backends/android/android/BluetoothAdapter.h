#pragma once

#include "jni/Common.hpp"

#include "BluetoothScanner.h"

namespace SimpleBLE {
namespace Android {

class BluetoothAdapter {
  public:
    BluetoothAdapter(JNI::Object obj);

    std::string getName() ;
    std::string getAddress();
    bool isEnabled();
    int getState();

    BluetoothScanner getBluetoothLeScanner();

    // NOTE: The correct way to request a BluetoothAdapter is to go though the BluetoothManager,
    // as described in https://developer.android.com/reference/android/bluetooth/BluetoothManager#getAdapter()
    // However, for simplicity, we are using a direct call to BluetoothAdapter.getDefaultAdapter() which is
    // deprecated in API 31 but still works. We'll need to implement a backend bypass to get a Context
    // object and call getSystemService(Context.BLUETOOTH_SERVICE) to get the BluetoothManager.

    // NOTE: Android BluetoothAdapter and BluetoothScanner classes are singletons, but so is this
    // class, so we don't need to make them static.
    static BluetoothAdapter getDefaultAdapter();

  private:
    static JNI::Class _cls;

    static jmethodID _method_getName;
    static jmethodID _method_getAddress;
    static jmethodID _method_isEnabled;
    static jmethodID _method_getState;
    static jmethodID _method_getBluetoothLeScanner;
    static jmethodID _method_getDefaultAdapter;
    static void initialize();
    void check_initialized() const;
    JNI::Object _obj;
};

}  // namespace Android
}  // namespace SimpleBLE