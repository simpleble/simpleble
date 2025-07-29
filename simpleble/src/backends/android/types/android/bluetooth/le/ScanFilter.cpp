#include "ScanFilter.h"
#include <simplejni/Registry.hpp>

namespace SimpleBLE {
namespace Android {

// ScanFilter
SimpleJNI::GlobalRef<jclass> ScanFilter::_cls;
jmethodID ScanFilter::_constructor;

const SimpleJNI::JNIDescriptor ScanFilter::descriptor{
    "android/bluetooth/le/ScanFilter",
    &ScanFilter::_cls,
    {
        {"<init>", "()V", &ScanFilter::_constructor},
    }
};
const SimpleJNI::AutoRegister<ScanFilter> ScanFilter::registrar{&ScanFilter::descriptor};

// ScanFilter::Builder
SimpleJNI::GlobalRef<jclass> ScanFilter::Builder::_cls;
jmethodID ScanFilter::Builder::_constructor;
jmethodID ScanFilter::Builder::_method_build;

const SimpleJNI::JNIDescriptor ScanFilter::Builder::descriptor{
    "android/bluetooth/le/ScanFilter$Builder",
    &ScanFilter::Builder::_cls,
    {
        {"<init>", "()V", &ScanFilter::Builder::_constructor},
        {"build", "()Landroid/bluetooth/le/ScanFilter;", &ScanFilter::Builder::_method_build},
    }
};
const SimpleJNI::AutoRegister<ScanFilter::Builder> ScanFilter::Builder::registrar{&ScanFilter::Builder::descriptor};

ScanFilter::Builder::Builder() : SimpleJNI::Object<SimpleJNI::GlobalRef>(SimpleJNI::Object<SimpleJNI::LocalRef>(SimpleJNI::VM::env()->NewObject(_cls.get(), _constructor)).to_global()) {}

ScanFilter ScanFilter::Builder::build() {
    return ScanFilter(call_object_method(_method_build));
}

}  // namespace Android
}  // namespace SimpleBLE
