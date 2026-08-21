#pragma once

#include <string>
#include <utility>

#include "simplejni/Common.hpp"
#include "simplejni/Registry.hpp"

namespace SimpleBLE::Android {

class Context {
  public:
    explicit Context(SimpleJNI::Object<SimpleJNI::GlobalRef, jobject> object) : _obj(std::move(object)) {}

    SimpleJNI::Object<SimpleJNI::GlobalRef, jobject> getSystemService(const std::string& name) const;
    jobject get() const { return _obj.get(); }

  private:
    SimpleJNI::Object<SimpleJNI::GlobalRef, jobject> _obj;

    static SimpleJNI::GlobalRef<jclass> _cls;
    static jmethodID _get_system_service;
    static const SimpleJNI::JNIDescriptor descriptor;
    static const SimpleJNI::AutoRegister<Context> registrar;
};

}  // namespace SimpleBLE::Android
