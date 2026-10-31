#include <pybind11/pybind11.h>
#include <pybind11/stl.h>

#include <optional>
#include <string>

#include "simpleble/Advanced.h"

#include "python_callback.hpp"

namespace py = pybind11;

constexpr auto kDocsDonglSetPasskeyRequestCallback = R"pbdoc(
    Register a callback that returns the six-digit passkey requested by the peer, or None to reject
)pbdoc";

constexpr auto kDocsDonglSetPasskeyDisplayCallback = R"pbdoc(
    Register a callback that receives the six-digit passkey the peer must enter
)pbdoc";

constexpr auto kDocsDonglSetNumericComparisonCallback = R"pbdoc(
    Register a callback that returns True only if the peer shows the same six-digit number
)pbdoc";

constexpr auto kDocsDonglSecureConnection = R"pbdoc(
    Secure the connection with LE Secure Connections, also authenticating the peer if mitm is True
)pbdoc";

constexpr auto kDocsSetAdvertisementLocalName = R"pbdoc(
    Set the local name advertised by a local peripheral, or None to remove the override
)pbdoc";

constexpr auto kDocsLinuxSetPasskeyRequestCallback = R"pbdoc(
    Register a callback that receives the peer's address and returns the six-digit passkey it requested, or None to reject
)pbdoc";

constexpr auto kDocsLinuxSetPasskeyDisplayCallback = R"pbdoc(
    Register a callback that receives the peer's address and the six-digit passkey the peer must enter
)pbdoc";

constexpr auto kDocsLinuxSetNumericComparisonCallback = R"pbdoc(
    Register a callback that receives the peer's address and returns True only if the peer shows the same six-digit number
)pbdoc";

constexpr auto kDocsLinuxSetPairingAuthorizationCallback = R"pbdoc(
    Register a callback that receives the peer's address and returns True to accept Just Works pairing
)pbdoc";

// NOTE: SimpleBLE validates the passkey and rejects the pairing request if a callback raises.
static std::optional<std::string> passkey_answer(const py::object& passkey) {
    if (passkey.is_none()) return std::nullopt;
    if (!py::isinstance<py::str>(passkey)) {
        throw py::type_error(std::string("passkey must be a str or None, not ") + Py_TYPE(passkey.ptr())->tp_name);
    }
    return passkey.cast<std::string>();
}

static bool accept_answer(const py::object& accept, const char* what) {
    if (!PyBool_Check(accept.ptr())) {
        throw py::type_error(std::string(what) + " result must be a bool, not " + Py_TYPE(accept.ptr())->tp_name);
    }
    return accept.ptr() == Py_True;
}

