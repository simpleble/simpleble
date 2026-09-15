#include "simpleble/Advanced.h"

#include <utility>

#if SIMPLEBLE_BACKEND_WINDOWS
namespace SimpleBLE::Advanced::Windows {}

#endif

#if SIMPLEBLE_BACKEND_MACOS
#include "BuildVec.h"
#include "backends/macos/AdapterMac.h"
#include "backends/macos/LocalPeripheralMac.h"

namespace SimpleBLE::Advanced::MacOS {

void set_advertisement_local_name(Local::Peripheral& peripheral, std::optional<std::string> local_name) {
    Factory::get_internal<Local::PeripheralMac>(peripheral).set_advertisement_local_name(std::move(local_name));
}

std::vector<Peripheral> retrieve_cached_peripherals(Adapter& adapter,
                                                    const std::vector<BluetoothAddress>& identifiers) {
    return Factory::vector(Factory::get_internal<AdapterMac>(adapter).retrieve_cached_peripherals(identifiers));
}

}  // namespace SimpleBLE::Advanced::MacOS

#endif

#if SIMPLEBLE_BACKEND_IOS
#include "BuildVec.h"
#include "backends/macos/AdapterMac.h"
#include "backends/macos/LocalPeripheralMac.h"

namespace SimpleBLE::Advanced::iOS {

void set_advertisement_local_name(Local::Peripheral& peripheral, std::optional<std::string> local_name) {
    Factory::get_internal<Local::PeripheralMac>(peripheral).set_advertisement_local_name(std::move(local_name));
}

std::vector<Peripheral> retrieve_cached_peripherals(Adapter& adapter,
                                                    const std::vector<BluetoothAddress>& identifiers) {
    return Factory::vector(Factory::get_internal<AdapterMac>(adapter).retrieve_cached_peripherals(identifiers));
}

}  // namespace SimpleBLE::Advanced::iOS

#endif

#if SIMPLEBLE_BACKEND_ANDROID

#include "backends/android/BackendAndroid.h"
#include "simplejni/VM.hpp"

namespace SimpleBLE::Advanced::Android {

JavaVM* get_jvm() { return SimpleJNI::VM::jvm(); }
void set_jvm(JavaVM* jvm) { SimpleJNI::VM::jvm(jvm); }
void set_context(jobject context) { SimpleBLE::BackendAndroid::set_application_context(context); }

}  // namespace SimpleBLE::Advanced::Android

#endif

#if SIMPLEBLE_BACKEND_LINUX
#include "BuilderBase.h"
#include "backends/linux/LocalPeripheralLinux.h"

namespace SimpleBLE::Advanced::Linux {

void set_advertisement_local_name(Local::Peripheral& peripheral, std::optional<std::string> local_name) {
    Factory::get_internal<Local::PeripheralLinux>(peripheral).set_advertisement_local_name(std::move(local_name));
}

}  // namespace SimpleBLE::Advanced::Linux

#endif
