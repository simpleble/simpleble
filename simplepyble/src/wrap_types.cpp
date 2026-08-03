#include <pybind11/functional.h>
#include <pybind11/pybind11.h>
#include <pybind11/stl.h>

#include "simpleble/Types.h"

namespace py = pybind11;

void wrap_types(py::module& m) {
    py::enum_<SimpleBLE::OperatingSystem>(m, "OperatingSystem")
        .value("WINDOWS", SimpleBLE::OperatingSystem::WINDOWS)
        .value("MACOS", SimpleBLE::OperatingSystem::MACOS)
        .value("IOS", SimpleBLE::OperatingSystem::IOS)
        .value("LINUX", SimpleBLE::OperatingSystem::LINUX)
        .value("ANDROID", SimpleBLE::OperatingSystem::ANDROID)
        .export_values();

    py::enum_<SimpleBLE::BluetoothAddressType>(m, "BluetoothAddressType")
        .value("PUBLIC", SimpleBLE::BluetoothAddressType::PUBLIC)
        .value("RANDOM", SimpleBLE::BluetoothAddressType::RANDOM)
        .value("UNSPECIFIED", SimpleBLE::BluetoothAddressType::UNSPECIFIED)
        .export_values();
}