void wrap_advanced(py::module& m) {
    auto advanced = m.def_submodule("advanced", "Platform-specific advanced SimpleBLE APIs");

    auto dongl = advanced.def_submodule("dongl", "SimpleBLE Dongl pairing APIs");
    dongl.def(
        "set_passkey_request_callback",
        [](SimpleBLE::Peripheral& peripheral, std::optional<py::function> cb) {
            if (!cb) {
                SimpleBLE::Advanced::Dongl::set_passkey_request_callback(peripheral, nullptr);
                return;
            }
            simplepyble::PythonCallback callback(std::move(*cb));
            SimpleBLE::Advanced::Dongl::set_passkey_request_callback(
                peripheral, [callback]() {
                    return callback.call([](const py::function& callable) { return passkey_answer(callable()); });
                });
        },
        py::call_guard<py::gil_scoped_release>(), kDocsDonglSetPasskeyRequestCallback);
    dongl.def(
        "set_passkey_display_callback",
        [](SimpleBLE::Peripheral& peripheral, std::optional<py::function> cb) {
            SimpleBLE::Advanced::Dongl::set_passkey_display_callback(
                peripheral, simplepyble::native_callback<void(const std::string&)>(std::move(cb)));
        },
        py::call_guard<py::gil_scoped_release>(), kDocsDonglSetPasskeyDisplayCallback);
    dongl.def(
        "set_numeric_comparison_callback",
        [](SimpleBLE::Peripheral& peripheral, std::optional<py::function> cb) {
            if (!cb) {
                SimpleBLE::Advanced::Dongl::set_numeric_comparison_callback(peripheral, nullptr);
                return;
            }
            simplepyble::PythonCallback callback(std::move(*cb));
            SimpleBLE::Advanced::Dongl::set_numeric_comparison_callback(
                peripheral, [callback](const std::string& passkey) {
                    return callback.call([&](const py::function& callable) {
                        return accept_answer(callable(passkey), "numeric comparison");
                    });
                });
        },
        py::call_guard<py::gil_scoped_release>(), kDocsDonglSetNumericComparisonCallback);
    dongl.def("secure_connection", &SimpleBLE::Advanced::Dongl::secure_connection, py::arg("peripheral"),
              py::arg("mitm") = false, py::call_guard<py::gil_scoped_release>(), kDocsDonglSecureConnection);

#if SIMPLEPYBLE_HAS_MACOS_ADVANCED
    auto macos = advanced.def_submodule("macos", "Advanced CoreBluetooth APIs");
    macos.def("set_advertisement_local_name", &SimpleBLE::Advanced::MacOS::set_advertisement_local_name,
              py::call_guard<py::gil_scoped_release>(), kDocsSetAdvertisementLocalName);
    macos.def("retrieve_cached_peripherals", &SimpleBLE::Advanced::MacOS::retrieve_cached_peripherals,
              py::call_guard<py::gil_scoped_release>(),
              "Retrieve peripherals that CoreBluetooth can resolve from its system cache");
#endif

#if SIMPLEPYBLE_HAS_LINUX_ADVANCED
    auto linux_module = advanced.def_submodule("linux", "Advanced BlueZ APIs");
    linux_module.def("set_advertisement_local_name", &SimpleBLE::Advanced::Linux::set_advertisement_local_name,
                     py::call_guard<py::gil_scoped_release>(), kDocsSetAdvertisementLocalName);

    // NOTE: These callbacks run on SimpleBLE's BlueZ event thread, which waits for their answer.
    linux_module.def(
        "set_passkey_request_callback",
        [](SimpleBLE::Adapter& adapter, std::optional<py::function> cb) {
            if (!cb) {
                SimpleBLE::Advanced::Linux::set_passkey_request_callback(adapter, nullptr);
                return;
            }
            simplepyble::PythonCallback callback(std::move(*cb));
            SimpleBLE::Advanced::Linux::set_passkey_request_callback(
                adapter, [callback](SimpleBLE::BluetoothAddress peer) {
                    return callback.call(
                        [&](const py::function& callable) { return passkey_answer(callable(peer)); });
                });
        },
        py::call_guard<py::gil_scoped_release>(), kDocsLinuxSetPasskeyRequestCallback);
    linux_module.def(
        "set_passkey_display_callback",
        [](SimpleBLE::Adapter& adapter, std::optional<py::function> cb) {
            SimpleBLE::Advanced::Linux::set_passkey_display_callback(
                adapter,
                simplepyble::native_callback<void(SimpleBLE::BluetoothAddress, const std::string&)>(std::move(cb)));
        },
        py::call_guard<py::gil_scoped_release>(), kDocsLinuxSetPasskeyDisplayCallback);
    linux_module.def(
        "set_numeric_comparison_callback",
        [](SimpleBLE::Adapter& adapter, std::optional<py::function> cb) {
            if (!cb) {
                SimpleBLE::Advanced::Linux::set_numeric_comparison_callback(adapter, nullptr);
                return;
            }
            simplepyble::PythonCallback callback(std::move(*cb));
            SimpleBLE::Advanced::Linux::set_numeric_comparison_callback(
                adapter, [callback](SimpleBLE::BluetoothAddress peer, const std::string& passkey) {
                    return callback.call([&](const py::function& callable) {
                        return accept_answer(callable(peer, passkey), "numeric comparison");
                    });
                });
        },
        py::call_guard<py::gil_scoped_release>(), kDocsLinuxSetNumericComparisonCallback);
    linux_module.def(
        "set_pairing_authorization_callback",
        [](SimpleBLE::Adapter& adapter, std::optional<py::function> cb) {
            if (!cb) {
                SimpleBLE::Advanced::Linux::set_pairing_authorization_callback(adapter, nullptr);
                return;
            }
            simplepyble::PythonCallback callback(std::move(*cb));
            SimpleBLE::Advanced::Linux::set_pairing_authorization_callback(
                adapter, [callback](SimpleBLE::BluetoothAddress peer) {
                    return callback.call([&](const py::function& callable) {
                        return accept_answer(callable(peer), "pairing authorization");
                    });
                });
        },
        py::call_guard<py::gil_scoped_release>(), kDocsLinuxSetPairingAuthorizationCallback);
#endif
}
