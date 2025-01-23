#pragma once

#include <simpleble/Exceptions.h>
#include <simpleble/Types.h>

#include "../common/AdapterBase.h"

#include <kvn_safe_callback.hpp>

#include <atomic>
#include <functional>
#include <memory>
#include <string>
#include <vector>

#include "android/BluetoothAdapter.h"
#include "android/BluetoothScanner.h"
#include "bridge/ScanCallback.h"
#include "jni/Common.hpp"

namespace SimpleBLE {

class BackendAndroid;
class PeripheralAndroid;
class Peripheral;

class AdapterAndroid : public AdapterBase {
  public:
    AdapterAndroid(std::shared_ptr<BackendAndroid> backend);
    virtual ~AdapterAndroid();

    virtual void* underlying() const override;

    virtual std::string identifier() override;
    virtual BluetoothAddress address() override;

    virtual void scan_start() override;
    virtual void scan_stop() override;
    virtual void scan_for(int timeout_ms) override;
    virtual bool scan_is_active() override;
    virtual std::vector<std::shared_ptr<PeripheralBase>> scan_get_results() override;

    virtual void set_callback_on_scan_start(std::function<void()> on_scan_start) override;
    virtual void set_callback_on_scan_stop(std::function<void()> on_scan_stop) override;
    virtual void set_callback_on_scan_updated(std::function<void(Peripheral)> on_scan_updated) override;
    virtual void set_callback_on_scan_found(std::function<void(Peripheral)> on_scan_found) override;

    virtual std::vector<std::shared_ptr<PeripheralBase>> get_paired_peripherals() override;

    virtual bool bluetooth_enabled() override;

    // NOTE: The following methods have been made public to allow the JNI layer to call them, but
    // should not be called directly by the user.

    void onScanResultCallback(JNIEnv* env, jobject thiz, jint callback_type, jobject result);
    void onBatchScanResultsCallback(JNIEnv* env, jobject thiz, jobject results);
    void onScanFailedCallback(JNIEnv* env, jobject thiz, jint error_code);

    // static std::map<jobject, AdapterBase*, JNI::JObjectComparator> _scanCallbackMap;

  private:
    std::shared_ptr<BackendAndroid> backend_;

    Android::BluetoothAdapter _btAdapter = Android::BluetoothAdapter::getDefaultAdapter();
    Android::BluetoothScanner _btScanner = _btAdapter.getBluetoothLeScanner();
    Android::Bridge::ScanCallback _btScanCallback;

    std::map<BluetoothAddress, std::shared_ptr<PeripheralAndroid>> peripherals_;
    std::map<BluetoothAddress, std::shared_ptr<PeripheralAndroid>> seen_peripherals_;

    kvn::safe_callback<void()> callback_on_scan_start_;
    kvn::safe_callback<void()> callback_on_scan_stop_;
    kvn::safe_callback<void(Peripheral)> callback_on_scan_updated_;
    kvn::safe_callback<void(Peripheral)> callback_on_scan_found_;

    std::atomic<bool> scanning_{false};
};

}  // namespace SimpleBLE
