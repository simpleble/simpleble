#include "Iterator.h"

namespace SimpleJNI {
namespace Types {

Class Iterator::_cls;
jmethodID Iterator::_method_hasNext = nullptr;
jmethodID Iterator::_method_next = nullptr;

void Iterator::initialize() {
    Env env;

    if (_cls.get() == nullptr) {
        _cls = env.find_class("java/util/Iterator");
    }

    if (!_method_hasNext) {
        _method_hasNext = env->GetMethodID(_cls.get(), "hasNext", "()Z");
    }

    if (!_method_next) {
        _method_next = env->GetMethodID(_cls.get(), "next", "()Ljava/lang/Object;");
    }
}

Iterator::Iterator(Object obj) : _obj(obj) { initialize(); }

void Iterator::check_initialized() const {
    if (!_obj) throw std::runtime_error("Iterator is not initialized");
}

bool Iterator::hasNext() {
    check_initialized();
    return _obj.call_boolean_method(_method_hasNext);
}

Object Iterator::next() {
    check_initialized();
    return _obj.call_object_method(_method_next);
}

}  // namespace Types
}  // namespace SimpleJNI