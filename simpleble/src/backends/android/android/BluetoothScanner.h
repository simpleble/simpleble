#pragma once

#include "jni/Common.hpp"

#include "bridge/ScanCallback.h"

namespace SimpleBLE {
namespace Android {

class BluetoothScanner {
  public:
    BluetoothScanner(JNI::Object obj);

    void startScan(Bridge::ScanCallback &callback);
    void stopScan(Bridge::ScanCallback &callback);

    std::string toString();

  private:
    static JNI::Class _cls;
    static jmethodID _method_startScan;
    static jmethodID _method_stopScan;
    static jmethodID _method_toString;

    static void initialize();
    void check_initialized() const;
    JNI::Object _obj;
};

}  // namespace Android
}  // namespace SimpleBLE
