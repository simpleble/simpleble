#include <pybind11/pybind11.h>

#include <chrono>

#include "simpleble/Config.h"

namespace py = pybind11;

// Wrapper classes for Python bindings (not exposed to C++)
namespace PyWrappers {
    struct WinRT {
        static bool get_experimental_use_own_mta_apartment() {
            return SimpleBLE::Config::WinRT::experimental_use_own_mta_apartment;
        }
        static void set_experimental_use_own_mta_apartment(bool value) {
            SimpleBLE::Config::WinRT::experimental_use_own_mta_apartment = value;
        }
        static bool get_experimental_reinitialize_winrt_apartment_on_main_thread() {
            return SimpleBLE::Config::WinRT::experimental_reinitialize_winrt_apartment_on_main_thread;
        }
        static void set_experimental_reinitialize_winrt_apartment_on_main_thread(bool value) {
            SimpleBLE::Config::WinRT::experimental_reinitialize_winrt_apartment_on_main_thread = value;
        }
        static bool get_use_deferred_disconnect() {
            return SimpleBLE::Config::WinRT::use_deferred_disconnect;
        }
        static void set_use_deferred_disconnect(bool value) {
            SimpleBLE::Config::WinRT::use_deferred_disconnect = value;
        }

        static void reset() {
            SimpleBLE::Config::WinRT::reset();
        }
    };

    struct SimpleBluez {
        static bool get_use_system_bus() {
            return SimpleBLE::Config::SimpleBluez::use_system_bus;
        }
        static void set_use_system_bus(bool value) {
            SimpleBLE::Config::SimpleBluez::use_system_bus = value;
        }
        static int64_t get_connection_timeout_ms() {
            return std::chrono::duration_cast<std::chrono::milliseconds>(
                       SimpleBLE::Config::SimpleBluez::connection_timeout)
                .count();
        }
        static void set_connection_timeout_ms(int64_t value) {
            SimpleBLE::Config::SimpleBluez::connection_timeout = std::chrono::milliseconds(value);
        }
        static int64_t get_disconnection_timeout_ms() {
            return std::chrono::duration_cast<std::chrono::milliseconds>(
                       SimpleBLE::Config::SimpleBluez::disconnection_timeout)
                .count();
        }
        static void set_disconnection_timeout_ms(int64_t value) {
            SimpleBLE::Config::SimpleBluez::disconnection_timeout = std::chrono::milliseconds(value);
        }

        static void reset() {
            SimpleBLE::Config::SimpleBluez::reset();
        }
    };

    struct CoreBluetooth {
        static void reset() {
            SimpleBLE::Config::CoreBluetooth::reset();
        }
    };

    struct Android {
        static SimpleBLE::Config::Android::ConnectionPriorityRequest get_connection_priority_request() {
            return SimpleBLE::Config::Android::connection_priority_request;
        }
        static void set_connection_priority_request(SimpleBLE::Config::Android::ConnectionPriorityRequest value) {
            SimpleBLE::Config::Android::connection_priority_request = value;
        }

        static void reset() {
            SimpleBLE::Config::Android::reset();
        }
    };

    struct Dongl {
        static bool get_use_dongl_backend() {
            return SimpleBLE::Config::Dongl::use_dongl_backend;
        }
        static void set_use_dongl_backend(bool value) {
            SimpleBLE::Config::Dongl::use_dongl_backend = value;
        }
        static bool get_auto_update() {
            return SimpleBLE::Config::Dongl::auto_update;
        }
        static void set_auto_update(bool value) {
            SimpleBLE::Config::Dongl::auto_update = value;
        }
        static bool get_force_update() {
            return SimpleBLE::Config::Dongl::force_update;
        }
        static void set_force_update(bool value) {
            SimpleBLE::Config::Dongl::force_update = value;
        }

        static void reset() {
            SimpleBLE::Config::Dongl::reset();
        }
    };

    struct Base {
        static void reset_all() {
            SimpleBLE::Config::Base::reset_all();
        }
    };
}

// Documentation strings
constexpr auto kDocsConfigModule = R"pbdoc(
    Configuration options for SimpleBLE. Set configuration values before retrieving any adapter.
)pbdoc";

constexpr auto kDocsConfigWinRTClass = R"pbdoc(
    WinRT-specific configuration options
)pbdoc";

