#include "Integer.h"

namespace SimpleJNI {
namespace Types {

Class Integer::_cls;
jmethodID Integer::_method_init;

void Integer::initialize() {
    Env env;

    if (_cls.get() == nullptr) {
        _cls = env.find_class("java/lang/Integer");
    }

    if (!_method_init) {
        _method_init = env->GetMethodID(_cls.get(), "<init>", "(I)V");
    }
}

Integer::Integer(int value) : _obj(nullptr) {
    initialize();

    Env env;
    _obj = env.new_object(_cls, _method_init, value);
}

Integer::Integer(Object obj) : _obj(obj) { initialize(); }

void Integer::check_initialized() const {
    if (!_obj) throw std::runtime_error("Integer is not initialized");
}

}  // namespace Types
}  // namespace SimpleJNI