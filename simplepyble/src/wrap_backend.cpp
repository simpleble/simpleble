#include <pybind11/pybind11.h>
#include <pybind11/stl.h>

#include "simpleble/Backend.h"

namespace py = pybind11;

constexpr auto kDocsBackend = R"pbdoc(
    SimpleBLE backend
)pbdoc";

constexpr auto kDocsBackendGetBackends = R"pbdoc(
    Get all enabled SimpleBLE backends
)pbdoc";

constexpr auto kDocsBackendGetNative = R"pbdoc(
    Get the backend of the operating system's Bluetooth stack
)pbdoc";

constexpr auto kDocsBackendGetDongl = R"pbdoc(
    Get the Dongl backend, whether or not config.dongl.use_dongl_backend lists it in get_backends()
)pbdoc";

constexpr auto kDocsBackendGetSimulator = R"pbdoc(
    Get the Simulator backend, which only has adapters while a simulation is active
)pbdoc";

constexpr auto kDocsBackendInitialized = R"pbdoc(
    Whether the backend is initialized
)pbdoc";

constexpr auto kDocsBackendAdapters = R"pbdoc(
    Get all adapters provided by this backend
)pbdoc";

constexpr auto kDocsBackendBluetoothEnabled = R"pbdoc(
    Whether Bluetooth is enabled for this backend
)pbdoc";

constexpr auto kDocsBackendIdentifier = R"pbdoc(
    Identifier of the backend
)pbdoc";

void wrap_backend(py::module& m) {
    py::class_<SimpleBLE::Backend>(m, "Backend", kDocsBackend)
        .def_static("get_backends", &SimpleBLE::Backend::get_backends, kDocsBackendGetBackends)
        .def_static("get_native", &SimpleBLE::Backend::get_native, kDocsBackendGetNative)
        .def_static("get_dongl", &SimpleBLE::Backend::get_dongl, kDocsBackendGetDongl)
        .def_static("get_simulator", &SimpleBLE::Backend::get_simulator, kDocsBackendGetSimulator)
        .def("initialized", &SimpleBLE::Backend::initialized, kDocsBackendInitialized)
        .def("adapters", &SimpleBLE::Backend::adapters, kDocsBackendAdapters)
        .def("bluetooth_enabled", &SimpleBLE::Backend::bluetooth_enabled, kDocsBackendBluetoothEnabled)
        .def("identifier", &SimpleBLE::Backend::identifier, kDocsBackendIdentifier);
}
