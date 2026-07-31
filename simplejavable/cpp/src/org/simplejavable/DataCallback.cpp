#include "DataCallback.h"

#include <simpleble/Logging.h>

namespace Org {
namespace SimpleJavaBLE {

// Define static JNI resources
SimpleJNI::GlobalRef<jclass> DataCallback::_cls;
jmethodID DataCallback::_method_on_data_received = nullptr;

// Define the JNI descriptor
const SimpleJNI::JNIDescriptor DataCallback::descriptor{
    "org/simplejavable/Peripheral$DataCallback",  // Java interface name (inner class notation)
    &_cls,                                        // Where to store the jclass
    {                                             // Methods to preload
     {"onDataReceived", "([B)V", &_method_on_data_received}}};

// Define the AutoRegister instance
const SimpleJNI::AutoRegister<DataCallback> DataCallback::registrar{&descriptor};

DataCallback::DataCallback(jobject obj) : _obj(obj, _cls.get()) {
    if (!_cls.get()) {
        throw std::runtime_error("DataCallback JNI resources not preloaded");
    }
}

void DataCallback::on_data_received(jbyteArray data) noexcept {
    try {
        if (_obj) {
            _obj.to_local().call_void_method(_method_on_data_received, data);
        }
    } catch (const std::exception& exception) {
        SimpleBLE::Logging::Logger::get()->log(
            SimpleBLE::Logging::Level::Error, "SimpleJavaBLE", __FILE__, __LINE__, __func__,
            "Java notification callback failed: " + std::string(exception.what()));
    } catch (...) {
        SimpleBLE::Logging::Logger::get()->log(SimpleBLE::Logging::Level::Error, "SimpleJavaBLE", __FILE__, __LINE__,
                                              __func__, "Java notification callback failed with an unknown error");
    }
}

}  // namespace SimpleJavaBLE
}  // namespace Org
