#include "List.h"
#include "Iterator.h"

namespace SimpleBLE {
namespace Android {

namespace {
    SimpleJNI::GlobalRef<jclass> array_list_cls;
    jmethodID array_list_constructor;
    jmethodID array_list_add;

    struct ArrayList {
        static const SimpleJNI::JNIDescriptor descriptor;
    };

    const SimpleJNI::JNIDescriptor ArrayList::descriptor = {
        "java/util/ArrayList",
        &array_list_cls,
        {
            {"<init>", "()V", &array_list_constructor},
            {"add", "(Ljava/lang/Object;)Z", &array_list_add},
        }
    };
    const SimpleJNI::AutoRegister<ArrayList> array_list_registrar{&ArrayList::descriptor};
}

SimpleJNI::GlobalRef<jclass> List::_cls;
jmethodID List::_method_iterator = nullptr;

const SimpleJNI::JNIDescriptor List::descriptor{
    "java/util/List", // Java class name
    &_cls,            // Where to store the jclass
    {                 // Methods to preload
     {"iterator", "()Ljava/util/Iterator;", &_method_iterator}
    }
};

const SimpleJNI::AutoRegister<List> List::registrar{&descriptor};

List::List(SimpleJNI::Object<SimpleJNI::GlobalRef, jobject> obj) : _obj(obj) {}

List::List() : _obj(SimpleJNI::Object<SimpleJNI::LocalRef>(SimpleJNI::VM::env()->NewObject(array_list_cls.get(), array_list_constructor)).to_global()) {}

void List::add(SimpleJNI::Object<SimpleJNI::GlobalRef, jobject>& obj) {
    _obj.call_boolean_method(array_list_add, obj.get());
}

Iterator List::iterator() {
    if (!_obj) throw std::runtime_error("List is not initialized");
    SimpleJNI::Object<SimpleJNI::LocalRef> iterator_obj = _obj.call_object_method(_method_iterator);
    return Iterator(iterator_obj.to_global());
}

jobject List::get() const {
    return _obj.get();
}

}  // namespace Android
}  // namespace SimpleBLE
