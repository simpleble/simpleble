#include "AdvertiseData.h"

namespace SimpleBLE::Android {

SimpleJNI::GlobalRef<jclass> AdvertiseData::Builder::_cls;
jmethodID AdvertiseData::Builder::_constructor = nullptr;
jmethodID AdvertiseData::Builder::_add_service_uuid = nullptr;
jmethodID AdvertiseData::Builder::_set_include_device_name = nullptr;
jmethodID AdvertiseData::Builder::_build = nullptr;

const SimpleJNI::JNIDescriptor AdvertiseData::Builder::descriptor{
    "android/bluetooth/le/AdvertiseData$Builder",
    &_cls,
    {{"<init>", "()V", &_constructor},
     {"addServiceUuid", "(Landroid/os/ParcelUuid;)Landroid/bluetooth/le/AdvertiseData$Builder;", &_add_service_uuid},
     {"setIncludeDeviceName", "(Z)Landroid/bluetooth/le/AdvertiseData$Builder;", &_set_include_device_name},
     {"build", "()Landroid/bluetooth/le/AdvertiseData;", &_build}}};
const SimpleJNI::AutoRegister<AdvertiseData::Builder> AdvertiseData::Builder::registrar{&descriptor};

AdvertiseData::Builder::Builder() {
    auto object = SimpleJNI::Object<SimpleJNI::LocalRef, jobject>::call_new_object(_cls.get(), _constructor);
    _obj = object.to_global();
}

AdvertiseData::Builder& AdvertiseData::Builder::addServiceUuid(const ParcelUUID& uuid) {
    _obj.call_object_method(_add_service_uuid, uuid.get());
    return *this;
}

AdvertiseData::Builder& AdvertiseData::Builder::setIncludeDeviceName(bool include) {
    _obj.call_object_method(_set_include_device_name, include);
    return *this;
}

AdvertiseData AdvertiseData::Builder::build() { return AdvertiseData(_obj.call_object_method(_build).to_global()); }

}  // namespace SimpleBLE::Android
