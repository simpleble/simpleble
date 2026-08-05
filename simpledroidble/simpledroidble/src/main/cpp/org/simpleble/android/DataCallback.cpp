#include "DataCallback.h"

#include "Callback.h"

namespace Org {
namespace SimpleBLE {
namespace Android {

jmethodID DataCallback::_method_on_data_received = nullptr;

const SimpleJNI::JNIDescriptor DataCallback::descriptor{
    "org/simpleble/android/Peripheral$DataCallback",
    nullptr,
    {{"onDataReceived", "([B)V", &_method_on_data_received}}};

const SimpleJNI::AutoRegister<DataCallback> DataCallback::registrar{&descriptor};

DataCallback::DataCallback(jobject callback) : _obj(callback) {}

void DataCallback::on_data_received(const ::SimpleBLE::ByteArray& data) const noexcept {
    invoke_callback(__func__, [this, &data] {
        if (!_obj) return;
        SimpleJNI::ByteArray<SimpleJNI::LocalRef> payload(data);
        _obj.to_local().call_void_method(_method_on_data_received, payload.get());
    });
}

}  // namespace Android
}  // namespace SimpleBLE
}  // namespace Org
