#include "ScanSettings.h"
#include <simplejni/Registry.hpp>

namespace SimpleBLE {
namespace Android {

// ScanSettings
SimpleJNI::GlobalRef<jclass> ScanSettings::_cls;
jmethodID ScanSettings::_constructor;

const SimpleJNI::JNIDescriptor ScanSettings::descriptor{
    "android/bluetooth/le/ScanSettings",
    &ScanSettings::_cls,
    {
        {"<init>", "()V", &ScanSettings::_constructor},
    }
};
const SimpleJNI::AutoRegister<ScanSettings> ScanSettings::registrar{&ScanSettings::descriptor};

// ScanSettings::Builder
SimpleJNI::GlobalRef<jclass> ScanSettings::Builder::_cls;
jmethodID ScanSettings::Builder::_constructor;
jmethodID ScanSettings::Builder::_method_setLegacy;
jmethodID ScanSettings::Builder::_method_build;

const SimpleJNI::JNIDescriptor ScanSettings::Builder::descriptor{
    "android/bluetooth/le/ScanSettings$Builder",
    &ScanSettings::Builder::_cls,
    {
        {"<init>", "()V", &ScanSettings::Builder::_constructor},
        {"setLegacy", "(Z)Landroid/bluetooth/le/ScanSettings$Builder;", &ScanSettings::Builder::_method_setLegacy},
        {"build", "()Landroid/bluetooth/le/ScanSettings;", &ScanSettings::Builder::_method_build},
    }
};
const SimpleJNI::AutoRegister<ScanSettings::Builder> ScanSettings::Builder::registrar{&ScanSettings::Builder::descriptor};

ScanSettings::Builder::Builder() : SimpleJNI::Object<SimpleJNI::GlobalRef>(SimpleJNI::Object<SimpleJNI::LocalRef>(SimpleJNI::VM::env()->NewObject(_cls.get(), _constructor)).to_global()) {}

ScanSettings::Builder& ScanSettings::Builder::setLegacy(bool legacy) {
    call_object_method(_method_setLegacy, (jboolean)legacy);
    return *this;
}

ScanSettings ScanSettings::Builder::build() {
    return ScanSettings(call_object_method(_method_build));
}

}  // namespace Android
}  // namespace SimpleBLE
