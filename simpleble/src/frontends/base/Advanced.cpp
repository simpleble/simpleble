#include "simpleble/Advanced.h"

#if defined(_WIN32)
namespace SimpleBLE::Advanced::Windows {}

#endif

#if TARGET_OS_OSX
#include "BuildVec.h"
#include "backends/macos/AdapterMac.h"

namespace SimpleBLE::Advanced::MacOS {

std::vector<Peripheral> retrieve_cached_peripherals(Adapter& adapter,
                                                    const std::vector<BluetoothAddress>& identifiers) {
    return Factory::vector(Factory::get_internal<AdapterMac>(adapter).retrieve_cached_peripherals(identifiers));
}

}  // namespace SimpleBLE::Advanced::MacOS

#endif

#if TARGET_OS_IOS
#include "BuildVec.h"
#include "backends/macos/AdapterMac.h"

namespace SimpleBLE::Advanced::iOS {

std::vector<Peripheral> retrieve_cached_peripherals(Adapter& adapter,
                                                    const std::vector<BluetoothAddress>& identifiers) {
    return Factory::vector(Factory::get_internal<AdapterMac>(adapter).retrieve_cached_peripherals(identifiers));
}

}  // namespace SimpleBLE::Advanced::iOS

#endif

#if defined(__ANDROID__)

#include "simplejni/VM.hpp"

namespace SimpleBLE::Advanced::Android {

JavaVM* get_jvm() { return SimpleJNI::VM::jvm(); }
void set_jvm(JavaVM* jvm) { SimpleJNI::VM::jvm(jvm); }

}  // namespace SimpleBLE::Advanced::Android

#endif

#if defined(__linux__) && !defined(__ANDROID__)
namespace SimpleBLE::Advanced::Linux {}

#endif
