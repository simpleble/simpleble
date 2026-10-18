#include "DataCallback.h"

#include "Callback.h"

namespace Org {
namespace SimpleKtBLE {

jmethodID DataCallback::_method_on_data_received = nullptr;
jmethodID DataCallback::_method_on_ended = nullptr;

const SimpleJNI::JNIDescriptor DataCallback::descriptor{
    "org/simplektble/DataCallback",
    nullptr,
    {{"onDataReceived", "([B)V", &_method_on_data_received}, {"onEnded", "()V", &_method_on_ended}}};

const SimpleJNI::AutoRegister<DataCallback> DataCallback::registrar{&descriptor};

DataCallback::DataCallback(jobject callback) : _obj(callback) {}

void DataCallback::on_data_received(const ::SimpleBLE::ByteArray& data) const noexcept {
    invoke_callback(__func__, [this, &data] {
        if (!_obj) return;
        SimpleJNI::ByteArray<SimpleJNI::LocalRef> payload(data);
        _obj.to_local().call_void_method(_method_on_data_received, payload.get());
    });
}

void DataCallback::on_ended() const noexcept {
    invoke_callback(__func__, [this] {
        if (_obj) _obj.to_local().call_void_method(_method_on_ended);
    });
}

}  // namespace SimpleKtBLE
}  // namespace Org