constexpr auto kDocsConfigSimpleBluezClass = R"pbdoc(
    SimpleBluez-specific configuration options
)pbdoc";

constexpr auto kDocsConfigCoreBluetoothClass = R"pbdoc(
    CoreBluetooth-specific configuration options
)pbdoc";

constexpr auto kDocsConfigAndroidClass = R"pbdoc(
    Android-specific configuration options
)pbdoc";

constexpr auto kDocsConfigDonglClass = R"pbdoc(
    SimpleBLE Dongl configuration options
)pbdoc";

constexpr auto kDocsConfigBaseClass = R"pbdoc(
    Base configuration options
)pbdoc";

constexpr auto kDocsConfigWinRTExperimentalMTA = R"pbdoc(
    Deprecated: SimpleBLE uses its own MTA apartment by default. This compatibility flag will be removed in a future release.
)pbdoc";

constexpr auto kDocsConfigWinRTExperimentalReinitializeMTAOnMainThread = R"pbdoc(
    Reinitialize the WinRT apartment on the main thread (experimental)
)pbdoc";

constexpr auto kDocsConfigWinRTUseDeferredDisconnect = R"pbdoc(
    Defer WinRT disconnect completion so disconnect calls return without waiting for the OS
)pbdoc";

constexpr auto kDocsConfigWinRTReset = R"pbdoc(
    Reset WinRT configuration options to their default values
)pbdoc";

constexpr auto kDocsConfigSimpleBluezReset = R"pbdoc(
    Reset SimpleBluez configuration options to their default values
)pbdoc";

constexpr auto kDocsConfigSimpleBluezUseSystemBus = R"pbdoc(
    Use the system D-Bus instead of the session D-Bus
)pbdoc";

constexpr auto kDocsConfigSimpleBluezConnectionTimeout = R"pbdoc(
    Connection timeout in milliseconds
)pbdoc";

constexpr auto kDocsConfigSimpleBluezDisconnectionTimeout = R"pbdoc(
    Disconnection timeout in milliseconds
)pbdoc";

constexpr auto kDocsConfigCoreBluetoothReset = R"pbdoc(
    Reset CoreBluetooth configuration options to their default values
)pbdoc";

constexpr auto kDocsConfigAndroidReset = R"pbdoc(
    Reset Android configuration options to their default values
)pbdoc";

constexpr auto kDocsConfigAndroidConnectionPriority = R"pbdoc(
    Android connection priority request
)pbdoc";

constexpr auto kDocsConfigDonglReset = R"pbdoc(
    Reset SimpleBLE Dongl configuration options to their default values
)pbdoc";

constexpr auto kDocsConfigDonglUseBackend = R"pbdoc(
    Enable the SimpleBLE Dongl backend
)pbdoc";

constexpr auto kDocsConfigDonglAutoUpdate = R"pbdoc(
    Automatically update SimpleBLE Dongl firmware
)pbdoc";

constexpr auto kDocsConfigDonglForceUpdate = R"pbdoc(
    Force a SimpleBLE Dongl firmware update
)pbdoc";

constexpr auto kDocsConfigBaseResetAll = R"pbdoc(
    Reset all configuration options to their default values
)pbdoc";

