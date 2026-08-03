#include <pybind11/pybind11.h>
#include <pybind11/stl.h>

#include "simpleble/Advanced.h"

namespace py = pybind11;

void wrap_advanced(py::module& m) {
    auto advanced = m.def_submodule("advanced", "Platform-specific advanced SimpleBLE APIs");

#if SIMPLEPYBLE_HAS_MACOS_ADVANCED
    auto macos = advanced.def_submodule("macos", "Advanced CoreBluetooth APIs");
    macos.def("retrieve_cached_peripherals", &SimpleBLE::Advanced::MacOS::retrieve_cached_peripherals,
              py::call_guard<py::gil_scoped_release>(),
              "Retrieve peripherals that CoreBluetooth can resolve from its system cache");
#endif
}
