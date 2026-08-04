#include "ScanSettings.h"

namespace SimpleBLE {
namespace Android {

SimpleJNI::GlobalRef<jclass> ScanSettings::Builder::_cls;
jmethodID ScanSettings::Builder::_constructor = nullptr;
jmethodID ScanSettings::Builder::_method_setLegacy = nullptr;
jmethodID ScanSettings::Builder::_method_setPhy = nullptr;
jmethodID ScanSettings::Builder::_method_build = nullptr;

const SimpleJNI::JNIDescriptor ScanSettings::Builder::descriptor{
    "android/bluetooth/le/ScanSettings$Builder",
    &_cls,
    {
        {"<init>", "()V", &_constructor},
        {"setLegacy", "(Z)Landroid/bluetooth/le/ScanSettings$Builder;", &_method_setLegacy},
        {"setPhy", "(I)Landroid/bluetooth/le/ScanSettings$Builder;", &_method_setPhy},
        {"build", "()Landroid/bluetooth/le/ScanSettings;", &_method_build},
    }};

const SimpleJNI::AutoRegister<ScanSettings::Builder> ScanSettings::Builder::registrar{&descriptor};

ScanSettings::ScanSettings(SimpleJNI::Object<SimpleJNI::GlobalRef, jobject> obj) : _obj(obj) {}

ScanSettings::Builder::Builder()
    : _obj(SimpleJNI::Object<SimpleJNI::LocalRef>::call_new_object(_cls.get(), _constructor).to_global()) {}

ScanSettings::Builder& ScanSettings::Builder::setLegacy(bool legacy) {
    _obj.call_object_method(_method_setLegacy, static_cast<jboolean>(legacy));
    return *this;
}

ScanSettings::Builder& ScanSettings::Builder::setPhy(int phy) {
    _obj.call_object_method(_method_setPhy, phy);
    return *this;
}

ScanSettings ScanSettings::Builder::build() {
    return ScanSettings(_obj.call_object_method(_method_build).to_global());
}

}  // namespace Android
}  // namespace SimpleBLE
