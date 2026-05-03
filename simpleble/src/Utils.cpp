#include <simpleble/Utils.h>
#include "Constants.h"
#if SIMPLEBLE_BACKEND_MACOS || SIMPLEBLE_BACKEND_IOS
#include "TargetConditionals.h"
#endif

namespace SimpleBLE {

OperatingSystem get_operating_system() {
#ifdef _WIN32
    return OperatingSystem::WINDOWS;
#elif TARGET_OS_OSX
    return OperatingSystem::MACOS;
#elif TARGET_OS_IOS
    return OperatingSystem::IOS;
#elif __linux__
    return OperatingSystem::LINUX;
#elif __ANDROID__
    return OperatingSystem::ANDROID;
#endif
}

std::string get_simpleble_version() { return SIMPLEBLE_VERSION; }

std::string get_company_name(uint16_t company_id) {
    auto it = COMPANY_IDENTIFIERS.find(company_id);
    return (it != COMPANY_IDENTIFIERS.end()) ? it->second : "Unknown";
}

}  // namespace SimpleBLE