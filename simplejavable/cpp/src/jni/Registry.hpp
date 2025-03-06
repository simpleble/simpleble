#pragma once
#include "Common.hpp"
#include <string>
#include <vector>

namespace SimpleJNI {

/**
 * @brief Descriptor for a single Java method to be preloaded.
 *
 * This struct holds the metadata for a method that a Java class needs to expose to C++.
 * It links a method name and signature to a jmethodID pointer where the resolved ID will
 * be stored during JNI_OnLoad.
 */
struct MethodDescriptor {
    std::string name;           ///< Name of the Java method (e.g., "startScan").
    std::string signature;      ///< JNI signature of the method (e.g., "(Landroid/bluetooth/le/ScanCallback;)V").
    jmethodID* target = nullptr; ///< Pointer to where the resolved jmethodID will be stored.

    MethodDescriptor(const std::string& n, const std::string& sig, jmethodID* tgt)
        : name(n), signature(sig), target(tgt) {}
};

/**
 * @brief Descriptor for a Java class and its methods to be preloaded.
 *
 * This struct defines a Java class by its fully qualified name and a list of methods
 * that need JNI access. It ties the class and its methods to storage locations
 * (GlobalRef<jclass> and jmethodID pointers) that will be populated during preloading.
 */
struct JNIDescriptor {
    std::string class_name;                  ///< Fully qualified Java class name (e.g., "android/bluetooth/le/BluetoothLeScanner").
    SimpleJNI::GlobalRef<jclass>* class_target; ///< Pointer to where the resolved jclass will be stored.
    std::vector<MethodDescriptor> methods;    ///< List of methods to preload for this class.

    JNIDescriptor(const std::string& name, SimpleJNI::GlobalRef<jclass>* cls_target,
                  std::initializer_list<MethodDescriptor> meths)
        : class_name(name), class_target(cls_target), methods(meths) {}
};

/**
 * @brief Singleton registrar for collecting and preloading JNI class descriptors.
 *
 * The Registrar collects JNIDescriptor instances at compile-time (via static initialization)
 * and preloads their JNI resources (jclass and jmethodID) during JNI_OnLoad. This ensures
 * all necessary JNI metadata is resolved early on the correct thread, avoiding runtime
 * ClassLoader or thread-context issues.
 */
class Registrar {
  public:
    /// @brief Get the singleton instance of the Registrar.
    static Registrar& get() {
        static Registrar instance;
        return instance;
    }

    /**
     * @brief Register a JNIDescriptor for preloading.
     *
     * Called automatically by AutoRegister during static initialization. Adds the descriptor
     * to an internal list that will be processed in preload().
     * @param descriptor Pointer to the JNIDescriptor to register.
     */
    void register_descriptor(const JNIDescriptor* descriptor) {
        descriptors.push_back(descriptor);
    }

    /**
     * @brief Preload all registered JNI resources.
     *
     * Called from JNI_OnLoad to resolve jclass and jmethodID for all registered descriptors.
     * Uses the provided JNIEnv to perform JNI lookups on the thread that loads the library,
     * ensuring access to the correct ClassLoader context (e.g., system classes on Android).
     * @param env JNIEnv pointer from JNI_OnLoad.
     * @throws std::runtime_error if a class or method cannot be resolved.
     */
    void preload(JNIEnv* env) {
        for (const JNIDescriptor* desc : descriptors) {
            // Load the Java class
            jclass local_cls = env->FindClass(desc->class_name.c_str());
            if (!local_cls) {
                throw std::runtime_error("Failed to load class: " + desc->class_name);
            }
            *desc->class_target = SimpleJNI::GlobalRef<jclass>(local_cls);
            env->DeleteLocalRef(local_cls); // Clean up the local reference

            // Load each method
            for (const MethodDescriptor& method : desc->methods) {
                *method.target = env->GetMethodID(desc->class_target->get(),
                                                 method.name.c_str(),
                                                 method.signature.c_str());
                if (!*method.target) {
                    throw std::runtime_error("Failed to get method: " + desc->class_name + "." + method.name);
                }
            }
        }
    }

  private:
    Registrar() = default;
    ~Registrar() = default;
    Registrar(const Registrar&) = delete;
    Registrar& operator=(const Registrar&) = delete;

    std::vector<const JNIDescriptor*> descriptors; ///< List of registered descriptors.
};

/**
 * @brief Helper to automatically register a JNIDescriptor at compile time.
 *
 * This template class uses static initialization to register a descriptor with the Registrar
 * when the program loads. The constructor runs before main() or JNI_OnLoad, ensuring all
 * classes are registered without manual intervention.
 * @tparam T The class type (e.g., BluetoothScanner) that owns the descriptor.
 */
template <typename T>
struct AutoRegister {
    AutoRegister(const JNIDescriptor* desc) {
        Registrar::get().register_descriptor(desc);
    }
};

}  // namespace SimpleJNI