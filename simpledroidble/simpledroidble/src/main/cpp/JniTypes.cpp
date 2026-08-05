#include "JniTypes.h"

#include <simpleble/Logging.h>

namespace SimpleDroidJNI {
namespace {

SimpleJNI::GlobalRef<jclass> array_list_class;
jmethodID array_list_constructor = nullptr;
jmethodID array_list_add = nullptr;

SimpleJNI::GlobalRef<jclass> hash_map_class;
jmethodID hash_map_constructor = nullptr;
jmethodID hash_map_put = nullptr;

SimpleJNI::GlobalRef<jclass> integer_class;
jmethodID integer_constructor = nullptr;

SimpleJNI::GlobalRef<jclass> service_class;
jmethodID service_constructor = nullptr;

SimpleJNI::GlobalRef<jclass> characteristic_class;
jmethodID characteristic_constructor = nullptr;

SimpleJNI::GlobalRef<jclass> descriptor_class;
jmethodID descriptor_constructor = nullptr;

SimpleJNI::GlobalRef<jclass> exception_class;

const SimpleJNI::JNIDescriptor array_list_descriptor{
    "java/util/ArrayList",
    &array_list_class,
    {{"<init>", "()V", &array_list_constructor}, {"add", "(Ljava/lang/Object;)Z", &array_list_add}}};

const SimpleJNI::JNIDescriptor hash_map_descriptor{
    "java/util/HashMap",
    &hash_map_class,
    {{"<init>", "()V", &hash_map_constructor},
     {"put", "(Ljava/lang/Object;Ljava/lang/Object;)Ljava/lang/Object;", &hash_map_put}}};

const SimpleJNI::JNIDescriptor integer_descriptor{
    "java/lang/Integer", &integer_class, {{"<init>", "(I)V", &integer_constructor}}};

const SimpleJNI::JNIDescriptor service_descriptor{
    "org/simpleble/android/Service",
    &service_class,
    {{"<init>", "(Ljava/lang/String;Ljava/util/List;)V", &service_constructor}}};

const SimpleJNI::JNIDescriptor characteristic_descriptor{
    "org/simpleble/android/Characteristic",
    &characteristic_class,
    {{"<init>", "(Ljava/lang/String;Ljava/util/List;ZZZZZ)V", &characteristic_constructor}}};

const SimpleJNI::JNIDescriptor descriptor_descriptor{"org/simpleble/android/Descriptor",
                                                     &descriptor_class,
                                                     {{"<init>", "(Ljava/lang/String;)V", &descriptor_constructor}}};

const SimpleJNI::JNIDescriptor exception_descriptor{
    "org/simpleble/android/SimpleDroidBleException", &exception_class, {}};

struct JavaTypes {};

const SimpleJNI::AutoRegister<JavaTypes> register_array_list{&array_list_descriptor};
const SimpleJNI::AutoRegister<JavaTypes> register_hash_map{&hash_map_descriptor};
const SimpleJNI::AutoRegister<JavaTypes> register_integer{&integer_descriptor};
const SimpleJNI::AutoRegister<JavaTypes> register_service{&service_descriptor};
const SimpleJNI::AutoRegister<JavaTypes> register_characteristic{&characteristic_descriptor};
const SimpleJNI::AutoRegister<JavaTypes> register_descriptor{&descriptor_descriptor};
const SimpleJNI::AutoRegister<JavaTypes> register_exception{&exception_descriptor};

SimpleJNI::Object<SimpleJNI::LocalRef> new_array_list() {
    return SimpleJNI::Object<SimpleJNI::LocalRef>::call_new_object(array_list_class.get(), array_list_constructor);
}

void add_to_list(const SimpleJNI::Object<SimpleJNI::LocalRef>& list, jobject value) {
    list.call_boolean_method(array_list_add, value);
}

void log_callback_error(const char* callback, const std::exception& exception) noexcept {
    SimpleBLE::Logging::Logger::get()->log(SimpleBLE::Logging::Level::Error, "SimpleDroidBLE", __FILE__, __LINE__,
                                           callback, std::string("Java callback failed: ") + exception.what());
}

void log_unknown_callback_error(const char* callback) noexcept {
    SimpleBLE::Logging::Logger::get()->log(SimpleBLE::Logging::Level::Error, "SimpleDroidBLE", __FILE__, __LINE__,
                                           callback, "Java callback failed with an unknown error");
}

template <typename Func>
void invoke_callback(const char* name, Func&& func) noexcept {
    try {
        std::forward<Func>(func)();
    } catch (const std::exception& exception) {
        log_callback_error(name, exception);
    } catch (...) {
        log_unknown_callback_error(name);
    }
}

}  // namespace

SimpleJNI::GlobalRef<jclass> AdapterCallback::_cls;
jmethodID AdapterCallback::_on_scan_start = nullptr;
jmethodID AdapterCallback::_on_scan_stop = nullptr;
jmethodID AdapterCallback::_on_scan_updated = nullptr;
jmethodID AdapterCallback::_on_scan_found = nullptr;

const SimpleJNI::JNIDescriptor AdapterCallback::_descriptor{"org/simpleble/android/Adapter$Callback",
                                                            &_cls,
                                                            {{"onScanStart", "()V", &_on_scan_start},
                                                             {"onScanStop", "()V", &_on_scan_stop},
                                                             {"onScanUpdated", "(J)V", &_on_scan_updated},
                                                             {"onScanFound", "(J)V", &_on_scan_found}}};

const SimpleJNI::AutoRegister<AdapterCallback> AdapterCallback::_registrar{&_descriptor};

AdapterCallback::AdapterCallback(jobject callback) : _callback(callback) {
    if (!_cls.get()) throw std::runtime_error("Adapter callback JNI resources are not loaded");
}

void AdapterCallback::on_scan_start() const noexcept {
    invoke_callback(__func__, [this] {
        if (_callback.is_valid()) _callback.to_local().call_void_method(_on_scan_start);
    });
}

void AdapterCallback::on_scan_stop() const noexcept {
    invoke_callback(__func__, [this] {
        if (_callback.is_valid()) _callback.to_local().call_void_method(_on_scan_stop);
    });
}

void AdapterCallback::on_scan_updated(int64_t peripheral_id) const noexcept {
    invoke_callback(__func__, [this, peripheral_id] {
        if (_callback.is_valid()) {
            _callback.to_local().call_void_method(_on_scan_updated, static_cast<jlong>(peripheral_id));
        }
    });
}

void AdapterCallback::on_scan_found(int64_t peripheral_id) const noexcept {
    invoke_callback(__func__, [this, peripheral_id] {
        if (_callback.is_valid()) {
            _callback.to_local().call_void_method(_on_scan_found, static_cast<jlong>(peripheral_id));
        }
    });
}

SimpleJNI::GlobalRef<jclass> PeripheralCallback::_cls;
jmethodID PeripheralCallback::_on_connected = nullptr;
jmethodID PeripheralCallback::_on_disconnected = nullptr;

const SimpleJNI::JNIDescriptor PeripheralCallback::_descriptor{
    "org/simpleble/android/Peripheral$Callback",
    &_cls,
    {{"onConnected", "()V", &_on_connected}, {"onDisconnected", "()V", &_on_disconnected}}};

const SimpleJNI::AutoRegister<PeripheralCallback> PeripheralCallback::_registrar{&_descriptor};

PeripheralCallback::PeripheralCallback(jobject callback) : _callback(callback) {
    if (!_cls.get()) throw std::runtime_error("Peripheral callback JNI resources are not loaded");
}

void PeripheralCallback::on_connected() const noexcept {
    invoke_callback(__func__, [this] {
        if (_callback.is_valid()) _callback.to_local().call_void_method(_on_connected);
    });
}

void PeripheralCallback::on_disconnected() const noexcept {
    invoke_callback(__func__, [this] {
        if (_callback.is_valid()) _callback.to_local().call_void_method(_on_disconnected);
    });
}

SimpleJNI::GlobalRef<jclass> DataCallback::_cls;
jmethodID DataCallback::_on_data_received = nullptr;

const SimpleJNI::JNIDescriptor DataCallback::_descriptor{
    "org/simpleble/android/Peripheral$DataCallback", &_cls, {{"onDataReceived", "([B)V", &_on_data_received}}};

const SimpleJNI::AutoRegister<DataCallback> DataCallback::_registrar{&_descriptor};

DataCallback::DataCallback(jobject callback) : _callback(callback) {
    if (!_cls.get()) throw std::runtime_error("Data callback JNI resources are not loaded");
}

void DataCallback::on_data_received(const SimpleBLE::ByteArray& data) const noexcept {
    invoke_callback(__func__, [this, &data] {
        if (!_callback) return;
        SimpleJNI::ByteArray<SimpleJNI::LocalRef> payload(data);
        _callback.to_local().call_void_method(_on_data_received, payload.get());
    });
}

jobject to_services(const std::vector<SimpleBLE::Service>& services) {
    auto service_list = new_array_list();

    for (auto service : services) {
        auto characteristic_list = new_array_list();

        for (auto characteristic : service.characteristics()) {
            auto descriptor_list = new_array_list();

            for (auto descriptor : characteristic.descriptors()) {
                SimpleJNI::String<SimpleJNI::LocalRef> uuid(descriptor.uuid());
                auto descriptor_object = SimpleJNI::Object<SimpleJNI::LocalRef>::call_new_object(
                    descriptor_class.get(), descriptor_constructor, uuid.get());
                add_to_list(descriptor_list, descriptor_object.get());
            }

            SimpleJNI::String<SimpleJNI::LocalRef> uuid(characteristic.uuid());
            auto characteristic_object = SimpleJNI::Object<SimpleJNI::LocalRef>::call_new_object(
                characteristic_class.get(), characteristic_constructor, uuid.get(), descriptor_list.get(),
                static_cast<jboolean>(characteristic.can_read()),
                static_cast<jboolean>(characteristic.can_write_request()),
                static_cast<jboolean>(characteristic.can_write_command()),
                static_cast<jboolean>(characteristic.can_notify()),
                static_cast<jboolean>(characteristic.can_indicate()));
            add_to_list(characteristic_list, characteristic_object.get());
        }

        SimpleJNI::String<SimpleJNI::LocalRef> uuid(service.uuid());
        auto service_object = SimpleJNI::Object<SimpleJNI::LocalRef>::call_new_object(
            service_class.get(), service_constructor, uuid.get(), characteristic_list.get());
        add_to_list(service_list, service_object.get());
    }

    SimpleJNI::Object<SimpleJNI::ReleasableLocalRef> result(service_list);
    return result.release();
}

jobject to_manufacturer_data(const std::map<uint16_t, SimpleBLE::ByteArray>& manufacturer_data) {
    auto hash_map = SimpleJNI::Object<SimpleJNI::LocalRef>::call_new_object(hash_map_class.get(), hash_map_constructor);

    for (const auto& [company_id, data] : manufacturer_data) {
        auto key = SimpleJNI::Object<SimpleJNI::LocalRef>::call_new_object(integer_class.get(), integer_constructor,
                                                                           static_cast<jint>(company_id));
        SimpleJNI::ByteArray<SimpleJNI::LocalRef> value(data);
        hash_map.call_object_method(hash_map_put, key.get(), value.get());
    }

    SimpleJNI::Object<SimpleJNI::ReleasableLocalRef> result(hash_map);
    return result.release();
}

void throw_exception(JNIEnv* env, const std::string& message) noexcept {
    SimpleBLE::Logging::Logger::get()->log(SimpleBLE::Logging::Level::Error, "SimpleDroidBLE", __FILE__, __LINE__,
                                           __func__, "Throwing exception: " + message);

    if (exception_class.get()) {
        env->ThrowNew(exception_class.get(), message.c_str());
    }
}

}  // namespace SimpleDroidJNI
