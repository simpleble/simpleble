#include <simplecble/utils.h>

#include <simplecble/types.h>
#include <simpleble/Utils.h>

#include <string>

simpleble_os_t simpleble_get_operating_system() {
#ifdef _WIN32
    return SIMPLEBLE_OS_WINDOWS;
#elif __APPLE__
    return SIMPLEBLE_OS_MACOS;
#elif __linux__
    return SIMPLEBLE_OS_LINUX;
#endif
}

const char* simpleble_get_version() { return SIMPLEBLE_VERSION; }

const char* simpleble_get_company_name(uint16_t company_id) {
    try {
        static thread_local std::string result;
        result = SimpleBLE::get_company_name(company_id);
        return result.c_str();
    } catch (...) {
        return nullptr;
    }
}