void wrap_config(py::module& m) {
    py::enum_<SimpleBLE::Config::Android::ConnectionPriorityRequest>(m, "AndroidConnectionPriority")
        .value("DISABLED", SimpleBLE::Config::Android::ConnectionPriorityRequest::DISABLED)
        .value("BALANCED", SimpleBLE::Config::Android::ConnectionPriorityRequest::BALANCED)
        .value("HIGH", SimpleBLE::Config::Android::ConnectionPriorityRequest::HIGH)
        .value("LOW_POWER", SimpleBLE::Config::Android::ConnectionPriorityRequest::LOW_POWER)
        .value("DCK", SimpleBLE::Config::Android::ConnectionPriorityRequest::DCK);

    auto config = m.def_submodule("config", kDocsConfigModule);

    // Define classes directly under config
    py::class_<PyWrappers::WinRT> winrt_config(config, "winrt", kDocsConfigWinRTClass, py::metaclass());
    winrt_config
        .def_property_static("experimental_use_own_mta_apartment",
            [](py::object) { return PyWrappers::WinRT::get_experimental_use_own_mta_apartment(); },
            [](py::object, bool value) { PyWrappers::WinRT::set_experimental_use_own_mta_apartment(value); },
            kDocsConfigWinRTExperimentalMTA)
        .def_property_static("experimental_reinitialize_winrt_apartment_on_main_thread",
            [](py::object) { return PyWrappers::WinRT::get_experimental_reinitialize_winrt_apartment_on_main_thread(); },
            [](py::object, bool value) { PyWrappers::WinRT::set_experimental_reinitialize_winrt_apartment_on_main_thread(value); },
            kDocsConfigWinRTExperimentalReinitializeMTAOnMainThread)
        .def_property_static("use_deferred_disconnect",
            [](py::object) { return PyWrappers::WinRT::get_use_deferred_disconnect(); },
            [](py::object, bool value) { PyWrappers::WinRT::set_use_deferred_disconnect(value); },
            kDocsConfigWinRTUseDeferredDisconnect)
        .def_static("reset", &PyWrappers::WinRT::reset, kDocsConfigWinRTReset);

    py::class_<PyWrappers::SimpleBluez> simplebluez_config(config, "simplebluez", kDocsConfigSimpleBluezClass, py::metaclass());
    simplebluez_config
        .def_property_static("use_system_bus",
            [](py::object) { return PyWrappers::SimpleBluez::get_use_system_bus(); },
            [](py::object, bool value) { PyWrappers::SimpleBluez::set_use_system_bus(value); },
            kDocsConfigSimpleBluezUseSystemBus)
        .def_property_static("connection_timeout_ms",
            [](py::object) { return PyWrappers::SimpleBluez::get_connection_timeout_ms(); },
            [](py::object, int64_t value) { PyWrappers::SimpleBluez::set_connection_timeout_ms(value); },
            kDocsConfigSimpleBluezConnectionTimeout)
        .def_property_static("disconnection_timeout_ms",
            [](py::object) { return PyWrappers::SimpleBluez::get_disconnection_timeout_ms(); },
            [](py::object, int64_t value) { PyWrappers::SimpleBluez::set_disconnection_timeout_ms(value); },
            kDocsConfigSimpleBluezDisconnectionTimeout)
        .def_static("reset", &PyWrappers::SimpleBluez::reset, kDocsConfigSimpleBluezReset);

    py::class_<PyWrappers::CoreBluetooth> corebluetooth_config(config, "corebluetooth", kDocsConfigCoreBluetoothClass, py::metaclass());
    corebluetooth_config
        .def_static("reset", &PyWrappers::CoreBluetooth::reset, kDocsConfigCoreBluetoothReset);

    py::class_<PyWrappers::Android> android_config(config, "android", kDocsConfigAndroidClass, py::metaclass());
    android_config
        .def_property_static("connection_priority_request",
            [](py::object) { return PyWrappers::Android::get_connection_priority_request(); },
            [](py::object, SimpleBLE::Config::Android::ConnectionPriorityRequest value) {
                PyWrappers::Android::set_connection_priority_request(value);
            },
            kDocsConfigAndroidConnectionPriority)
        .def_static("reset", &PyWrappers::Android::reset, kDocsConfigAndroidReset);

    py::class_<PyWrappers::Dongl> dongl_config(config, "dongl", kDocsConfigDonglClass, py::metaclass());
    dongl_config
        .def_property_static("use_dongl_backend",
            [](py::object) { return PyWrappers::Dongl::get_use_dongl_backend(); },
            [](py::object, bool value) { PyWrappers::Dongl::set_use_dongl_backend(value); },
            kDocsConfigDonglUseBackend)
        .def_property_static("auto_update",
            [](py::object) { return PyWrappers::Dongl::get_auto_update(); },
            [](py::object, bool value) { PyWrappers::Dongl::set_auto_update(value); },
            kDocsConfigDonglAutoUpdate)
        .def_property_static("force_update",
            [](py::object) { return PyWrappers::Dongl::get_force_update(); },
            [](py::object, bool value) { PyWrappers::Dongl::set_force_update(value); },
            kDocsConfigDonglForceUpdate)
        .def_static("reset", &PyWrappers::Dongl::reset, kDocsConfigDonglReset);

    py::class_<PyWrappers::Base> base_config(config, "base", kDocsConfigBaseClass, py::metaclass());
    base_config
        .def_static("reset_all", &PyWrappers::Base::reset_all, kDocsConfigBaseResetAll);
}
