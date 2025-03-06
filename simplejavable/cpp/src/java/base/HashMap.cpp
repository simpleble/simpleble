#include "HashMap.h"

namespace SimpleJNI {
namespace Types {

Class HashMap::_cls;
jmethodID HashMap::_method_init;
jmethodID HashMap::_method_put;

void HashMap::initialize() {
    Env env;

    if (_cls.get() == nullptr) {
        _cls = env.find_class("java/util/HashMap");
    }

    if (!_method_init) {
        _method_init = env->GetMethodID(_cls.get(), "<init>", "()V");
    }

    if (!_method_put) {
        _method_put = env->GetMethodID(_cls.get(), "put", "(Ljava/lang/Object;Ljava/lang/Object;)Ljava/lang/Object;");
    }
}

HashMap::HashMap() : _obj(nullptr) {
    initialize();

    Env env;
    _obj = env.new_object(_cls, _method_init);
}

HashMap::HashMap(Object obj) : _obj(obj) { initialize(); }

Object HashMap::put(Object key, Object value) {
    check_initialized();
    return _obj.call_object_method(_method_put, key.get(), value.get());
}

void HashMap::check_initialized() const {
    if (!_obj) throw std::runtime_error("HashMap is not initialized");
}

}  // namespace Types
}  // namespace SimpleJNI