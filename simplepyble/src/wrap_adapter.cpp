#include <pybind11/pybind11.h>
#include <pybind11/stl.h>

#include "simpleble/SimpleBLE.h"

#include "python_callback.hpp"

namespace py = pybind11;

constexpr auto kDocsAdapter = R"pbdoc(
    Adapter
)pbdoc";

constexpr auto kDocsAdapterBluetoothEnabled = R"pbdoc(
    Whether Bluetooth is enabled
)pbdoc";

constexpr auto kDocsAdapterGetAdapters = R"pbdoc(
    Get all available adapters
)pbdoc";

constexpr auto kDocsAdapterInitialized = R"pbdoc(
    Whether the adapter is initialized
)pbdoc";

constexpr auto kDocsAdapterIdentifier = R"pbdoc(
    Identifier of the adapter
)pbdoc";

constexpr auto kDocsAdapterAddress = R"pbdoc(
    Address of the adapter
)pbdoc";

constexpr auto kDocsAdapterPowerOn = R"pbdoc(
    Power on the adapter
)pbdoc";

constexpr auto kDocsAdapterPowerOff = R"pbdoc(
    Power off the adapter
)pbdoc";

constexpr auto kDocsAdapterIsPowered = R"pbdoc(
    Whether the adapter is powered
)pbdoc";

constexpr auto kDocsAdapterScanStart = R"pbdoc(
    Start scanning for peripherals
)pbdoc";

constexpr auto kDocsAdapterScanStop = R"pbdoc(
    Stop scanning for peripherals
)pbdoc";

constexpr auto kDocsAdapterScanIsActive = R"pbdoc(
    Whether the adapter is scanning
)pbdoc";

constexpr auto kDocsAdapterScanFor = R"pbdoc(
    Scan for peripherals for a given duration
)pbdoc";

constexpr auto kDocsAdapterScanGetResults = R"pbdoc(
    Get the results of the last scan
)pbdoc";

constexpr auto kDocsAdapterSetCallbackOnScanStart = R"pbdoc(
    Set the callback to be called when scanning starts
)pbdoc";

constexpr auto kDocsAdapterSetCallbackOnScanStop = R"pbdoc(
    Set the callback to be called when scanning stops
)pbdoc";

constexpr auto kDocsAdapterSetCallbackOnScanFound = R"pbdoc(
    Set the callback to be called when a peripheral is found
)pbdoc";

constexpr auto kDocsAdapterSetCallbackOnScanUpdated = R"pbdoc(
    Set the callback to be called when a peripheral is updated
)pbdoc";

constexpr auto kDocsAdapterSetCallbackOnPowerOn = R"pbdoc(
    Set the callback to be called when the adapter is powered on
)pbdoc";

constexpr auto kDocsAdapterSetCallbackOnPowerOff = R"pbdoc(
    Set the callback to be called when the adapter is powered off
)pbdoc";

constexpr auto kDocsAdapterGetPairedPeripherals = R"pbdoc(
    Get all paired peripherals
)pbdoc";

constexpr auto kDocsAdapterGetConnectedPeripherals = R"pbdoc(
    Get all connected peripherals
)pbdoc";

constexpr auto kDocsAdapterCreateLocalPeripheral = R"pbdoc(
    Create a local peripheral hosted by this adapter
)pbdoc";

void wrap_adapter(py::module& m) {
    // TODO: Add __str__ and __repr__ methods
    py::class_<SimpleBLE::Adapter>(m, "Adapter")
        .def_static("bluetooth_enabled", &SimpleBLE::Adapter::bluetooth_enabled, py::call_guard<py::gil_scoped_release>(), kDocsAdapterBluetoothEnabled)
        .def_static("get_adapters", &SimpleBLE::Adapter::get_adapters, py::call_guard<py::gil_scoped_release>(), kDocsAdapterGetAdapters)
        .def("initialized", &SimpleBLE::Adapter::initialized, py::call_guard<py::gil_scoped_release>(), kDocsAdapterInitialized)
        .def("identifier", &SimpleBLE::Adapter::identifier, py::call_guard<py::gil_scoped_release>(), kDocsAdapterIdentifier)
        .def("address", &SimpleBLE::Adapter::address, py::call_guard<py::gil_scoped_release>(), kDocsAdapterAddress)
        .def("power_on", &SimpleBLE::Adapter::power_on, py::call_guard<py::gil_scoped_release>(), kDocsAdapterPowerOn)
        .def("power_off", &SimpleBLE::Adapter::power_off, py::call_guard<py::gil_scoped_release>(), kDocsAdapterPowerOff)
        .def("is_powered", &SimpleBLE::Adapter::is_powered, py::call_guard<py::gil_scoped_release>(), kDocsAdapterIsPowered)
        .def("scan_start", &SimpleBLE::Adapter::scan_start, py::call_guard<py::gil_scoped_release>(), kDocsAdapterScanStart)
        .def("scan_stop", &SimpleBLE::Adapter::scan_stop, py::call_guard<py::gil_scoped_release>(), kDocsAdapterScanStop)
        .def("scan_is_active", &SimpleBLE::Adapter::scan_is_active, py::call_guard<py::gil_scoped_release>(), kDocsAdapterScanIsActive)
        .def("scan_for", &SimpleBLE::Adapter::scan_for, py::call_guard<py::gil_scoped_release>(), kDocsAdapterScanFor)
        .def("scan_get_results", &SimpleBLE::Adapter::scan_get_results, py::call_guard<py::gil_scoped_release>(), kDocsAdapterScanGetResults)
        .def("set_callback_on_scan_start", simplepyble::callback_setter(&SimpleBLE::Adapter::set_callback_on_scan_start), py::call_guard<py::gil_scoped_release>(), kDocsAdapterSetCallbackOnScanStart)
        .def("set_callback_on_scan_stop", simplepyble::callback_setter(&SimpleBLE::Adapter::set_callback_on_scan_stop), py::call_guard<py::gil_scoped_release>(), kDocsAdapterSetCallbackOnScanStop)
        .def("set_callback_on_scan_found", simplepyble::callback_setter(&SimpleBLE::Adapter::set_callback_on_scan_found), py::call_guard<py::gil_scoped_release>(), kDocsAdapterSetCallbackOnScanFound)
        .def("set_callback_on_scan_updated", simplepyble::callback_setter(&SimpleBLE::Adapter::set_callback_on_scan_updated), py::call_guard<py::gil_scoped_release>(), kDocsAdapterSetCallbackOnScanUpdated)
        .def("set_callback_on_power_on", simplepyble::callback_setter(&SimpleBLE::Adapter::set_callback_on_power_on), py::call_guard<py::gil_scoped_release>(), kDocsAdapterSetCallbackOnPowerOn)
        .def("set_callback_on_power_off", simplepyble::callback_setter(&SimpleBLE::Adapter::set_callback_on_power_off), py::call_guard<py::gil_scoped_release>(), kDocsAdapterSetCallbackOnPowerOff)
        .def("get_paired_peripherals", &SimpleBLE::Adapter::get_paired_peripherals, py::call_guard<py::gil_scoped_release>(), kDocsAdapterGetPairedPeripherals)
        .def("get_connected_peripherals", &SimpleBLE::Adapter::get_connected_peripherals, py::call_guard<py::gil_scoped_release>(), kDocsAdapterGetConnectedPeripherals)
        .def("create_local_peripheral", &SimpleBLE::Adapter::create_local_peripheral, py::call_guard<py::gil_scoped_release>(), kDocsAdapterCreateLocalPeripheral);
}
