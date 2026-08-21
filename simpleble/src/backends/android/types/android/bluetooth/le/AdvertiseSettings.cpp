#include "AdvertiseSettings.h"

namespace SimpleBLE::Android {

SimpleJNI::GlobalRef<jclass> AdvertiseSettings::Builder::_cls;
jmethodID AdvertiseSettings::Builder::_constructor = nullptr;
jmethodID AdvertiseSettings::Builder::_set_connectable = nullptr;
jmethodID AdvertiseSettings::Builder::_set_advertise_mode = nullptr;
jmethodID AdvertiseSettings::Builder::_build = nullptr;

const SimpleJNI::JNIDescriptor AdvertiseSettings::Builder::descriptor{
    "android/bluetooth/le/AdvertiseSettings$Builder",
    &_cls,
    {{"<init>", "()V", &_constructor},
     {"setConnectable", "(Z)Landroid/bluetooth/le/AdvertiseSettings$Builder;", &_set_connectable},
     {"setAdvertiseMode", "(I)Landroid/bluetooth/le/AdvertiseSettings$Builder;", &_set_advertise_mode},
     {"build", "()Landroid/bluetooth/le/AdvertiseSettings;", &_build}}};
const SimpleJNI::AutoRegister<AdvertiseSettings::Builder> AdvertiseSettings::Builder::registrar{&descriptor};

AdvertiseSettings::Builder::Builder() {
    auto object = SimpleJNI::Object<SimpleJNI::LocalRef, jobject>::call_new_object(_cls.get(), _constructor);
    _obj = object.to_global();
}

AdvertiseSettings::Builder& AdvertiseSettings::Builder::setConnectable(bool connectable) {
    _obj.call_object_method(_set_connectable, connectable);
    return *this;
}

AdvertiseSettings::Builder& AdvertiseSettings::Builder::setAdvertiseMode(int mode) {
    _obj.call_object_method(_set_advertise_mode, mode);
    return *this;
}

AdvertiseSettings AdvertiseSettings::Builder::build() {
    return AdvertiseSettings(_obj.call_object_method(_build).to_global());
}

}  // namespace SimpleBLE::Android
