#include <simplecble/utils.h>

#include <simplecble/types.h>

#if __APPLE__
#include "TargetConditionals.h"
#endif

simpleble_os_t simpleble_get_operating_system() {
#ifdef _WIN32
    return SIMPLEBLE_OS_WINDOWS;
#elif TARGET_OS_OSX
    return SIMPLEBLE_OS_MACOS;
#elif TARGET_OS_IOS
    return SIMPLEBLE_OS_IOS;
#elif __ANDROID__
    return SIMPLEBLE_OS_ANDROID;
#elif __linux__
    return SIMPLEBLE_OS_LINUX;
#else
    return SIMPLEBLE_OS_UNKNOWN;
#endif
}

const char* simpleble_get_version() { return SIMPLEBLE_VERSION; }
